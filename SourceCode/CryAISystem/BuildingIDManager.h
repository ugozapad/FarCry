#pragma once
#include <vector>

typedef std::vector<bool> VectorBools;

class CBuildingIDManager
{

	VectorBools	m_vAvailable;

public:
	CBuildingIDManager(void);
	~CBuildingIDManager(void);
	int GetId(void);
	void FreeId(int nID);
	void FreeAll(void);
};
