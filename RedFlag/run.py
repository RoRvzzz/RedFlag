#!/usr/bin/env python3
import sys
import os
import argparse
import subprocess

# Ensure the current directory is in the Python path so we can import redflag
script_dir = os.path.dirname(os.path.abspath(__file__))
if script_dir not in sys.path:
    sys.path.insert(0, script_dir)

from redflag.core.engine import RedFlagScanner
from redflag.core.config import BANNER
from redflag.core.utils import UI
from redflag.core.updater import auto_update, get_current_version, apply_pending_update

def main():
    # check for and apply any pending updates from staging directory
    try:
        apply_pending_update()
    except Exception:
        pass  # don't fail if update application fails, just log for debugging
    parser = argparse.ArgumentParser(description="RedFlag Malware Analysis")
    parser.add_argument("path", nargs="?", help="Path to project or file")
    parser.add_argument("--skip-update", action="store_true", help="Skip update check")
    parser.add_argument("--auto-update", action="store_true", help="Automatically install updates without asking")
    parser.add_argument("--version", action="store_true", help="Show version and exit")
    parser.add_argument("--no-definitions", action="store_true", help="Hide technical definitions in output")
    parser.add_argument("--json", type=str, metavar="FILE", help="Export results to JSON file")
    parser.add_argument("--review", action="store_true", help="Interactive review mode after scan")
    parser.add_argument("--verbose", action="store_true", help="Show internal errors/debug info")
    args = parser.parse_args()
    
    # show version and exit
    if args.version:
        version = get_current_version()
        print(f"RedFlag v{version}")
        return
    
    # check for updates (unless skipped)
    if not args.skip_update:
        try:
            if args.auto_update:
                auto_update(ask_user=False, prefer_pip=True)
            else:
                auto_update(ask_user=True, prefer_pip=True)
        except Exception as e:
            # don't fail if update check fails, but log for debugging
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
    scanner.verbose = bool(args.verbose)
    try:
        version = get_current_version()
        UI.print_banner(BANNER)
        UI.log(f"  [dim]RedFlag v{version}[/dim]\n")
        UI.print_panel(f"Target: [bold cyan]{scanner.target_path}[/bold cyan]", title="Initialization")
        scanner.run()
        
        # export to json if requested
        if args.json:
            from redflag.cogs.verdict import export_to_json
            export_to_json(scanner, args.json)
            UI.log(f"\n  [green]✓ Results exported to {args.json}[/green]")

        # Interactive review mode
        if args.review:
            try:
                _review_mode(scanner)
            except KeyboardInterrupt:
                UI.log("\n  [dim]Review cancelled.[/dim]")
    except KeyboardInterrupt:
        print("\nScan aborted.")

    # CI/CD exit codes (fail build on HIGH/CRITICAL)
    has_critical = any(getattr(f, "severity", "").upper() == "CRITICAL" for f in (scanner.findings or []))
    has_high = any(getattr(f, "severity", "").upper() == "HIGH" for f in (scanner.findings or []))
    if has_critical or has_high:
        sys.exit(1)
    sys.exit(0)

def _review_mode(scanner: RedFlagScanner):
    """
    Simple interactive triage loop to avoid scrolling back.
    """
    def severity_rank(s: str) -> int:
        order = {"CRITICAL": 4, "HIGH": 3, "MEDIUM": 2, "LOW": 1, "INFO": 0, "CLEAN": 0}
        return order.get((s or "").upper(), 0)

    findings = list(scanner.findings or [])
    findings.sort(key=lambda f: (severity_rank(getattr(f, "severity", "")), getattr(f, "score", 0)), reverse=True)

    UI.log("\n[bold white]Review Mode[/bold white]")
    UI.log("  [dim]Commands: high / medium / all / open / quit[/dim]")

    while True:
        cmd = input("\nReview> ").strip().lower()
        if cmd in ("q", "quit", "exit"):
            break

        if cmd in ("high", "h"):
            subset = [f for f in findings if severity_rank(f.severity) >= 3]
        elif cmd in ("medium", "m"):
            subset = [f for f in findings if severity_rank(f.severity) >= 2]
        elif cmd in ("all", "a", "list"):
            subset = findings
        elif cmd.startswith("open"):
            # open <index>
            parts = cmd.split()
            if len(parts) < 2 or not parts[1].isdigit():
                print("Usage: open <index>")
                continue
            idx = int(parts[1])
            if idx < 0 or idx >= len(findings):
                print("Index out of range.")
                continue
            f = findings[idx]
            rel = getattr(f, "file", "")
            abs_path = rel if os.path.isabs(rel) else os.path.abspath(os.path.join(scanner.target_dir, rel))
            if os.path.exists(abs_path):
                try:
                    _open_file(abs_path)
                    print(f"Opened: {abs_path}")
                except Exception as e:
                    print(f"Could not open: {abs_path} ({type(e).__name__})")
            else:
                print(f"File not found: {abs_path}")
            continue
        else:
            print("Commands: high / medium / all / open <index> / quit")
            continue

        if not subset:
            print("No findings in this view.")
            continue

        # show a compact list with stable indexes from the full sorted list
        print("\nFindings:")
        for f in subset[:50]:
            idx = findings.index(f)
            sev = getattr(f, "severity", "INFO")
            desc = getattr(f, "description", "")
            file = getattr(f, "file", "")
            line = getattr(f, "line", 0)
            print(f"[{idx:03}] {sev:8} {file}:{line} - {desc}")

def _open_file(path: str):
    if sys.platform == "win32":
        os.startfile(path)  # type: ignore[attr-defined]
    elif sys.platform == "darwin":
        subprocess.call(("open", path))
    else:
        subprocess.call(("xdg-open", path))

if __name__ == "__main__":
    main()
