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
    'x64', 'x86', 'debug', 'release'
}

# Files to never scan (the tool itself)
IGNORE_FILES = {'redflag.py', 'check.py', 'checker.py', 'main.py', '__init__.py'}

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
        (r'ShellExecute', 3, 'API Shell Execution'),
        (r'CreateProcess', 3, 'API Process Creation'),
        (r'system\(', 3, 'System Command'),
        (r'WinExec', 3, 'Legacy Execution API'),
    ],
    'MEMORY': [
        (r'VirtualAlloc', 4, 'Memory Allocation (RWX Potential)'),
        (r'WriteProcessMemory', 5, 'Process Memory Injection'),
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
        (r'IsDebuggerPresent', 3, 'Anti-Debugging Check'),
    ]
}

# Pre-compiled patterns will be generated in scanner init, or lazily here?
# Let's keep config purely data for now.
BASE64_REGEX = re.compile(r'(?:[A-Za-z0-9+/]{4}){20,}(?:[A-Za-z0-9+/]{2}==|[A-Za-z0-9+/]{3}=)?')
