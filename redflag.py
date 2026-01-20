#!/usr/bin/env python3
"""
RedFlag - Step-by-Step Malware Analysis Tool
"""

import os
import re
import math
import argparse
import sys
import time
import xml.etree.ElementTree as ET
import binascii
from collections import defaultdict
from typing import List, Dict, Set, Optional, Tuple
from dataclasses import dataclass, field

# Try to import Rich for beautiful output
try:
    from rich.console import Console
    from rich.panel import Panel
    from rich.text import Text
    from rich.table import Table
    from rich.progress import Progress, SpinnerColumn, TextColumn, BarColumn
    from rich.tree import Tree
    from rich.layout import Layout
    from rich.live import Live
    from rich.markdown import Markdown
    from rich.style import Style
    RICH_AVAILABLE = True
    console = Console()
except ImportError:
    RICH_AVAILABLE = False
    print("For best experience, install rich: pip install rich")

# ASCII Art
BANNER = r"""
$$$$$$$\                  $$\ $$$$$$$$\ $$\                      oo_____________
$$  __$$\                 $$ |$$  _____|$$ |                     ||\\\\////\\\\||
$$ |  $$ | $$$$$$\   $$$$$$$ |$$ |      $$ | $$$$$$\   $$$$$$\   ||\\\\////\\\\||
$$$$$$$  |$$  __$$\ $$  __$$ |$$$$$\    $$ | \____$$\ $$  __$$\  ||////\\\\\\\\||
$$  __$$< $$$$$$$$ |$$ /  $$ |$$  __|   $$ | $$$$$$$ |$$ /  $$ | ||////\\\\\\\\||
$$ |  $$ |$$   ____|$$ |  $$ |$$ |      $$ |$$  __$$ |$$ |  $$ | ||
$$ |  $$ |\$$$$$$$\ \$$$$$$$ |$$ |      $$ |\$$$$$$$ |\$$$$$$$ | ||
\__|  \__| \_______| \_______|\__|      \__| \_______| \____$$ | ||
                                                      $$\   $$ | ||
                                                      \$$$$$$  | ||
                                                       \______/  ||
"""

@dataclass
class Finding:
    category: str
    description: str
    file: str
    line: int
    context: str
    score: int
    severity: str  # LOW, MEDIUM, HIGH, CRITICAL

class RedFlagScanner:
    def __init__(self, target_path):
        # Handle both file and directory inputs
        self.target_path = os.path.abspath(target_path)
        self.is_file = os.path.isfile(self.target_path)
        self.target_dir = os.path.dirname(self.target_path) if self.is_file else self.target_path
        
        self.findings: List[Finding] = []
        self.stats = defaultdict(int)
        self.project_type = "Unknown"
        self.suspicious_build_events = []
        

        
        # Configuration
        self.ignore_dirs = {
            '.git', '.svn', '.vs', '.vscode', '.idea',
            'build', 'out', 'bin', 'obj', 'node_modules', '__pycache__',
            'artifacts', 'dist', 'target', 'vendor', 'ext', 'external',
            'x64', 'x86', 'debug', 'release'
        }
        self.ignore_files = {'redflag.py', 'check.py', 'checker.py', os.path.basename(__file__)}
        # Skip binary files, libraries, and build artifacts that naturally contain API names
        self.skip_exts = {
            '.exe', '.dll', '.obj', '.pdb', '.idb', '.ilk', '.png', '.jpg', '.ico', '.pdf',
            '.lib', '.a', '.so', '.dylib', '.exp', # Libraries
            '.ifc', '.ifcast', '.ipch', '.pch', # C++ Module/Precompiled Header artifacts
            '.suo', '.user', '.filters' # Visual Studio user files
        }
        
        # Compile patterns
        self._init_patterns()

    def _init_patterns(self):
        self.patterns = {
            'EXECUTION': [
                (r'cmd\.exe', 3, 'Command Prompt Execution'),
                (r'powershell', 4, 'PowerShell Execution'),
                (r'ShellExecute', 3, 'API Shell Execution'),
                (r'CreateProcess', 3, 'API Process Creation'),
                (r'system\(', 3, 'System Command'),
                (r'WinExec', 3, 'Legacy Execution API'),
            ],
            'MEMORY': [
                (r'VirtualAlloc', 4, 'Memory Allocation (RWX Potential)'),
                (r'WriteProcessMemory', 5, 'Process Memory Injection'),
                (r'CreateRemoteThread', 5, 'Remote Thread Injection'),
                (r'ReflectiveLoader', 5, 'Reflective DLL Injection Artifact'),
                (r'RtlMoveMemory', 2, 'Memory Manipulation'),
            ],
            'NETWORK': [
                (r'URLDownloadToFile', 3, 'File Download API'),
                (r'InternetOpen', 2, 'WinINet API'),
                (r'socket', 2, 'Raw Socket'),
                (r'curl\s+', 3, 'Curl Command'),
                (r'wget\s+', 3, 'Wget Command'),
                (r'Invoke-WebRequest', 4, 'PowerShell Download'),
                (r'bitsadmin', 4, 'BITS Persistence/Download'),
            ],
            'CRYPTO': [
                (r'CryptEncrypt', 2, 'Windows Crypto API'),
                (r'AESManaged', 2, '.NET AES'),
                (r'Rijndael', 2, 'Rijndael Cipher'),
                (r'Xor\s+', 1, 'XOR Operation (Potential Obfuscation)'),
            ],
            'OBFUSCATION': [
                (r'FromBase64String', 3, 'Base64 Decoding'),
                (r'-enc\s+', 4, 'PowerShell Encoded Command'),
                (r'IsDebuggerPresent', 3, 'Anti-Debugging Check'),
            ]
        }
        
        self.compiled_patterns = {}
        for cat, pats in self.patterns.items():
            self.compiled_patterns[cat] = [(re.compile(p, re.IGNORECASE), s, d) for p, s, d in pats]
            
        self.base64_regex = re.compile(r'(?:[A-Za-z0-9+/]{4}){20,}(?:[A-Za-z0-9+/]{2}==|[A-Za-z0-9+/]{3}=)?')

    def log(self, message, style=""):
        if RICH_AVAILABLE:
            console.print(message, style=style)
        else:
            print(message)

    def run(self):
        self.print_banner()
        
        # Step 1: Verify Structure
        if not self.step_verify_structure():
            return

        # Step 2: Build Files
        self.step_scan_build_files()
        
        # Step 3: Source Scan
        self.step_scan_source()
        
        # Step 4: Correlation
        self.step_correlate()
        
        # Step 5: Verdict
        self.step_verdict()

    def print_banner(self):
        if RICH_AVAILABLE:
            console.clear()
            console.print(Text(BANNER, style="bold red"))
            console.print(Panel(f"Target: [bold cyan]{self.target_path}[/bold cyan]", title="Initialization", border_style="red"))
        else:
            print(BANNER)
            print(f"Target: {self.target_path}\n")

    def step_verify_structure(self):
        self.log("\n[bold white]Step 1: Analyzing Target...[/bold white]")
        
        if not os.path.exists(self.target_path):
            self.log("[bold red]❌ Error: Path does not exist.[/bold red]")
            return False

        if self.is_file:
            self.project_type = "Single File"
            icon = "📄"
            self.log(f"  {icon} Analyzing single file: [bold cyan]{os.path.basename(self.target_path)}[/bold cyan]")
            return True

        files = os.listdir(self.target_dir)
        has_sln = any(f.endswith('.sln') for f in files)
        has_vcxproj = any(f.endswith('.vcxproj') for f in files)
        has_makefile = 'Makefile' in files
        has_git = '.git' in files
        
        if has_sln or has_vcxproj:
            self.project_type = "Visual Studio C++"
            icon = "🔷"
        elif has_makefile:
            self.project_type = "Make/C++"
            icon = "🔧"
        else:
            self.project_type = "Generic Folder"
            icon = "📁"
            
        self.log(f"  {icon} Type identified: [bold cyan]{self.project_type}[/bold cyan]")
        if has_git:
            self.log("  🌲 Git repository detected")
        
        return True

    def step_scan_build_files(self):
        self.log("\n[bold white]Step 2: Scanning Build Configuration (High Priority)...[/bold white]")
        
        build_files = []
        if self.is_file:
            if self.target_path.endswith('.vcxproj'):
                build_files.append(self.target_path)
        else:
            for root, dirs, files in os.walk(self.target_dir):
                for f in files:
                    if f.endswith('.vcxproj'):
                        build_files.append(os.path.join(root, f))
        
        if not build_files:
            self.log("  ℹ No build files (.vcxproj) found.")
            return

        for bf in build_files:
            # For single file scan, show relative to its own dir to keep path short
            rel_base = self.target_dir if not self.is_file else os.path.dirname(self.target_dir)
            rel_path = os.path.relpath(bf, rel_base)
            self._analyze_vcxproj(bf, rel_path)

    def _analyze_vcxproj(self, path, rel_path):
        try:
            tree = ET.parse(path)
            root = tree.getroot()
            ns = {'ns': root.tag.split('}')[0].strip('{')} if '}' in root.tag else {}
            
            events = ['PreBuildEvent', 'PostBuildEvent', 'PreLinkEvent']
            for event in events:
                # Handle XML namespaces properly if needed, mostly simplistic here
                for item in root.iter():
                    if item.tag.endswith(event):
                        cmd_elem = item.find('Command' if not ns else f"{{ {ns['ns']} }}Command")
                        # Sometimes structure is different, just search specifically for Command tag under event
                        if cmd_elem is None:
                             for child in item:
                                 if child.tag.endswith('Command'):
                                     cmd_elem = child
                                     break
                                     
                        if cmd_elem is not None and cmd_elem.text:
                            cmd = cmd_elem.text.strip()
                            self.log(f"  [bold yellow]⚠️  {event} Detected in {rel_path}[/bold yellow]")
                            self.log(f"     Command: [dim]{cmd[:100]}[/dim]")
                            
                            self._analyze_command(cmd, rel_path, event)
                            
        except Exception as e:
            self.log(f"  [red]Error parsing {rel_path}: {e}[/red]")

    def _analyze_command(self, cmd, file, event_type):
        # Normalize
        cmd_lower = cmd.lower()
        score = 0
        severity = "LOW"
        reasons = []

        if 'powershell' in cmd_lower or 'pwsh' in cmd_lower:
            score += 5
            reasons.append("PowerShell Execution")
        if 'cmd.exe' in cmd_lower or 'cmd /c' in cmd_lower:
            score += 3
            reasons.append("Shell Execution")
        if 'download' in cmd_lower or 'wget' in cmd_lower or 'curl' in cmd_lower or 'iwr' in cmd_lower:
            score += 5
            reasons.append("Network Activity")
        if '-enc' in cmd_lower or '-encodedcommand' in cmd_lower:
            score += 5
            reasons.append("Encoded Command")

        if score > 0:
            severity = "CRITICAL" if score >= 8 else "HIGH" if score >= 5 else "MEDIUM"
            self.findings.append(Finding(
                category="BUILD_EVENT",
                description=f"Suspicious Build Event ({', '.join(reasons)})",
                file=file,
                line=0,
                context=cmd,
                score=score,
                severity=severity
            ))

    def step_scan_source(self):
        self.log("\n[bold white]Step 3: Deep Source Analysis...[/bold white]")
        
        files_to_scan = []
        
        # Helper to check ignores case-insensitively
        def is_ignored_dir(d):
            return d.lower() in self.ignore_dirs
        
        if self.is_file:
            # If target is a source file or any text file, scan it
            if not any(self.target_path.lower().endswith(x) for x in self.skip_exts):
                files_to_scan.append(self.target_path)
        else:
            for root, dirs, files in os.walk(self.target_dir):
                # Modify dirs in-place to skip ignored directories
                dirs[:] = [d for d in dirs if not is_ignored_dir(d)]
                
                for f in files:
                    if f in self.ignore_files: continue
                    if not any(f.lower().endswith(x) for x in self.skip_exts):
                        files_to_scan.append(os.path.join(root, f))
        
        if RICH_AVAILABLE:
            with Progress(
                SpinnerColumn(),
                TextColumn("[progress.description]{task.description}"),
                BarColumn(),
                TextColumn("{task.percentage:>3.0f}%"),
            ) as progress:
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
            # For single file mode, report absolute path is fine or relative to dir
            rel_base = self.target_dir if not self.is_file else os.path.dirname(self.target_dir)
            rel_path = os.path.relpath(path, rel_base)
            
            with open(path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
            
            # 1. Regex Scan
            for cat, patterns in self.compiled_patterns.items():
                for pat, score, desc in patterns:
                    for match in pat.finditer(content):
                        line_no = content[:match.start()].count('\n') + 1
                        ctx = content[max(0, match.start()-50):min(len(content), match.end()+50)].replace('\n', ' ')
                        
                        self.findings.append(Finding(
                            category=cat,
                            description=desc,
                            file=rel_path,
                            line=line_no,
                            context=ctx,
                            score=score,
                            severity=self._get_severity(score)
                        ))

            # 2. Entropy / Base64 Scan
            self._scan_blobs(content, rel_path)

        except Exception:
            pass

    def _scan_blobs(self, content, rel_path):
        for match in self.base64_regex.finditer(content):
            blob = match.group()
            if len(blob) < 50: continue

            # Decoding Attempt
            try:
                decoded = binascii.a2b_base64(blob)
                
                # Check entropy of decoded content
                entropy = self._calculate_entropy(decoded)
                
                # Look for strings inside decoded content
                decoded_str = decoded.decode('utf-8', errors='ignore')
                
                score = 0
                notes = []
                
                if entropy > 6.0:
                    score += 2
                    notes.append(f"High Entropy ({entropy:.2f})")
                
                # Check for risky keywords INSIDE the blob
                for cat, patterns in self.compiled_patterns.items():
                    for pat, s, d in patterns:
                        if pat.search(decoded_str):
                            score += s + 2 # Boost score for hidden threats
                            notes.append(f"Contains {d}")

                if b'http' in decoded or b'https' in decoded:
                    score += 3
                    notes.append("Contains URL")

                if score >= 3:
                    line_no = content[:match.start()].count('\n') + 1
                    self.findings.append(Finding(
                        category="HIDDEN_PAYLOAD",
                        description=f"Suspicious Encoded Blob ({', '.join(notes)})",
                        file=rel_path,
                        line=line_no,
                        context=blob[:50] + "...",
                        score=score,
                        severity=self._get_severity(score)
                    ))

            except Exception:
                pass

    def _calculate_entropy(self, data):
        if not data: return 0
        counts = defaultdict(int)
        for b in data: counts[b] += 1
        entropy = 0
        for count in counts.values():
            p = count / len(data)
            entropy -= p * math.log2(p)
        return entropy

    def step_correlate(self):
        # Boost scores if multiple categories appear in the same file
        file_cats = defaultdict(set)
        for f in self.findings:
            file_cats[f.file].add(f.category)
            
        for file, cats in file_cats.items():
            if 'CRYPTO' in cats and ('EXECUTION' in cats or 'NETWORK' in cats):
                self.findings.append(Finding(
                    category="CORRELATION",
                    description="Crypto utilized alongside Execution/Network capabilities",
                    file=file,
                    line=0,
                    context="File-wide Analysis",
                    score=5,
                    severity="HIGH"
                ))

    def step_verdict(self):
        total_score = sum(f.score for f in self.findings)
        max_severity = "CLEAN"
        
        # Determine Max Severity
        severities = [f.severity for f in self.findings]
        if "CRITICAL" in severities: max_severity = "CRITICAL"
        elif "HIGH" in severities: max_severity = "HIGH"
        elif "MEDIUM" in severities: max_severity = "MEDIUM"
        elif "LOW" in severities: max_severity = "LOW"
        elif total_score > 0: max_severity = "INFO"

        self.log("\n[bold white]Step 5: Final Verdict...[/bold white]")
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
        
        if RICH_AVAILABLE:
            console.print(Panel(
                f"[bold]Risk Level: {max_severity}[/bold]\nTotal Threat Score: {total_score}",
                title="VERDICT",
                style=verdict_color,
                expand=False
            ))
        else:
            print(f"VERDICT: {max_severity} (Score: {total_score})")

        # Top Findings
        if self.findings:
            self.log("\n[bold]Top Findings:[/bold]")
            self.findings.sort(key=lambda x: x.score, reverse=True)
            
            seen = set()
            count = 0
            for f in self.findings:
                key = f"{f.file}:{f.line}:{f.description}"
                if key in seen: continue
                seen.add(key)
                
                if count >= 10: break # Top 10 only
                
                if RICH_AVAILABLE:
                    s_color = color_map.get(f.severity, "white")
                    console.print(f" • [{s_color}]{f.severity:8}[/{s_color}] {f.file}:{f.line} - [bold]{f.description}[/bold]")
                    if f.line > 0:
                        console.print(f"   [dim]{f.context.strip()[:80]}[/dim]")
                else:
                    print(f" - [{f.severity}] {f.file}:{f.line} : {f.description}")
                
                count += 1

    def _get_severity(self, score):
        if score >= 5: return "HIGH"
        if score >= 3: return "MEDIUM"
        return "LOW"

def main():
    parser = argparse.ArgumentParser(description="RedFlag Malware Analysis")
    parser.add_argument("path", nargs="?", help="Path to project or file")
    args = parser.parse_args()
    
    path = args.path
    if not path:
        # If running interactive
        if RICH_AVAILABLE:
            console.print(Text(BANNER, style="bold red"))
        else:
            print(BANNER)
        path = input("Enter project path to scan: ").strip('"').strip("'")
        
    if not path:
        print("No path provided.")
        return

    scanner = RedFlagScanner(path)
    scanner.run()

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nScan aborted.")
