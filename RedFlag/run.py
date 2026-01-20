#!/usr/bin/env python3
import sys
import argparse
from redflag.core.engine import RedFlagScanner
from redflag.core.config import BANNER
from redflag.core.utils import UI

def main():
    parser = argparse.ArgumentParser(description="RedFlag Malware Analysis")
    parser.add_argument("path", nargs="?", help="Path to project or file")
    args = parser.parse_args()
    
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

    scanner = RedFlagScanner(path)
    try:
        UI.print_banner(BANNER)
        UI.print_panel(f"Target: [bold cyan]{scanner.target_path}[/bold cyan]", title="Initialization")
        scanner.run()
    except KeyboardInterrupt:
        print("\nScan aborted.")

if __name__ == "__main__":
    main()
