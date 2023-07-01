#include "stdafx.h"
#include "formation.h"
#include "AIObject.h"
#include "CAISystem.h"
#include <Cry_Math.h>

#if !defined(LINUX)
#include <assert.h>
#endif

//#include <malloc.h>
//#include <Cry_Matrix.h>
#include <ISystem.h>

#include <IRenderer.h>
#include <I3DEngine.h>

CFormation::CFormation(CAISystem *pAISystem)
{
	m_pAISystem = pAISystem;
	m_bReservationAllowed = true;
}

CFormation::~CFormation(void)
{
	if (!m_vWorldPoints.empty())
	{
		FormationDummies::iterator fi;
		for (fi=m_vWorldPoints.begin();fi!=m_vWorldPoints.end();fi++)
		{
			
			m_pAISystem->RemoveDummyObject((*fi));

		}

		m_vWorldPoints.clear();
	}
}

// fills the formation class with all necessary information
void CFormation::Create(FormationDescriptor & desc)
{
	VectorOfVectors::iterator vi;
	int i=0;

	for (vi=desc.vOffsets.begin();vi!=desc.vOffsets.end();vi++)
	{
		Vec3d pos = (*vi);
		CAIObject *pFormationDummy = m_pAISystem->CreateDummyObject();

		pFormationDummy->SetPos(pos);
		pFormationDummy->SetAngles(Vec3d(0,0,0));
		char name[255];
		sprintf(name,"FORMATION_%d",i++);
		pFormationDummy->SetName(name);

		m_vPoints.push_back(pos);
		m_vWorldPoints.push_back(pFormationDummy);
		m_vReservations.push_back(false);
	}

}

// Update of the formation (refreshes position of formation points)
void CFormation::Update(CAIObject *pOwner)
{
	if (!m_bReservationAllowed)
		return;
	Matrix44 mat;
	int count = m_vPoints.size();

	for (int i=0;i<count;i++)
	{
		Vec3d pos = m_vPoints[i];
		CAIObject *pFormationDummy = m_vWorldPoints[i];


		//mat.Identity();
		//mat=GetRotationZYX44(-gf_DEGTORAD*pOwner->GetAngles() )*mat; //NOTE: anges in radians and negated 
		//mat=GetTranslationMat(pos)*mat;

		//OPTIMIZED_BY_IVO
		Matrix34 t		= Matrix34::CreateTranslationMat(pos);
		Matrix33 r33	=	Matrix33::CreateRotationXYZ( Deg2Rad(pOwner->GetAngles()) );	
//		mat	=	r33*t;
		mat			    = Matrix44(r33*t);
        
		mat			    = GetTransposed44(mat); 


		pos = mat.TransformPointOLD(pos);


		pos+=pOwner->GetPos();
		pos.z = m_pAISystem->m_pSystem->GetI3DEngine()->GetTerrainElevation(pos.x,pos.y);
		pFormationDummy->SetPos(pos);

	}
}

// returns an available formation point, if that exists right now by proximity
CAIObject * CFormation::GetFormationPoint(CAIObject *pRequester)
{
	CAIObject *pPoint = NULL;
	float mindist = 2000;
	int size = m_vPoints.size();
	int index = -1;

		
	for (int i=0;i<size;i++)
	{
			if (m_vReservations[i] == pRequester)
				m_vReservations[i] = 0;

			if (!m_vReservations[i])
			{
				CAIObject *pThisPoint = m_vWorldPoints[i];
				float dist = (pThisPoint->GetPos() - pRequester->GetPos()).GetLength();
				if (dist < mindist)
				{
					index = i;
					mindist = dist;
					pPoint = pThisPoint;
				}
			}
	

	}

	if ((index >= 0) && (m_bReservationAllowed))
		m_vReservations[index] = pRequester;

	if (pPoint)
	{
		//Vec3d pos = pPoint->GetPos();
		//pos.z = pRequester->GetPos().z;
		//pPoint->SetPos(pos);
		pPoint->SetEyeHeight(1.4f);
	}
	else
		int A=5;

	return pPoint;
}

void CFormation::Draw(IRenderer * pRenderer)
{
	FormationDummies::iterator fi;

	for (fi=m_vWorldPoints.begin();fi!=m_vWorldPoints.end();fi++)
	{
		Vec3d pos = (*fi)->GetPos();
		pRenderer->SetMaterialColor(0,1.f,0,1.f);
		pRenderer->DrawBall(pos,0.5f);
	}

}

void CFormation::FreeFormationPoint(CAIObject * pCurrentHolder)
{
	int size = m_vReservations.size();
	
	for (int i=0;i<size;i++)
	{
		if (m_vReservations[i] == pCurrentHolder)
			m_vReservations[i] = 0;
	}
}


