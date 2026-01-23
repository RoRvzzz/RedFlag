#pragma once

// VMProtect SDK Integration
// Include the official VMProtect SDK header
// VMProtectSDK.h is located in the same directory (util/protection/vmp/)
#include "VMProtectSDK.h"

// Convenience macros for code protection markers
#define VMP_BEGIN_MARKER(name) VMProtectBegin(name)
#define VMP_END_MARKER() VMProtectEnd()
#define VMP_VIRTUALIZATION(name) VMProtectBeginVirtualization(name)
#define VMP_MUTATION(name) VMProtectBeginMutation(name)
#define VMP_ULTRA(name) VMProtectBeginUltra(name)

// Utility functions wrapper
#define VMP_IS_PROTECTED() VMProtectIsProtected()
#define VMP_IS_DEBUGGER() VMProtectIsDebuggerPresent(false)
#define VMP_IS_VM() VMProtectIsVirtualMachinePresent()

// String decryption wrappers
#define VMP_DECRYPT_STR_A(str) VMProtectDecryptStringA(str)
#define VMP_DECRYPT_STR_W(str) VMProtectDecryptStringW(str)
#define VMP_FREE_STR(ptr) VMProtectFreeString(ptr)
