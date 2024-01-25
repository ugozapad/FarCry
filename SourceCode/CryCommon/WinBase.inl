
char* strlwr(char* str)
{
	char* cp;               /* traverses string for C locale conversion */

	for (cp = str; *cp; ++cp)
	{
		if ('A' <= *cp && *cp <= 'Z')
			*cp += 'a' - 'A';
	}
	return str;
}

char* strupr(char* str)
{
	char* cp;               /* traverses string for C locale conversion */

	for (cp = str; *cp; ++cp)
	{
		if ('a' <= *cp && *cp <= 'z')
			*cp += 'A' - 'a';
	}
	return str;
}

void _makepath(char* path, const char* drive, const char* dir, const char* filename, const char* ext)
{
	char ch;
	char tmp[MAX_PATH];
	if (!path)
		return;
	tmp[0] = '\0';
	if (drive && drive[0])
	{
		tmp[0] = drive[0];
		tmp[1] = ':';
		tmp[2] = 0;
	}
	if (dir && dir[0])
	{
		cry_strcat(tmp, dir);
		ch = tmp[strlen(tmp) - 1];
		if (ch != '/' && ch != '\\')
			cry_strcat(tmp, "\\");
	}
	if (filename && filename[0])
	{
		cry_strcat(tmp, filename);
		if (ext && ext[0])
		{
			if (ext[0] != '.')
				cry_strcat(tmp, ".");
			cry_strcat(tmp, ext);
		}
	}
	strcpy(path, tmp);
}

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

bool IsBadReadPtr(void* ptr, unsigned int size)
{
	//too complicated to really support it
	return ptr ? false : true;
}

void OutputDebugString(const char* outputString)
{
	#if _DEBUG
	// There is no such thing as a debug console on XCode
	fprintf(stderr, "debug: %s\n", outputString);
	#endif
}

BOOL SetFileAttributes(
  LPCSTR lpFileName,
  DWORD dwFileAttributes)
{
	//TODO: implement
	printf("SetFileAttributes not properly implemented yet, should not matter\n");
	return TRUE;
}
