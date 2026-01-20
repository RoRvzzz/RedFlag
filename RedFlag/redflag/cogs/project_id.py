"""
Cog: Project Identification
"""
import os
from ..core.utils import UI

class ProjectIdentityCog:
    def __init__(self, scanner):
        self.scanner = scanner

    def run(self):
        UI.log("\n[bold white]Step 1: Analyzing Target...[/bold white]")
        
        if not os.path.exists(self.scanner.target_path):
            UI.log("[bold red]❌ Error: Path does not exist.[/bold red]")
            return

        if self.scanner.is_file:
            self.scanner.project_type = "Single File"
            icon = "📄"
            UI.log(f"  {icon} Analyzing single file: [bold cyan]{os.path.basename(self.scanner.target_path)}[/bold cyan]")
            return

        files = os.listdir(self.scanner.target_dir)
        has_sln = any(f.endswith('.sln') for f in files)
        has_vcxproj = any(f.endswith('.vcxproj') for f in files)
        has_makefile = 'Makefile' in files
        has_git = '.git' in files
        
        if has_sln or has_vcxproj:
            self.scanner.project_type = "Visual Studio C++"
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
