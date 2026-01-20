"""
Utilities for RedFlag
"""
import math
import sys
from collections import defaultdict

# Rich imports
try:
    from rich.console import Console
    from rich.panel import Panel
    from rich.text import Text
    from rich.table import Table
    from rich.progress import Progress, SpinnerColumn, TextColumn, BarColumn
    from rich.style import Style
    RICH_AVAILABLE = True
    console = Console()
except ImportError:
    RICH_AVAILABLE = False
    console = None

class UI:
    @staticmethod
    def print_banner(text):
        if RICH_AVAILABLE:
            console.clear()
            console.print(Text(text, style="bold red"))
        else:
            print(text)

    @staticmethod
    def print_panel(content, title, style="red"):
        if RICH_AVAILABLE:
            console.print(Panel(content, title=title, border_style=style))
        else:
            print(f"[{title}] {content}")

    @staticmethod
    def log(message, style=""):
        if RICH_AVAILABLE:
            console.print(message, style=style)
        else:
            # Strip rich markup for plain text if basic
            print(message)

    @staticmethod
    def get_progress():
        if RICH_AVAILABLE:
            return Progress(
                SpinnerColumn(),
                TextColumn("[progress.description]{task.description}"),
                BarColumn(),
                TextColumn("{task.percentage:>3.0f}%"),
            )
        return None

def calculate_entropy(data):
    if not data: return 0
    counts = defaultdict(int)
    for b in data: counts[b] += 1
    entropy = 0
    for count in counts.values():
        p = count / len(data)
        entropy -= p * math.log2(p)
    return entropy

def get_severity(score):
    if score >= 8: return "CRITICAL"
    if score >= 5: return "HIGH"
    if score >= 3: return "MEDIUM"
    return "LOW"
