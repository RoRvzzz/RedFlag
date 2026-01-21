"""
Cog: YARA Scanning
"""
import os
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

        UI.log("\n[bold white]Step 3a: YARA Pattern Scanning...[/bold white]")
        
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
                task = progress.add_task(f"YARA Scanning {len(files_to_scan)} files...", total=len(files_to_scan))
                for f in files_to_scan:
                    self._scan_file(f)
                    progress.advance(task)
        else:
            for f in files_to_scan:
                self._scan_file(f)

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
        except Exception as e:
            # Just log verbose error if needed, don't crash
            pass
