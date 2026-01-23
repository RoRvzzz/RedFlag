# RedFlag 🔴

**Enterprise-Ready Malware Analysis Tool for C++ Projects**

RedFlag is a comprehensive static analysis tool designed to detect malicious indicators, obfuscation techniques, and suspicious patterns in C++ source code. Built with security researchers, code auditors, and CI/CD pipelines in mind.

## Features

### Core Detection Capabilities

- **Pattern-Based Scanning**: Detects execution APIs, memory manipulation, network activity, obfuscation, and crypto usage
- **YARA Integration**: Rule-based pattern matching for advanced threat detection
- **String Analysis**: Extracts and analyzes URLs, IP addresses, and suspicious strings
- **Build System Analysis**: Scans Visual Studio project files (`.vcxproj`, `.sln`) for suspicious build events and linked libraries
- **Metadata Extraction**: File hashing (SHA256), file type identification, and binary detection
- **MITRE ATT&CK Mapping**: Maps findings to relevant MITRE ATT&CK tactics and techniques
- **Embedded Image Extraction**: Detects and extracts embedded images from `unsigned char` arrays

### Advanced Anti-Evasion

- **String Normalization**: Detects string concatenation evasion (`"c" "md" ".exe"`) and comment-based obfuscation
- **Macro Detection**: Identifies dangerous API aliasing via `#define` directives
- **XOR Brute-Force**: Deobfuscates XOR-encrypted content with optimized translation tables
- **Entropy Analysis**: Contextual entropy filtering to reduce false positives on game assets and lookup tables

### Developer Experience

- **Interactive Review Mode**: Post-scan triage interface for quick audits
- **JSON Export**: Machine-readable output for CI/CD integration
- **Project Configuration**: Per-project `.redflag` config files for custom rules and ignore lists
- **Verbose Mode**: Detailed error logging for debugging
- **Auto-Update**: Built-in version checking and update mechanism

## Installation

### From Source

```bash
git clone https://github.com/RoRvzzz/RedFlag.git
cd RedFlag
pip install -r requirements.txt
```

### From PyPI (Coming Soon)

```bash
pip install redflag
```

## Usage

### Basic Scan

```bash
python run.py /path/to/project
```

### CLI Options

```bash
python run.py [path] [options]

Options:
  --skip-update          Skip update check
  --auto-update          Automatically install updates without asking
  --version              Show version and exit
  --no-definitions      Hide technical definitions in output
  --json FILE            Export results to JSON file
  --review               Interactive review mode after scan
  --verbose              Show internal errors/debug info
```

### Examples

```bash
# Scan with JSON export for CI/CD
python run.py ./myproject --json results.json

# Interactive review mode
python run.py ./myproject --review

# Verbose output for debugging
python run.py ./myproject --verbose
```

## Exit Codes

RedFlag exits with:
- **0**: No HIGH or CRITICAL findings (scan passed)
- **1**: HIGH or CRITICAL findings detected (scan failed)

This makes it suitable for CI/CD pipelines:

```yaml
# GitHub Actions example
- name: Run RedFlag
  run: python run.py ./src
  # Build fails if HIGH/CRITICAL threats found
```

## Project Configuration

Create a `.redflag` file in your project root to customize scanning behavior:

```json
{
  "ignore_paths": [
    "tests/",
    "vendor/legacy_library/"
  ],
  "ignore_rules": [
    "Mismatched File Extension"
  ],
  "custom_rules": [
    {
      "category": "SECRETS",
      "pattern": "AKIA[0-9A-Z]{16}",
      "score": 10,
      "description": "AWS Access Key ID"
    }
  ]
}
```

See `redflag.example.json` for a complete template.

### Configuration Options

- **`ignore_paths`**: List of relative path prefixes to skip during scanning
- **`ignore_rules`**: List of rule description substrings to suppress
- **`custom_rules`**: Array of custom regex patterns to scan for
  - `category`: Category name (e.g., "SECRETS", "POLICY")
  - `pattern` or `regex`: Regular expression pattern
  - `score`: Threat score (1-10)
  - `description`: Human-readable description

## Review Mode

After scanning, use `--review` to enter an interactive triage interface:

```
Review> high          # Show HIGH/CRITICAL findings
Review> medium        # Show MEDIUM+ findings
Review> all           # Show all findings
Review> open 5        # Open file for finding #5
Review> quit          # Exit review mode
```

## Output Format

### Console Output

RedFlag provides color-coded, organized output with:
- Risk level summary (CRITICAL/HIGH/MEDIUM/LOW)
- Total threat score
- Top findings grouped by severity
- Technical definitions (optional)
- MITRE ATT&CK mappings

### JSON Export

The `--json` flag exports a complete machine-readable report:

```json
{
  "version": "1.0",
  "timestamp": "2026-01-19T...",
  "target": "/path/to/project",
  "summary": {
    "risk_level": "MEDIUM",
    "total_threat_score": 101,
    "findings_by_severity": {
      "CRITICAL": 0,
      "HIGH": 2,
      "MEDIUM": 5,
      "LOW": 10
    }
  },
  "findings": [...]
}
```

## Detection Categories

- **EXECUTION**: Process creation, shell execution, PowerShell
- **MEMORY**: Remote thread injection, reflective loading
- **NETWORK**: WinINet API, raw sockets, download commands
- **CRYPTO**: Encryption APIs, XOR operations
- **OBFUSCATION**: Base64 encoding, hex-escaped strings, stack strings
- **BUILD_EVENT**: Suspicious pre/post-build commands
- **BUILD_CONFIG**: Suspicious linked libraries
- **NETWORK_IOC**: URLs, IP addresses
- **HIDDEN_PAYLOAD**: High-entropy encoded blobs

## False Positive Reduction

RedFlag uses contextual analysis to reduce false positives:

- **Entropy-based filtering**: Distinguishes between malicious obfuscation and legitimate assets
- **Library code detection**: Reduces severity for known HTTP clients and utility code
- **Path-based filtering**: Suppresses findings in known asset/resource directories
- **Project configuration**: Per-project ignore lists via `.redflag`

## Testing

Run unit tests:

```bash
python -m pytest tests/
# or
python tests/test_normalization.py
```

## Requirements

- Python 3.7+
- `rich` (for enhanced console output)
- `urlextract` (for URL/IP extraction)
- `filetype` (for file type identification)
- `yara-python` (optional, for YARA scanning)

## License

[Add your license here]

## Contributing

Contributions welcome! Please open an issue or submit a pull request.

## Acknowledgments

Built for the security research community. Special thanks to all contributors and testers.

---

**⚠️ Disclaimer**: RedFlag is a static analysis tool. It cannot detect runtime behavior or guarantee the absence of malicious code. Always combine with dynamic analysis and manual review.
