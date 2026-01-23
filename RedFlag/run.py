#!/usr/bin/env python3
import sys
import argparse
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
                auto_update(ask_user=False)
            else:
                auto_update(ask_user=True)
        except Exception:
            pass  # Don't fail if update check fails
    
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
