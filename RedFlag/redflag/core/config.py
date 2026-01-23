"""
Configuration and constants for RedFlag
"""
import os
import re

# Load banner from assets folder
def _load_banner():
    """Load the ASCII art banner from assets/ascii_art.txt"""
    # Get the directory where this config file is located
    config_dir = os.path.dirname(os.path.abspath(__file__))
    # Navigate to assets folder (go up from redflag/core to redflag, then up to root, then into assets)
    root_dir = os.path.dirname(os.path.dirname(os.path.dirname(config_dir)))
    banner_path = os.path.join(root_dir, 'assets', 'ascii_art.txt')
    
    try:
        with open(banner_path, 'r', encoding='utf-8') as f:
            return f.read()
    except (FileNotFoundError, IOError):
        # Fallback to hardcoded banner if file not found
        return r"""
$$$$$$$\                  $$\ $$$$$$$$\ $$\                      oo_____________
$$  __$$\                 $$ |$$  _____|$$ |                     ||\\\\////\\\\||
$$ |  $$ | $$$$$$\   $$$$$$$ |$$ |      $$ | $$$$$$\   $$$$$$\   ||\\\\////\\\\||
$$$$$$$  |$$  __$$\ $$  __$$ |$$$$$\    $$ | \____$$\ $$  __$$\  ||////\\\\\\\\||
$$  __$$< $$$$$$$$ |$$ /  $$ |$$  __|   $$ | $$$$$$$ |$$ /  $$ | ||////\\\\\\\\||
$$ |  $$ |$$   ____|$$ |  $$ |$$ |      $$ |$$  __$$ |$$ |  $$ | ||
$$ |  $$ |\$$$$$$$\ \$$$$$$$ |$$ |      $$ |\$$$$$$$ |\$$$$$$$ | ||
\__|  \__| \_______| \_______|\__|      \__| \_______| \____$$ | ||
                                                      $$\   $$ | ||
                                                      \$$$$$$  | ||
                                                       \______/  ||
"""

BANNER = _load_banner()

IGNORE_DIRS = {
    '.git', '.svn', '.vs', '.vscode', '.idea',
    'build', 'out', 'bin', 'obj', 'node_modules', '__pycache__',
    'artifacts', 'dist', 'target', 'vendor', 'ext', 'external',
    'x64', 'x86', 'debug', 'release',
    'libs', 'imgui', 'include'
}

# Files to never scan (the tool itself)
IGNORE_FILES = {
    'redflag.py', 'check.py', 'checker.py', 'main.py', '__init__.py', 
    'fonts.h', 'prot.hxx', 'obfusheader.h', 'VMProtectSDK.h', 'vmprotect.h',
    'gui.cpp', 'desync.cpp', 'configsystem.cpp'
}

# Extensions to skip (binaries, images, etc.)
SKIP_EXTS = {
    '.exe', '.dll', '.obj', '.pdb', '.idb', '.ilk', '.png', '.jpg', '.ico', '.pdf',
    '.lib', '.a', '.so', '.dylib', '.exp', # Libraries
    '.ifc', '.ifcast', '.ipch', '.pch', # C++ artifacts
    '.suo', '.user', '.filters'
}

# Benign domains to filter out from URL findings (common legitimate services)
BENIGN_DOMAINS = {
    'github.com', 'github.io', 'raw.githubusercontent.com', 'gitlab.com',
    'microsoft.com', 'windowsupdate.com',
    'google.com', 'gstatic.com', 'googleapis.com',
    'stackoverflow.com', 'stackexchange.com',
    'wikipedia.org', 'wikimedia.org',
    'mozilla.org', 'mozilla.com',
    'sourceforge.net',
    'example.com', 'example.org', 'localhost',
    'roblox.com', 'robloxlabs.com',  # Game platform
    'youtube.com', 'youtu.be',  # Video platform
    'discord.com', 'discord.gg',  # Communication
    'reddit.com', 'imgur.com',  # Social media
}

# Regex Patterns
PATTERNS = {
    'EXECUTION': [
        (r'cmd\.exe', 3, 'Command Prompt Execution'),
        (r'powershell', 4, 'PowerShell Execution'),
        (r'ShellExecute(A|W)?\(.*?\b(http|https|explorer)\b', 1, 'Safe Shell Execution (URL/Explorer)'), # Lower score for common UI actions
        (r'ShellExecute(A|W)?(?!\(.*?\b(http|https|explorer)\b)', 3, 'API Shell Execution'),
        (r'\bstd::system\(|(?<![a-zA-Z_])system\s*\(', 3, 'System Command'), # Match system() calls (lowercase only)
        (r'CreateProcess', 3, 'API Process Creation'),
        (r'WinExec', 3, 'Legacy Execution API'),
    ],
    'MEMORY': [
        #(r'VirtualAlloc', 4, 'Memory Allocation (RWX Potential)'),
        #(r'WriteProcessMemory', 5, 'Process Memory Injection'),
        (r'CreateRemoteThread', 5, 'Remote Thread Injection'),
        (r'ReflectiveLoader', 5, 'Reflective DLL Injection Artifact'),
        (r'RtlMoveMemory', 2, 'Memory Manipulation'),
    ],
    'NETWORK': [
        (r'URLDownloadToFile', 3, 'File Download API'),
        (r'InternetOpen', 2, 'WinINet API'),
        (r'\bsocket\s*\(', 2, 'Raw Socket'), # Only match socket() function calls, not variable names
        (r'curl\s+', 3, 'Curl Command'),
        (r'\bwget\s+', 3, 'Wget Command'), # Word boundary to avoid matching "rawget"
        (r'Invoke-WebRequest', 4, 'PowerShell Download'),
        (r'bitsadmin', 4, 'BITS Persistence/Download'),
    ],
    'CRYPTO': [
        (r'CryptEncrypt', 2, 'Windows Crypto API'),
        (r'AESManaged', 2, '.NET AES'),
        (r'Rijndael', 2, 'Rijndael Cipher'),
        (r'Xor\s+', 1, 'XOR Operation (Potential Obfuscation)'),
    ],
    'OBFUSCATION': [
        (r'FromBase64String', 3, 'Base64 Decoding'),
        (r'-enc\s+', 4, 'PowerShell Encoded Command'),
        #(r'IsDebuggerPresent', 3, 'Anti-Debugging Check'),
        (r'(char|byte|uint8_t)\s+\w+\[\]\s*=\s*\{(\s*0x[0-9a-fA-F]{2}\s*,?)+\}', 4, 'Stack String / Shellcode Array'), # Stack string detection
        (r'["\'](?:\\x[0-9a-fA-F]{2}){20,}["\']', 4, 'Hex-Escaped String (Potential Obfuscation)'), # Detect \x escaped strings
    ]
}

# Pre-compiled patterns will be generated in scanner init, or lazily here?
# Let's keep config purely data for now.
BASE64_REGEX = re.compile(r'(?:[A-Za-z0-9+/]{4}){20,}(?:[A-Za-z0-9+/]{2}==|[A-Za-z0-9+/]{3}=)?')

# Default YARA Rules
DEFAULT_YARA_RULES = r"""
rule Suspicious_Powershell {
    meta:
        description = "Detects suspicious PowerShell commands"
        severity = "HIGH"
    strings:
        $s1 = "powershell" nocase
        $s2 = "-enc" nocase
        $s3 = "IEX" nocase
        $s4 = "Invoke-Expression" nocase
    condition:
        $s1 and ($s2 or $s3 or $s4)
}

rule PE_Header_In_Source {
    meta:
        description = "Detects embedded PE header in source code (shellcode/dropper)"
        severity = "CRITICAL"
    strings:
        $mz_hdr = { 4D 5A }
        $mz_hex = /0x4[dD],\s*0x5[aA]/
    condition:
        $mz_hdr at 0 or $mz_hex
}

rule Suspicious_API_Combinations {
    meta:
        description = "Detects combinations of APIs often used for injection"
        severity = "HIGH"
    strings:
        $valloc = "VirtualAlloc" fullword ascii wide
        $wpm = "WriteProcessMemory" fullword ascii wide
        $crt = "CreateRemoteThread" fullword ascii wide
        $ct = "CreateThread" fullword ascii wide
        $ll = "LoadLibrary" fullword ascii wide
        $gp = "GetProcAddress" fullword ascii wide
    condition:
        ($valloc and $wpm and ($crt or $ct)) or
        ($ll and $gp)
}
"""
