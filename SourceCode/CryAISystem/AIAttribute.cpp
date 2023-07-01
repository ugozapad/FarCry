#include "stdafx.h"
#include "aiattribute.h"

CAIAttribute::CAIAttribute(void)
{
	m_pPrincipalObject = 0;
}

CAIAttribute::~CAIAttribute(void)
{
}

void CAIAttribute::ParseParameters( const AIObjectParameters &params)
{
}

void CAIAttribute::Event(unsigned short eType, SAIEVENT *pEvent)
{
}

void CAIAttribute::OnObjectRemoved(CAIObject *pObject)
{
	if (m_pPrincipalObject == pObject)
		m_pPrincipalObject = 0;
}

void CAIAttribute::Update()
{
}

bool CAIAttribute::CanBeConvertedTo(unsigned short type, void **pConverted)
{
	if (type == AIOBJECT_ATTRIBUTE)
	{
		*pConverted = (CAIAttribute*) this;
		return true;
	}
	return false;
}

CAIObject * CAIAttribute::GetPrincipalObject(void)
{
	return m_pPrincipalObject;
}
