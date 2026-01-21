"""
Configuration and constants for RedFlag
"""
import re

BANNER = r"""
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

# Regex Patterns
PATTERNS = {
    'EXECUTION': [
        (r'cmd\.exe', 3, 'Command Prompt Execution'),
        (r'powershell', 4, 'PowerShell Execution'),
        (r'ShellExecute(A|W)?\(.*?\b(http|https|explorer)\b', 1, 'Safe Shell Execution (URL/Explorer)'), # Lower score for common UI actions
        (r'ShellExecute(A|W)?(?!\(.*?\b(http|https|explorer)\b)', 3, 'API Shell Execution'),
        (r'\bsystem\((?!"cls"|"pause").*?\)', 3, 'System Command'), # Ignore cls and pause
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
        (r'socket', 2, 'Raw Socket'),
        (r'curl\s+', 3, 'Curl Command'),
        (r'wget\s+', 3, 'Wget Command'),
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
