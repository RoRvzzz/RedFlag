
from ..core.models import Finding
from ..core.utils import UI

class MitreMappingCog:
    def __init__(self, scanner):
        self.scanner = scanner

    def run(self):
        UI.log("\n[bold white]Step 4: mapping MITRE ATT&CK techniques...[/bold white]")
        
        # Post-process existing findings to add specific MITRE tags based on context
        for finding in self.scanner.findings:
            mitre_tags = []
            
            # --- EXECUTION ---
            if finding.category == 'EXECUTION':
                # T1059.001: PowerShell
                if 'powershell' in finding.context.lower():
                    mitre_tags.append('T1059.001 - PowerShell')
                # T1059.003: Windows Command Shell
                elif 'cmd' in finding.context.lower():
                    mitre_tags.append('T1059.003 - Windows Command Shell')
            
            # --- MEMORY / INJECTION ---
            elif finding.category == 'MEMORY':
                # T1055: Process Injection (Generic)
                if 'WriteProcessMemory' in finding.context:
                    mitre_tags.append('T1055 - Process Injection')
                # T1055.002: Portable Executable Injection (ReflectiveLoader)
                if 'ReflectiveLoader' in finding.context:
                    mitre_tags.append('T1055.002 - Portable Executable Injection')
            
            # --- NETWORK ---
            elif finding.category == 'NETWORK':
                # T1105: Ingress Tool Transfer
                if any(x in finding.context.lower() for x in ['download', 'wget', 'curl', 'bitsadmin']):
                    mitre_tags.append('T1105 - Ingress Tool Transfer')
            
            # --- OBFUSCATION ---
            elif finding.category == 'OBFUSCATION':
                # T1027: Obfuscated Files or Information
                # Only map if it's actual encoding/decoding, not just random strings
                if 'Base64' in finding.description or 'XOR' in finding.description:
                    mitre_tags.append('T1027 - Obfuscated Files or Information')
                
                # T1027.009: Embedded Payload
                if finding.category == 'HIDDEN_PAYLOAD':
                    mitre_tags.append('T1027.009 - Embedded Payload')

            # --- BUILD EVENTS ---
            elif finding.category == 'BUILD_EVENT':
                # T1036: Masquerading (abusing build process to hide execution)
                # T1127: Trusted Developer Utilities Proxy Execution (MSBuild)
                mitre_tags.append('T1127 - Trusted Developer Utilities Proxy Execution')

            # Store tags in metadata
            if mitre_tags:
                finding.metadata['mitre'] = mitre_tags
