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
    """Calculate Shannon entropy of data"""
    if not data: return 0
    # Optimization: Use built-in encoding if data is string
    if isinstance(data, str):
        data = data.encode('utf-8', errors='ignore')
    
    data_len = len(data)
    if data_len == 0: return 0
    
    counts = defaultdict(int)
    for b in data: counts[b] += 1
    
    entropy = 0
    for count in counts.values():
        p = count / data_len
        entropy -= p * math.log2(p)
    return entropy

def get_severity(score):
    if score >= 8: return "CRITICAL"
    if score >= 5: return "HIGH"
    if score >= 3: return "MEDIUM"
    return "LOW"

def xor_brute(data, min_length=4):
    """
    Optimized XOR brute-force using translation tables for speed.
    Returns a list of (key, decoded_text) tuples where the result looks printable.
    """
    results = []
    # If data is string, convert to bytes (latin1 preserves byte values 1:1)
    if isinstance(data, str):
        data = data.encode('latin1', errors='ignore')
            
    if not data or len(data) < min_length:
        return []

    for key in range(1, 256):
        try:
            # Optimization: Use translation table - O(1) lookup inside C implementation
            # Create translation table: map each byte to itself XOR key
            trans_table = bytes.maketrans(
                bytes(range(256)),
                bytes(i ^ key for i in range(256))
            )
            decoded_bytes = data.translate(trans_table)
            
            # Stricter validation: Require alphanumeric characters, not just printable
            # Fast check: If it contains too many control chars, likely not text
            # Allow tab (9), newline (10), carriage return (13)
            non_printable = sum(1 for b in decoded_bytes if (b < 32 and b not in (9, 10, 13)) or b > 126)
            
            if non_printable / len(decoded_bytes) < 0.08:  # Stricter: 8% threshold (was 10%)
                text = decoded_bytes.decode('utf-8', errors='ignore')
                
                # Additional check: Must contain alphanumeric characters (not just symbols)
                alnum_count = sum(1 for c in text if c.isalnum())
                if alnum_count / len(text) < 0.3:  # At least 30% alphanumeric
                    continue
                
                # Skip if it's mostly whitespace or code delimiters
                whitespace_delim_count = sum(1 for c in text if c.isspace() or c in '{;}')
                if whitespace_delim_count / len(text) > 0.5:  # More than 50% whitespace/delimiters
                    continue
                
                results.append((key, text))
        except (ValueError, UnicodeDecodeError):
            continue
            
    return results
