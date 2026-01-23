
import os
import binascii
import re
from concurrent.futures import ThreadPoolExecutor, as_completed
from ..core.config import IGNORE_DIRS, IGNORE_FILES, SKIP_EXTS, BASE64_REGEX
from ..core.models import Finding
from ..core.utils import UI, calculate_entropy, get_severity, xor_brute

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
            rel_base = self.scanner.target_dir if not self.scanner.is_file else os.path.dirname(self.scanner.target_dir)
            rel_path = os.path.relpath(path, rel_base)
            
            with open(path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
            
            # Extract images first (to help filter false positives)
            self._extract_images(content, rel_path)
            
            # regex scan
            for cat, patterns in self.scanner.compiled_patterns.items():
                for pat, score, desc in patterns:
                    for match in pat.finditer(content):
                        line_no = content[:match.start()].count('\n') + 1
                        ctx = content[max(0, match.start()-50):min(len(content), match.end()+50)].replace('\n', ' ')
                        
                        # Filter out benign stack strings (embedded assets)
                        if cat == 'OBFUSCATION' and 'Stack String' in desc:
                            skip_finding = False
                            
                            if any(x in ctx.lower() for x in ['font', 'image', 'icon', 'bytes', 'data', 'texture']):
                                skip_finding = True
                            elif any(x in rel_path.lower() for x in ['font', 'image', 'icon', 'resource', 'asset', 'protect', 'crypto']):
                                skip_finding = True
                            # Check if we've already identified this as a valid image (thread-safe read)
                            else:
                                with self.scanner._lock:
                                    for img_info in self.scanner.extracted_images:
                                        if img_info['file'] == rel_path and abs(img_info['line'] - line_no) < 5:
                                            # This array was identified as a valid image, suppress the finding
                                            skip_finding = True
                                            break
                            
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
                        
                        # Filter out example/documentation code for network APIs
                        if cat == 'NETWORK' and ('WinINet API' in desc or 'Raw Socket' in desc):
                            lower_ctx = ctx.lower()
                            lower_rel_path = rel_path.lower()
                            
                            # Skip if it's clearly example/documentation code
                            if 'example' in lower_ctx or 'demo' in lower_ctx or 'test' in lower_ctx:
                                # Check if it's in a comment or string literal (likely documentation)
                                if ctx.strip().startswith('//') or '/*' in ctx or '*/' in ctx or '"Example' in ctx or "'Example" in ctx:
                                    continue
                            
                            # Skip if file is in a library directory (common library code patterns)
                            if any(x in lower_rel_path for x in ['library', 'lib', 'util', 'utils', 'common', 'shared', 'misc']):
                                # Check if it's a wrapper or legitimate library code
                                if 'wrapper' in lower_ctx or 'struct' in lower_ctx or 'class' in lower_ctx or 'namespace' in lower_ctx:
                                    continue
                            
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
                            
                            # Skip WinINet if it's in error handling or library context
                            if 'WinINet API' in desc:
                                # Skip if it's error handling code (cerr, error, failed, etc.)
                                if any(x in lower_ctx for x in ['cerr', 'error', 'failed', 'null', 'if (', 'return']):
                                    # But only if it's clearly library/error handling, not actual malicious usage
                                    if any(x in lower_rel_path for x in ['library', 'lib', 'util', 'misc']) or 'example' in lower_ctx:
                                        continue

                        self.scanner.add_finding(Finding(
                            category=cat,
                            description=desc,
                            file=rel_path,
                            line=line_no,
                            context=ctx,
                            score=score,
                            severity=get_severity(score)
                        ))

            # base64 scan
            self._scan_blobs(content, rel_path)
            
            # XOR brute force on suspicious byte arrays or strings
            # Simple heuristic: scan "suspiciously long" continuous strings that aren't base64
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

            except Exception:
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
        # look for potential byte arrays or obfuscated strings
        # heuristic: long hex strings or byte array initializers
        
        # hex strings like "4d5a9000..."
        hex_blobs = re.findall(r'(?:0x[0-9a-fA-F]{2},\s*){10,}', content)
        # string literals that look random/long
        long_strs = re.findall(r'"([^"]{50,})"', content)
        
        candidates = []
        
        # parse hex blobs
        for blob in hex_blobs:
            try:
                # cleanup: "0x41, 0x42" -> b'AB'
                clean = blob.replace('0x', '').replace(',', '').replace(' ', '').replace('\n', '')
                candidates.append(binascii.unhexlify(clean))
            except:
                pass
                
        # candidates from strings
        for s in long_strs:
            candidates.append(s)
            
        for candidate in candidates:
            # try xor brute force
            results = xor_brute(candidate)
            for key, decoded_text in results:
                # check if decoded text has anything interesting
                for cat, patterns in self.scanner.compiled_patterns.items():
                    for pat, s, d in patterns:
                        if pat.search(decoded_text):
                            self.scanner.add_finding(Finding(
                                category="OBFUSCATION",
                                description=f"XOR Obfuscated Content (Key: {hex(key)}) - Contains {d}",
                                file=rel_path,
                                line=0, # hard to track line for complex extractions
                                context=decoded_text[:100] + "...",
                                score=5,
                                severity="HIGH"
                            ))
                            return # found a match, stop checking this candidate
