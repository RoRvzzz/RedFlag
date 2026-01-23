"""
Cog: YARA Scanning
"""
import os
from concurrent.futures import ThreadPoolExecutor, as_completed
from ..core.utils import UI, YARA_AVAILABLE, get_severity
from ..core.config import DEFAULT_YARA_RULES, SKIP_EXTS, IGNORE_DIRS, IGNORE_FILES
from ..core.models import Finding

class YaraScanCog:
    def __init__(self, scanner):
        self.scanner = scanner
        self.rules = None

    def run(self):
        if not YARA_AVAILABLE:
            UI.log("  [yellow]YARA module not found. Skipping YARA scan.[/yellow]")
            return

        UI.log("\n[bold white]Step 3a: scanning with yara rules...[/bold white]")
        
        if not self._compile_rules():
            return

        self._scan_files()

    def _compile_rules(self):
        try:
            import yara
            sources = {'default': DEFAULT_YARA_RULES}
            self.rules = yara.compile(sources=sources)
            UI.log(f"  ✅ Compiled internal YARA rules")
            return True
        except Exception as e:
            UI.log(f"  [red]Failed to compile YARA rules: {e}[/red]")
            return False

    def _scan_files(self):
        # Use cached file list for efficiency
        files_to_scan = self.scanner.get_files_to_scan(include_binaries=False)

        if not files_to_scan:
            return

        # Use multi-threading for faster scanning
        max_workers = min(8, len(files_to_scan))
        
        progress = UI.get_progress()
        if progress:
            with progress:
                task = progress.add_task(f"YARA Scanning {len(files_to_scan)} files...", total=len(files_to_scan))
                with ThreadPoolExecutor(max_workers=max_workers) as executor:
                    futures = {executor.submit(self._scan_file, f): f for f in files_to_scan}
                    for future in as_completed(futures):
                        progress.advance(task)
        else:
            with ThreadPoolExecutor(max_workers=max_workers) as executor:
                executor.map(self._scan_file, files_to_scan)

    def _scan_file(self, path):
        try:
            matches = self.rules.match(path)
            if matches:
                rel_base = self.scanner.target_dir if not self.scanner.is_file else os.path.dirname(self.scanner.target_dir)
                rel_path = os.path.relpath(path, rel_base)
                
                for match in matches:
                    rule_name = match.rule
                    meta = match.meta
                    desc = meta.get('description', rule_name)
                    severity_str = meta.get('severity', 'MEDIUM')
                    
                    # Filter false positives: PE header matches in embedded assets
                    if rule_name == 'PE_Header_In_Source':
                        # Check if file path or filename suggests embedded asset (font, image, texture, etc.)
                        rel_path_lower = rel_path.lower()
                        filename_lower = os.path.basename(rel_path).lower()
                        asset_keywords = ['font', 'image', 'texture', 'icon', 'bitmap', 'resource', 'asset', 'data', 'pay', 'preview']
                        # Check both path and filename for asset indicators
                        if any(keyword in rel_path_lower for keyword in asset_keywords) or \
                           any(keyword in filename_lower for keyword in asset_keywords):
                            # Likely an embedded asset, skip this false positive
                            continue
                    
                    # Map severity string to score
                    score_map = {'CRITICAL': 10, 'HIGH': 5, 'MEDIUM': 3, 'LOW': 1}
                    score = score_map.get(severity_str.upper(), 3)
                    
                    # Get matched strings for context
                    context = ""
                    if match.strings:
                        try:
                            # match.strings is a list of (offset, identifier, data)
                            context = f"Matched: {match.strings[0][2].decode('utf-8', errors='ignore')}"
                        except:
                            context = "Binary match"

                    self.scanner.add_finding(Finding(
                        category="YARA",
                        description=f"YARA Match: {desc}",
                        file=rel_path,
                        line=0,
                        context=context,
                        score=score,
                        severity=severity_str.upper(),
                        metadata={'rule': rule_name, 'meta': meta}
                    ))
        except (OSError, PermissionError) as e:
            # Expected file access errors - silently skip
            pass
        except Exception as e:
            # Log unexpected errors for debugging (YARA errors are usually expected)
            if hasattr(self.scanner, 'verbose') and self.scanner.verbose:
                UI.log(f"  [dim red]YARA error scanning {path}: {type(e).__name__}[/dim red]")
            pass
