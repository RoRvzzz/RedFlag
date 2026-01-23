
import os
import re
from concurrent.futures import ThreadPoolExecutor, as_completed
from urlextract import URLExtract
from ..core.models import Finding
from ..core.utils import UI, get_severity
from ..core.config import IGNORE_DIRS, IGNORE_FILES, SKIP_EXTS, BENIGN_DOMAINS

class StringAnalysisCog:
    def __init__(self, scanner):
        self.scanner = scanner
        self.extractor = URLExtract()

    def run(self):
        UI.log("\n[bold white]Step 3b: scanning strings (urls & ips)...[/bold white]")
        
        # Use cached file list for efficiency
        files_to_scan = self.scanner.get_files_to_scan(include_binaries=False)

        if not files_to_scan:
            return

        # Use multi-threading for faster scanning
        max_workers = min(8, len(files_to_scan))
        
        progress = UI.get_progress()
        if progress:
            with progress:
                task = progress.add_task(f"Scanning Strings in {len(files_to_scan)} files...", total=len(files_to_scan))
                with ThreadPoolExecutor(max_workers=max_workers) as executor:
                    futures = {executor.submit(self._scan_file, f): f for f in files_to_scan}
                    for future in as_completed(futures):
                        progress.advance(task)
        else:
            with ThreadPoolExecutor(max_workers=max_workers) as executor:
                executor.map(self._scan_file, files_to_scan)

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
                
                # Validate that it's actually a URL (must start with http:// or https://)
                if not (clean_url.startswith('http://') or clean_url.startswith('https://')):
                    continue
                
                # Filter out false positives: common C++ member access patterns
                # Skip if it looks like .Data, .ID, .TextA, etc. (these aren't URLs)
                if re.match(r'^[a-z_][a-z0-9_]*\.(data|id|texta|textw|path|buffer)', clean_url, re.IGNORECASE):
                    continue
                
                # Must have a valid domain structure (at least one dot after http://)
                if not re.match(r'https?://[^/]+\.[^/]+', clean_url):
                    continue
                
                # IMPROVED BENIGN DOMAIN CHECK
                is_benign = False
                try:
                    # Parse domain from URL for accurate checking
                    from urllib.parse import urlparse
                    parsed = urlparse(clean_url)
                    hostname = parsed.hostname
                    
                    if hostname:
                        for domain in BENIGN_DOMAINS:
                            # 1. Exact match: "google.com"
                            # 2. Subdomain: "api.google.com"
                            if hostname == domain or hostname.endswith('.' + domain):
                                is_benign = True
                                break
                except:
                    # Fallback to string matching if parsing fails
                    for domain in BENIGN_DOMAINS:
                        if clean_url.startswith(f'http://{domain}') or clean_url.startswith(f'https://{domain}') or \
                           f'.{domain}' in clean_url or clean_url.endswith(domain):
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
                
                # Elevate score if suspicious context found (but not for common game/platform URLs)
                suspicious_keywords = ['download', 'curl', 'wget', 'socket', 'connect', 'upload', 'exfil', 'httprequest', 'fetch']
                if any(x in ctx.lower() for x in suspicious_keywords):
                    # But don't flag if it's just a comment or example code
                    if not ('example' in ctx.lower() or 'comment' in ctx.lower() or '//' in ctx[:20]):
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
