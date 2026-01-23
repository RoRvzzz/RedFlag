# RedFlag

![Python](https://img.shields.io/badge/Python-3.12%2B-FF0000?style=for-the-badge&logo=python&logoColor=white)
![Platform](https://img.shields.io/badge/Platform-Win%20%7C%20Linux-FF0000?style=for-the-badge&logo=windows&logoColor=white)
![License](https://img.shields.io/badge/License-GNU-FF0000?style=for-the-badge&logo=open-source-initiative&logoColor=white)
![Methodology](https://img.shields.io/badge/Methodology-Static_Analysis-FF0000?style=for-the-badge&logo=code-review&logoColor=white)
![Status](https://img.shields.io/badge/Status-Active-success?style=for-the-badge&logo=activity&logoColor=white)

[![Discord](https://img.shields.io/badge/Discord-FF0000?style=for-the-badge&logo=open-source-initiative&logoColor=white)](https://discord.gg/macrostack)


**RedFlag** is a static analysis tool designed to flag risky patterns in C++ projects, Visual Studio solutions, and build systems.

Rather than claiming to "understand" code behavior, RedFlag acts as an advanced grep-tool that scans for **known dangerous signatures**, **suspicious API combinations**, and **obfuscation techniques**. It aggregates these static indicators into a weighted risk score, helping ~~developers and researchers~~ pasters quickly identify "Red Flags" in third-party code that warrant manual review.

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

When cloning repositories or inheriting legacy C++ projects, manual auditing is time-consuming. A project might look clean in the `.cpp` files but execute a reverse shell via a `PreBuildEvent` in the `.vcxproj` file.

**RedFlag solves this by:**

1.  ![Scanning](https://img.shields.io/badge/SCANNING-Build_Systems-black?style=flat-square&color=333) Parsing `.vcxproj` files to find command-line executions hidden in Pre/Post build events.
2.  ![Heuristics](https://img.shields.io/badge/ANALYSIS-Weighted_Scoring-black?style=flat-square&color=333) Using context-aware regex (e.g., flagging Crypto APIs only when used near Network APIs).
3.  ![Detection](https://img.shields.io/badge/DETECTION-Entropy_Check-black?style=flat-square&color=333) extracting and decoding Base64 blobs to check for high-entropy strings often associated with shellcode.

---

![Features](https://img.shields.io/badge/02-Features-b91c1c?style=for-the-badge&logo=star&logoColor=white)

*   ![Build](https://img.shields.io/badge/Target-Build_Events-red)
    Detects `PowerShell`, `cmd.exe`, `curl`, and `wget` commands buried in Visual Studio build events.

*   ![Patterns](https://img.shields.io/badge/Target-API_Signatures-red)
    Scans for specific API categories using regex signatures:
    *   ![Exec](https://img.shields.io/badge/TYPE-Execution-gray) `ShellExecute`, `CreateProcess`, `system()`
    *   ![Mem](https://img.shields.io/badge/TYPE-Memory-gray) `VirtualAlloc`, `ReflectiveLoader`, `WriteProcessMemory`
    *   ![Net](https://img.shields.io/badge/TYPE-Network-gray) `URLDownloadToFile`, `socket`, `InternetOpen`
    *   ![Crypto](https://img.shields.io/badge/TYPE-Crypto-gray) `CryptEncrypt`, `Xor`, `FromBase64String`

*   ![Entropy](https://img.shields.io/badge/Target-Entropy_Analysis-red)
    Identifies high-entropy strings and Base64 encoded blobs that statistically resemble encrypted payloads.

*   ![Risk](https://img.shields.io/badge/Target-Risk_Scoring-red)
    Assigns a severity level (LOW, MEDIUM, HIGH, CRITICAL) based on the accumulation of "bad" points.

*   ![UI](https://img.shields.io/badge/Target-Rich_UI-red)
    Uses the `rich` library for formatted tables, progress bars, and colored output.

---

![Installation](https://img.shields.io/badge/03-Installation-b91c1c?style=for-the-badge&logo=pypi&logoColor=white)

You need **Python 3** installed on your machine.

**1. Clone the repository:**
```bash
git clone https://github.com/rorvzzz/redflag.git
cd redflag
```

**2. Install dependencies:**
For the best visual experience, install the `rich` library. The tool will still work without it, but the output will be plain text.
```bash
pip install -r requirements.txt
```

---

![How To Use](https://img.shields.io/badge/04-How_To_Use-b91c1c?style=for-the-badge&logo=terminal&logoColor=white)

You can run RedFlag against a single file or an entire directory (like a Visual Studio project folder).

![Mode](https://img.shields.io/badge/Mode-Basic_Usage-black?style=flat-square)
```bash
python run.py "C:\Path\To\Suspicious\Project"
```

![Mode](https://img.shields.io/badge/Mode-Interactive-black?style=flat-square)
If you run the script without arguments, it will prompt you for the path:
```bash
python run.py
```

---

![How It Works](https://img.shields.io/badge/05-How_It_Works-b91c1c?style=for-the-badge&logo=gears&logoColor=white)

RedFlag performs analysis in 5 steps:

1.  ![Step 1](https://img.shields.io/badge/1-Structure_Verification-374151) Identifies if the target is a VS Project, Make project, or single file.
2.  ![Step 2](https://img.shields.io/badge/2-Build_File_Scan-374151) Parses XML in `.vcxproj` files to find command-line executions in build events.
3.  ![Step 3](https://img.shields.io/badge/3-Source_Scan-374151) Regex-based scanning of source files (ignoring binary/library files) for suspicious API calls.
4.  ![Step 4](https://img.shields.io/badge/4-Aggregation-374151) Sums up risk points if multiple dangerous categories (e.g., Crypto + Network) appear in the same file.
5.  ![Step 5](https://img.shields.io/badge/5-Verdict-b91c1c) Calculates a total score and provides a final verdict (CLEAN to CRITICAL).

---

![Screenshots](https://img.shields.io/badge/06-Screenshots-b91c1c?style=for-the-badge&logo=google-photos&logoColor=white)

<p align="center">
  <img src="RedFlag/assets/redflag.png" alt="RedFlag Logo" width="500">
</p>

### See it in action!
[Watch the RedFlag demo video here!](https://streamable.com/ot16it)

---

![Disclaimer](https://img.shields.io/badge/WARNING-Disclaimer-orange?style=for-the-badge&logo=alert&logoColor=white)

This tool is still very innaccurate and only meant to be a first measure before a manual check, although soon it will be good enough to check for people who don't know C++. Currently this only detects basic vulns.

---

![License](https://img.shields.io/badge/License-GNU-blue?style=for-the-badge&logo=open-source-initiative&logoColor=white)

Distributed under the GNU License. See `LICENSE` for more information.
