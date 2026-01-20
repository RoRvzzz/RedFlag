
import os
import binascii
import re
from ..core.config import IGNORE_DIRS, IGNORE_FILES, SKIP_EXTS, BASE64_REGEX
from ..core.models import Finding
from ..core.utils import UI, calculate_entropy, get_severity, xor_brute

class CodeScanCog:
    def __init__(self, scanner):
        self.scanner = scanner

    def run(self):
        UI.log("\n[bold white]Step 3: Deep Source Analysis...[/bold white]")
        
        files_to_scan = []
        
        def is_ignored_dir(d):
            return d.lower() in IGNORE_DIRS
        
        if self.scanner.is_file:
            if not any(self.scanner.target_path.lower().endswith(x) for x in SKIP_EXTS):
                files_to_scan.append(self.scanner.target_path)
        else:
            for root, dirs, files in os.walk(self.scanner.target_dir):
                dirs[:] = [d for d in dirs if not is_ignored_dir(d)]
                
                for f in files:
                    if f in IGNORE_FILES: continue
                    if not any(f.lower().endswith(x) for x in SKIP_EXTS):
                        files_to_scan.append(os.path.join(root, f))
        
        if not files_to_scan:
            UI.log("  ℹ No suitable source files found to scan.")
            return

        progress = UI.get_progress()
        if progress:
            with progress:
                task = progress.add_task(f"Scanning {len(files_to_scan)} files...", total=len(files_to_scan))
                for f in files_to_scan:
                    self._scan_single_file(f)
                    progress.advance(task)
        else:
            print(f"Scanning {len(files_to_scan)} files...")
            for f in files_to_scan:
                self._scan_single_file(f)

    def _scan_single_file(self, path):
        try:
            rel_base = self.scanner.target_dir if not self.scanner.is_file else os.path.dirname(self.scanner.target_dir)
            rel_path = os.path.relpath(path, rel_base)
            
            with open(path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
            
            # regex scan
            for cat, patterns in self.scanner.compiled_patterns.items():
                for pat, score, desc in patterns:
                    for match in pat.finditer(content):
                        line_no = content[:match.start()].count('\n') + 1
                        ctx = content[max(0, match.start()-50):min(len(content), match.end()+50)].replace('\n', ' ')
                        
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

    def _scan_xor(self, content, rel_path):
        # Look for potential byte arrays or obfuscated strings
        # Heuristic: long hex strings or byte array initializers
        
        # 1. Hex strings like "4d5a9000..."
        hex_blobs = re.findall(r'(?:0x[0-9a-fA-F]{2},\s*){10,}', content)
        # 2. String literals that look random/long
        long_strs = re.findall(r'"([^"]{50,})"', content)
        
        candidates = []
        
        # Parse hex blobs
        for blob in hex_blobs:
            try:
                # Cleanup: "0x41, 0x42" -> b'AB'
                clean = blob.replace('0x', '').replace(',', '').replace(' ', '').replace('\n', '')
                candidates.append(binascii.unhexlify(clean))
            except:
                pass
                
        # Candidates from strings
        for s in long_strs:
            candidates.append(s)
            
        for candidate in candidates:
            # Try XOR brute
            results = xor_brute(candidate)
            for key, decoded_text in results:
                # Check if decoded text has anything interesting
                for cat, patterns in self.scanner.compiled_patterns.items():
                    for pat, s, d in patterns:
                        if pat.search(decoded_text):
                            self.scanner.add_finding(Finding(
                                category="OBFUSCATION",
                                description=f"XOR Obfuscated Content (Key: {hex(key)}) - Contains {d}",
                                file=rel_path,
                                line=0, # Hard to track line for complex extractions
                                context=decoded_text[:100] + "...",
                                score=5,
                                severity="HIGH"
                            ))
                            return # Found a match, stop checking this candidate
