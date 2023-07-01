#include "stdafx.h"
#include "aivehicle.h"

//#include <algorithm>
#include "GoalOp.h"
//#include "Puppet.h"
//#include "Graph.h"
#include <Cry_Math.h>
#include <Cry_Camera.h>

#include <IPhysics.h>
#include <ISystem.h>
#include <ITimer.h>
#include <IConsole.h>
#include "VertexList.h"
#include <stream.h>


CAIVehicle::CAIVehicle(void)
{

	Reset();

	m_bLastHideResult = false;
	m_vLastHidePoint = Vec3d(0,0,0);
	m_bHaveLiveTarget = false;
	m_pCurrentGoalPipe = 0;
	m_pAttentionTarget = 0;
	m_pPrevAttentionTarget = 0;
	m_bBlocked = false;
	m_bAllowedToFire = false;
	m_bEnabled = true;
	m_bUpdateInternal = true;
	m_pLastOpResult = 0;
	m_vDEBUG_VECTOR(0,0,0);
	m_bDEBUG_Unstuck=false;
	m_bLooseAttention = false;
	m_pLooseAttentionTarget = 0;
	m_vActiveGoals.reserve(20);
	m_bSmartFire = true;

	m_Gunner = 0;

	m_Threat = 0;
	m_fPassRadius = 3.0f;//4;

}

//
//---------------------------------------------------------------------------------------------------------
CAIVehicle::~CAIVehicle(void)
{
	if (m_pProxy)
	{
		m_pProxy->Release();
		m_pProxy = 0;
	}


}

//
//---------------------------------------------------------------------------------------------------------
void CAIVehicle::Update()
{
FUNCTION_PROFILER( m_pAISystem->m_pSystem,PROFILE_AI );

	if (/*(!m_bDryUpdate) &&*/ m_bMoving && (m_VehicleType == AIVEHICLE_CAR))
//	if ((!m_bDryUpdate))
	{
		AlertPuppets();
	}


	if (!m_lstBindings.empty())
		UpdateHierarchy();


	if (!m_bDryUpdate)
	{	
	FRAME_PROFILER("AI system vehicle full update",m_pAISystem->m_pSystem,PROFILE_AI);
			m_State.Reset();
			m_State.turnleft = false;
			m_State.turnright = false;
			m_State.fValue = 0.0f;


			// change state here----------------------------------------

			if(!m_pCurrentGoalPipe && m_vActiveGoals.empty())
				m_State.left = true;	// do break if nothing else
			 
			GetStateFromActiveGoals(m_State);

			// ovrride target if sticking
			if( m_State.fStickDist>0 && m_pAttentionTarget)
			{
				m_State.vTargetPos = m_pAttentionTarget->GetPos();
			}



			// affect puppet parameters here----------------------------------------
			//if (!m_Parameters.m_bIgnoreTargets)
				UpdatePuppetInternalState();
//				UpdateVehicleInternalState();



	}

	//--------------------------------------------------------
	// Orient towards the attention target always
	Navigate(m_pAttentionTarget);

	
	// ---------- update proxy object

	if (m_pAttentionTarget)
	{
		m_State.nTargetType = m_pAttentionTarget->GetType();
		m_State.bTargetEnabled = m_pAttentionTarget->m_bEnabled;
	}

	UpdateThread();


	m_State.fValueAux = GetEyeHeight();
	if (m_pAISystem->m_cvUpdateProxy->GetIVal())
		m_pProxy->Update(&m_State);	

}

//
//---------------------------------------------------------------------------------------------------------
void CAIVehicle::ParseParameters(const AIObjectParameters &params)
{
	 // do custom parse on the parameters
	m_Parameters  = params.m_sParamStruct;
	m_fEyeHeight = params.fEyeHeight;
	m_State.fHealth = m_Parameters.m_fMaxHealth;

	if(m_Parameters.m_fHorizontalFov <0 || m_Parameters.m_fHorizontalFov > 180 )	// see all around
		m_fHorizontalFOVrad = -1.0f;
	else
		m_fHorizontalFOVrad = (float) (cos((m_Parameters.m_fHorizontalFov/2.f) * (3.14/180)));

	IVehicleProxy *pProxy;
	if (params.pProxy->QueryProxy(AIPROXY_VEHICLE, (void **) &pProxy))
		m_pProxy = pProxy;
	else
		m_pProxy = NULL;

	//m_Parameters.m_fSpeciesHostility = 2.f;
	//m_Parameters.m_fGroupHostility = 0.f;

	m_Parameters.m_fPersistence = (1.f-m_Parameters.m_fPersistence) * 0.005f;
	
	if (m_Parameters.m_fPersistence == 0)
		m_Parameters.m_fPersistence = 0.005f;

	if (m_Parameters.m_nGroup)
		m_pAISystem->AddToGroup(this,m_Parameters.m_nGroup);

	if (m_Parameters.m_nSpecies)
		m_pAISystem->AddToSpecies(this,m_Parameters.m_nSpecies);

	m_bNeedsPathOutdoor = params.bUsePathfindOutdoors;

}

//
//---------------------------------------------------------------------------------------------------------
// Steers the hevicle outdoors and makes it avoid the immediate obstacles
void CAIVehicle::Reset(void)
{

	CPuppet::Reset();

}

//
//---------------------------------------------------------------------------------------------------------
// Steers the hevicle outdoors and makes it avoid the immediate obstacles
void CAIVehicle::Event(unsigned short eType, SAIEVENT *pEvent)
{
	switch( eType )
	{
		case AIEVENT_PATHFINDON:
			m_bNeedsPathOutdoor = true;
			break;
		case AIEVENT_PATHFINDOFF:
			m_bNeedsPathOutdoor = false;
			break;
		case AIEVENT_AGENTDIED:
			{
				if (GetAISystem()->GetAutoBalanceInterface())
					GetAISystem()->GetAutoBalanceInterface()->RegisterVehicleDestroyed();
			}
			break;
		default:
				CPuppet::Event(eType, pEvent);
	}
}

//
//---------------------------------------------------------------------------------------------------------
// Steers the hevicle outdoors and makes it avoid the immediate obstacles
void CAIVehicle::Steer(const Vec3d & vTargetPos, GraphNode * pNode)
{
float	frameTime = m_pAISystem->m_pSystem->GetITimer()->GetFrameTime();
float	steerAmmount = .25f;
pe_status_dynamics  dSt;
float	curVelocity;


//return; 
	// steer ammount depends on current speed
	m_pProxy->GetPhysics()->GetStatus(&dSt);
	curVelocity = dSt.v.len();
	if(m_bNeedsPathOutdoor)
	{
		steerAmmount = .65f - ( curVelocity/20.0f )*.4f;
		if(steerAmmount<.3f)
			steerAmmount = .3f;
	}
	else
		steerAmmount = 1.0f;

//steerAmmount = 0.5f;
//steerAmmount = 0.0f;


	if(GetVehicleType() == AIVEHICLE_HELICOPTER)
		return;

	if(GetVehicleType() == AIVEHICLE_BOAT)
		return;




	m_State.fDesiredSpeed = 1.0f;
	m_State.vTargetPos = vTargetPos;//targetPos;

//return;

	if (pNode->nBuildingID!=-1) 
	{
//		Vec3d targetPos = (*m_lstPath.begin());
		m_State.vMoveDir = m_State.vTargetPos - GetPos();

		if(!m_lstPath.empty())
		{
			float	dist2target = m_State.vMoveDir.len();

			if(dist2target<12.0f)
			{

			Vec3d	curSeg = m_State.vMoveDir;
			Vec3d nextSeg = (*m_lstPath.begin()) - m_State.vTargetPos;
			Vec3d vAngles = GetAngles();
			Vec3d curFwd = Vec3d(0, -1, 0);
			Matrix44 mat;
				mat.SetIdentity();
				mat=Matrix44::CreateRotationZYX(-gf_DEGTORAD*vAngles)*mat; //NOTE: angles in radians and negated 
				curFwd = mat.TransformPointOLD(curFwd);

				curSeg.z=nextSeg.z=0.0f;
				curSeg.normalize();
				nextSeg.normalize();

				float	curSegDifference = -(curFwd.x * curSeg.x + curFwd.y * curSeg.y);
				float nextSegDifference = Ffabs(nextSeg.x * curSeg.y - nextSeg.y * curSeg.x);	// 0- stright line, 2- 180degree dif
				float	dotz = nextSeg.x * curSeg.x + nextSeg.y * curSeg.y;

				if(dotz<0.0f)
					nextSegDifference = 2.0f - nextSegDifference;

				m_State.fDesiredSpeed	= 1.0f - nextSegDifference/2.0f;
				m_State.fDesiredSpeed	*= curSegDifference;
				if( m_State.fDesiredSpeed<0.85f )
				{
					m_State.fDesiredSpeed	*= (dist2target/12.0f);
					if(m_State.fDesiredSpeed<0.5f)
						m_State.fDesiredSpeed = 0.5f;
				}
			}
			else
				m_State.fDesiredSpeed = 1.0f;
		}
		return;
	}


	if (pNode->vertex.empty())
	{
		return;
	}
			
	// it has exactly three vertices, since it is a triangle
	// and indoors this vector is empty anyway
	ObstacleIndexVector::iterator vi;

	float maxlength = 2000;
	Vec3d closest;
	Vec3d curr_dir = m_State.vMoveDir;
	curr_dir = dSt.v;	// take real velocity
	curr_dir.z=0.0f;
	curr_dir.Normalize();

	for (vi=pNode->vertex.begin(); vi!=pNode->vertex.end(); vi++)
	{
		Vec3d vtx = m_pAISystem->m_VertexList.GetVertex((*vi)).vPos;
		vtx.z = m_vPosition.z;
		Vec3d guypos = vtx-m_vPosition;
		float poslength = guypos.GetLength();
		guypos.Normalize();

	
		float fdot = guypos.Dot(curr_dir);
		float	startTurnCos = .94f;	// start to tern if obstacle is within 15 degree  
		
		if( poslength<7.0f ) 
			startTurnCos = poslength/6.73f;

		if (fdot > startTurnCos && poslength<27)
		{
		float obsSize=0.0f;

			for(VectorOfLinks::iterator theLink=pNode->link.begin();theLink!=pNode->link.end();++theLink)
			{
				if((*theLink).nStartIndex!=(*vi) && (*theLink).nEndIndex!=(*vi))
					continue;
				float size = ((*theLink).vEdgeCenter - m_pAISystem->m_VertexList.GetVertex((*vi)).vPos).len2()
								- (*theLink).fMaxRadius*(*theLink).fMaxRadius;
				if(size>obsSize)
					obsSize = size;
			}
			if(obsSize<.1f)	// don't steer - obstacle is empty/too small
				continue;

		float zcross = guypos.x*curr_dir.y - guypos.y*curr_dir.x; 

			// only influence movement if puppet on any kind of collision course
//			Matrix44 m;
//			m.SetIdentity();
			Vec3d correction;// = m.TransformPointOLD(guypos);
			if (zcross > 0) 
				//m.RotateMatrix_fix(Vec3d(0,0,90));
//		    m=GetRotationZYX44(-gf_DEGTORAD*Vec3d(0,0,+90) )*m; //NOTE: anges in radians and negated 
				correction.Set(-guypos.y, guypos.x, 0.0f);
			else
				//m.RotateMatrix_fix(Vec3d(0,0,-90));
//			  m=GetRotationZYX44(-gf_DEGTORAD*Vec3d(0,0,-90) )*m; //NOTE: anges in radians and negated 
				correction.Set(guypos.y, -guypos.x, 0.0f);
						
			//m_vDEBUG_VECTOR = vtx;
//			if (poslength < 27)
			{
				float amount = fdot*(1.3f - (poslength/27.0f));
//				float amount = fdot*fdot*fdot*fdot*(7.3f - (poslength/25.0f));

				correction*=amount; // scale correction
				m_State.vMoveDir +=correction*steerAmmount;
				if(poslength < 10 && fdot > .97 )
				{
				float speed = (poslength/10.0f)*(1.0f-fdot)*100.0f;
					if(m_State.fDesiredSpeed > speed)
						m_State.fDesiredSpeed = speed;
				}
			}
		}
	}
}



//
//---------------------------------------------------------------------------------------------------------
void CAIVehicle::SetParameters(AgentParameters & sParams)
{
	if (sParams.m_nGroup != m_Parameters.m_nGroup)
	{
		m_pAISystem->RemoveFromGroup(m_Parameters.m_nGroup,this);
//		if (m_pFormation)
//			m_pAISystem->ReleaseFormation(m_Parameters.m_nGroup);

		m_pAISystem->AddToGroup(this,sParams.m_nGroup);
		CAIObject *pBeacon = m_pAISystem->GetBeacon(m_Parameters.m_nGroup);
		if (pBeacon)
			m_pAISystem->UpdateBeacon(sParams.m_nGroup,pBeacon->GetPos());
	}
	m_Parameters = sParams;
}



//
//---------------------------------------------------------------------------------------------------------
bool CAIVehicle::CanBeConvertedTo(unsigned short type, void **pConverted)
{

	if (type == AIOBJECT_VEHICLE)
	{
		*pConverted = (IVehicle *) this;
		return true;
	}

	if (type == AIOBJECT_CVEHICLE)
	{
		*pConverted = (CAIVehicle *) this;
		return true;
	}

	if (type == AIOBJECT_PIPEUSER)
	{
		*pConverted = (IPipeUser *) this;
		return true;
	}

	if (type == AIOBJECT_CPIPEUSER)
	{
		*pConverted = (CPipeUser *) this;
		return true;
	}

	if (type == AIOBJECT_CPUPPET)
	{
		*pConverted = (CPuppet *) this;
		return true;
	}

	*pConverted = 0;
	return false;
}


//
//---------------------------------------------------------------------------------------------------------
/*
void CAIVehicle::UpdateVehicleInternalState()
{

	float fMaxThreat=0, fMaxInterest=0;
	bool bSoundThreat = false;
	bool bSoundInterest = false;
	bool bVisualThreat = false;
	CAIObject *pThreat = 0, *pInterest  =0;



	
	//-----------------------------------------------------------------------




	// prepare report for the proxy -------------------------------------------
	if (m_pAttentionTarget && m_pAttentionTarget->m_bEnabled)
	{
		m_State.bHaveTarget = true;
		if (fMaxThreat > 0)
		{
			m_State.fThreat = fMaxThreat;
			m_State.fInterest = 0;
		}
		else
		{
			m_State.fThreat = 0;
			m_State.fInterest = fMaxInterest;
		}
	}
	else
	{
		m_State.bHaveTarget = false;
	}

	
}
*/

//
//---------------------------------------------------------------------------------------------------------
void CAIVehicle::Navigate(CAIObject *pTarget)
{
CAIObject *pNavigationTarget = pTarget;;
CAIObject *pLookTarget = 0;
Vec3d		gunnerAngles(0,0,0);


//	if(!m_bUpdateInternal)
//		return;
	if (m_bLooseAttention)
	{
		if (m_pLooseAttentionTarget)
			pNavigationTarget = m_pLooseAttentionTarget;
		else
			return;
	}

 	Vec3d vDir, vAngles, vTargetPos;

	if (pNavigationTarget)
	{
		Matrix44 mat;
		mat.SetIdentity();
		Vec3d fwd(0.f,-1.f,0.f);
		// follow this attention target
		vAngles = GetAngles();
		Vec3d puppetAngles = vAngles;
		vTargetPos = pNavigationTarget->GetPos();
//		m_State.vTargetPos = vTargetPos;
		vDir = m_State.vTargetPos - m_vPosition;
		float distance = vDir.GetLength();
		if (m_pAttentionTarget)
			m_State.nTargetType = m_pAttentionTarget->GetType();
		m_State.fDistanceFromTarget = distance;
	}

	if( pTarget && GetVehicleType()==AIVEHICLE_BOAT )
	{
		int TargetType = pTarget->GetType();
		if ( (TargetType != AIOBJECT_PLAYER) && (TargetType != AIOBJECT_PUPPET) ) 
			return;
	}

	CAIObject *pGunnerTarget = 0;
//	if( 0 )
//	if( m_Gunner && m_State.bodystate!=1 )
//	if( m_Gunner && m_State.fStickDist > 1.0f )	// if has gunner and in attack mode - trun gunner side to target
	if( m_Gunner && m_State.bodystate == 3 )	// if has gunner and in attack mode - trun gunner side to target
	{
//		gunnerAngles = m_Gunner->GetAngles( 1 );
//		m_Gunner->SetAngles( GetAngles() + gunnerAngles );
		pGunnerTarget = m_Gunner->m_pAttentionTarget;
		if(pGunnerTarget && pGunnerTarget->GetType() == AIOBJECT_PLAYER )
		{
			pLookTarget = pGunnerTarget;
			gunnerAngles = Vec3d(0,0,50);
		}
		else
		{
			pLookTarget = pTarget;
			gunnerAngles = Vec3d(0,0,40);
		}
	}
	else
		pLookTarget = pTarget;

	if (pLookTarget)
	{
		Matrix44 mat;
		mat.SetIdentity();
		Vec3d fwd(0.f,-1.f,0.f);
		// follow this attention target
		vAngles = GetAngles() + gunnerAngles;
		Vec3d puppetAngles = vAngles;
		vTargetPos = pLookTarget->GetPos();
		vDir = vTargetPos - m_vPosition;
//		float distance = vDir.GetLength();
//		if (m_pAttentionTarget)
//			m_State.nTargetType = m_pAttentionTarget->GetType();

		vDir.Normalize();
	//	mat.RotateMatrix_fix(vAngles);
		mat=Matrix44::CreateRotationZYX(-gf_DEGTORAD*vAngles)*mat; //NOTE: angles in radians and negated 

		vAngles = mat.TransformPointOLD(fwd);
		//vAngles.ConvertToRadAngles();

		
		float zcross =  vDir.x * vAngles.y - vDir.y * vAngles.x;
		if( GetVehicleType() == AIVEHICLE_BOAT )
		{
			float zdot = vDir.x * vAngles.x + vDir.y * vAngles.y;
			m_State.fValue =  (zcross);// * m_Parameters.m_fResponsiveness;
			if( zdot > 0 )
			{
				if(zcross<0)
					m_State.fValue = -(2.0f+zcross);
				else
					m_State.fValue = (2.0f-zcross);
			}
			if (zcross > 0.3f)
				m_State.turnright = true;
			else if (zcross < -0.3f)
				m_State.turnleft = true;

		}
		else
		{
			m_State.fValue =  (-zcross) * m_Parameters.m_fResponsiveness;
			if (zcross > 0.0f)
				m_State.turnright = true;
			else 
				m_State.turnleft = true;
		}


		int TargetType = pLookTarget->GetType();
		if ( (TargetType == AIOBJECT_PLAYER) || (TargetType == AIOBJECT_PUPPET) ) 
		//if ((TargetType != AIOBJECT_DUMMY) && (TargetType != AIOBJECT_HIDEPOINT) && (TargetType != AIOBJECT_WAYPOINT))
		{
			Vec3d vertCorrection = vDir;
		//	mat.Identity();
			//mat.RotateMatrix(vertCorrection);
			//vertCorrection = mat.TransformPoint(fwd);
			vertCorrection=ConvertVectorToCameraAngles(vertCorrection);

			if (vertCorrection.x > 90.f)
				vertCorrection.x = 90.f;
			if (vertCorrection.x < -90.f)
				vertCorrection.x = 360+vertCorrection.x;
	
			m_State.fValueAux = -(puppetAngles.x - vertCorrection.x)*0.1f;
		}
	}
}


//
//---------------------------------------------------------------------------------------------------------
void CAIVehicle::Bind(IAIObject* bind)
{
	if( bind )
	{
		bind->CanBeConvertedTo( AIOBJECT_CPUPPET, (void**)&m_Gunner );
	}
}

//
//---------------------------------------------------------------------------------------------------------
void CAIVehicle::Unbind( )
{
	m_Gunner = 0;
}



//
//---------------------------------------------------------------------------------------------------------
/*
void CAIVehicle::Event(unsigned short eType, SAIEVENT *pEvent)
{
	switch (eType)
	{
		case AIEVENT_DISABLE:
			m_bEnabled = false;
			m_bSleeping = true;	
		break;
		case AIEVENT_ENABLE:
			m_bEnabled = true;
			m_bSleeping = false;	
		break;
	}
		
}
*/
//
//---------------------------------------------------------------------------------------------------------
void CAIVehicle::OnObjectRemoved(CAIObject *pObject)
{
	CPuppet::OnObjectRemoved( pObject );

	if( pObject == m_Gunner )
		m_Gunner = 0;

	if( pObject == m_Threat )
		m_Threat = 0;

}


//
//---------------------------------------------------------------------------------------------------------
void CAIVehicle::UpdateThread()
{
	m_State.dodge = false;

	if( m_State.jump )
		m_Threat = m_pAttentionTarget;

	if(!m_Threat)
		return;

	Vec3d correction = m_pProxy->UpdateThreat( m_Threat->GetAssociation() );

	if( correction.x==0 && correction.y==0 && correction.z==0 )	// no thread
	{
		m_Threat = NULL;
		return ; // its behind him 
	}

	m_State.vMoveDir = correction;
	m_State.dodge = true;

}

//
//---------------------------------------------------------------------------------------------------------
/*
void CAIVehicle::Event(unsigned short eType, SAIEVENT *pEvent)
{
	switch (eType)
	{
		case AIEVENT_DRIVEROUT:
			m_CanDrive = false;
		break;
		case AIEVENT_DRIVERIN:
			m_CanDrive = true;
		break;
		default:
			CPuppet::Event(eType, pEvent);
	}
}
*/
//*/
//
//
//---------------------------------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------------------------------



void CAIVehicle::AlertPuppets(void)
{
	AIObjects::iterator iend = m_pAISystem->m_Objects.end();
	AIObjects::iterator pi = m_pAISystem->m_Objects.find(AIOBJECT_PUPPET);

	if (!m_pProxy || !m_pProxy->GetPhysics())
		return;

	while (pi!=iend)
	{
		if (pi->first != AIOBJECT_PUPPET)
			break;
	
		CAIObject *pObject = pi->second;
		Vec3d puppetDir = pObject->GetPos();

		if(pObject->GetProxy() && !pObject->GetProxy()->CheckStatus( AIPROXYSTATUS_INVEHICLE ))
		{
			puppetDir-=m_vPosition;
			float fLenght = puppetDir.len2();
			puppetDir.Normalize();

			pe_status_dynamics  dSt;
			m_pProxy->GetPhysics()->GetStatus(&dSt);
			

			if ((puppetDir.Dot(dSt.v.normalized())>0.8f) && (fLenght<400.f))
				pObject->SetSignal(1,"OnVehicleDanger",GetAssociation());
		}
		++pi;
	}
	
}

void CAIVehicle::Save(CStream &stm)
{
	CPuppet::Save(stm);

	if (m_pProxy)
		m_pProxy->Save(stm);
	// serialize attention target - it's not done for puppets
	if (m_pAttentionTarget)
	{
		stm.Write((int)1);
		stm.Write(m_pAttentionTarget->GetName());
	}
	else
		stm.Write((int)0);
	stm.Write(m_State.vTargetPos);
	stm.Write(m_State.fStickDist);
	stm.Write(m_State.fValueAux);
}

void CAIVehicle::Load(CStream &stm)
{
	CPuppet::Load(stm);

	if (m_pProxy)
		m_pProxy->Load(stm);

	// attention target
	// serialize attention target
	int nHasAttentionTarget;
	stm.Read(nHasAttentionTarget);
	if (nHasAttentionTarget)
	{
		char sName[255];
		stm.Read(sName,255);
		CAIObject *pLOP = m_pAISystem->GetAIObjectByName(sName);
		SetAttentionTarget(pLOP);
	}
	stm.Read(m_State.vTargetPos);
	stm.Read(m_State.fStickDist);
	stm.Read(m_State.fValueAux);
}	
