
import os
import re
from ..core.utils import UI

class ProjectIdentityCog:
    def __init__(self, scanner):
        self.scanner = scanner
        # Store referenced projects to scan specifically later
        self.scanner.project_files = []

    def run(self):
        UI.log("\n[bold white]Step 1: analyzing target...[/bold white]")
        
        if not os.path.exists(self.scanner.target_path):
            UI.log("[bold red]Error: Path does not exist.[/bold red]")
            return

        if self.scanner.is_file:
            self.scanner.project_type = "Single File"
            icon = "📄"
            UI.log(f"  {icon} Analyzing single file: [bold cyan]{os.path.basename(self.scanner.target_path)}[/bold cyan]")
            return

        # Search for .sln files recursively and also check for .vs folders
        sln_files = []
        vs_folders = []
        
        for root, dirs, files in os.walk(self.scanner.target_dir):
            # Check for .vs folder (Visual Studio cache folder - indicates VS project)
            if '.vs' in dirs:
                vs_folders.append(os.path.join(root, '.vs'))
            
            # Check for .sln files
            for f in files:
                if f.endswith('.sln'):
                    sln_files.append(os.path.join(root, f))
        
        # Also check root directory
        root_files = os.listdir(self.scanner.target_dir)
        has_vcxproj = any(f.endswith('.vcxproj') for f in root_files)
        has_makefile = 'Makefile' in root_files
        has_git = '.git' in root_files
        
        # If we found .sln files, use the first one (or closest to root)
        if sln_files:
            # Sort by depth (prefer files closer to root)
            sln_files.sort(key=lambda x: x.count(os.sep))
            self.scanner.project_type = "Visual Studio Solution"
            icon = "🔷"
            # Parse SLN to find projects
            self._parse_sln_files(sln_files)
        elif vs_folders:
            # Found .vs folder but no .sln in root - might be in subfolder
            self.scanner.project_type = "Visual Studio Project (detected via .vs folder)"
            icon = "🔷"
        elif has_vcxproj:
            self.scanner.project_type = "Visual Studio Project"
            icon = "🔷"
        elif has_makefile:
            self.scanner.project_type = "Make/C++"
            icon = "🔧"
        else:
            self.scanner.project_type = "Generic Folder"
            icon = "📁"
            
        UI.log(f"  {icon} Type identified: [bold cyan]{self.scanner.project_type}[/bold cyan]")
        if has_git:
            UI.log("  🌲 Git repository detected")

    def _parse_sln_files(self, sln_file_paths):
        """Parse .sln files to find all referenced projects"""
        for sln_path in sln_file_paths:
            try:
                # Get the directory containing the .sln file (solution root)
                sln_dir = os.path.dirname(sln_path)
                
                with open(sln_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                
                # Regex to find Project entries
                # Project("{...}") = "Name", "Path\To\Project.vcxproj", "{...}"
                matches = re.findall(r'Project\("{.*?}"\)\s*=\s*".*?",\s*"(.*?)",', content)
                
                for match in matches:
                    # Normalize path separators and handle Windows backslashes
                    rel_proj_path = match.replace('\\', os.sep).replace('/', os.sep)
                    
                    # Security: Prevent path traversal attacks
                    if '..' in rel_proj_path:
                        # Skip paths with .. traversal
                        continue
                    
                    # Sanitize: Remove any leading/trailing whitespace and normalize
                    rel_proj_path = rel_proj_path.strip()
                    
                    # Join relative to the .sln file's directory
                    full_proj_path = os.path.normpath(os.path.join(sln_dir, rel_proj_path))
                    
                    # Security: Ensure resolved path is still within solution directory
                    try:
                        # Resolve any remaining .. or . components
                        full_proj_path = os.path.abspath(full_proj_path)
                        sln_dir_abs = os.path.abspath(sln_dir)
                        
                        # Check that the resolved path is within or at the solution directory
                        if not full_proj_path.startswith(sln_dir_abs):
                            # Path traversal detected, skip
                            continue
                    except (OSError, ValueError):
                        # Invalid path, skip
                        continue
                    
                    if os.path.exists(full_proj_path) and full_proj_path.endswith('.vcxproj'):
                        if full_proj_path not in self.scanner.project_files:
                            self.scanner.project_files.append(full_proj_path)
                            
                if self.scanner.project_files:
                    UI.log(f"  📌 Found {len(self.scanner.project_files)} referenced projects in solution.")
            except Exception as e:
                UI.log(f"  [yellow]Warning: Failed to parse SLN file {os.path.basename(sln_path)}: {e}[/yellow]")
