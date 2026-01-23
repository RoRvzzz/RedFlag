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
GITHUB_REPO_API_URL = f"https://api.github.com/repos/{GITHUB_REPO}"

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
        
        # First, try to get the latest release
        req = urllib.request.Request(GITHUB_API_URL)
        req.add_header('User-Agent', 'RedFlag-Updater/1.0')
        
        try:
            with urllib.request.urlopen(req, timeout=10) as response:
                if response.status == 200:
                    data = json.loads(response.read().decode())
                    # Strip both 'v' and 'V' from tag name
                    latest_version = data.get('tag_name', '').lstrip('vV')
                    
                    if latest_version:
                        comparison = _compare_versions(latest_version, current_version)
                        if comparison > 0:
                            return latest_version, data
                        elif comparison == 0:
                            # Versions are equal - no update needed
                            return None, None
        except urllib.error.HTTPError as e:
            if e.code == 404:
                # No releases found - check if we can get version from repo default branch
                # This handles the case where code is updated but no release is created
                try:
                    repo_req = urllib.request.Request(f"{GITHUB_REPO_API_URL}/contents/redflag/__init__.py")
                    repo_req.add_header('User-Agent', 'RedFlag-Updater/1.0')
                    with urllib.request.urlopen(repo_req, timeout=10) as repo_response:
                        if repo_response.status == 200:
                            repo_data = json.loads(repo_response.read().decode())
                            # Try to get version from file content (base64 encoded)
                            import base64
                            file_content = base64.b64decode(repo_data.get('content', '')).decode('utf-8')
                            # Extract version from __version__ = "x.x.x"
                            import re
                            version_match = re.search(r'__version__\s*=\s*["\']([^"\']+)["\']', file_content)
                            if version_match:
                                repo_version = version_match.group(1)
                                comparison = _compare_versions(repo_version, current_version)
                                if comparison > 0:
                                    # Create a mock release data structure
                                    mock_release = {
                                        'tag_name': f'v{repo_version}',
                                        'body': 'Update available from repository',
                                        'zipball_url': f"https://github.com/{GITHUB_REPO}/archive/refs/heads/main.zip"
                                    }
                                    return repo_version, mock_release
                except:
                    pass
            return None, None
            
    except urllib.error.URLError as e:
        UI.log(f"  [dim]Could not check for updates: {e}[/dim]")
        return None, None
    except json.JSONDecodeError as e:
        UI.log(f"  [dim]Failed to parse update response[/dim]")
        return None, None
    except Exception as e:
        # Log the error for debugging but don't interrupt the scan
        UI.log(f"  [dim]Update check failed: {type(e).__name__}[/dim]")
        return None, None
    
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
            return None, None
        
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

def install_update_pip(git_url, tag_name):
    """
    Use pip to update the package directly from git.
    This handles file locking and permission errors much better than manual file moves.
    """
    UI.log("  [yellow]Installing update via pip...[/yellow]")
    try:
        # Install directly from the repo's tag
        # Format: git+https://github.com/User/Repo.git@tag
        result = subprocess.run(
            [sys.executable, "-m", "pip", "install", "--upgrade", "--force-reinstall", git_url],
            capture_output=True,
            text=True,
            timeout=300
        )
        
        if result.returncode == 0:
            UI.log("  [bold green]✓ Update installed successfully![/bold green]")
            UI.log("  [dim]Please restart RedFlag to use the new version.[/dim]")
            return True
        else:
            UI.log(f"  [red]Update failed: {result.stderr}[/red]")
            UI.log("  [dim]Try running the tool as Administrator or use manual update method.[/dim]")
            return False
    except subprocess.TimeoutExpired:
        UI.log("  [red]Update timed out. Please try again.[/red]")
        return False
    except Exception as e:
        UI.log(f"  [red]Update failed: {e}[/red]")
        return False

def install_update(zip_path, temp_dir, install_dir=None):
    """
    Install the downloaded update using manual file copy.
    NOTE: On Windows, this may fail if files are locked. Use pip method instead.
    """
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
        
        # 1. Find the root folder extracted from GitHub
        extracted_root_name = os.listdir(extract_dir)[0]
        extracted_root_path = os.path.join(extract_dir, extracted_root_name)
        
        # 2. INTELLIGENT PATH FINDING
        # Check if 'redflag' is a subfolder (Common case)
        potential_source = os.path.join(extracted_root_path, 'redflag')
        potential_assets = os.path.join(extracted_root_path, 'assets')
        
        if os.path.exists(potential_source):
            source_redflag = potential_source
            source_assets = potential_assets
        else:
            # Fallback: Maybe the root itself IS the package?
            # Check if __init__.py exists in the root
            if os.path.exists(os.path.join(extracted_root_path, '__init__.py')):
                source_redflag = extracted_root_path
                source_assets = None  # Assets might be at root level
            else:
                UI.log(f"  [red]Error: Could not find valid package structure in update.[/red]")
                UI.log(f"  [dim]Checked: {potential_source}[/dim]")
                UI.log(f"  [dim]Root path: {extracted_root_path}[/dim]")
                return False
        
        if not os.path.exists(source_redflag):
            UI.log("  [red]Could not find redflag directory in update[/red]")
            return False
        
        # On Windows, we can't move files that are currently in use
        # So we'll copy to a staging area and instruct user to restart
        current_redflag = os.path.join(install_dir, 'redflag')
        current_assets = os.path.join(install_dir, 'assets')
        
        # Create staging directory for new files
        staging_dir = os.path.join(install_dir, 'redflag_update_staging')
        if os.path.exists(staging_dir):
            shutil.rmtree(staging_dir)
        
        # Copy new version to staging area
        staging_redflag = os.path.join(staging_dir, 'redflag')
        shutil.copytree(source_redflag, staging_redflag)
        
        # Copy assets folder if it exists in the update
        if source_assets and os.path.exists(source_assets):
            staging_assets = os.path.join(staging_dir, 'assets')
            shutil.copytree(source_assets, staging_assets)
        
        # Verify critical __init__.py files exist
        critical_files = [
            os.path.join(staging_redflag, '__init__.py'),
            os.path.join(staging_redflag, 'core', '__init__.py'),
            os.path.join(staging_redflag, 'cogs', '__init__.py')
        ]
        for critical_file in critical_files:
            if not os.path.exists(critical_file):
                UI.log(f"  [yellow]Warning: Missing {os.path.basename(critical_file)} - creating it[/yellow]")
                os.makedirs(os.path.dirname(critical_file), exist_ok=True)
                with open(critical_file, 'w') as f:
                    f.write('')
        
        UI.log("  [bold yellow]⚠ Update files prepared in staging area.[/bold yellow]")
        UI.log("  [yellow]Due to Windows file locking, please restart RedFlag to complete the update.[/yellow]")
        UI.log(f"  [dim]Staging directory: {staging_dir}[/dim]")
        UI.log("  [dim]On next startup, the update will be applied automatically.[/dim]")
        
        # Create a flag file to indicate update is pending
        update_flag = os.path.join(install_dir, '.redflag_update_pending')
        with open(update_flag, 'w') as f:
            f.write(staging_dir)
        
        return True
        
    except PermissionError as e:
        UI.log(f"  [red]Permission denied: {e}[/red]")
        UI.log("  [yellow]Try running RedFlag as Administrator, or use the pip update method.[/yellow]")
        return False
    except Exception as e:
        UI.log(f"  [red]Failed to install update: {e}[/red]")
        # Try to restore backup
        backup_dir = os.path.join(install_dir, 'redflag_backup')
        current_redflag = os.path.join(install_dir, 'redflag')
        if os.path.exists(backup_dir) and not os.path.exists(current_redflag):
            shutil.move(backup_dir, current_redflag)
            UI.log("  [yellow]Restored previous version from backup[/yellow]")
        return False

def auto_update(ask_user=True, prefer_pip=True):
    """
    Check for updates and optionally install them
    
    Args:
        ask_user: Whether to prompt the user before updating
        prefer_pip: If True, try pip method first (recommended). If False, use manual zip method.
    """
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
                    if prefer_pip:
                        # Try pip method first
                        git_url = f"git+https://github.com/{GITHUB_REPO}.git@{release_data.get('tag_name', 'main')}"
                        if install_update_pip(git_url, release_data.get('tag_name', 'main')):
                            return True
                        # Fallback to manual method
                        UI.log("  [yellow]Pip update failed, trying manual method...[/yellow]")
                    
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
            if prefer_pip:
                # Try pip method first
                git_url = f"git+https://github.com/{GITHUB_REPO}.git@{release_data.get('tag_name', 'main')}"
                if install_update_pip(git_url, release_data.get('tag_name', 'main')):
                    return True
                # Fallback to manual method
                UI.log("  [yellow]Pip update failed, trying manual method...[/yellow]")
            
            zip_path, temp_dir = download_update(release_data)
            if zip_path:
                return install_update(zip_path, temp_dir)
    
    return False

def apply_pending_update():
    """
    Check for and apply any pending updates from the staging directory.
    This is called on startup to complete updates that were staged but not applied
    due to Windows file locking.
    """
    install_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    update_flag = os.path.join(install_dir, '.redflag_update_pending')
    
    if not os.path.exists(update_flag):
        return False
    
    try:
        # Read staging directory path from flag file
        with open(update_flag, 'r') as f:
            staging_dir = f.read().strip()
        
        if not os.path.exists(staging_dir):
            # Staging directory doesn't exist, remove flag
            os.remove(update_flag)
            return False
        
        UI.log("  [yellow]Applying pending update...[/yellow]")
        
        current_redflag = os.path.join(install_dir, 'redflag')
        current_assets = os.path.join(install_dir, 'assets')
        staging_redflag = os.path.join(staging_dir, 'redflag')
        staging_assets = os.path.join(staging_dir, 'assets')
        
        # Backup current installation
        backup_dir = os.path.join(install_dir, 'redflag_backup')
        if os.path.exists(backup_dir):
            shutil.rmtree(backup_dir)
        os.makedirs(backup_dir, exist_ok=True)
        
        # Backup current files
        if os.path.exists(current_redflag):
            backup_redflag = os.path.join(backup_dir, 'redflag')
            shutil.move(current_redflag, backup_redflag)
        
        if os.path.exists(current_assets):
            backup_assets = os.path.join(backup_dir, 'assets')
            shutil.move(current_assets, backup_assets)
        
        # Move staged files to actual location
        shutil.move(staging_redflag, current_redflag)
        
        if os.path.exists(staging_assets):
            if os.path.exists(current_assets):
                shutil.rmtree(current_assets)
            shutil.move(staging_assets, current_assets)
        
        # Cleanup staging directory and flag
        shutil.rmtree(staging_dir)
        os.remove(update_flag)
        
        # Cleanup backup
        if os.path.exists(backup_dir):
            shutil.rmtree(backup_dir)
        
        UI.log("  [bold green]✓ Update applied successfully![/bold green]")
        UI.log("  [dim]Please restart RedFlag to use the new version.[/dim]")
        return True
        
    except PermissionError as e:
        UI.log(f"  [yellow]Could not apply update: {e}[/yellow]")
        UI.log("  [dim]Update will be applied on next restart.[/dim]")
        return False
    except Exception as e:
        UI.log(f"  [dim]Could not apply pending update: {e}[/dim]")
        # Don't remove flag if update failed - try again next time
        return False
