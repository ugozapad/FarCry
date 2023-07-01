#ifndef _AI_ATTRIBUTE_
#define _AI_ATTRIBUTE_

#include "aiobject.h"

class CAIAttribute : public CAIObject
{

	// this attribute is attributed to this REAL object
	CAIObject *m_pPrincipalObject;

public:
	CAIAttribute(void);
	~CAIAttribute(void);

	void ParseParameters( const AIObjectParameters &params);
	void Update();
	void Event(unsigned short eType, SAIEVENT *pEvent);
	bool CanBeConvertedTo(unsigned short type, void **pConverted);
	void OnObjectRemoved(CAIObject *pObject);
	CAIObject * GetPrincipalObject(void);

	void Bind(IAIObject* bind) { m_pPrincipalObject = (CAIObject*) bind; }
	void Unbind() { m_pPrincipalObject = 0;	}
};

#endif