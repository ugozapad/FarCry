
#ifndef __Linux_Win32Wrapper_h__
#define __Linux_Win32Wrapper_h__
#pragma once

#define RemoveCRLF(...) //TODO: Add real function or delete RemoveCRLF from code

#define _fstat64 fstat64
#define DebugBreak() do { __asm__ volatile ("int $3"); } while(0)
#define Int32x32To64(a, b) ((uint64)((uint64)(a)) * (uint64)((uint64)(b)))

#define CONST const
#define DECLARE_HANDLE(name) struct name##__ { int unused; }; typedef struct name##__ *name

extern HANDLE CreateFile(const char* lpFileName,DWORD dwDesiredAccess,DWORD dwShareMode,void* lpSecurityAttributes,DWORD dwCreationDisposition,DWORD dwFlagsAndAttributes,HANDLE hTemplateFile);
extern BOOL CloseHandle(HANDLE hObject);
extern BOOL CancelIo(HANDLE hFile);
extern BOOL RemoveDirectory(LPCSTR lpPathName);
extern BOOL DeleteFile(LPCSTR lpFileName);
extern DWORD GetFileSize(HANDLE hFile, DWORD *lpFileSizeHigh);
extern BOOL ReadFile(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped
    );
extern BOOL ReadFileEx(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPOVERLAPPED lpOverlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);
extern DWORD SetFilePointer(
    HANDLE hFile,
    LONG lDistanceToMove,
    PLONG lpDistanceToMoveHigh,
    DWORD dwMoveMethod
    );
extern DWORD GetCurrentDirectory(DWORD nBufferLength, char* lpBuffer);
extern BOOL SetFileAttributes(LPCSTR, DWORD attributes);
extern BOOL MakeSureDirectoryPathExists(PCSTR DirPath);
extern BOOL SwapBuffers(HDC);
extern int _mkdir(const char *dirname);

extern BOOL SetFileTime(
    HANDLE hFile,
    const FILETIME *lpCreationTime,
    const FILETIME *lpLastAccessTime,
    const FILETIME *lpLastWriteTime
    );
extern BOOL GetFileTime(HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime);
extern uint64_t __rdtsc();

extern HRESULT GetOverlappedResult(HANDLE hFile, void* lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait);

extern const BOOL compareTextFileStrings(const char* cpReadFromFile, const char* cpToCompareWith);
extern unsigned long timeGetTime(void);


typedef struct _MEMORYSTATUS
{
    DWORD dwLength;
    DWORD dwMemoryLoad;
    SIZE_T dwTotalPhys;
    SIZE_T dwAvailPhys;
    SIZE_T dwTotalPageFile;
    SIZE_T dwAvailPageFile;
    SIZE_T dwTotalVirtual;
    SIZE_T dwAvailVirtual;
} MEMORYSTATUS, * LPMEMORYSTATUS;

extern void GlobalMemoryStatus(LPMEMORYSTATUS lpmem);


typedef struct tagRECT
{
    LONG    left;
    LONG    top;
    LONG    right;
    LONG    bottom;
} RECT, * PRECT;

typedef union _ULARGE_INTEGER
{
    struct
    {
        DWORD LowPart;
        DWORD HighPart;
    };
    unsigned long long QuadPart;
} ULARGE_INTEGER;

#ifdef __cplusplus
inline LONG CompareFileTime(const FILETIME* lpFileTime1, const FILETIME* lpFileTime2)
#else
static LONG CompareFileTime(const FILETIME* lpFileTime1, const FILETIME* lpFileTime2)
#endif
{
    ULARGE_INTEGER u1, u2;
    memcpy(&u1, lpFileTime1, sizeof u1);
    memcpy(&u2, lpFileTime2, sizeof u2);
    if (u1.QuadPart < u2.QuadPart)
    {
        return -1;
    }
    else
    if (u1.QuadPart > u2.QuadPart)
    {
        return 1;
    }
    return 0;
}

typedef struct _SYSTEMTIME
{
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, * PSYSTEMTIME, * LPSYSTEMTIME;

//////////////////////////////////////////////////////////////////////////
extern BOOL SystemTimeToFileTime(const SYSTEMTIME* syst, LPFILETIME ft);
//Win32API function declarations actually used
extern bool IsBadReadPtr(void* ptr, unsigned int size);

// Defined in the launcher.
void OutputDebugString(const char*);


//critical section stuff
typedef pthread_mutex_t CRITICAL_SECTION;
#ifdef __cplusplus
inline void InitializeCriticalSection(CRITICAL_SECTION *lpCriticalSection)
{
    pthread_mutexattr_t pthread_mutexattr_def;
    pthread_mutexattr_settype(&pthread_mutexattr_def, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(lpCriticalSection, &pthread_mutexattr_def);
}
inline void EnterCriticalSection(CRITICAL_SECTION *lpCriticalSection){pthread_mutex_lock(lpCriticalSection);}
inline void LeaveCriticalSection(CRITICAL_SECTION *lpCriticalSection){pthread_mutex_unlock(lpCriticalSection);}
inline void DeleteCriticalSection(CRITICAL_SECTION *lpCriticalSection){}
#else
static void InitializeCriticalSection(CRITICAL_SECTION *lpCriticalSection)
{
    pthread_mutexattr_t pthread_mutexattr_def;
    pthread_mutexattr_settype(&pthread_mutexattr_def, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(lpCriticalSection, &pthread_mutexattr_def);
}
static void EnterCriticalSection(CRITICAL_SECTION *lpCriticalSection){pthread_mutex_lock(lpCriticalSection);}
static void LeaveCriticalSection(CRITICAL_SECTION *lpCriticalSection){pthread_mutex_unlock(lpCriticalSection);}
static void DeleteCriticalSection(CRITICAL_SECTION *lpCriticalSection){}
#endif


extern bool QueryPerformanceCounter(LARGE_INTEGER* counter);
extern bool QueryPerformanceFrequency(LARGE_INTEGER* frequency);
#ifdef __cplusplus

inline uint32 GetTickCount()
{
    LARGE_INTEGER count, freq;
    QueryPerformanceCounter(&count);
    QueryPerformanceFrequency(&freq);
    return uint32(count.QuadPart * 1000 / freq.QuadPart);
}

#define IGNORE              0       // Ignore signal
#define INFINITE            0xFFFFFFFF  // Infinite timeout
#endif

#include <dirent.h>

typedef int64 __time64_t;     /* 64-bit time value */

typedef struct __finddata64_t
{
    //!< atributes set by find request
    unsigned    int attrib;         //!< attributes, only directory and readonly flag actually set
    int64  time_create;        //!< creation time, cannot parse under linux, last modification time is used instead (game does nowhere makes decision based on this values)
    int64  time_access;        //!< last access time
    int64  time_write;         //!< last modification time
    int64  size;                       //!< file size (for a directory it will be the block size)
    char        name[256];          //!< file/directory name

private:
    int                                 m_LastIndex;                    //!< last index for findnext
    char                                m_DirectoryName[260];           //!< directory name, needed when getting file attributes on the fly
    char                                m_ToMatch[260];                     //!< pattern to match with
    DIR*                                m_Dir;                                  //!< directory handle
    std::vector<string> m_Entries;                      //!< all file entries in the current directories
public:

    inline __finddata64_t()
        : attrib(0)
        , time_create(0)
        , time_access(0)
        , time_write(0)
        , size(0)
        , m_LastIndex(-1)
        , m_Dir(NULL)
    {
        memset(name, '0', 256);
    }
    ~__finddata64_t();

    //!< copies and retrieves the data for an actual match (to not waste any effort retrioeving data for unused files)
    void CopyFoundData(const char* rMatchedFileName);

public:
    //!< global _findfirst64 function using struct above, can't be a member function due to required semantic match
    friend intptr_t _findfirst64(const char* pFileName, __finddata64_t* pFindData);
    //!< global _findnext64 function using struct above, can't be a member function due to required semantic match
    friend int _findnext64(intptr_t last, __finddata64_t* pFindData);
}__finddata64_t;

typedef struct _finddata_t
    : public __finddata64_t
{}_finddata_t;//!< need inheritance since in many places it get used as struct _finddata_t
extern int _findnext64(intptr_t last, __finddata64_t* pFindData);
extern intptr_t _findfirst64(const char* pFileName, __finddata64_t* pFindData);
extern int _findclose(intptr_t handle);

extern DWORD GetFileAttributes(LPCSTR lpFileName);

extern const bool GetFilenameNoCase(const char* file, char*, const bool cCreateNew = false);

extern BOOL GetUserName(LPSTR lpBuffer, LPDWORD nSize);

//error code stuff
//not thread specific, just a coarse implementation for the main thread
inline DWORD GetLastError() { return errno; }
inline void SetLastError(DWORD dwErrCode) { errno = dwErrCode; }

//////////////////////////////////////////////////////////////////////////
#define GENERIC_READ                     (0x80000000L)
#define GENERIC_WRITE                    (0x40000000L)
#define GENERIC_EXECUTE                  (0x20000000L)
#define GENERIC_ALL                      (0x10000000L)

#define CREATE_NEW          1
#define CREATE_ALWAYS       2
#define OPEN_EXISTING       3
#define OPEN_ALWAYS         4
#define TRUNCATE_EXISTING   5

#define FILE_SHARE_READ                     0x00000001
#define FILE_SHARE_WRITE                    0x00000002
#define OPEN_EXISTING                           3
#define FILE_FLAG_OVERLAPPED            0x40000000
#define INVALID_FILE_SIZE                   ((DWORD)0xFFFFFFFFl)
#define FILE_BEGIN                              0
#define FILE_CURRENT                            1
#define FILE_END                                    2
#define ERROR_NO_SYSTEM_RESOURCES 1450L
#define ERROR_INVALID_USER_BUFFER   1784L
#define ERROR_INVALID_PIXEL_FORMAT  2000L
#define ERROR_INVALID_DATA       13L
#define ERROR_NOT_ENOUGH_MEMORY   8L
#define ERROR_PATH_NOT_FOUND      3L
#define FILE_FLAG_SEQUENTIAL_SCAN 0x08000000

//////////////////////////////////////////////////////////////////////////
extern threadID GetCurrentThreadId();

//////////////////////////////////////////////////////////////////////////
extern HANDLE CreateEvent(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCSTR lpName
    );


//////////////////////////////////////////////////////////////////////////
extern DWORD Sleep(DWORD dwMilliseconds);

//////////////////////////////////////////////////////////////////////////
extern DWORD SleepEx(DWORD dwMilliseconds, BOOL bAlertable);

//////////////////////////////////////////////////////////////////////////
extern DWORD WaitForSingleObjectEx(
    HANDLE hHandle,
    DWORD dwMilliseconds,
    BOOL bAlertable);

//////////////////////////////////////////////////////////////////////////
extern DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);

//////////////////////////////////////////////////////////////////////////
extern BOOL SetEvent(HANDLE hEvent);

//////////////////////////////////////////////////////////////////////////
extern BOOL ResetEvent(HANDLE hEvent);

//////////////////////////////////////////////////////////////////////////
typedef DWORD (* PTHREAD_START_ROUTINE)(LPVOID lpThreadParameter);
typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;

//////////////////////////////////////////////////////////////////////////
extern HANDLE CreateThread(
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    SIZE_T dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId
    );

//////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus

//helper function
extern void adaptFilenameToLinux(string& rAdjustedFilename);
extern const int comparePathNames(const char* cpFirst, const char* cpSecond, unsigned int len);//returns 0 if identical
extern void replaceDoublePathFilename(char* szFileName);//removes "\.\" to "\" and "/./" to "/"

//////////////////////////////////////////////////////////////////////////
extern char* _fullpath(char* absPath, const char* relPath, size_t maxLength);
//////////////////////////////////////////////////////////////////////////
extern void _makepath(char* path, const char* drive, const char* dir, const char* filename, const char* ext);

extern void _splitpath(const char* inpath, char* drv, char* dir, char* fname, char* ext);

//////////////////////////////////////////////////////////////////////////
extern int memicmp(LPCSTR s1, LPCSTR s2, DWORD len);

extern "C" char* strlwr (char* str);
extern "C" char* strupr(char* str);

extern char* _strtime(char* date);
extern char* _strdate(char* date);

#endif //__cplusplus

#endif // __Linux_Win32Wrapper_h__
