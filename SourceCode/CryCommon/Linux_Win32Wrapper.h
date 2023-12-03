
#ifndef __Linux_Win32Wrapper_h__
#define __Linux_Win32Wrapper_h__
#pragma once

extern "C" char* strlwr(char* str);
extern "C" char* strupr(char* str);

extern void _makepath(char* path, const char* drive, const char* dir, const char* filename, const char* ext);

extern bool IsBadReadPtr(void* ptr, unsigned int size);
void OutputDebugString(const char*);
extern BOOL SetFileAttributes(LPCSTR, DWORD attributes);

#endif // __Linux_Win32Wrapper_h__
