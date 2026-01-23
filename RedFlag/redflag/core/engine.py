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
    def __init__(self, target_path, show_definitions=True):
        self.target_path = os.path.abspath(target_path)
        self.is_file = os.path.isfile(self.target_path)
        self.target_dir = os.path.dirname(self.target_path) if self.is_file else self.target_path
        
        self.findings = []
        self.stats = defaultdict(int)
        self.project_type = "Unknown"
        self.suspicious_build_events = []
        self.show_definitions = show_definitions  # Whether to show definitions in output
        
        # Thread safety
        self._lock = threading.Lock()
        self.extracted_images = []
        
        # Cached file list (populated once, shared across cogs for efficiency)
        self._cached_files = None
        self._cached_all_files = None  # For metadata scan (includes binaries)
        
        # Compile regexes
        self.compiled_patterns = {}
        for cat, pats in PATTERNS.items():
            self.compiled_patterns[cat] = [(re.compile(p, re.IGNORECASE), s, d) for p, s, d in pats]
    
    def get_files_to_scan(self, include_binaries=False):
        """Get list of files to scan, with caching for efficiency"""
        from .config import IGNORE_DIRS, IGNORE_FILES, SKIP_EXTS
        
        # Return cached if available
        if not include_binaries and self._cached_files is not None:
            return self._cached_files
        if include_binaries and self._cached_all_files is not None:
            return self._cached_all_files
        
        files_to_scan = []
        
        # Pre-calculate sets for O(1) lookups
        ignore_dirs_set = {d.lower() for d in IGNORE_DIRS}
        skip_exts_set = {ext.lower() for ext in SKIP_EXTS}
        ignore_files_set = {f.lower() for f in IGNORE_FILES}
        
        if self.is_file:
            if include_binaries or not any(self.target_path.lower().endswith(x) for x in SKIP_EXTS):
                files_to_scan.append(self.target_path)
        else:
            # Single walk through directory tree
            for root, dirs, files in os.walk(self.target_dir, topdown=True):
                # Modify dirs in-place to skip ignored directories immediately
                # This prevents os.walk from even entering '.git' or 'node_modules'
                dirs[:] = [d for d in dirs if d.lower() not in ignore_dirs_set]
                
                for f in files:
                    f_lower = f.lower()
                    if f_lower in ignore_files_set:
                        continue
                    
                    # Optimization: Check extension using os.path.splitext
                    _, ext = os.path.splitext(f_lower)
                    is_binary_ext = ext in skip_exts_set
                    
                    if include_binaries or not is_binary_ext:
                        files_to_scan.append(os.path.join(root, f))
        
        # Cache the result
        if include_binaries:
            self._cached_all_files = files_to_scan
        else:
            self._cached_files = files_to_scan
        
        return files_to_scan

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
