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

# YARA imports
try:
    import yara
    YARA_AVAILABLE = True
except ImportError:
    YARA_AVAILABLE = False

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

def xor_brute(data, min_length=4):
    """
    Attempt to XOR brute-force a byte string with single-byte keys.
    Returns a list of (key, decoded_bytes) tuples where the result looks printable.
    """
    results = []
    # If data is string, convert to bytes (latin1/ascii safe)
    if isinstance(data, str):
        try:
            data = data.encode('latin1')
        except:
            return []
            
    if not data or len(data) < min_length:
        return []

    for key in range(1, 256):
        decoded = bytearray(len(data))
        for i in range(len(data)):
            decoded[i] = data[i] ^ key
        
        # Heuristic: Check if the result is mostly printable text
        try:
            # Try to decode as utf-8 or ascii
            text = decoded.decode('utf-8')
            # Check for high ratio of printable chars
            printable = sum(1 for c in text if c.isprintable())
            if printable / len(text) > 0.90:
                results.append((key, text))
        except UnicodeDecodeError:
            continue
            
    return results
