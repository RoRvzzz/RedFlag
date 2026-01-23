"""
Scanner Engine - Orchestrates the analysis
"""
import os
import re
import threading
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
        
        # Thread safety
        self._lock = threading.Lock()
        self.extracted_images = []
        
        # Compile regexes
        self.compiled_patterns = {}
        for cat, pats in PATTERNS.items():
            self.compiled_patterns[cat] = [(re.compile(p, re.IGNORECASE), s, d) for p, s, d in pats]

    def add_finding(self, finding):
        with self._lock:
            self.findings.append(finding)
    
    def add_extracted_image(self, image_info):
        with self._lock:
            self.extracted_images.append(image_info)

    def run(self):
        # Import cogs here to avoid circular imports if they need engine types, 
        # but here we just instantiate them and pass self
        from ..cogs.project_id import ProjectIdentityCog
        from ..cogs.build_scan import BuildScanCog
        from ..cogs.metadata import MetadataScanCog
        from ..cogs.code_scan import CodeScanCog
        from ..cogs.yara_scan import YaraScanCog
        from ..cogs.strings import StringAnalysisCog
        from ..cogs.mitre import MitreMappingCog
        from ..cogs.verdict import VerdictCog

        cogs = [
            ProjectIdentityCog(self),      # Step 1
            BuildScanCog(self),            # Step 2
            MetadataScanCog(self),         # Step 2b
            CodeScanCog(self),             # Step 3
            YaraScanCog(self),             # Step 3a
            StringAnalysisCog(self),       # Step 3b
            MitreMappingCog(self),         # Step 4
            VerdictCog(self)               # Step 5
        ]

        for cog in cogs:
            cog.run()
