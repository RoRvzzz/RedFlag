"""
Scanner Engine - Orchestrates the analysis
"""
import os
import re
from collections import defaultdict
from .config import PATTERNS, IGNORE_FILES
from .models import Finding
from .utils import UI

class RedFlagScanner:
    def __init__(self, target_path):
        self.target_path = os.path.abspath(target_path)
        self.is_file = os.path.isfile(self.target_path)
        self.target_dir = os.path.dirname(self.target_path) if self.is_file else self.target_path
        
        self.findings = []
        self.stats = defaultdict(int)
        self.project_type = "Unknown"
        self.suspicious_build_events = []
        
        # Compile regexes
        self.compiled_patterns = {}
        for cat, pats in PATTERNS.items():
            self.compiled_patterns[cat] = [(re.compile(p, re.IGNORECASE), s, d) for p, s, d in pats]

    def add_finding(self, finding):
        self.findings.append(finding)

    def run(self):
        # Import cogs here to avoid circular imports if they need engine types, 
        # but here we just instantiate them and pass self
        from ..cogs.project_id import ProjectIdentityCog
        from ..cogs.build_scan import BuildScanCog
        from ..cogs.code_scan import CodeScanCog
        from ..cogs.verdict import VerdictCog

        cogs = [
            ProjectIdentityCog(self),
            BuildScanCog(self),
            CodeScanCog(self),
            VerdictCog(self)
        ]

        for cog in cogs:
            cog.run()
