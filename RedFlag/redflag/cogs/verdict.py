
import time
from collections import defaultdict
from ..core.models import Finding
from ..core.utils import UI, get_severity
from ..core.definitions import get_definition

class VerdictCog:
    def __init__(self, scanner):
        self.scanner = scanner

    def run(self):
        self.correlate()
        self.report()

    def correlate(self):
        # Build file-based capability map
        file_caps = defaultdict(set)
        for f in self.scanner.findings:
            # Simplistic categorization for correlation
            if f.category in ['EXECUTION', 'BUILD_EVENT']:
                file_caps[f.file].add('EXEC')
            elif f.category in ['NETWORK', 'NETWORK_IOC']:
                file_caps[f.file].add('NET')
            elif f.category == 'OBFUSCATION':
                file_caps[f.file].add('OBFUSC')
            elif f.category == 'MEMORY':
                file_caps[f.file].add('MEM')
            elif f.category == 'HIDDEN_PAYLOAD':
                file_caps[f.file].add('PAYLOAD')

        # Behavioral Correlation Rules
        for file, caps in file_caps.items():
            # Rule 1: Obfuscation + Execution (Dropper behavior)
            if 'OBFUSC' in caps and 'EXEC' in caps:
                self.scanner.add_finding(Finding(
                    category="BEHAVIOR",
                    description="Obfuscated Execution Chain (Dropper Pattern)",
                    file=file,
                    line=0,
                    context="File contains both obfuscation routines and execution commands",
                    score=7,
                    severity="HIGH",
                    confidence="HIGH",
                    metadata={'mitre': ['T1027 - Obfuscated Files or Information']}
                ))

            # Rule 2: Memory Injection + Network (RAT/Downloader behavior)
            if 'MEM' in caps and 'NET' in caps:
                self.scanner.add_finding(Finding(
                    category="BEHAVIOR",
                    description="Memory Injection with Network Capability (RAT Pattern)",
                    file=file,
                    line=0,
                    context="File performs memory manipulation and network connections",
                    score=9,
                    severity="CRITICAL",
                    confidence="HIGH",
                    metadata={'mitre': ['T1055 - Process Injection', 'T1105 - Ingress Tool Transfer']}
                ))
            
            # Rule 3: Hidden Payload + Execution (Stager behavior)
            if 'PAYLOAD' in caps and 'EXEC' in caps:
                self.scanner.add_finding(Finding(
                    category="BEHAVIOR",
                    description="Embedded Payload Execution (Stager Pattern)",
                    file=file,
                    line=0,
                    context="High entropy blob detected alongside execution logic",
                    score=8,
                    severity="HIGH",
                    confidence="HIGH"
                ))

    def report(self):
        # Calculate score ignoring INFO findings
        total_score = sum(f.score for f in self.scanner.findings if f.severity != "INFO")
        max_severity = "CLEAN"
        
        severities = [f.severity for f in self.scanner.findings]
        
        if "CRITICAL" in severities: max_severity = "CRITICAL"
        elif "HIGH" in severities: max_severity = "HIGH"
        elif "MEDIUM" in severities: max_severity = "MEDIUM"
        elif "LOW" in severities: max_severity = "LOW"
        elif "INFO" in severities: max_severity = "INFO"
        
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
                
                # Format MITRE tags if present
                mitre_text = ""
                if f.metadata and 'mitre' in f.metadata:
                    tags = ", ".join(f.metadata['mitre'])
                    mitre_text = f" [dim cyan][{tags}][/dim cyan]"

                if ']' in s_color: 
                     UI.log(f" • [{s_color}]{f.severity:8}[/{s_color}] {f.file}:{f.line} - [bold]{f.description}[/bold]{mitre_text}")
                else:
                     UI.log(f" • [{s_color}]{f.severity:8}[/{s_color}] {f.file}:{f.line} - [bold]{f.description}[/bold]{mitre_text}")
                
                if f.line > 0:
                    ctx = f.context.strip()[:80]
                    if ctx:
                        UI.log(f"   [dim]{ctx}[/dim]")
                
                # Show definition if available and enabled
                if self.scanner.show_definitions:
                    defn = get_definition(f.description)
                    if defn:
                        UI.log(f"   [dim cyan]ℹ {defn['description']}[/dim cyan]")
                        UI.log(f"   [dim yellow]⚠ Risk: {defn['risk']}[/dim yellow]")
                
                count += 1
