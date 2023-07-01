#ifndef __CRYAISYSTEM_H_
#define __CRYAISYSTEM_H_


#if defined(_XBOX) || defined(PS2) || defined(LINUX)
#define CRYAIAPI
#else
#ifdef CRYAISYSTEM_EXPORTS
#define CRYAIAPI	__declspec(dllexport)
#else
#define CRYAIAPI	__declspec(dllimport)
#endif
//#else
//#define CRYAIAPI
#endif


struct IAISystem;
struct ISystem;



extern "C"
{

	CRYAIAPI IAISystem *CreateAISystem( ISystem *pSystem);

}

#endif //__CRYAISYSTEM_H_