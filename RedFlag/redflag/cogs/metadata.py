
import os
import hashlib
import filetype
from concurrent.futures import ThreadPoolExecutor, as_completed
from ..core.models import Finding
from ..core.utils import UI, get_severity
from ..core.config import SKIP_EXTS, IGNORE_DIRS, IGNORE_FILES

class MetadataScanCog:
    def __init__(self, scanner):
        self.scanner = scanner

    def run(self):
        UI.log("\n[bold white]Step 2b: extracting metadata...[/bold white]")
        
        # Use cached file list (metadata scan includes binaries)
        files_to_scan = self.scanner.get_files_to_scan(include_binaries=True)

        if not files_to_scan:
            return

        # Use multi-threading for faster scanning
        max_workers = min(8, len(files_to_scan))
        
        progress = UI.get_progress()
        if progress:
            with progress:
                task = progress.add_task(f"Extracting Metadata for {len(files_to_scan)} files...", total=len(files_to_scan))
                with ThreadPoolExecutor(max_workers=max_workers) as executor:
                    futures = {executor.submit(self._scan_file, f): f for f in files_to_scan}
                    for future in as_completed(futures):
                        progress.advance(task)
        else:
            with ThreadPoolExecutor(max_workers=max_workers) as executor:
                executor.map(self._scan_file, files_to_scan)

    def _scan_file(self, path):
        try:
            rel_base = self.scanner.target_dir if not self.scanner.is_file else os.path.dirname(self.scanner.target_dir)
            rel_path = os.path.relpath(path, rel_base)
            
            # 1. File Type Detection (Magic Numbers)
            kind = filetype.guess(path)
            file_type = "Unknown"
            mime = "application/octet-stream"
            
            if kind:
                file_type = kind.extension
                mime = kind.mime
            
            # 2. Hashing (SHA256)
            sha256_hash = self._get_file_hash(path)
            
            # Store metadata in stats or special findings?
            # Flag if file extension doesn't match detected type (but DLLs detected as exe is normal - both are PE)
            if file_type == 'exe' and path.lower().endswith('.dll'):
                # DLL detected as exe is normal (both are PE files), skip
                pass
            elif file_type in ['exe', 'dll', 'elf'] and not path.lower().endswith(f".{file_type}"):
                self.scanner.add_finding(Finding(
                    category="METADATA",
                    description=f"Mismatched File Extension (Detected: {file_type}, MIME: {mime})",
                    file=rel_path,
                    line=0,
                    context=f"SHA256: {sha256_hash}",
                    score=3,
                    severity="MEDIUM"
                ))
                
            # Log binary files in source directories (but skip DLLs in common locations)
            if file_type in ['exe', 'dll', 'elf'] and not any(x in rel_path.lower() for x in ['bin', 'obj', 'lib', 'dll']):
                 self.scanner.add_finding(Finding(
                    category="METADATA",
                    description=f"Suspicious Binary in Source Directory ({file_type})",
                    file=rel_path,
                    line=0,
                    context=f"SHA256: {sha256_hash}",
                    score=2,
                    severity="LOW"
                ))

        except (OSError, PermissionError, UnicodeError) as e:
            # Expected file access errors - silently skip
            pass
        except Exception as e:
            # Log unexpected errors for debugging
            if hasattr(self.scanner, 'verbose') and self.scanner.verbose:
                UI.log(f"  [dim red]Error extracting metadata from {rel_path}: {type(e).__name__}[/dim red]")
            pass

    def _get_file_hash(self, path):
        sha256 = hashlib.sha256()
        try:
            with open(path, "rb") as f:
                for chunk in iter(lambda: f.read(4096), b""):
                    sha256.update(chunk)
            return sha256.hexdigest()
        except (OSError, PermissionError, IOError):
            return "ERROR"
