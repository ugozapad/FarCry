// AIObject.cpp: implementation of the CAIObject class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AIObject.h"
#include "CAIsystem.h"
#include "Graph.h"
#include <float.h>
#include <ISystem.h>
#include <ILog.h>

#ifdef LINUX
#	include <platform.h>
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAIObject::CAIObject():
m_bNeedsPathIndoor(true),
m_bNeedsPathOutdoor(true),
m_bForceTargetPos(false),
m_vBoundPosition(0,0,0),
m_bIsBoind(false),
m_fPassRadius(0.6f)
{
	m_bDEBUGDRAWBALLS = false;
	m_bEnabled = true;
	m_bSleeping = false;
	m_pLastNode = 0;
	m_pAISystem = 0;
	m_bMoving = false;
	m_pProxy = 0;
	m_fRadius = 0;
	m_bCloaked = false;
	m_bCanReceiveSignals = true;
}

CAIObject::~CAIObject()
{
AIBINDLISTiterator itr = m_lstBindings.begin();

	for(;itr!=m_lstBindings.end();itr++)
	{
		CAIObject *pChild = *itr;
		GetAISystem()->RemoveObject( pChild );
	}
	m_lstBindings.clear();
}




void CAIObject::SetPos(const Vec3d &pos, bool bKeepEyeHeight)
{
	if (_isnan(pos.x) || _isnan(pos.y) || _isnan(pos.z))
	{
		AIWarning("NotANumber tried to be set for position of AI entity %s",m_sName.c_str());
		return;
	}
	m_vLastPosition = m_vPosition;
	m_vPosition = pos;


	// fixed eyeHeight for vehicles
	// m_fEyeHeight used for other stuff 
	if(GetType() == AIOBJECT_VEHICLE)
	{
	float	vehicleEyeHeight = 2.5f;
		m_vLastPosition.z-=vehicleEyeHeight;
		if ( !IsEquivalent(m_vLastPosition,pos,VEC_EPSILON) )
			m_bMoving = true;
		else
			m_bMoving = false;
		m_vPosition.z += vehicleEyeHeight;
		return;		
	}

	if (bKeepEyeHeight) 
		m_vLastPosition.z-=m_fEyeHeight;
	if ( !IsEquivalent(m_vLastPosition,pos,VEC_EPSILON) )
		m_bMoving = true;
	else
		m_bMoving = false;
	if (bKeepEyeHeight)
		m_vPosition.z += m_fEyeHeight;
}

const Vec3d &CAIObject::GetPos()
{
	return  m_vPosition;
}

unsigned short CAIObject::GetType()
{
	return m_nObjectType;
}

void CAIObject::SetType(unsigned short type)
{
	m_nObjectType = type;
}

void * CAIObject::GetAssociation()
{
	return m_pAssociation;
}

void CAIObject::SetAssociation(void *pAssociation)
{
	m_pAssociation = pAssociation;
}

void CAIObject::Update()
{
	if (m_pProxy)
		m_pProxy->Update(&m_State);
	if (!m_lstBindings.empty())
		UpdateHierarchy();
}

void CAIObject::UpdateHierarchy()
{
AIBINDLISTiterator itr = m_lstBindings.begin();

	Matrix44 mat	=	Matrix34::CreateRotationXYZ( Deg2Rad(m_vOrientation), m_vPosition - Vec3d(0,0,m_fEyeHeight));
	mat=GetTransposed44(mat); //TODO: remove this after E3 and use Matrix34 instead of Matrix44

	for(;itr!=m_lstBindings.end();itr++)
	{
	CAIObject *pChild = *itr;
		pChild->SetPos(mat.TransformPointOLD(pChild->GetPosBound()));
//		m_vPosition = mat.TransformPoint(m_vBoundPosition);

		mat.NoScale();
		CryQuat cxquat = Quat( mat );
		CryQuat rxquat;
		rxquat.SetRotationXYZ(DEG2RAD(Vec3(0,0,0)));
		CryQuat result = cxquat*rxquat;
		Vec3d finalangles = Ang3::GetAnglesXYZ(Matrix33(result));
		pChild->SetAngles(RAD2DEG(finalangles));
	}
}

void	CAIObject::CreateBoundObject( unsigned short type, const Vec3d& vBindPos, const Vec3d& vBindAngl)
{
CAIObject	*pChild = (CAIObject*)m_pAISystem->CreateAIObject( type, NULL );
string name;
char	buffer[5];
	sprintf( buffer, "%d\0", m_lstBindings.size()+1 );
	name = GetName() + string("_bound_") + string(buffer);
	pChild->SetName(name.c_str());
	m_lstBindings.push_back( pChild );
	pChild->SetPosBound(vBindPos);
	UpdateHierarchy();
}

void	CAIObject::SetPosBound(const Vec3d &pos)
{
	m_vBoundPosition = pos;
}

const Vec3d &CAIObject::GetPosBound()
{
	return m_vBoundPosition;
}

void CAIObject::SetEyeHeight(float height)
{
	if (_isnan(height))
	{
		AIWarning(" NotANumber set for eye height of AI Object %s",m_sName.c_str());
		return;
	}

	m_fEyeHeight = height;
}

/*
void CAIObject::SetMinAlt(float height)
{
	m_fEyeHeight = height;
}
*/

void CAIObject::ParseParameters(const AIObjectParameters &params)
{
//	m_fEyeHeight = params.fEyeHeight;


}



bool CAIObject::CanBeConvertedTo(unsigned short type, void **pConverted)
{
	*pConverted = 0;
	return false;
}

void CAIObject::SetName(const char *pName)
{
	char str[128];
	strcpy(str,pName);
	int i=1;
	while (GetAISystem()->GetAIObjectByName(str))
	{
 		sprintf(str,"%s_%02d",pName,i);
        i++;
	}
	m_sName = str;
}

char * CAIObject::GetName()
{
  return (char*)m_sName.c_str();
}

void CAIObject::SetAngles(const Vec3d &angles)
{

	int ax,ay,az;
	ax = (int) angles.x;
	ax/= 360;
	ay = (int) angles.y;
	ay/= 360;
	az = (int) angles.z;
	az/= 360;

	m_vOrientation.x = angles.x - (ax*360);
	m_vOrientation.y = angles.y - (ay*360);
	m_vOrientation.z = angles.z - (az*360);


}

void CAIObject::IsEnabled(bool enabled)
{
	m_bEnabled = enabled;
}

void CAIObject::GetLastPosition(Vec3d &pos)
{
	pos = m_vLastPosition;
}

void CAIObject::SetAISystem(CAISystem *pSystem)
{
	m_pAISystem = pSystem;
}

void CAIObject::Reset(void)
{
	m_pLastNode = 0;
}

float CAIObject::GetEyeHeight(void)
{
	return m_fEyeHeight;
}

// returns the state of this object
SOBJECTSTATE * CAIObject::GetState(void)
{
	return &m_State;
}

// nSignalID = 73 allow duplicating signals
//
void CAIObject::SetSignal(int nSignalID, const char * szText, void *pSender)
{
	if( nSignalID != 10	)
	{
		std::vector<AISIGNAL>::iterator ai;
		for (ai=m_State.vSignals.begin();ai!=m_State.vSignals.end();ai++)
		{
		//	if ((*ai).strText == szText)
			if (!stricmp((*ai).strText,szText))
				return;
		}
	}
	if( !stricmp(szText, "wakeup") )
	{
		Event( AIEVENT_WAKEUP, NULL ); 
//		return;
	}

	if(!m_bEnabled && (nSignalID!=0))
		return;

	if((nSignalID >= 0) && !m_bCanReceiveSignals)
		return;


	//GetAISystem()->m_pSystem->GetILog()->Log("\001 ENEMY %s processing signal %s.",m_sName.c_str(),szText);

	AISIGNAL signal;
	signal.nSignal = nSignalID;
	//signal.strText = szText;
	strcpy(signal.strText,szText);
	signal.pSender = pSender;

	m_State.vSignals.push_back(signal);



//	m_bSleeping = false;
//	m_bEnabled = true;

	/*
	if (m_State.nSignal == 0) 
	{
		m_State.nSignal = nSignalID;
		m_State.szSignalText = szText;
	}s
	else
	{
		int a=1;
	}
	*/

}



void CAIObject::EDITOR_DrawRanges(bool bEnable)
{
	m_bDEBUGDRAWBALLS = bEnable;
}

void CAIObject::SetRadius(float fRadius)
{
	m_fRadius = fRadius;
}

void CAIObject::Save(CStream & stm)
{
}

void CAIObject::Load(CStream & stm)
{
}
