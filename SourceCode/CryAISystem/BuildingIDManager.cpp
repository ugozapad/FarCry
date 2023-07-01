#include "stdafx.h"
#include "buildingidmanager.h"

CBuildingIDManager::CBuildingIDManager(void)
{
	m_vAvailable.resize(100);
	VectorBools::iterator bi;
	for (bi=m_vAvailable.begin();bi!=m_vAvailable.end();bi++)
		(*bi) = false;
}

CBuildingIDManager::~CBuildingIDManager(void)
{
}

int CBuildingIDManager::GetId(void)
{
	int index = 0;
	VectorBools::iterator bi;
	for (bi=m_vAvailable.begin();bi!=m_vAvailable.end();bi++,index++)
		if (!(*bi))
		{
			(*bi) = true;
			return index;
		}

	return index;
}



void CBuildingIDManager::FreeId(int nID)
{
	if (nID>=0)
		m_vAvailable[nID] = false;
}

void CBuildingIDManager::FreeAll(void)
{
	VectorBools::iterator bi;
	for (bi=m_vAvailable.begin();bi!=m_vAvailable.end();bi++)
		(*bi) = false;
}
