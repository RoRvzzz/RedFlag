
from ..core.models import Finding
from ..core.utils import UI

class MitreMappingCog:
    def __init__(self, scanner):
        self.scanner = scanner
        # Simple mapping for now, can be expanded with mitreattack-python for full data
        self.mapping = {
            'EXECUTION': 'T1059 - Command and Scripting Interpreter',
            'MEMORY': 'T1055 - Process Injection',
            'NETWORK': 'T1105 - Ingress Tool Transfer',
            'CRYPTO': 'T1027 - Obfuscated Files or Information',
            'OBFUSCATION': 'T1027 - Obfuscated Files or Information',
            'BUILD_EVENT': 'T1036 - Masquerading (Build Process)',
            'HIDDEN_PAYLOAD': 'T1027.009 - Embedded Payload',
            'YARA': 'T1204 - User Execution (Malicious File)', # Generic placeholder
            'METADATA': 'T1036 - Masquerading'
        }

    def run(self):
        # Post-process existing findings to add MITRE tags
        for finding in self.scanner.findings:
            if finding.category in self.mapping:
                # Append technique to description if not already present
                tech = self.mapping[finding.category]
                if tech not in finding.description:
                    finding.description = f"{finding.description} [{tech}]"
        
        # We don't print a separate log step here, as it modifies data in place for the Verdict cog to display
