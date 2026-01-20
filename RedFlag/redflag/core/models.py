"""
Data models
"""
from dataclasses import dataclass

@dataclass
class Finding:
    category: str
    description: str
    file: str
    line: int
    context: str
    score: int
    severity: str  # LOW, MEDIUM, HIGH, CRITICAL
