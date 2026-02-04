"""
Luckyware/Darkside Remediation Module
Checks for system-wide infection and offers remediation.
"""
import os
import sys
import ctypes
import shutil
import re
from pathlib import Path
from .utils import UI

LUCKYWARE_DOMAINS = [
    "i-like.boats",
    "devruntime.cy",
    "zetolacs-cloud.top",
    "frozi.cc",
    "exo-api.tf",
    "nuzzyservices.com",
    "darkside.cy",
    "balista.lol",
    "phobos.top",
    "phobosransom.com",
    "pee-files.nl",
    "vcc-library.uk",
    "luckyware.co",
    "luckyware.cc",
    "91.92.243.218",
    "dhszo.darkside.cy",
    "188.114.96.11",
    "risesmp.net",
    "luckystrike.pw",
    "krispykreme.top",
    "vcc-redistrbutable.help",
    "i-slept-with-ur.mom"
]

def is_admin():
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except:
        return False

def check_hosts_file(fix=False):
    """
    Checks if the hosts file contains the blocking entries.
    If fix=True, appends them.
    """
    hosts_path = r"C:\Windows\System32\drivers\etc\hosts"
    if not os.path.exists(hosts_path):
        UI.log(f"  [yellow]System hosts file not found at {hosts_path}[/yellow]")
        return False

    try:
        with open(hosts_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
    except PermissionError:
        UI.log("  [red]Permission denied reading hosts file. Run as Admin.[/red]")
        return False

    missing_domains = [d for d in LUCKYWARE_DOMAINS if d not in content]
    
    if not missing_domains:
        UI.log("  [green]✓ Hosts file already blocks Luckyware domains.[/green]")
        return True
    
    UI.log(f"  [red]! Hosts file is missing blocks for {len(missing_domains)} Luckyware domains.[/red]")
    
    if fix:
        if not is_admin():
            UI.log("  [red]Cannot update hosts file: Admin privileges required.[/red]")
            return False
            
        try:
            with open(hosts_path, 'a', encoding='utf-8') as f:
                f.write("\n# Blocked Luckyware/Darkside domains\n")
                for domain in missing_domains:
                    f.write(f"0.0.0.0 {domain}\n")
            UI.log("  [green]✓ Successfully updated hosts file.[/green]")
            return True
        except PermissionError:
            UI.log("  [red]Failed to write to hosts file (Permission Denied).[/red]")
        except Exception as e:
            UI.log(f"  [red]Error writing to hosts file: {e}[/red]")
            
    return False

def check_windows_kits(fix=False):
    """
    Checks Windows Kits winnetwk.h for VCCHelper infection.
    """
    base_path = r"C:\Program Files (x86)\Windows Kits\10\Include"
    if not os.path.exists(base_path):
        # UI.log("  [dim]Windows Kits not found in standard location.[/dim]")
        return True # Not infected if not present

    infected_files = []
    
    # Iterate through SDK versions
    for version in os.listdir(base_path):
        ver_path = os.path.join(base_path, version)
        if not os.path.isdir(ver_path):
            continue
        
        target_file = os.path.join(ver_path, "um", "winnetwk.h")
        if os.path.exists(target_file):
            try:
                with open(target_file, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    
                if "VCCHelper" in content or "VCCLibraries_" in content:
                    infected_files.append(target_file)
            except PermissionError:
                UI.log(f"  [red]Permission denied reading {target_file}[/red]")

    if not infected_files:
        UI.log("  [green]✓ No infection found in Windows Kits (winnetwk.h).[/green]")
        return True

    for fpath in infected_files:
        UI.log(f"  [bold red]!!! INFECTED FILE FOUND: {fpath}[/bold red]")

    if fix:
        if not is_admin():
            UI.log("  [red]Cannot clean Windows Kits: Admin privileges required.[/red]")
            return False

        success = True
        for fpath in infected_files:
            try:
                # Backup first
                shutil.copy2(fpath, fpath + ".bak")
                
                with open(fpath, 'r', encoding='utf-8', errors='ignore') as f:
                    lines = f.readlines()
                
                # Remove infected lines (usually at the end)
                # Heuristic: Remove lines containing VCCHelper or the namespace
                clean_lines = [
                    line for line in lines 
                    if "VCCHelper" not in line 
                    and "VCCLibraries_" not in line 
                    and "Fwfkuuv157" not in line
                    and "Rundolayyyy" not in line
                ]
                
                if len(clean_lines) < len(lines):
                    with open(fpath, 'w', encoding='utf-8') as f:
                        f.writelines(clean_lines)
                    UI.log(f"  [green]✓ Cleaned {fpath}[/green]")
                
            except Exception as e:
                UI.log(f"  [red]Failed to clean {fpath}: {e}[/red]")
                success = False
        return success
        
    return False

def check_project_files(target_dir):
    """
    Checks specifically for imgui_impl_win32.cpp infection and malicious .vcxproj commands.
    """
    UI.log("\n[bold white]Checking project-specific files...[/bold white]")
    
    # 1. Check imgui
    for root, _, files in os.walk(target_dir):
        for file in files:
            if file.lower() == "imgui_impl_win32.cpp":
                full_path = os.path.join(root, file)
                try:
                    with open(full_path, 'r', encoding='utf-8', errors='ignore') as f:
                        # check first 4k bytes (header usually)
                        head = f.read(4096)
                        if "Fwfkuuv157wg2gjthwla0lwbo1493h7" in head or "VCCLibraries_" in head:
                            UI.log(f"  [bold red]!!! INFECTED SOURCE: {full_path}[/bold red]")
                            UI.log("  [yellow]Action: Delete or restore this file from a clean source.[/yellow]")
                except Exception:
                    pass

            if file.lower().endswith(".vcxproj"):
                full_path = os.path.join(root, file)
                try:
                    with open(full_path, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()
                        if "cmd.exe" in content and "powershell" in content and "Hidden" in content and "iwr" in content:
                            UI.log(f"  [bold red]!!! INFECTED PROJECT: {full_path}[/bold red]")
                            UI.log("  [yellow]Action: Remove malicious PreBuild/PostBuild events.[/yellow]")
                except Exception:
                    pass

def cleanup_artifacts(target_dir, perform_delete=False):
    """
    Finds and optionally deletes .vs and .suo files/folders.
    """
    targets = []
    for root, dirs, files in os.walk(target_dir):
        # check dirs
        for d in list(dirs):
            if d.lower() == ".vs":
                targets.append(os.path.join(root, d))
        # check files
        for f in files:
            if f.lower().endswith(".suo"):
                targets.append(os.path.join(root, f))
    
    if targets:
        UI.log(f"\n[bold white]Found {len(targets)} Visual Studio artifacts (.vs, .suo) to clean:[/bold white]")
        for t in targets[:5]:
             UI.log(f"  - {t}")
        if len(targets) > 5:
            UI.log(f"  - ... and {len(targets)-5} more")
            
        if perform_delete:
            UI.log("  [yellow]Cleaning artifacts...[/yellow]")
            for t in targets:
                try:
                    if os.path.isdir(t):
                        shutil.rmtree(t)
                    else:
                        os.remove(t)
                except Exception as e:
                    UI.log(f"  [red]Failed to delete {t}: {e}[/red]")
            UI.log("  [green]Artifacts cleaned.[/green]")
        else:
            UI.log("  [dim]Skipping deletion (use --fix-system to clean)[/dim]")
    else:
        UI.log("\n  [green]✓ No .vs/.suo artifacts found.[/green]")

def run_system_check(target_dir=None, interactive=True):
    """
    Runs the full Luckyware system check suite.
    """
    UI.print_panel("Checking for Luckyware/Darkside Infection...", "System Check")
    
    admin = is_admin()
    if not admin:
        UI.log("[yellow]Running without Admin privileges. Some checks/fixes may be limited.[/yellow]")
        if interactive:
            # On Windows, we can't easily prompt to escalate within the python script 
            # without restarting. We'll just warn.
            pass

    # 1. Hosts File
    UI.log("\n[bold white]Checking Hosts File...[/bold white]")
    hosts_ok = check_hosts_file()
    
    # 2. Windows Kits
    UI.log("\n[bold white]Checking Windows Kits (winnetwk.h)...[/bold white]")
    kits_ok = check_windows_kits()

    # Determine if remediation is needed
    needs_fix = not hosts_ok or not kits_ok
    
    if needs_fix:
        if interactive:
            UI.log("\n[bold yellow]Infection traces or vulnerabilities found.[/bold yellow]")
            if not admin:
                UI.log("[red]You must restart RedFlag as Administrator to perform fixes.[/red]")
            else:
                response = input("  Do you want to attempt automatic remediation (Hosts + SDK)? [y/N] ").strip().lower()
                if response == 'y':
                    check_hosts_file(fix=True)
                    check_windows_kits(fix=True)
        else:
            # If not interactive but needs fix, we can't do much unless a --fix flag was passed
            # handled by caller usually
            pass

    # 3. Workspace checks (if target provided)
    if target_dir:
        check_project_files(target_dir)
        cleanup_artifacts(target_dir, perform_delete=True if interactive else False)

