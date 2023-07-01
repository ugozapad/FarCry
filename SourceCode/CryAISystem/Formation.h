#ifndef _FORMATION_
#define _FORMATION_

#include <vector>

#ifdef LINUX
#	include "platform.h"
#endif


class CAIObject;
class CAISystem;
struct IRenderer;

typedef std::vector<Vec3> VectorOfVectors;

typedef struct FormationDescriptor
{
	string sName;
	VectorOfVectors vOffsets;
} FormationDescriptor;

typedef std::vector<CAIObject*> FormationDummies;
//typedef std::vector<bool> AvailabilityVector;

class CFormation
{
	VectorOfVectors m_vPoints;
	FormationDummies m_vWorldPoints;
	FormationDummies m_vReservations;

	Vec3 m_vPos;
	Vec3 m_vAngles;

	CAISystem *m_pAISystem;

	bool m_bReservationAllowed;

public:
	CFormation(CAISystem *pAISystem);
	~CFormation(void);
	// fills the formation class with all necessary information
	void Create(FormationDescriptor & desc);
	// Update of the formation (refreshes position of formation points)
	void Update(CAIObject *pOwner);
	// returns an available formation point, if that exists
	CAIObject * GetFormationPoint(CAIObject *pRequester);
	void Draw(IRenderer * pRenderer);
	void FreeFormationPoint(CAIObject * pCurrentHolder);

};

#endif