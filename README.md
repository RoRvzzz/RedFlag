

# RedFlag 

![Python Version](https://img.shields.io/badge/python-3.8%2B-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Status](https://img.shields.io/badge/status-active-success)
![Rich Support](https://img.shields.io/badge/UI-Rich-purple)

**RedFlag** is a static analysis tool designed to inspect C++ projects, Visual Studio solutions, and build systems for risky behaviors.

Unlike standard linters that look for syntax errors, RedFlag scans source code and build events for execution anomalies, obfuscation, crypto misuse, and hidden payloads. It correlates these signals to provide a transparent risk score, helping security researchers and developers identify potential malware or malicious code injection in third-party projects.

##  Table of Contents
- [Why RedFlag?](#why-redflag)
- [Features](#features)
- [Installation](#installation)
- [How to Use](#how-to-use)
- [How it Works](#how-it-works)
- [Screenshots](#screenshots)
- [Contributing](#contributing)
- [License](#license)

## Why RedFlag?
When cloning repositories or inheriting legacy C++ projects, it is difficult to spot malicious intent hidden deep within build configurations. A project might look clean in the `.cpp` files but execute a reverse shell via a `PostBuildEvent` in the `.vcxproj` file.

RedFlag solves this by:
1. **Scanning Build Systems:** It parses `.vcxproj` files to find hidden commands in Pre/Post build events.
2. **Heuristic Analysis:** It doesn't just look for strings; it scores context (e.g., Crypto APIs used alongside Network APIs).
3. **Hidden Payload Detection:** It automatically extracts and decodes Base64 blobs to check for high-entropy malicious code.

##  Features
*   **Build Event Inspection:** Detects `PowerShell`, `cmd.exe`, `curl`, and `wget` commands buried in Visual Studio build events.
*   **Heuristic Pattern Matching:** Scans for specific API categories:
    *   *Execution:* `ShellExecute`, `CreateProcess`, `system()`
    *   *Memory:* `VirtualAlloc`, `ReflectiveLoader`, `WriteProcessMemory`
    *   *Network:* `URLDownloadToFile`, `socket`, `InternetOpen`
    *   *Crypto & Obfuscation:* `CryptEncrypt`, `Xor`, `FromBase64String`
*   **Entropy Analysis:** Identifies high-entropy strings and Base64 encoded blobs that often hide shellcode.
*   **Risk Scoring:** Assigns a severity level (LOW, MEDIUM, HIGH, CRITICAL) based on a weighted scoring system.
*   **Beautiful UI:** Uses the `rich` library for formatted tables, progress bars, and colored output.

## 🛠 Installation

You need **Python 3** installed on your machine.

1. **Clone the repository:**
   ```bash
   git clone https://github.com/yourusername/redflag.git
   cd redflag
   ```

2. **Install dependencies:**
   For the best visual experience, install the `rich` library. The tool will still work without it, but the output will be plain text.
   ```bash
   pip install rich
   ```

## 💻 How to Use

You can run RedFlag against a single file or an entire directory (like a Visual Studio project folder).

### Basic Usage
```bash
python redflag.py "C:\Path\To\Suspicious\Project"
```

### Interactive Mode
If you run the script without arguments, it will prompt you for the path:
```bash
python redflag.py
```

## How it Works
RedFlag performs analysis in 5 steps:
1.  **Structure Verification:** Identifies if the target is a VS Project, Make project, or single file.
2.  **Build File Scan:** Parses XML in `.vcxproj` files to find command-line executions in build events.
3.  **Source Scan:** regex-based scanning of source files (ignoring binary/library files) for suspicious API calls.
4.  **Correlation:** Increases the risk score if multiple dangerous categories (e.g., Crypto + Network) appear in the same file.
5.  **Verdict:** Calculates a total threat score and provides a final verdict (CLEAN to CRITICAL).

## 📸 Screenshots

*(Example output representation)*

```text
VERDICT: HIGH (Score: 18)

Top Findings:
 • CRITICAL  src/utils.cpp:45 - Remote Thread Injection
   CreateRemoteThread(hProcess, NULL, 0, ...
 • HIGH      project.vcxproj:0 - Suspicious Build Event (PowerShell Execution)
   powershell.exe -enc aWZyYW1l...
```

## 🤝 Contributing
Contributions are what make the open-source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

1.  Fork the Project
2.  Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3.  Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4.  Push to the Branch (`git push origin feature/AmazingFeature`)
5.  Open a Pull Request

## Disclaimer
This tool is for educational and security research purposes only. Use it to audit your own code or code you have permission to analyze. The authors are not responsible for any misuse of this tool.

##  License
Distributed under the MIT License. See `LICENSE` for more information.

---
