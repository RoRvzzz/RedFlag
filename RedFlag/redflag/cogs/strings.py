
import os
from urlextract import URLExtract
from ..core.models import Finding
from ..core.utils import UI, get_severity
from ..core.config import IGNORE_DIRS, IGNORE_FILES, SKIP_EXTS, BENIGN_DOMAINS

class StringAnalysisCog:
    def __init__(self, scanner):
        self.scanner = scanner
        self.extractor = URLExtract()

    def run(self):
        UI.log("\n[bold white]Step 3b: Advanced String Analysis (URLs & IPs)...[/bold white]")
        
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
            return

        progress = UI.get_progress()
        if progress:
            with progress:
                task = progress.add_task(f"Scanning Strings in {len(files_to_scan)} files...", total=len(files_to_scan))
                for f in files_to_scan:
                    self._scan_file(f)
                    progress.advance(task)
        else:
            for f in files_to_scan:
                self._scan_file(f)

    def _scan_file(self, path):
        try:
            rel_base = self.scanner.target_dir if not self.scanner.is_file else os.path.dirname(self.scanner.target_dir)
            rel_path = os.path.relpath(path, rel_base)
            
            with open(path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
            
            # Extract URLs
            urls = self.extractor.find_urls(content)
            
            for url in urls:
                # Basic cleanup
                clean_url = url.lower().strip()
                
                # Filter out benign domains
                is_benign = False
                for domain in BENIGN_DOMAINS:
                    if domain in clean_url:
                        is_benign = True
                        break
                
                if is_benign:
                    continue
                    
                line_no = content[:content.find(url)].count('\n') + 1
                
                # Check context for download/exfil indicators
                ctx_start = max(0, content.find(url) - 50)
                ctx_end = min(len(content), content.find(url) + len(url) + 50)
                ctx = content[ctx_start:ctx_end].replace('\n', ' ')
                
                # Default to INFO severity for generic URLs
                score = 0 
                severity = "INFO"
                desc = "URL Detected"
                
                # Elevate score if suspicious context found
                if any(x in ctx.lower() for x in ['download', 'curl', 'wget', 'socket', 'connect', 'upload', 'exfil']):
                    score = 3
                    severity = "MEDIUM"
                    desc = "Suspicious URL (Network Activity Context)"
                
                # Elevate if it looks like a raw IP
                if any(c.isdigit() for c in url) and not any(c.isalpha() for c in url.split('/')[2] if c not in '.:'):
                     score = 3
                     severity = "MEDIUM"
                     desc = "Hardcoded IP Address"

                self.scanner.add_finding(Finding(
                    category="NETWORK_IOC",
                    description=desc,
                    file=rel_path,
                    line=line_no,
                    context=url,
                    score=score,
                    severity=severity
                ))

        except Exception:
            pass
