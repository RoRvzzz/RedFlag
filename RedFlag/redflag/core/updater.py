"""
Auto-update functionality for RedFlag
"""
import os
import sys
import json
import shutil
import tempfile
import subprocess
from pathlib import Path
import urllib.request
import urllib.error
from .utils import UI

# GitHub repository info
GITHUB_REPO = "RoRvzzz/RedFlag"
GITHUB_API_URL = f"https://api.github.com/repos/{GITHUB_REPO}/releases/latest"

def get_current_version():
    """Get the current version of RedFlag"""
    try:
        from .. import __version__
        return __version__
    except ImportError:
        return "0.0.0"

def check_for_updates(current_version=None):
    """Check if a newer version is available on GitHub"""
    if current_version is None:
        current_version = get_current_version()
    
    try:
        UI.log("  [dim]Checking for updates...[/dim]")
        
        # Fetch latest release info from GitHub API
        req = urllib.request.Request(GITHUB_API_URL)
        req.add_header('User-Agent', 'RedFlag-Updater/1.0')
        
        with urllib.request.urlopen(req, timeout=5) as response:
            data = json.loads(response.read().decode())
            
        latest_version = data.get('tag_name', '').lstrip('v')
        if not latest_version:
            return None, None
        
        # Compare versions (simple string comparison for semantic versions)
        if _compare_versions(latest_version, current_version) > 0:
            return latest_version, data
        else:
            return None, None
            
    except urllib.error.URLError:
        UI.log("  [dim]Could not check for updates (no internet connection)[/dim]")
        return None, None
    except Exception as e:
        # Silently fail - don't interrupt the scan
        return None, None

def _compare_versions(v1, v2):
    """Compare two version strings. Returns 1 if v1 > v2, -1 if v1 < v2, 0 if equal"""
    def version_tuple(v):
        parts = v.split('.')
        return tuple(int(x) for x in parts if x.isdigit())
    
    try:
        t1 = version_tuple(v1)
        t2 = version_tuple(v2)
        if t1 > t2:
            return 1
        elif t1 < t2:
            return -1
        return 0
    except:
        # Fallback to string comparison
        return 1 if v1 > v2 else (-1 if v1 < v2 else 0)

def download_update(release_data, target_dir=None):
    """Download the latest release from GitHub"""
    try:
        # Find the source code zipball
        zipball_url = release_data.get('zipball_url')
        if not zipball_url:
            UI.log("  [red]No download URL found in release[/red]")
            return False
        
        UI.log(f"  [yellow]Downloading update...[/yellow]")
        
        # Create temp directory
        temp_dir = tempfile.mkdtemp(prefix='redflag_update_')
        zip_path = os.path.join(temp_dir, 'update.zip')
        
        # Download the zip
        req = urllib.request.Request(zipball_url)
        req.add_header('User-Agent', 'RedFlag-Updater/1.0')
        
        with urllib.request.urlopen(req, timeout=30) as response:
            with open(zip_path, 'wb') as f:
                shutil.copyfileobj(response, f)
        
        return zip_path, temp_dir
        
    except Exception as e:
        UI.log(f"  [red]Failed to download update: {e}[/red]")
        return None, None

def install_update(zip_path, temp_dir, install_dir=None):
    """Install the downloaded update"""
    if install_dir is None:
        # Try to find the installation directory
        install_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    
    try:
        import zipfile
        
        UI.log(f"  [yellow]Installing update to {install_dir}...[/yellow]")
        
        # Extract zip
        extract_dir = os.path.join(temp_dir, 'extracted')
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            zip_ref.extractall(extract_dir)
        
        # Find the redflag directory in extracted files
        extracted_root = os.listdir(extract_dir)[0]  # Usually one root folder
        source_redflag = os.path.join(extract_dir, extracted_root, 'redflag')
        
        if not os.path.exists(source_redflag):
            UI.log("  [red]Could not find redflag directory in update[/red]")
            return False
        
        # Backup current installation
        backup_dir = os.path.join(install_dir, 'redflag_backup')
        if os.path.exists(backup_dir):
            shutil.rmtree(backup_dir)
        
        current_redflag = os.path.join(install_dir, 'redflag')
        if os.path.exists(current_redflag):
            shutil.move(current_redflag, backup_dir)
        
        # Copy new version
        shutil.copytree(source_redflag, current_redflag)
        
        # Cleanup backup if successful
        if os.path.exists(backup_dir):
            shutil.rmtree(backup_dir)
        
        UI.log("  [bold green]✓ Update installed successfully![/bold green]")
        UI.log("  [dim]Please restart RedFlag to use the new version.[/dim]")
        return True
        
    except Exception as e:
        UI.log(f"  [red]Failed to install update: {e}[/red]")
        # Try to restore backup
        backup_dir = os.path.join(install_dir, 'redflag_backup')
        current_redflag = os.path.join(install_dir, 'redflag')
        if os.path.exists(backup_dir) and not os.path.exists(current_redflag):
            shutil.move(backup_dir, current_redflag)
            UI.log("  [yellow]Restored previous version from backup[/yellow]")
        return False
    finally:
        # Cleanup temp files
        try:
            if temp_dir and os.path.exists(temp_dir):
                shutil.rmtree(temp_dir)
        except:
            pass

def auto_update(ask_user=True):
    """Check for updates and optionally install them"""
    current_version = get_current_version()
    latest_version, release_data = check_for_updates(current_version)
    
    if latest_version:
        UI.log(f"\n  [bold yellow]⚠ Update Available![/bold yellow]")
        UI.log(f"  Current version: [dim]{current_version}[/dim]")
        UI.log(f"  Latest version:  [bold green]{latest_version}[/bold green]")
        
        if release_data:
            release_notes = release_data.get('body', '')[:200]  # First 200 chars
            if release_notes:
                UI.log(f"  [dim]{release_notes}...[/dim]")
        
        if ask_user:
            try:
                response = input("\n  [yellow]Would you like to update now? (y/n): [/yellow]").strip().lower()
                if response in ['y', 'yes']:
                    zip_path, temp_dir = download_update(release_data)
                    if zip_path:
                        success = install_update(zip_path, temp_dir)
                        if success:
                            return True
            except KeyboardInterrupt:
                UI.log("\n  [dim]Update cancelled.[/dim]")
                return False
        else:
            # Auto-update without asking
            zip_path, temp_dir = download_update(release_data)
            if zip_path:
                return install_update(zip_path, temp_dir)
    
    return False
