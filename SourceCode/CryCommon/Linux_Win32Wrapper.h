
#ifndef __Linux_Win32Wrapper_h__
#define __Linux_Win32Wrapper_h__
#pragma once

#define RemoveCRLF(...) //TODO: Add real function or delete RemoveCRLF from code

#define DebugBreak() do { __asm__ volatile ("int $3"); } while(0)

//////////////////////////////////////////////////////////////////////////
extern char*    _fullpath(char* absPath, const char* relPath, size_t maxLength);
extern intptr_t _findfirst64(const char* filespec, struct __finddata64_t* fileinfo);
extern int      _findnext64(intptr_t handle, struct __finddata64_t* fileinfo);
extern int      _findclose(intptr_t handle);

extern "C" char* strlwr(char* str);
extern "C" char* strupr(char* str);

extern void _makepath(char* path, const char* drive, const char* dir, const char* filename, const char* ext);
extern void _splitpath(const char* inpath, char* drv, char* dir, char* fname, char* ext);

extern bool QueryPerformanceCounter(LARGE_INTEGER* counter);
extern bool QueryPerformanceFrequency(LARGE_INTEGER* frequency);
inline uint32 GetTickCount()
{
	LARGE_INTEGER count, freq;
	QueryPerformanceCounter(&count);
	QueryPerformanceFrequency(&freq);
	return uint32(count.QuadPart * 1000 / freq.QuadPart);
}

//begin--------------------------------findfirst/-next declaration/implementation----------------------------------------------------

#include <dirent.h>

typedef int64 __time64_t;     /* 64-bit time value */

//! < Atributes set by find request.
typedef struct __finddata64_t
{
	unsigned int attrib;            //!< Attributes, only directory and readonly flag actually set.
	__time64_t   time_create;       //!< Creation time, cannot parse under linux, last modification time is used instead (game does nowhere makes decision based on this values).
	__time64_t   time_access;       //!< Last access time.
	__time64_t   time_write;        //!< Last modification time.
	__time64_t   size;              //!< File size (for a directory it will be the block size).
	char         name[256];         //!< File/directory name.

private:
	int                 m_LastIndex;          //!< Last index for findnext.
	char                m_DirectoryName[260]; //!< Directory name, needed when getting file attributes on the fly.
	char                m_ToMatch[260];       //!< Pattern to match with.
	DIR*                m_Dir;                //!< Directory handle.
	std::vector<string> m_Entries;            //!< All file entries in the current directories.
public:

	inline __finddata64_t() :
		attrib(0), time_create(0), time_access(0), time_write(0),
		size(0), m_LastIndex(-1), m_Dir(NULL)
	{
		memset(name, '0', 256);
	}
	~__finddata64_t();

	//! Copies and retrieves the data for an actual match (to not waste any effort retrioeving data for unused files).
	void CopyFoundData(const char* rMatchedFileName);

public:
	//! Global _findfirst64 function using struct above, can't be a member function due to required semantic match.
	friend intptr_t _findfirst64(const char* pFileName, __finddata64_t* pFindData);

	//! Global _findnext64 function using struct above, can't be a member function due to required semantic match.
	friend int _findnext64(intptr_t last, __finddata64_t* pFindData);
}__finddata64_t;

//! Need inheritance since in many places it get used as struct _finddata_t.
typedef struct _finddata_t : public __finddata64_t
{}_finddata_t;

extern int      _findnext64(intptr_t last, __finddata64_t* pFindData);
extern intptr_t _findfirst64(const char* pFileName, __finddata64_t* pFindData);

//end--------------------------------findfirst/-next declaration/implementation----------------------------------------------------

extern bool IsBadReadPtr(void* ptr, unsigned int size);
void OutputDebugString(const char*);
extern BOOL SetFileAttributes(LPCSTR, DWORD attributes);

bool MakeSureDirectoryPathExists(const char* path);

#endif // __Linux_Win32Wrapper_h__
