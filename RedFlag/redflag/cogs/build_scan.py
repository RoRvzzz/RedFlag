"""
Cog: Build System Scanner (VCXPROJ)
"""
import os
import xml.etree.ElementTree as ET
from ..core.models import Finding
from ..core.utils import UI, get_severity

class BuildScanCog:
    def __init__(self, scanner):
        self.scanner = scanner

    def run(self):
        UI.log("\n[bold white]Step 2: scanning build configuration...[/bold white]")
        
        build_files = []
        if self.scanner.is_file:
            if self.scanner.target_path.endswith('.vcxproj'):
                build_files.append(self.scanner.target_path)
        else:
            for root, dirs, files in os.walk(self.scanner.target_dir):
                for f in files:
                    if f.endswith('.vcxproj'):
                        build_files.append(os.path.join(root, f))
        
        if not build_files:
            UI.log("  ℹ No build files (.vcxproj) found.")
            return

        for bf in build_files:
            rel_base = self.scanner.target_dir if not self.scanner.is_file else os.path.dirname(self.scanner.target_dir)
            rel_path = os.path.relpath(bf, rel_base)
            self._analyze_vcxproj(bf, rel_path)

    def _analyze_vcxproj(self, path, rel_path):
        try:
            tree = ET.parse(path)
            root = tree.getroot()
            ns = {'ns': root.tag.split('}')[0].strip('{')} if '}' in root.tag else {}
            
            # scan for linked libraries (AdditionalLibraryDirectories, AdditionalDependencies)
            self._scan_linked_libraries(root, rel_path, ns)
            
            events = ['PreBuildEvent', 'PostBuildEvent', 'PreLinkEvent']
            for event in events:
                for item in root.iter():
                    if item.tag.endswith(event):
                        cmd_elem = item.find('Command' if not ns else f"{{ {ns['ns']} }}Command")
                        if cmd_elem is None:
                             for child in item:
                                 if child.tag.endswith('Command'):
                                     cmd_elem = child
                                     break
                                     
                        if cmd_elem is not None and cmd_elem.text:
                            cmd = cmd_elem.text.strip()
                            UI.log(f"  [bold yellow]⚠️  {event} Detected in {rel_path}[/bold yellow]")
                            UI.log(f"     Command: [dim]{cmd[:100]}[/dim]")
                            
                            self._analyze_command(cmd, rel_path)
                            
        except (ET.ParseError, OSError, PermissionError) as e:
            # Expected XML parsing errors
            pass
        except Exception as e:
            UI.log(f"  [red]Error parsing {rel_path}: {e}[/red]")
    
    def _scan_linked_libraries(self, root, rel_path, ns):
        """scan for suspicious linked libraries in project configuration"""
        suspicious_libs = {
            'wininet.lib': 3,
            'urlmon.lib': 3,
            'ws2_32.lib': 2,
            'winhttp.lib': 2,
            'crypt32.lib': 2,
            'advapi32.lib': 1,  # common but can be used maliciously
        }
        
        # find additionaldependencies elements
        for item in root.iter():
            if item.tag.endswith('AdditionalDependencies') or item.tag.endswith('AdditionalLibraryDirectories'):
                deps_text = item.text or ""
                for lib_name, score in suspicious_libs.items():
                    if lib_name.lower() in deps_text.lower():
                        self.scanner.add_finding(Finding(
                            category="BUILD_CONFIG",
                            description=f"Suspicious Linked Library: {lib_name}",
                            file=rel_path,
                            line=0,
                            context=f"Library linked in project configuration",
                            score=score,
                            severity=get_severity(score)
                        ))
                        break  # only report once per library per file

    def _analyze_command(self, cmd, file):
        cmd_lower = cmd.lower()
        score = 0
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
            self.scanner.add_finding(Finding(
                category="BUILD_EVENT",
                description=f"Suspicious Build Event ({', '.join(reasons)})",
                file=file,
                line=0,
                context=cmd,
                score=score,
                severity=get_severity(score)
            ))
