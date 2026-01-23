
import os
import binascii
import re
from concurrent.futures import ThreadPoolExecutor, as_completed
from ..core.config import IGNORE_DIRS, IGNORE_FILES, SKIP_EXTS, BASE64_REGEX
from ..core.models import Finding
from ..core.utils import UI, calculate_entropy, get_severity, xor_brute

# Config
MAX_FILE_SIZE = 10 * 1024 * 1024  # 10 MB limit per file for code scanning

class CodeScanCog:
    def __init__(self, scanner):
        self.scanner = scanner

    def run(self):
        UI.log("\n[bold white]Step 3: Analyzing source code...[/bold white]")
        
        # Ensure extracted images list exists (initialized in scanner __init__)
        
        # Use cached file list for efficiency (shared across cogs)
        files_to_scan = self.scanner.get_files_to_scan(include_binaries=False)
        
        if not files_to_scan:
            UI.log("  [yellow]No suitable source files found to scan.[/yellow]")
            return

        # Track image count before scanning
        image_count_before = len(self.scanner.extracted_images)

        # Use multi-threading for faster scanning
        max_workers = min(8, len(files_to_scan))  # Cap at 8 threads or file count
        
        progress = UI.get_progress()
        if progress:
            with progress:
                task = progress.add_task(f"Scanning {len(files_to_scan)} files...", total=len(files_to_scan))
                with ThreadPoolExecutor(max_workers=max_workers) as executor:
                    futures = {executor.submit(self._scan_single_file, f): f for f in files_to_scan}
                    for future in as_completed(futures):
                        progress.advance(task)
        else:
            print(f"Scanning {len(files_to_scan)} files...")
            with ThreadPoolExecutor(max_workers=max_workers) as executor:
                executor.map(self._scan_single_file, files_to_scan)
        
        # Show summary of extracted images
        image_count_after = len(self.scanner.extracted_images)
        if image_count_after > image_count_before:
            extracted_count = image_count_after - image_count_before
            output_dir = os.path.join(self.scanner.target_dir if not self.scanner.is_file else os.path.dirname(self.scanner.target_dir), 'redflag_extracted_images')
            abs_output_dir = os.path.abspath(output_dir)
            UI.log(f"  [bold green]✓ Extracted {extracted_count} embedded image(s)[/bold green]")
            UI.log(f"  [dim]Images saved to: {abs_output_dir}[/dim]")

    def _scan_single_file(self, path):
        try:
            # Safety: Skip huge files
            if os.path.getsize(path) > MAX_FILE_SIZE:
                return
            
            rel_base = self.scanner.target_dir if not self.scanner.is_file else os.path.dirname(self.scanner.target_dir)
            rel_path = os.path.relpath(path, rel_base)
            
            # Only scan C++ source files for XOR obfuscation (not project files, HTML, etc.)
            source_extensions = {'.cpp', '.h', '.hpp', '.c', '.cc', '.cxx', '.hxx'}
            _, ext = os.path.splitext(path.lower())
            is_source_file = ext in source_extensions
            
            with open(path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
            
            # 1. Extract Images (Heavy Regex) - Only run if likely to contain images
            # Only run if "unsigned" or "char" and "{" appear to save CPU
            if "{" in content and ("char" in content or "byte" in content or "unsigned" in content):
                self._extract_images(content, rel_path)
            
            # regex scan
            for cat, patterns in self.scanner.compiled_patterns.items():
                for pat, score, desc in patterns:
                    for match in pat.finditer(content):
                        line_no = content[:match.start()].count('\n') + 1
                        ctx = content[max(0, match.start()-50):min(len(content), match.end()+50)].replace('\n', ' ')
                        
                        # Contextual "Stack String" filtering using entropy analysis
                        if cat == 'OBFUSCATION' and 'Stack String' in desc:
                            skip_finding = False
                            
                            # Try to extract the actual byte array data for entropy analysis
                            try:
                                # Extract hex bytes from context: { 0x41, 0x42, ... }
                                hex_match = re.search(r'\{((?:\s*0x[0-9a-fA-F]{2}\s*,?\s*)+)\}', ctx)
                                if hex_match:
                                    hex_data = hex_match.group(1)
                                    clean_hex = hex_data.replace('0x', '').replace(',', '').replace(' ', '').replace('\n', '').replace('\r', '').replace('\t', '')
                                    if len(clean_hex) >= 20:  # Only analyze if substantial data
                                        try:
                                            binary_data = binascii.unhexlify(clean_hex)
                                            entropy = calculate_entropy(binary_data)
                                            
                                            # Entropy-based filtering:
                                            # < 3.5: English text/code (likely benign)
                                            # > 7.5: Compressed/Encrypted (suspicious)
                                            # 4.0-6.0: Lookup table/font map (lower severity)
                                            
                                            if entropy < 3.5:
                                                # Low entropy = likely text/code, skip
                                                skip_finding = True
                                            elif 4.0 <= entropy <= 6.0:
                                                # Medium entropy = likely lookup table/font, reduce severity
                                                score = max(2, score - 1)  # Reduce by 1, minimum 2
                                                desc = f"{desc} (Lookup Table/Font - Lower Risk)"
                                            # entropy > 7.5 keeps original high score
                                        except (binascii.Error, ValueError):
                                            pass  # If hex parsing fails, fall through to other checks
                            except Exception:
                                pass  # If extraction fails, fall through to other checks
                            
                            # Fallback: Check if we've already identified this as a valid image
                            if not skip_finding:
                                with self.scanner._lock:
                                    for img_info in self.scanner.extracted_images:
                                        if img_info['file'] == rel_path and abs(img_info['line'] - line_no) < 5:
                                            skip_finding = True
                                            break
                            
                            # Additional context checks
                            if not skip_finding:
                                if any(x in ctx.lower() for x in ['font', 'image', 'icon', 'bytes', 'data', 'texture']):
                                    skip_finding = True
                                elif any(x in rel_path.lower() for x in ['font', 'image', 'icon', 'resource', 'asset', 'protect', 'crypto']):
                                    skip_finding = True
                            
                            if skip_finding:
                                continue
                        
                        # Decode and analyze hex-escaped strings
                        if cat == 'OBFUSCATION' and 'Hex-Escaped String' in desc:
                            try:
                                # Extract the hex-escaped string
                                str_match = re.search(r'["\']((?:\\x[0-9a-fA-F]{2})+?)["\']', ctx)
                                if str_match:
                                    hex_str = str_match.group(1)
                                    # Decode \x escapes
                                    decoded_bytes = bytes.fromhex(hex_str.replace('\\x', ''))
                                    decoded_text = decoded_bytes.decode('utf-8', errors='ignore')
                                    
                                    # Check for suspicious content in decoded string
                                    suspicious_keywords = ['powershell', 'cmd', 'iwr', 'download', 'invoke', 'start-process', 
                                                          'http', 'https', 'exe', 'dll', 'base64', 'encoded']
                                    if any(kw in decoded_text.lower() for kw in suspicious_keywords):
                                        # Elevate severity and add decoded content to context
                                        score = 8  # High severity for decoded malicious content
                                        desc = f"Hex-Escaped String (Decoded: {decoded_text[:50]}...)"
                                        ctx = f"Original: {ctx[:100]}... | Decoded: {decoded_text[:200]}"
                            except Exception:
                                pass  # If decoding fails, use original finding

                        # Context-aware refinement for System Commands
                        if cat == 'EXECUTION' and 'System Command' in desc:
                            # Filter out false positives: function declarations, macro definitions, and capitalized System()
                            lower_ctx = ctx.lower()
                            
                            # Skip if it's a function declaration (void System, int System, etc.)
                            if re.search(r'\b(void|int|bool|string|std::string)\s+System\s*\(', ctx, re.IGNORECASE):
                                continue
                            
                            # Skip if it's a macro definition (#define l_system, #define System, etc.)
                            if '#define' in ctx or '#ifdef' in ctx or '#ifndef' in ctx:
                                continue
                            
                            # Skip if it's calling a capitalized System() function (not system())
                            # Check for namespace-qualified System() calls like utils::output::System()
                            if re.search(r'(?:::|\.)\s*System\s*\(', ctx) or ('System(' in ctx and 'system(' not in lower_ctx):
                                continue
                            
                            # Skip if it's in a comment
                            if ctx.strip().startswith('//') or '/*' in ctx or '*/' in ctx:
                                continue
                            
                            # Context-aware scoring for legitimate uses
                            if 'firewall' in lower_ctx or 'netsh' in lower_ctx:
                                desc = "System Command (Firewall/Network Config)"
                                score = 2
                            elif 'ipconfig' in lower_ctx or 'ping' in lower_ctx:
                                desc = "System Command (Network Info)"
                                score = 1
                            elif 'cls' in lower_ctx or 'pause' in lower_ctx:
                                # Already filtered in pattern, but double-check
                                continue
                        
                        # Context-aware filtering for network APIs (reduce severity, don't skip entirely)
                        if cat == 'NETWORK' and ('WinINet API' in desc or 'Raw Socket' in desc):
                            lower_ctx = ctx.lower()
                            lower_rel_path = rel_path.lower()
                            
                            # Skip if it's clearly example/documentation code
                            if 'example' in lower_ctx or 'demo' in lower_ctx or 'test' in lower_ctx:
                                # Check if it's in a comment or string literal (likely documentation)
                                if ctx.strip().startswith('//') or '/*' in ctx or '*/' in ctx or '"Example' in ctx or "'Example" in ctx:
                                    continue
                            
                            # Reduce severity (don't skip) if it's in HTTP client/library code
                            # This way we still detect malicious use but reduce false positives
                            if any(x in lower_rel_path for x in ['library', 'lib', 'util', 'utils', 'common', 'shared', 'misc', 'http_client', 'httpclient', 'network', 'net']):
                                # Check if it's a wrapper or legitimate library code
                                if 'wrapper' in lower_ctx or 'struct' in lower_ctx or 'class' in lower_ctx or 'namespace' in lower_ctx or 'httpclient' in lower_ctx or 'http_client' in lower_ctx:
                                    # Reduce score but still report (could be hiding malicious code)
                                    score = max(1, score - 2)  # Reduce by 2, minimum 1
                                    desc = f"{desc} (Library Code - Lower Risk)"
                            
                            # Skip Raw Socket if it's matching variable names, not function calls
                            if 'Raw Socket' in desc:
                                # Check if it's matching variable names like socket_wrapper, closesocket, etc.
                                if any(x in lower_ctx for x in ['socket_wrapper', 'closesocket', 'socket_wrapper(', 'socket_wrapper{', 'socket_wrapper=']):
                                    continue
                                # Check if it's just a struct/class definition, not actual socket() call
                                if 'struct' in lower_ctx or 'class' in lower_ctx or 'namespace' in lower_ctx:
                                    # Make sure it's not an actual socket() function call
                                    if not re.search(r'\bsocket\s*\(', lower_ctx):
                                        continue
                            
                            # Reduce severity for WinINet in error handling or library context
                            if 'WinINet API' in desc:
                                # Reduce score if it's error handling code (cerr, error, failed, etc.)
                                if any(x in lower_ctx for x in ['cerr', 'error', 'failed', 'null', 'if (', 'return']):
                                    # But only if it's clearly library/error handling, not actual malicious usage
                                    if any(x in lower_rel_path for x in ['library', 'lib', 'util', 'misc', 'http_client', 'httpclient']) or 'example' in lower_ctx:
                                        score = max(1, score - 1)  # Reduce by 1, minimum 1
                                        desc = f"{desc} (Error Handling - Lower Risk)"
                                # Reduce severity if it's clearly an HTTP client class/function
                                if 'httpclient' in lower_ctx or 'http_client' in lower_ctx or 'initialize' in lower_ctx:
                                    score = max(1, score - 1)  # Reduce by 1, minimum 1
                                    desc = f"{desc} (HTTP Client - Lower Risk)"
                        
                        # Context-aware filtering for XOR (reduce severity, don't skip entirely)
                        if cat == 'CRYPTO' and 'XOR Operation' in desc:
                            lower_ctx = ctx.lower()
                            lower_rel_path = rel_path.lower()
                            
                            # Reduce severity (don't skip) if it's in compression/utility code
                            # This way we still detect malicious XOR but reduce false positives
                            if any(x in lower_rel_path for x in ['compression', 'compress', 'util', 'utils', 'helper', 'utility']):
                                # Check if context mentions compression, encoding, or basic obfuscation
                                if any(x in lower_ctx for x in ['compression', 'compress', 'encode', 'decode', 'obfuscat', 'encrypt', 'decrypt', 'utility', 'helper']):
                                    # Reduce score but still report (could be hiding malicious code)
                                    score = max(1, score - 1)  # Reduce by 1, minimum 1
                                    desc = f"{desc} (Utility Code - Lower Risk)"
                            # Reduce severity if it's in a comment explaining it's for obfuscation/compression
                            elif 'simple xor' in lower_ctx or 'basic obfuscat' in lower_ctx or 'xor encryption' in lower_ctx:
                                score = max(1, score - 1)  # Reduce by 1, minimum 1
                                desc = f"{desc} (Documented Usage - Lower Risk)"
                            
                            # BUT: If XOR is combined with suspicious patterns, keep high severity
                            # Check surrounding context for suspicious combinations
                            suspicious_indicators = ['download', 'upload', 'execute', 'payload', 'shell', 'cmd', 'powershell']
                            if any(indicator in lower_ctx for indicator in suspicious_indicators):
                                # Keep original score - this is suspicious even in utility code
                                pass

                        self.scanner.add_finding(Finding(
                            category=cat,
                            description=desc,
                            file=rel_path,
                            line=line_no,
                            context=ctx,
                            score=score,
                            severity=get_severity(score)
                        ))

            # 3. Base64 & XOR Scan
            # Only scan if content length suggests it might hide payloads
            if len(content) > 100:  # Skip tiny files
                self._scan_blobs(content, rel_path)
                # Only scan XOR on actual C++ source files (not project files, HTML, etc.)
                if is_source_file:
                    self._scan_xor(content, rel_path)

        except Exception:
            pass

    def _scan_blobs(self, content, rel_path):
        for match in BASE64_REGEX.finditer(content):
            blob = match.group()
            if len(blob) < 50: continue

            try:
                decoded = binascii.a2b_base64(blob)
                entropy = calculate_entropy(decoded)
                decoded_str = decoded.decode('utf-8', errors='ignore')
                
                score = 0
                notes = []
                
                if entropy > 6.0:
                    score += 2
                    notes.append(f"High Entropy ({entropy:.2f})")
                
                # check keywords in decoded
                for cat, patterns in self.scanner.compiled_patterns.items():
                    for pat, s, d in patterns:
                        if pat.search(decoded_str):
                            score += s + 2
                            notes.append(f"Contains {d}")

                if b'http' in decoded or b'https' in decoded:
                    score += 3
                    notes.append("Contains URL")

                if score >= 3:
                    line_no = content[:match.start()].count('\n') + 1
                    self.scanner.add_finding(Finding(
                        category="HIDDEN_PAYLOAD",
                        description=f"Suspicious Encoded Blob ({', '.join(notes)})",
                        file=rel_path,
                        line=line_no,
                        context=blob[:50] + "...",
                        score=score,
                        severity=get_severity(score)
                    ))

            except (UnicodeError, OSError, PermissionError) as e:
                # Expected file read errors - silently skip
                pass
            except Exception as e:
                # Log internal errors for debugging (but don't crash the scan)
                # Only log in verbose mode to avoid cluttering output
                if hasattr(self.scanner, 'verbose') and self.scanner.verbose:
                    UI.log(f"  [dim red]Error scanning {rel_path}: {type(e).__name__}[/dim red]")
                pass

    def _extract_images(self, content, rel_path):
        """Extract and verify embedded images from unsigned char arrays"""
        # Pattern to match unsigned char arrays: unsigned char name[] = { 0x41, 0x42, ... };
        # Updated to handle multi-line arrays and different spacing
        array_pattern = re.compile(
            r'(?:unsigned\s+)?(?:char|byte|uint8_t)\s+\w+\[\]\s*=\s*\{'
            r'((?:\s*0x[0-9a-fA-F]{2}\s*,?\s*)+)'
            r'\s*\}',
            re.IGNORECASE | re.DOTALL
        )
        
        # Image magic bytes (file signatures)
        IMAGE_SIGNATURES = {
            b'\x89PNG\r\n\x1a\n': ('PNG', 'png'),
            b'\xff\xd8\xff': ('JPEG', 'jpg'),
            b'BM': ('BMP', 'bmp'),
            b'GIF87a': ('GIF87a', 'gif'),
            b'GIF89a': ('GIF89a', 'gif'),
            b'RIFF': ('WEBP/AVI', 'webp'),  # Need to check further for WEBP
        }
        
        for match in array_pattern.finditer(content):
            try:
                # Extract hex bytes - get everything between { and }
                hex_data = match.group(1)
                # Clean up: remove 0x, commas, whitespace, newlines
                clean_hex = hex_data.replace('0x', '').replace(',', '').replace(' ', '').replace('\n', '').replace('\r', '').replace('\t', '')
                
                # Debug: log if we found a potential array (for troubleshooting)
                # UI.log(f"  [dim]Found potential array in {rel_path}, hex length: {len(clean_hex)}[/dim]")
                
                if len(clean_hex) < 20:  # Too small to be an image
                    continue
                
                # Convert to bytes
                try:
                    binary_data = binascii.unhexlify(clean_hex)
                except binascii.Error:
                    continue
                
                if len(binary_data) < 10:  # Minimum image size
                    continue
                
                # Check for image signatures
                image_type = None
                image_ext = None
                
                for sig, (img_type, ext) in IMAGE_SIGNATURES.items():
                    if binary_data.startswith(sig):
                        image_type = img_type
                        image_ext = ext
                        break
                
                # Special check for WEBP (RIFF...WEBP)
                if binary_data.startswith(b'RIFF') and b'WEBP' in binary_data[:20]:
                    image_type = 'WEBP'
                    image_ext = 'webp'
                
                if image_type:
                    # Successfully identified as an image - this helps reduce false positives
                    line_no = content[:match.start()].count('\n') + 1
                    
                    # Store metadata about extracted image
                    # This can be used to suppress false positives for "Stack String" findings
                    if not hasattr(self.scanner, 'extracted_images'):
                        self.scanner.extracted_images = []
                    
                    # Create output directory for extracted images
                    output_dir = os.path.join(self.scanner.target_dir if not self.scanner.is_file else os.path.dirname(self.scanner.target_dir), 'redflag_extracted_images')
                    if not os.path.exists(output_dir):
                        os.makedirs(output_dir, exist_ok=True)
                    
                    # Generate safe filename from source file path and line number
                    safe_filename = rel_path.replace('\\', '_').replace('/', '_').replace(':', '_')
                    output_filename = f"{safe_filename}_line{line_no}.{image_ext}"
                    output_path = os.path.join(output_dir, output_filename)
                    
                    # Save the image
                    try:
                        with open(output_path, 'wb') as img_file:
                            img_file.write(binary_data)
                        saved_path = os.path.relpath(output_path, self.scanner.target_dir if not self.scanner.is_file else os.path.dirname(self.scanner.target_dir))
                    except Exception as e:
                        saved_path = None
                    
                    # Thread-safe append
                    self.scanner.add_extracted_image({
                        'file': rel_path,
                        'line': line_no,
                        'type': image_type,
                        'size': len(binary_data),
                        'saved_path': saved_path,
                        'data': binary_data[:100]  # Store first 100 bytes for reference
                    })
                    
                    # Don't log during scan to avoid cluttering output - will show summary later
                    # The images are saved and metadata is stored for false positive filtering
                    
            except Exception:
                continue

    def _scan_xor(self, content, rel_path):
        """
        Optimized XOR scanning - only analyze strings/arrays longer than 32 chars.
        Less false positives, faster scanning.
        """
        # Hex strings (e.g., 0x41, 0x42...)
        hex_blobs = re.findall(r'(?:0x[0-9a-fA-F]{2},\s*){16,}', content)
        
        # Long String literals (potential payloads)
        # Refined regex to capture content inside quotes more accurately
        long_strs = re.findall(r'"([^"]{64,})"', content)
        
        candidates = []
        
        for blob in hex_blobs:
            try:
                clean = blob.replace('0x', '').replace(',', '').replace(' ', '').replace('\n', '')
                candidates.append(binascii.unhexlify(clean))
            except (binascii.Error, ValueError):
                pass  # Expected hex parsing errors
            except Exception as e:
                # Log unexpected errors
                if hasattr(self.scanner, 'verbose') and self.scanner.verbose:
                    UI.log(f"  [dim red]Error in XOR scan for {rel_path}: {type(e).__name__}[/dim red]")
                pass
                
        for s in long_strs:
            # Skip strings that are mostly whitespace or code delimiters
            whitespace_delim_count = sum(1 for c in s if c.isspace() or c in '{;}')
            if whitespace_delim_count / len(s) > 0.3:  # More than 30% whitespace/delimiters
                continue
            
            # Only analyze if it looks like garbage (high entropy or weird chars)
            if any(ord(c) > 127 for c in s) or calculate_entropy(s) > 3.5:
                candidates.append(s)
            
        for candidate in candidates:
            results = xor_brute(candidate)
            if results:
                # We found something that decodes to text
                # Only report if the *decoded* text looks suspicious
                for key, decoded_text in results:
                    suspicious = False
                    
                    # More strict keyword check - must contain multiple suspicious indicators
                    decoded_lower = decoded_text.lower()
                    suspicious_keywords = ['http', 'cmd', 'powershell', 'exec', 'system', 'download', 'upload', 
                                          'invoke', 'iwr', 'curl', 'wget', 'base64', 'encoded', 'shell', 
                                          'process', 'inject', 'payload', 'malware', 'trojan', 'virus']
                    
                    # Count how many suspicious keywords are found
                    keyword_count = sum(1 for kw in suspicious_keywords if kw in decoded_lower)
                    
                    # Require at least 2 suspicious keywords, or one very high-risk keyword
                    high_risk_keywords = ['powershell', 'invoke', 'download', 'upload', 'payload', 'inject']
                    has_high_risk = any(kw in decoded_lower for kw in high_risk_keywords)
                    
                    if keyword_count >= 2 or has_high_risk:
                        suspicious = True
                    
                    # Also check if it looks like a command or URL
                    if not suspicious:
                        # Check if decoded text looks like a command (starts with common command prefixes)
                        if any(decoded_lower.startswith(prefix) for prefix in ['cmd', 'powershell', 'start', 'run']):
                            suspicious = True
                        # Check if it's a URL
                        elif decoded_lower.startswith('http://') or decoded_lower.startswith('https://'):
                            suspicious = True
                    
                    if suspicious:
                        self.scanner.add_finding(Finding(
                            category="OBFUSCATION",
                            description=f"XOR Obfuscated Content (Key: {hex(key)})",
                            file=rel_path,
                            line=0,
                            context=decoded_text[:100] + "...",
                            score=5,
                            severity="HIGH"
                        ))
                        return  # Stop after first valid hit per candidate
