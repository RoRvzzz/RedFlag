# RedFlag

![Python](https://img.shields.io/badge/Python-3.12%2B-FF0000?style=for-the-badge&logo=python&logoColor=white)
![Platform](https://img.shields.io/badge/Platform-Win%20%7C%20Linux-FF0000?style=for-the-badge&logo=windows&logoColor=white)
![License](https://img.shields.io/badge/License-MIT-FF0000?style=for-the-badge&logo=open-source-initiative&logoColor=white)
![UI](https://img.shields.io/badge/UI-Rich_CLI-FF0000?style=for-the-badge&logo=charm&logoColor=white)
![Status](https://img.shields.io/badge/Status-Active-success?style=for-the-badge&logo=activity&logoColor=white)

**RedFlag** is a static analysis tool designed to inspect C++ projects, Visual Studio solutions, and build systems for risky behaviors.

Unlike standard linters that look for syntax errors, RedFlag scans source code and build events for execution anomalies, obfuscation, crypto misuse, and hidden payloads. It correlates these signals to provide a transparent risk score, helping ~~security researchers and developers~~ pasters identify potential malware or malicious code injection in third-party projects.

![Table of Contents](https://img.shields.io/badge/00-Table_of_Contents-000000?style=flat-square)

- [Why RedFlag?](#why-redflag)
- [Features](#features)
- [Installation](#installation)
- [How to Use](#how-to-use)
- [How it Works](#how-it-works)
- [Screenshots](#screenshots)
- [Contributing](#contributing)
- [License](#license)

---

![Why RedFlag](https://img.shields.io/badge/01-Why_RedFlag%3F-b91c1c?style=for-the-badge&logo=help&logoColor=white)

When cloning repositories or inheriting legacy C++ projects, it is difficult to spot malicious intent hidden deep within build configurations. A project might look clean in the `.cpp` files but execute a reverse shell via a `PostBuildEvent` in the `.vcxproj` file.

**RedFlag solves this by:**

1.  ![Scanning](https://img.shields.io/badge/SCANNING-Build_Systems-black?style=flat-square&color=333) It parses `.vcxproj` files to find hidden commands in Pre/Post build events.
2.  ![Heuristics](https://img.shields.io/badge/ANALYSIS-Heuristic_Scoring-black?style=flat-square&color=333) It doesn't just look for strings; it scores context (e.g., Crypto APIs used alongside Network APIs).
3.  ![Detection](https://img.shields.io/badge/DETECTION-Hidden_Payloads-black?style=flat-square&color=333) It automatically extracts and decodes Base64 blobs to check for high-entropy malicious code.

---

![Features](https://img.shields.io/badge/02-Features-b91c1c?style=for-the-badge&logo=star&logoColor=white)

*   ![Build](https://img.shields.io/badge/Target-Build_Events-red)
    Detects `PowerShell`, `cmd.exe`, `curl`, and `wget` commands buried in Visual Studio build events.

*   ![Patterns](https://img.shields.io/badge/Target-API_Patterns-red)
    Scans for specific API categories:
    *   ![Exec](https://img.shields.io/badge/TYPE-Execution-gray) `ShellExecute`, `CreateProcess`, `system()`
    *   ![Mem](https://img.shields.io/badge/TYPE-Memory-gray) `VirtualAlloc`, `ReflectiveLoader`, `WriteProcessMemory`
    *   ![Net](https://img.shields.io/badge/TYPE-Network-gray) `URLDownloadToFile`, `socket`, `InternetOpen`
    *   ![Crypto](https://img.shields.io/badge/TYPE-Crypto-gray) `CryptEncrypt`, `Xor`, `FromBase64String`

*   ![Entropy](https://img.shields.io/badge/Target-Entropy_Analysis-red)
    Identifies high-entropy strings and Base64 encoded blobs that often hide shellcode.

*   ![Risk](https://img.shields.io/badge/Target-Risk_Scoring-red)
    Assigns a severity level (LOW, MEDIUM, HIGH, CRITICAL) based on a weighted scoring system.

*   ![UI](https://img.shields.io/badge/Target-Beautiful_UI-red)
    Uses the `rich` library for formatted tables, progress bars, and colored output.

---

![Installation](https://img.shields.io/badge/03-Installation-b91c1c?style=for-the-badge&logo=pypi&logoColor=white)

You need **Python 3** installed on your machine.

**1. Clone the repository:**
```bash
git clone https://github.com/yourusername/redflag.git
cd redflag
```

**2. Install dependencies:**
For the best visual experience, install the `rich` library. The tool will still work without it, but the output will be plain text.
```bash
pip install rich
```

---

![How To Use](https://img.shields.io/badge/04-How_To_Use-b91c1c?style=for-the-badge&logo=terminal&logoColor=white)

You can run RedFlag against a single file or an entire directory (like a Visual Studio project folder).

![Mode](https://img.shields.io/badge/Mode-Basic_Usage-black?style=flat-square)
```bash
python redflag.py "C:\Path\To\Suspicious\Project"
```

![Mode](https://img.shields.io/badge/Mode-Interactive-black?style=flat-square)
If you run the script without arguments, it will prompt you for the path:
```bash
python redflag.py
```

---

![How It Works](https://img.shields.io/badge/05-How_It_Works-b91c1c?style=for-the-badge&logo=gears&logoColor=white)

RedFlag performs analysis in 5 steps:

1.  ![Step 1](https://img.shields.io/badge/1-Structure_Verification-374151) Identifies if the target is a VS Project, Make project, or single file.
2.  ![Step 2](https://img.shields.io/badge/2-Build_File_Scan-374151) Parses XML in `.vcxproj` files to find command-line executions in build events.
3.  ![Step 3](https://img.shields.io/badge/3-Source_Scan-374151) Regex-based scanning of source files (ignoring binary/library files) for suspicious API calls.
4.  ![Step 4](https://img.shields.io/badge/4-Correlation-374151) Increases the risk score if multiple dangerous categories (e.g., Crypto + Network) appear in the same file.
5.  ![Step 5](https://img.shields.io/badge/5-Verdict-b91c1c) Calculates a total threat score and provides a final verdict (CLEAN to CRITICAL).

---

![Screenshots](https://img.shields.io/badge/06-Screenshots-b91c1c?style=for-the-badge&logo=google-photos&logoColor=white)

<p align="center">
  <img src="RedFlag/redflag/RedFlag.png" alt="RedFlag Logo" width="500">
</p>

---

![Disclaimer](https://img.shields.io/badge/WARNING-Disclaimer-orange?style=for-the-badge&logo=alert&logoColor=white)

This tool is for educational and security research purposes only. Use it to audit your own code or code you have permission to analyze. The authors are not responsible for any misuse of this tool.

---

![License](https://img.shields.io/badge/License-GNU-blue?style=for-the-badge&logo=open-source-initiative&logoColor=white)

Distributed under the MIT License. See `LICENSE` for more information.
