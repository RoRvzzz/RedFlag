#!/usr/bin/env python3
import sys
import os
import argparse

# Ensure the current directory is in the Python path so we can import redflag
script_dir = os.path.dirname(os.path.abspath(__file__))
if script_dir not in sys.path:
    sys.path.insert(0, script_dir)

from redflag.core.engine import RedFlagScanner
from redflag.core.config import BANNER
from redflag.core.utils import UI
from redflag.core.updater import auto_update, get_current_version

def main():
    parser = argparse.ArgumentParser(description="RedFlag Malware Analysis")
    parser.add_argument("path", nargs="?", help="Path to project or file")
    parser.add_argument("--skip-update", action="store_true", help="Skip update check")
    parser.add_argument("--auto-update", action="store_true", help="Automatically install updates without asking")
    parser.add_argument("--version", action="store_true", help="Show version and exit")
    parser.add_argument("--no-definitions", action="store_true", help="Hide technical definitions in output")
    args = parser.parse_args()
    
    # Show version and exit
    if args.version:
        version = get_current_version()
        print(f"RedFlag v{version}")
        return
    
    # Check for updates (unless skipped)
    if not args.skip_update:
        try:
            if args.auto_update:
                auto_update(ask_user=False, prefer_pip=True)
            else:
                auto_update(ask_user=True, prefer_pip=True)
        except Exception as e:
            # Don't fail if update check fails, but log for debugging
            pass
    
    path = args.path
    if not path:
        UI.print_banner(BANNER)
        try:
            path = input("Enter project path to scan: ").strip('"').strip("'")
        except KeyboardInterrupt:
            return
        
    if not path:
        print("No path provided.")
        return

    scanner = RedFlagScanner(path, show_definitions=not args.no_definitions)
    try:
        version = get_current_version()
        UI.print_banner(BANNER)
        UI.log(f"  [dim]RedFlag v{version}[/dim]\n")
        UI.print_panel(f"Target: [bold cyan]{scanner.target_path}[/bold cyan]", title="Initialization")
        scanner.run()
    except KeyboardInterrupt:
        print("\nScan aborted.")

if __name__ == "__main__":
    main()
