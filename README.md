
# RedFlag

![Python](https://img.shields.io/badge/Python-3.12%2B-FF0000?style=for-the-badge&logo=python&logoColor=white)
![Platform](https://img.shields.io/badge/Platform-Win%20%7C%20Linux-FF0000?style=for-the-badge&logo=windows&logoColor=white)
![License](https://img.shields.io/badge/License-GNU-FF0000?style=for-the-badge&logo=open-source-initiative&logoColor=white)
![Methodology](https://img.shields.io/badge/Methodology-Static_Analysis-FF0000?style=for-the-badge&logo=code-review&logoColor=white)
![Status](https://img.shields.io/badge/Status-Active-success?style=for-the-badge&logo=activity&logoColor=white)

[![Discord](https://img.shields.io/badge/Discord-FF0000?style=for-the-badge&logo=open-source-initiative&logoColor=white)](https://discord.gg/macrostack)

**RedFlag** is a static analysis tool designed to flag risky patterns in C++ projects, Visual Studio solutions, and build systems.

Rather than claiming to "understand" code behavior, RedFlag acts as an advanced heuristic engine that scans for **known dangerous signatures**, **suspicious API combinations**, **obfuscation techniques**, and **build pipeline attacks**. It aggregates these static indicators into a weighted risk score, helping developers and researchers quickly identify "Red Flags" in third-party code that warrant manual review.

![Table of Contents](https://img.shields.io/badge/00-Table_of_Contents-000000?style=flat-square)

- [Why RedFlag?](#why-redflag)
- [New Features](#features)
- [Installation](#installation)
- [Usage & CLI](#usage--cli)
- [Configuration](#configuration)
- [How it Works](#how-it-works)
- [Screenshots](#screenshots)
- [License](#license)

---

![Why RedFlag](https://img.shields.io/badge/01-Why_RedFlag%3F-b91c1c?style=for-the-badge&logo=help&logoColor=white)

When cloning repositories or inheriting legacy C++ projects, manual auditing is time-consuming. A project might look clean in the `.cpp` files but execute a reverse shell via a `PreBuildEvent` in the `.vcxproj` file, or hide a payload in a concatenated string.

**RedFlag solves this by:**

1.  ![Scanning](https://img.shields.io/badge/SCANNING-Build_Systems-black?style=flat-square&color=333) Parsing `.vcxproj` files to find command-line executions hidden in Pre/Post build events and linked libraries.
2.  ![Heuristics](https://img.shields.io/badge/ANALYSIS-Anti--Evasion-black?style=flat-square&color=333) Normalizing strings and macros to detect obfuscated API calls (e.g., `system("c" "md.exe")`).
3.  ![Detection](https://img.shields.io/badge/DETECTION-Entropy_Check-black?style=flat-square&color=333) Extracting and decoding Base64/XOR blobs to check for high-entropy strings often associated with shellcode.

---

![Features](https://img.shields.io/badge/02-Features-b91c1c?style=for-the-badge&logo=star&logoColor=white)

### Analysis Engine
*   **Build Event Scanning:** Detects `PowerShell`, `cmd.exe`, `curl`, and `wget` commands buried in Visual Studio XML configuration files.
*   **Dependency Scanning:** Flags suspicious linked libraries (e.g., `wininet.lib`, `urlmon.lib`) even if they aren't explicitly called in the source code.
*   **Anti-Evasion Normalization:**
    *   Merges concatenated C++ strings (`"A" "B"` → `"AB"`).
    *   Strips comments within logic to expose hidden commands.
    *   Detects API aliasing via `#define` (e.g., `#define RUN system`).
*   **Heuristic XOR & Entropy:**
    *   Brute-forces XORed strings to find hidden URLs or payloads.
    *   Context-aware entropy analysis (distinguishes between random assets and encrypted shellcode).

### Pattern Matching
*   **Execution:** `ShellExecute`, `CreateProcess`, `system()`
*   **Memory:** `VirtualAlloc`, `ReflectiveLoader`, `WriteProcessMemory`
*   **Network:** `URLDownloadToFile`, `socket`, `InternetOpen`
*   **Crypto:** `CryptEncrypt`, `Xor`, `FromBase64String`

### Operational
*   **Review Mode:** Interactive CLI to triage findings and open files immediately.
*   **CI/CD Ready:** Export results to JSON and exit with error codes if Critical/High threats are found.
*   **Rich UI:** Formatted tables, progress bars, and colored severity indicators.

---

![Installation](https://img.shields.io/badge/03-Installation-b91c1c?style=for-the-badge&logo=pypi&logoColor=white)

You need **Python 3.7+** installed on your machine.

**1. Clone the repository:**
```bash
git clone https://github.com/rorvzzz/redflag.git
cd redflag
```

**2. Install dependencies:**
```bash
pip install -r requirements.txt
```

---

![Usage](https://img.shields.io/badge/04-Usage_&_CLI-b91c1c?style=for-the-badge&logo=terminal&logoColor=white)

### Basic Scan
Run against a single file or directory.
```bash
python run.py "C:\Path\To\Project"
```

### Interactive Review Mode 
Launch a post-scan interactive session to filter results and open files in your default editor.
```bash
python run.py "C:\Path\To\Project" --review
```

### CI/CD & Automation 
Generate a machine-readable report. If High or Critical findings are detected, the script exits with code `1` (failure), otherwise `0` (success).
```bash
python run.py "C:\Path\To\Project" --json results.json
```

### Options
| Flag | Description |
| :--- | :--- |
| `--review` | Enter interactive mode after scanning to triage findings. |
| `--json <file>` | Export results to a JSON file. |
| `--no-definitions` | Hide technical definitions in the console output. |
| `--auto-update` | Check for and install updates automatically. |
| `--verbose` | Show detailed error logs (useful for debugging). |

---

![Configuration](https://img.shields.io/badge/05-Configuration-b91c1c?style=for-the-badge&logo=json&logoColor=white)

RedFlag supports project-specific configuration. Place a file named `.redflag` (JSON format) in the root of the target directory to customize the scan.

**Example `.redflag` content:**
```json
{
  "ignore_paths": [
    "vendor/third_party_lib/",
    "tests/data/"
  ],
  "ignore_rules": [
    "Mismatched File Extension"
  ],
  "custom_rules": [
    {
      "category": "SECRETS",
      "pattern": "AKIA[0-9A-Z]{16}",
      "score": 10,
      "description": "AWS Access Key ID Detected"
    }
  ]
}
```
*See `redflag.example.json` in the repository for a full template.*

---

![How It Works](https://img.shields.io/badge/06-How_It_Works-b91c1c?style=for-the-badge&logo=gears&logoColor=white)

RedFlag performs analysis in 5 steps:

1.  **Project Identity:** Detects if the target is a VS Solution, Make project, or single file to adjust scanning strategies.
2.  **Build Systems:** Parses `.vcxproj` and `.sln` files to identify command-line executions (PreBuild/PostBuild) and linked binary dependencies.
3.  **Normalization & Pre-processing:**
    *   Merges adjacent strings.
    *   Resolves macros.
    *   Strips comments to prevent evasion.
4.  **Deep Scan:**
    *   Runs regex signatures against normalized code.
    *   Extracts arrays for Entropy/Image analysis.
    *   Brute-forces XOR blobs.
5.  **Verdict:** Aggregates risk scores. If a file contains multiple correlated indicators (e.g., Obfuscation + Network + Execution), it generates a behavioral "High Confidence" finding.

---

![Screenshots](https://img.shields.io/badge/07-Screenshots-b91c1c?style=for-the-badge&logo=google-photos&logoColor=white)

<p align="center">
  <img src="RedFlag/assets/RedFlag.png" alt="RedFlag Logo" width="500">
</p>

### See it in action!
[Watch the RedFlag demo video here!](https://streamable.com/ot16it)

---

![Disclaimer](https://img.shields.io/badge/WARNING-Disclaimer-orange?style=for-the-badge&logo=alert&logoColor=white)

**RedFlag is a heuristic static analysis tool.**
While significantly improved with context-aware engines, it may still produce false positives on complex game engines or obfuscated legitimate code. It is designed to prioritize "Red Flags" for human review, not to replace a comprehensive security audit.

---

![License](https://img.shields.io/badge/License-GNU-blue?style=for-the-badge&logo=open-source-initiative&logoColor=white)

Distributed under the GNU License. See `LICENSE` for more information.
