#pragma once

#include "Windows.h"

void printf(char* string, DWORD hex);
void printf(char* string);
bool mem_zero(void* mem, int size);
bool mem_copy(void* src, void* dest, int size);
bool CompareString2(byte* str1, byte* str2, int len);
