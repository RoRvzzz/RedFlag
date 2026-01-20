
import time
from collections import defaultdict
from ..core.models import Finding
from ..core.utils import UI

class VerdictCog:
    def __init__(self, scanner):
        self.scanner = scanner

    def run(self):
        self.correlate()
        self.report()

    def correlate(self):
        file_cats = defaultdict(set)
        for f in self.scanner.findings:
            file_cats[f.file].add(f.category)
            
        for file, cats in file_cats.items():
            if 'CRYPTO' in cats and ('EXECUTION' in cats or 'NETWORK' in cats):
                self.scanner.add_finding(Finding(
                    category="CORRELATION",
                    description="Crypto utilized alongside Execution/Network capabilities",
                    file=file,
                    line=0,
                    context="File-wide Analysis",
                    score=5,
                    severity="HIGH"
                ))

    def report(self):
        total_score = sum(f.score for f in self.scanner.findings)
        max_severity = "CLEAN"
        
        severities = [f.severity for f in self.scanner.findings]
        if "CRITICAL" in severities: max_severity = "CRITICAL"
        elif "HIGH" in severities: max_severity = "HIGH"
        elif "MEDIUM" in severities: max_severity = "MEDIUM"
        elif "LOW" in severities: max_severity = "LOW"
        elif total_score > 0: max_severity = "INFO"

        UI.log("\n[bold white]Step 5: Final Verdict...[/bold white]")
        time.sleep(0.5)
        
        color_map = {
            "CRITICAL": "bold white on red",
            "HIGH": "bold red",
            "MEDIUM": "bold yellow",
            "LOW": "bold blue",
            "INFO": "green",
            "CLEAN": "bold green"
        }
        
        verdict_color = color_map.get(max_severity, "white")
        
        UI.print_panel(
            f"[bold]Risk Level: {max_severity}[/bold]\nTotal Threat Score: {total_score}",
            title="VERDICT",
            style=verdict_color
        )

        if self.scanner.findings:
            UI.log("\n[bold]Top Findings:[/bold]")
            self.scanner.findings.sort(key=lambda x: x.score, reverse=True)
            
            seen = set()
            count = 0
            for f in self.scanner.findings:
                key = f"{f.file}:{f.line}:{f.description}"
                if key in seen: continue
                seen.add(key)
                
                if count >= 10: break
                
                s_color = color_map.get(f.severity, "white")
                # handle rich style string
                if ']' in s_color: # Handle rich style string
                     UI.log(f" • [{s_color}]{f.severity:8}[/{s_color}] {f.file}:{f.line} - [bold]{f.description}[/bold]")
                else:
                     UI.log(f" • [{s_color}]{f.severity:8}[/{s_color}] {f.file}:{f.line} - [bold]{f.description}[/bold]")
                
                if f.line > 0:
                    ctx = f.context.strip()[:80]
                    if ctx:
                        UI.log(f"   [dim]{ctx}[/dim]")
                
                count += 1
