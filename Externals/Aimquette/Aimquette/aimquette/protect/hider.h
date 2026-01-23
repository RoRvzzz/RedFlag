#pragma once


#include <vector>
#include <random>
#include <wtypes.h>
#include <map>



void encryptMemory(void* memoryBlock, size_t size) {
    DWORD oldProtect;
    VirtualProtect(memoryBlock, size, PAGE_EXECUTE_READWRITE, &oldProtect);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    uint8_t* bytes = static_cast<uint8_t*>(memoryBlock);
    for (size_t i = 0; i < size; ++i) {
        bytes[i] ^= dis(gen);
    }

    VirtualProtect(memoryBlock, size, oldProtect, &oldProtect);
}
