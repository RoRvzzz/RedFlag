"""
Scanner Engine - Orchestrates the analysis
"""
import os
import re
import threading
import json
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
        self.show_definitions = show_definitions  # whether to show definitions in output
        self.verbose = False

        # Project-local config (.redflag)
        self.project_config_path = None
        self.ignore_paths = set()   # relative path prefixes to skip
        self.ignore_rules = set()   # rule/description substrings to suppress
        
        # thread safety
        self._lock = threading.Lock()
        self.extracted_images = []
        
        # cached file list (populated once, shared across cogs for efficiency)
        self._cached_files = None
        self._cached_all_files = None  # for metadata scan (includes binaries)
        
        # Compile regexes
        self.compiled_patterns = {}
        for cat, pats in PATTERNS.items():
            self.compiled_patterns[cat] = [(re.compile(p, re.IGNORECASE), s, d) for p, s, d in pats]

        # Load project-local config after defaults are ready
        self._load_project_config()

    def _load_project_config(self):
        """
        Load optional project config from .redflag in the target root.

        Format (JSON):
        {
          "ignore_paths": ["third_party", "src/misc/library"],
          "ignore_rules": ["URL Detected"],
          "custom_rules": [
            {"category": "CUSTOM", "name": "My Rule", "regex": "foo\\s+bar", "score": 2}
          ]
        }
        """
        root = self.target_dir
        cfg_path = os.path.join(root, ".redflag")
        if not os.path.exists(cfg_path):
            return

        self.project_config_path = cfg_path
        try:
            with open(cfg_path, "r", encoding="utf-8", errors="ignore") as f:
                raw = f.read().strip()

            # Keep it dependency-free: JSON only for now
            cfg = json.loads(raw) if raw else {}

            ignore_paths = cfg.get("ignore_paths", []) or []
            ignore_rules = cfg.get("ignore_rules", []) or []
            custom_rules = cfg.get("custom_rules", []) or []

            for p in ignore_paths:
                if not isinstance(p, str) or not p.strip():
                    continue
                norm = p.strip().replace("\\", "/").lstrip("./")
                self.ignore_paths.add(norm.lower())

            for r in ignore_rules:
                if not isinstance(r, str) or not r.strip():
                    continue
                self.ignore_rules.add(r.strip().lower())

            # Compile custom rules into the same pattern engine
            for rule in custom_rules:
                if not isinstance(rule, dict):
                    continue
                regex = rule.get("pattern") or rule.get("regex")
                if not isinstance(regex, str) or not regex.strip():
                    continue
                cat = rule.get("category") or "CUSTOM"
                name = rule.get("name") or "Custom Rule"
                desc = rule.get("description") or name
                score = rule.get("score")
                try:
                    score = int(score) if score is not None else 2
                except Exception:
                    score = 2

                try:
                    compiled = re.compile(regex, re.IGNORECASE)
                except re.error:
                    # Bad custom regex - skip
                    continue

                if cat not in self.compiled_patterns:
                    self.compiled_patterns[cat] = []
                self.compiled_patterns[cat].append((compiled, score, desc))

            if self.verbose:
                UI.log(f"  [dim]Loaded .redflag config: {cfg_path}[/dim]")
        except Exception as e:
            # Don't crash scans because of config parsing; only log in verbose mode
            if self.verbose:
                UI.log(f"  [dim red]Failed to load .redflag: {type(e).__name__}[/dim red]")

    def should_ignore_path(self, rel_path: str) -> bool:
        if not rel_path:
            return False
        if not self.ignore_paths:
            return False
        p = rel_path.replace("\\", "/").lstrip("./").lower()
        return any(p == ign or p.startswith(ign.rstrip("/") + "/") for ign in self.ignore_paths)

    def should_ignore_rule(self, description: str) -> bool:
        if not description:
            return False
        if not self.ignore_rules:
            return False
        d = description.lower()
        return any(r in d for r in self.ignore_rules)
    
    def get_files_to_scan(self, include_binaries=False):
        """get list of files to scan, with caching for efficiency"""
        from .config import IGNORE_DIRS, IGNORE_FILES, SKIP_EXTS
        
        # return cached if available
        if not include_binaries and self._cached_files is not None:
            return self._cached_files
        if include_binaries and self._cached_all_files is not None:
            return self._cached_all_files
        
        files_to_scan = []
        
        # pre-calculate sets for O(1) lookups
        ignore_dirs_set = {d.lower() for d in IGNORE_DIRS}
        skip_exts_set = {ext.lower() for ext in SKIP_EXTS}
        ignore_files_set = {f.lower() for f in IGNORE_FILES}
        
        if self.is_file:
            if include_binaries or not any(self.target_path.lower().endswith(x) for x in SKIP_EXTS):
                files_to_scan.append(self.target_path)
        else:
            # single walk through directory tree
            for root, dirs, files in os.walk(self.target_dir, topdown=True):
                # modify dirs in-place to skip ignored directories immediately
                # This prevents os.walk from even entering '.git' or 'node_modules'
                dirs[:] = [d for d in dirs if d.lower() not in ignore_dirs_set]

                # Apply project-local ignore_paths (relative to target_dir)
                if self.ignore_paths:
                    rel_root = os.path.relpath(root, self.target_dir).replace("\\", "/").lower()
                    # If we're inside an ignored subtree, skip descending further
                    if rel_root != "." and self.should_ignore_path(rel_root):
                        dirs[:] = []
                        continue
                
                for f in files:
                    f_lower = f.lower()
                    if f_lower in ignore_files_set:
                        continue
                    
                    # optimization: check extension using os.path.splitext
                    _, ext = os.path.splitext(f_lower)
                    is_binary_ext = ext in skip_exts_set
                    
                    full_path = os.path.join(root, f)
                    rel_path = os.path.relpath(full_path, self.target_dir).replace("\\", "/").lower()
                    if self.should_ignore_path(rel_path):
                        continue

                    if include_binaries or not is_binary_ext:
                        files_to_scan.append(full_path)
        
        # cache the result
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
        # import cogs here to avoid circular imports if they need engine types, 
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
