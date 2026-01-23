"""
Data models
"""
from dataclasses import dataclass, field
from typing import List, Dict, Any

@dataclass
class Finding:
    category: str
    description: str
    file: str
    line: int
    context: str
    score: int
    severity: str  # low, medium, high, critical
    confidence: str = "MEDIUM" # low,   medium, high
    metadata: Dict[str, Any] = field(default_factory=dict)
