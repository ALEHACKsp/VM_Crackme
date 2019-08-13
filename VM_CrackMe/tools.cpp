#include "tools.h"
#include "instr.h"

void printf(char* string, DWORD hex)
{
	char buf[512];
	DWORD tmp;
	if (lstrlenA(string))
	{
		wsprintfA(buf, "%s %p\n", string, hex);
		HANDLE std = GetStdHandle(STD_OUTPUT_HANDLE);
		WriteFile(std, buf, (DWORD)lstrlenA(buf), &tmp, 0);
	}
}

void printf(char* string)
{
	char buf[512];
	DWORD tmp;
	if (lstrlenA(string))
	{
		wsprintfA(buf, "%s\n", string);
		HANDLE std = GetStdHandle(STD_OUTPUT_HANDLE);
		WriteFile(std, buf, (DWORD)lstrlenA(buf), &tmp, 0);
	}
}

bool mem_zero(void* mem, int size)
{
	OBF_BEGIN
	int i = N(0);
	FOR(V(i), V(i) < V(size), V(i)++)
		((BYTE*)mem)[V(i)] = N(0);
	ENDFOR
	RETURN(TRUE);
	OBF_END
}

bool mem_copy(void* src, void* dest, int size)
{
	OBF_BEGIN
	int i = N(0);
	FOR (V(i), V(i) < V(size), V(i)++)
		((BYTE*)dest)[V(i)] = ((BYTE*)src)[V(i)];
	ENDFOR
	RETURN(TRUE);
	OBF_END
}


bool CompareString2(byte* str1, byte* str2, int len)
{
	OBF_BEGIN
	bool valid = true;
	int i = N(0);
	FOR (V(i), V(i) < V(len), V(i)++)
		IF (str1[V(i)] != str2[V(i)])
			valid = false;
		ENDIF
	ENDFOR
	RETURN (valid);
	OBF_END
}
