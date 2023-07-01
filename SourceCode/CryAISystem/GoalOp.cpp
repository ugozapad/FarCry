// GoalOp.cpp: implementation of the CGoalOp class.
//
//////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "GoalOp.h"
#include "Puppet.h"
#include "aivehicle.h"
#include <ISystem.h>
#include <ITimer.h>
#include <IPhysics.h>
#include <Cry_Math.h>
#include <ILog.h>



#include "PipeUser.h"

#if defined(WIN32) && defined(_DEBUG) 
#include <crtdbg.h> 
#define DEBUG_NEW_NORMAL_CLIENTBLOCK(file, line) new(_NORMAL_BLOCK, file, line) 
#define new DEBUG_NEW_NORMAL_CLIENTBLOCK( __FILE__, __LINE__) 
#endif




//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGoalOp::CGoalOp()
{

}

CGoalOp::~CGoalOp()
{

}


COPAcqTarget::COPAcqTarget(CAIObject *pTarget)
{
	m_pTarget = pTarget;
}

COPAcqTarget::~COPAcqTarget()
{
}

bool COPAcqTarget::Execute(CPipeUser *pOperand)
{

	if (m_pTarget)
	{
			pOperand->SetAttentionTarget(m_pTarget);
			return true;
	}
	else 
	{
			pOperand->SetAttentionTarget(pOperand->m_pLastOpResult);
			return true;
	}
		

	return false;
}

int COPAcqTarget::XMLExport(const char *pBuffer)
{
	strcat((char*)pBuffer,"<acqtarget type=\"Goal\" Description=\"Acquires target specified by name, or last operation target if name omitted.\">\n");
	strcat((char*)pBuffer,"	<Target type=\"string\" value=\"\" mandatory=\"NO\"/>\n");
	strcat((char*)pBuffer,"</acqtarget>\n");

	return strlen(pBuffer);
}

COPApproach::COPApproach(float distance, bool percent, bool useLastOpResalt)
{
	m_fDistance = distance;
	m_bPercent = percent;
	m_fInitialDistance = 0;
	m_nTicks = 0;
	m_pPathfindDirective = 0;
	m_pTraceDirective = 0;
	m_fTime = 0.0f;
	m_pTempTarget = 0;
	m_bUseLastOpResalt = useLastOpResalt;
}

COPApproach::~COPApproach()
{
	
}

void COPApproach::Reset(CPipeUser *pOperand)
{
	if (m_pPathfindDirective)
		delete m_pPathfindDirective;

	m_pPathfindDirective = 0;

	if (m_pTraceDirective) 
		delete m_pTraceDirective;

	m_pTraceDirective = 0;

	m_fInitialDistance = 0;
}


bool COPApproach::Execute(CPipeUser *pOperand)
{
	CAIObject *pTarget = pOperand->m_pAttentionTarget;
	CAISystem *pSystem = GetAISystem();
	Vec3d mypos,targetpos;
 
	if (!pTarget || m_bUseLastOpResalt) 
	{
		if (pOperand->m_pLastOpResult)
			pTarget = pOperand->m_pLastOpResult;
		else
		{
			// no target, nothing to approach to
			Reset(pOperand);
			return true;
		}
	}

	mypos = pOperand->GetPos();
	targetpos = pTarget->GetPos();

	if (m_bPercent && (m_fInitialDistance==0))
	{
		m_fInitialDistance = (targetpos-mypos).GetLength();
		m_fInitialDistance *= m_fDistance;
	}

	mypos-=targetpos;

	Vec3d	projectedDist = mypos;
	projectedDist.z = 0;
	float dist = projectedDist.GetLength();

	bool	bIndoor=false;
	int building;
	IVisArea *pArea;
	if (GetAISystem()->CheckInside(pOperand->GetPos(),building,pArea))
		bIndoor=true;
		
	//GraphNode *pThisNode = pSystem->GetGraph()->GetEnclosing(pOperand->GetPos(),pOperand->m_pLastNode);
	
	
	if(pOperand->m_bForceTargetPos)
		pOperand->m_State.vTargetPos = targetpos;

	if( ((!pOperand->m_bNeedsPathIndoor)&&bIndoor) || ((!pOperand->m_bNeedsPathOutdoor)&&(!bIndoor)) || 
		pOperand->m_State.pathUsage == SOBJECTSTATE::PU_NoPathfind )
//	if(!pOperand->m_bNeedsPath && !bIndoor)	// no path - just go to target
	{
		mypos-=targetpos;
		if (!m_bPercent)
		{
			if (dist < m_fDistance)
			{
					m_fInitialDistance = 0;
					return true;
			}
		}
		else
		{
			if (dist < m_fInitialDistance)
			{
					m_fInitialDistance = 0;
					return true;
			}
		}

		// no pathfinding - just approach
		pOperand->m_State.vTargetPos = targetpos;
		pOperand->m_State.vMoveDir = targetpos - pOperand->GetPos();
		pOperand->m_State.vMoveDir.Normalize();

		// try to steer now - only if it's a CAR
		CAIVehicle	*pVelicle;
		if(pOperand->CanBeConvertedTo( AIOBJECT_CVEHICLE, (void**)&pVelicle ))
			if( pVelicle->GetVehicleType() == AIVEHICLE_CAR)
		{
			GraphNode *pThisNode = pSystem->GetGraph()->GetEnclosing(pOperand->GetPos(),pOperand->m_pLastNode);
			pOperand->Steer(targetpos,pThisNode);
			pOperand->m_State.vMoveDir.Normalize();
		}

		m_fLastDistance = dist;
		return false;
	}

//	float dist = mypos.GetLength();

/*	if (!m_bPercent)
	{
		if (dist < m_fDistance)
		{
				Reset();
				return true;
		}
	}
	else
	{
		if (dist < m_fInitialDistance)
		{
				Reset();
				return true;
		}

	}
*/
	if (!m_pPathfindDirective)
	{
		// generate path to target
		m_pPathfindDirective = new COPPathFind("",pTarget);
		pOperand->m_nPathDecision = PATHFINDER_STILLTRACING;
		if (m_pPathfindDirective->Execute(pOperand))
		{
			if (pOperand->m_nPathDecision == PATHFINDER_NOPATH)
			{
					pOperand->m_State.vMoveDir.Set(0,0,0);
					Reset(pOperand);
					return true;
			}
		}

		if (m_pTempTarget)
		{
			pOperand->m_State.vMoveDir = GetNormalized(m_pTempTarget->GetPos()-pOperand->GetPos());
			pOperand->m_State.vTargetPos = m_pTempTarget->GetPos();
		}


		m_fTime = 0;
		return false;
	}
	else
	{
		// if we have a path, trace it
		if (pOperand->m_nPathDecision==PATHFINDER_PATHFOUND)
		{
			bool bDone=false;
			if (m_pTraceDirective)
			{
				// keep tracing - previous code will stop us when close enough
				bDone = m_pTraceDirective->Execute(pOperand);


//if(0)
				if (!bDone)
				{

					GraphNode *pThisNode = pSystem->GetGraph()->GetEnclosing(pOperand->GetPos(),pOperand->m_pLastNode);
					pOperand->m_pLastNode = pThisNode;
					if(m_pTraceDirective->m_pNavTarget)
						pOperand->Steer( m_pTraceDirective->m_pNavTarget->GetPos(),pThisNode);

				// for vehicles - regenirate path to target once in a while 
					if(pOperand->GetType() == AIOBJECT_VEHICLE)
					{

						pOperand->m_State.vMoveDir.Normalize();

						m_fTime+=1;
//					if(0)
						if(pOperand->m_State.pathUsage == SOBJECTSTATE::PU_NewPathWanted)
						if(m_fTime > 15)
						{
//							m_fTime = 0;
							if( m_pTraceDirective->m_pNavTarget )
							{
								m_pTempTarget = GetAISystem()->CreateDummyObject();
								m_pTempTarget->SetPos(m_pTraceDirective->m_pNavTarget->GetPos());
							}
							delete m_pTraceDirective;
							m_pTraceDirective = 0;
							delete m_pPathfindDirective;
							m_pPathfindDirective = 0;	

							if (m_pTempTarget)
							{
								pOperand->m_State.vMoveDir = GetNormalized(m_pTempTarget->GetPos()-pOperand->GetPos());
								pOperand->m_State.vTargetPos = m_pTempTarget->GetPos();
							}

							return false;
						}
					}

					
				}
			}
			else
			{
				if (m_pTempTarget)
				{
					GetAISystem()->RemoveDummyObject(m_pTempTarget);
					m_pTempTarget = 0;
				}

				bool bExact = false; //(pTarget->GetType()!=AIOBJECT_PUPPET) && (pTarget->GetType()!=AIOBJECT_PLAYER);
				m_pTraceDirective = new COPTrace(bExact,m_bPercent?m_fInitialDistance:m_fDistance);
				bDone = m_pTraceDirective->Execute(pOperand);
			}

			if (bDone)
			{
				if(pOperand->GetType() == AIOBJECT_VEHICLE)
				{
					Reset(pOperand);
					return true;
				}
				float fDesiredDistance = m_bPercent?m_fInitialDistance:m_fDistance;
				if (pOperand->m_pAttentionTarget && (pOperand->m_pAttentionTarget->GetType()==AIOBJECT_PLAYER))
				{
						//2D-ize the check
						Vec3d op_pos = pOperand->GetPos();
						Vec3d tg_pos = pOperand->m_pAttentionTarget->GetPos();
						op_pos.z = tg_pos.z;
						if (GetLength(op_pos-tg_pos)>fDesiredDistance)
						{
							// keep going
							
							{
								m_pTempTarget = GetAISystem()->CreateDummyObject();
								m_pTempTarget->SetPos(pOperand->m_pAttentionTarget->GetPos());
							}
							delete m_pTraceDirective;
							m_pTraceDirective = 0;
							delete m_pPathfindDirective;
							m_pPathfindDirective = 0;	

							m_pPathfindDirective = new COPPathFind("",pTarget);
							pOperand->m_nPathDecision = PATHFINDER_STILLTRACING;
							if (m_pPathfindDirective->Execute(pOperand))
							{
								if (pOperand->m_nPathDecision == PATHFINDER_NOPATH)
								{
									pOperand->m_State.vMoveDir.Set(0,0,0);
									Reset(pOperand);
									return true;
								}
							}
						}
						else
						{
							Reset(pOperand);
							return true;
						}
				}
				else
				{
					Reset(pOperand);
					return true;
				}
			}
		}
		else
		{
			if (pOperand->m_nPathDecision==PATHFINDER_NOPATH)
			{
				Reset(pOperand);
				return true;
			}
			else
{
				m_pPathfindDirective->Execute(pOperand);

//if(pOperand->GetType() == AIOBJECT_VEHICLE)
//GetAISystem()->m_pSystem->GetILog()->Log("\004 loooooking  ");

}
		}
	}

	//m_fLastDistance = dist;

//if(0)
	if (m_pTempTarget)
	{
		pOperand->m_State.vMoveDir = GetNormalized(m_pTempTarget->GetPos()-pOperand->GetPos());
		pOperand->m_State.vTargetPos = m_pTempTarget->GetPos();
	}
//	else
//	{
//		pOperand->m_State.vMoveDir = GetNormalized(-mypos);
//	}

	return false;

}


/*
bool COPApproach::Execute(CPipeUser *pOperand)
{
	
	CAIObject *pTarget = pOperand->m_pAttentionTarget;
	CAISystem *pSystem = GetAISystem();
	Vec3d mypos,targetpos;



	if (!pTarget) 
	{
		if (pOperand->m_pLastOpResult)
			pTarget = pOperand->m_pLastOpResult;
		else
		{
			m_fInitialDistance = 0;
			return true;
		}
	}


	mypos = pOperand->GetPos();
	targetpos = pTarget->GetPos();

	if (m_bPercent && (m_fInitialDistance==0))
	{
		m_fInitialDistance = (targetpos-mypos).GetLength();
		m_fInitialDistance *= m_fDistance;
	}

	mypos-=targetpos;

	Vec3d	projectedDist = mypos;
	projectedDist.z = 0;
	float dist = projectedDist.GetLength();

//	float dist = mypos.GetLength();

	if (!m_bPercent)
	{
		if (dist < m_fDistance)
		{
				m_fInitialDistance = 0;
				return true;
		}
	}
	else
	{
		if (dist < m_fInitialDistance)
		{
				m_fInitialDistance = 0;
				return true;
		}

	}

//	
//	if (fabs(dist-m_fLastDistance) < 0.01)
//	{
//		if (m_nTicks++ > 3)
//		{
//			m_nTicks = 0;
//			m_fInitialDistance = 0;
//			m_fLastDistance = 30000;
//			return true;
//		}
//	}
//	else
//		m_nTicks=0;
//	


	if (!pOperand->GetParameters().m_bIgnoreTargets)
	{
		GraphNode *pThisNode = pSystem->GetGraph()->GetEnclosing(pOperand->GetPos(),pOperand->m_pLastNode);
		if (!pThisNode)
			DEBUG_BREAK;
		GraphNode *pTargetNode = pSystem->GetGraph()->GetEnclosing(pTarget->GetPos(),pTarget->m_pLastNode);
		
		if (pThisNode!=pTargetNode)
		{
			// steer if outdoors
			if (pThisNode->nBuildingID==-1)
			{
				pOperand->m_State.vMoveDir = targetpos-pOperand->GetPos();
				pOperand->m_State.vMoveDir.Normalize();
				pOperand->Steer(pTarget->GetPos(),pThisNode);
				pOperand->m_State.vMoveDir.Normalize();
			}
			else
			{
				//pOperand->m_State.vMoveDir += (pThisNode->data.m_pos -pOperand->GetPos()).Normalized();
				Vec3d normalized = targetpos -pOperand->GetPos();
				normalized.Normalize();
				pOperand->m_State.vMoveDir +=  normalized;
				pOperand->m_State.vMoveDir.Normalize();
			}
		}
		else
		{
			// same triangle, just approach
			pOperand->m_State.vMoveDir = targetpos - pOperand->GetPos();
			pOperand->m_State.vMoveDir.Normalize();
		}

		pOperand->m_pLastNode = pThisNode;
		pTarget->m_pLastNode = pTargetNode;
	}
	else
	{
		pOperand->m_State.vMoveDir = targetpos - pOperand->GetPos();
		pOperand->m_State.vMoveDir.Normalize();
	}

//	pOperand->m_State.forward = true;
//	pOperand->m_State.back = false;
	

	m_fLastDistance = dist;

	return false;

}
*/

int COPApproach::XMLExport(const char *pBuffer)
{
	strcat((char*)pBuffer,"<approach type=\"Goal\" Description=\"Approaches distance meters to attention target, or last operation result if no target.\">\n");
	strcat((char*)pBuffer,"	<Distance type=\"float\" value=\"3.0\" mandatory=\"YES\" units=\"meters\"/>\n");
	strcat((char*)pBuffer,"</approach>\n");

	return strlen(pBuffer);
}


COPBackoff::COPBackoff(float distance, bool percent)
{
	m_fDistance = distance;
	m_bPercent = percent;
	m_bBackingOff = false;
}

COPBackoff::~COPBackoff()
{
}

bool COPBackoff::Execute(CPipeUser *pOperand)
{ 
//	if (!m_bBackingOff)
	{
		CAIObject *pTarget = pOperand->m_pAttentionTarget;
		if (!pTarget)
		{
			if (!(pTarget = pOperand->m_pLastOpResult))
				return true;
		}

		Vec3d mypos = pOperand->GetPos();
		Vec3d tgpos = pTarget->GetPos();

		if ( (mypos-tgpos).GetLength() >= m_fDistance )
			return true;

		if (pTarget->GetType()==AIOBJECT_VEHICLE)
		{
			tgpos+=pTarget->m_State.vMoveDir*10.f;
		}

		m_vSnapShot_of_Position = tgpos + (GetNormalized(mypos-tgpos))*m_fDistance;

		int nBuildingId;
		IVisArea *pArea;

		if (!GetAISystem()->CheckInside(mypos,nBuildingId,pArea))
		{
			Vec3d vUpdPos;
			if (GetAISystem()->IntersectsForbidden(mypos,m_vSnapShot_of_Position,vUpdPos))
				m_vSnapShot_of_Position = vUpdPos + GetNormalized(mypos-m_vSnapShot_of_Position);
		}
		else
		{
			Vec3d vUpdPos;
			if (GetAISystem()->IntersectsSpecialArea(mypos,m_vSnapShot_of_Position,vUpdPos))
				m_vSnapShot_of_Position = vUpdPos + GetNormalized(mypos-m_vSnapShot_of_Position);
		}

		pOperand->m_vDEBUG_VECTOR = m_vSnapShot_of_Position;
//		m_bBackingOff  = true;
	}
//	else
	{
			Vec3d mypos,targetpos;
			mypos =	pOperand->GetPos();
			targetpos = m_vSnapShot_of_Position;
			targetpos.z = mypos.z;
 
			targetpos-=mypos;

			if (targetpos.GetLength() < 0.4f)
			{
	//			m_bBackingOff = false;
				return true;
			}

			pOperand->m_State.vMoveDir = GetNormalized(targetpos);

	}

	return false;

}

int COPBackoff::XMLExport(const char *pBuffer)
{
	strcat((char*)pBuffer,"<backoff type=\"Goal\" Description=\"Back's off distance meters from attention target, or last operation result if no target.\">\n");
	strcat((char*)pBuffer,"	<Distance type=\"float\" value=\"3.0\" mandatory=\"YES\" units=\"meters\"/>\n");
	strcat((char*)pBuffer,"</backoff>\n");

	return strlen(pBuffer);
}


COPTimeout::COPTimeout(float interval, ITimer *pTimer,float interval_end)
{
	m_fInterval = interval;
	m_fIntervalCurrent = interval;
	m_fIntervalEnd = interval_end;

	if (m_fIntervalEnd > 0)
	{
		float zerotoone = (rand() % 100)/100.f;
		m_fIntervalCurrent += (m_fIntervalEnd - m_fInterval)*zerotoone;
	}

	m_fCount = 0;
	m_pTimer = pTimer;
	m_fLastTime = 0;
}

COPTimeout::~COPTimeout()
{
}

bool COPTimeout::Execute(CPipeUser *pOperand)
{
	float fTime = m_pTimer->GetCurrTime();

	if (m_fLastTime == 0)
			m_fLastTime = fTime;

	m_fCount += fTime - m_fLastTime;

	if (m_fCount > m_fIntervalCurrent)
	{
		m_fCount = 0;
		m_fLastTime = 0;
		m_fIntervalCurrent = m_fInterval;

		if (m_fIntervalEnd > 0)
		{
			float zerotoone = (rand() % 100)/100.f;
			m_fIntervalCurrent += (m_fIntervalEnd - m_fInterval)*zerotoone;
		}
		

		return true;
	}
	else 
	{
		m_fLastTime = fTime;
		return false;
	}
}

int COPTimeout::XMLExport(const char *pBuffer)
{
	strcat((char*)pBuffer,"<timeout type=\"Goal\" Description=\"Counts down the number of seconds specified as parameter.\">\n");
	strcat((char*)pBuffer,"	<Interval type=\"float\" value=\"1.0\" mandatory=\"YES\" units=\"seconds\"/>\n");
	strcat((char*)pBuffer,"</timeout>\n");

	return strlen(pBuffer);
}



COPStrafe::COPStrafe(float distance)
{
	m_fDistance = distance;
	m_fRemainingDistance = distance;
	m_fLastRemainingDistance = fabs(distance+distance);
	m_bPositionSnapshotTaken = false;
}

COPStrafe::~COPStrafe()
{
}

bool COPStrafe::Execute(CPipeUser *pOperand)
{

	if(pOperand->GetType() == AIOBJECT_VEHICLE)
	{
		if (m_fDistance > 0)
		{
			pOperand->m_State.left = true;
			pOperand->m_State.right = false;
		}
		else if (m_fDistance < 0)
		{
			pOperand->m_State.left = false;
			pOperand->m_State.right = true;
		}
		return true;
	}

	if (m_bPositionSnapshotTaken)
	{

		if (m_fDistance > 0)
		{
			pOperand->m_State.left = true;
			pOperand->m_State.right = false;
		}
		else if (m_fDistance < 0)
		{
			pOperand->m_State.left = false;
			pOperand->m_State.right = true;
		}


		// if reached desired point stop
		if (pOperand->m_pAttentionTarget)
		{
			m_bPositionSnapshotTaken = false;
		}

		Vec3d mypos = pOperand->GetPos();
		
		m_fRemainingDistance = (mypos-m_vTargetPos).GetLength();
		if ( ((m_fLastRemainingDistance-m_fRemainingDistance) < 0.01f)
			|| (m_fRemainingDistance < 0.5))
		{
			m_bPositionSnapshotTaken = false;
			pOperand->m_State.right = false;
			pOperand->m_State.left = false;
			m_fRemainingDistance = m_fDistance;
			m_fLastRemainingDistance = fabs(m_fDistance+m_fDistance);
			return true;
		}
		m_fLastRemainingDistance = m_fRemainingDistance;
	}
	
	{
		
		Vec3d mydir = pOperand->GetAngles();
		Vec3d mypos = pOperand->GetPos();
		mydir = ConvertToRadAngles(mydir);
		
		Vec3d vStrafeDir;
		if (m_fDistance>0)
			vStrafeDir.Set(-mydir.y,mydir.x,mydir.z);
		else
			vStrafeDir.Set(mydir.y,-mydir.x,mydir.z);
		
		m_vTargetPos = mypos+m_fRemainingDistance*vStrafeDir;
		m_vStartPos = mypos;

		int nBuildingId;
		IVisArea *pArea;

		if (!GetAISystem()->CheckInside(mypos,nBuildingId,pArea))
		{
			Vec3d vUpdPos;
			if (GetAISystem()->IntersectsForbidden(mypos,mypos+m_fRemainingDistance*vStrafeDir,vUpdPos))
				m_vTargetPos = vUpdPos - vStrafeDir;
		}
		else
		{
			Vec3d vUpdPos;
			if (GetAISystem()->IntersectsSpecialArea(mypos,mypos+m_fRemainingDistance*vStrafeDir,vUpdPos))
				m_vTargetPos = vUpdPos - vStrafeDir;
		}
		
		m_bPositionSnapshotTaken = true;
	}


	return false;
}

COPFireCmd::COPFireCmd(bool AllowedToFire,bool bSmartFire)
{
	m_bAllowedToFire = AllowedToFire;
	m_bSmartFire = bSmartFire;
}

COPFireCmd::~COPFireCmd()
{
}

bool COPFireCmd::Execute(CPipeUser *pOperand)
{
	pOperand->m_bAllowedToFire = m_bAllowedToFire;
	pOperand->m_bSmartFire = m_bSmartFire;
	return true;
}

int COPFireCmd::XMLExport(const char *pBuffer)
{
	strcat((char*)pBuffer,"<firecmd type=\"Goal\" Description=\"Instructs the agent to fire or hold his fire (if target is within agent's attack range)\">\n");
	strcat((char*)pBuffer,"	<FireAllowed type=\"bool\" mandatory=\"YES\" value=\"false\" />\n");
	strcat((char*)pBuffer,"</firecmd>\n");

	return strlen(pBuffer);
}



COPBodyCmd::COPBodyCmd(int bodypos)
{
	m_nBodyState = bodypos;
}

bool COPBodyCmd::Execute(CPipeUser *pOperand)
{
	pOperand->m_State.bodystate = m_nBodyState;
	return true;
}

int COPBodyCmd::XMLExport(const char *pBuffer)
{
	strcat((char*)pBuffer,"<bodypos type=\"Goal\" Description=\"Selects the stance of the agent, dependant on the parameter. 0-standing, 1-crouching, 2-proning\">\n");
	strcat((char*)pBuffer,"	<Stance type=\"integer\" value=\"0.0\" mandatory=\"YES\" />\n");
	strcat((char*)pBuffer,"</bodypos>\n");

	return strlen(pBuffer);
}


COPRunCmd::COPRunCmd(bool running)
{
	m_bRunning = running;
}

bool COPRunCmd::Execute(CPipeUser *pOperand)
{
	pOperand->m_State.run = m_bRunning;
	return true;
}

int COPRunCmd::XMLExport(const char *pBuffer)
{
	strcat((char*)pBuffer,"<run type=\"Goal\" Description=\"Selects running or walking state of agent.\">\n");
	strcat((char*)pBuffer,"	<Enabled type=\"bool\" value=\"false\" mandatory=\"YES\" />\n");
	strcat((char*)pBuffer,"</run>\n");

	return strlen(pBuffer);
}

/*
bool COPJumpCmd::Execute(CPipeUser *pOperand)
{
	pOperand->m_State.jump = true;

	Vec3d op_pos;
	if (pOperand->m_pLastOpResult)
		op_pos = pOperand->m_pLastOpResult->GetPos();
	else
		op_pos = pOperand->m_vLastHidePos;

	Vec3d vJump = op_pos - pOperand->GetPos();
	float d = vJump.GetLength();
	vJump/=d;
	pOperand->m_State.vJumpDirection = vJump+Vec3d(0,0,1);
	pOperand->m_State.vJumpDirection *= sqrt(9.81*d)*0.70710f;
	pOperand->m_State.vMoveDir = vJump/d;
	
	return true;
}

int COPJumpCmd::XMLExport(const char *pBuffer)
{
	strcat((char*)pBuffer,"<jump type=\"Goal\" Description=\"Makes the agent jump.\">\n");
	strcat((char*)pBuffer,"</jump>\n");
	return strlen(pBuffer);
}
*/

bool COPLookAt::Execute(CPipeUser *pOperand)
{
	//CAIObject *pObject = pOperand->m_pAttentionTarget;
	Vec3d myDir;
	Vec3d otherDir;

	if (!m_pDummyAttTarget)
	{ 
		// create a random new target to lookat
		m_pDummyAttTarget = GetAISystem()->CreateDummyObject();

		Vec3d mypos = pOperand->GetPos();
		Vec3d myangles = pOperand->GetAngles();

		
		if ( (m_fEndAngle == 0 ) && (m_fStartAngle==0) )
		{
			CAIObject *pOriant;	// the guy who will provide our orientation
			if (pOperand->m_pAttentionTarget)
				pOriant = pOperand->m_pAttentionTarget;
			else if (pOperand->m_pLastOpResult)
				pOriant = pOperand->m_pLastOpResult;
			else
			{
				if (m_pDummyAttTarget)
				{
					GetAISystem()->RemoveDummyObject(m_pDummyAttTarget);
					m_pDummyAttTarget = 0;
				}
				pOperand->m_bLooseAttention = false;
				pOperand->m_pLooseAttentionTarget = 0;
				return true;	// sorry no target and no last operation target
			}
				
			myangles = pOriant->GetAngles();


			Matrix44 m; 	m.SetIdentity();
			//m.RotateMatrix_fix(myangles);
			m=Matrix44::CreateRotationZYX(-myangles*gf_DEGTORAD )*m; //NOTE: anges in radians and negated 
			//POINT_CHANGED_BY_IVO 
			//myangles = m.TransformPoint(Vec3d(0,-1,0));
			myangles = GetTransposed44(m) * Vec3d(0,-1,0);



			myangles*=20.f;

		}
		else
		{
			// lets place it at a random spot around the operand

			myangles=ConvertToRadAngles(myangles);
			myangles.z = 0;
		
			float fi = (((float)(rand() & 255) / 255.0f ) * (float)fabs(m_fEndAngle - m_fStartAngle));
			fi+=m_fStartAngle;
				
			Matrix44 m;
			m.SetIdentity();
			//m.RotateMatrix_fix(Vec3d(0,0,fi));
			m=Matrix44::CreateRotationZYX(-Vec3d(0,0,fi)*gf_DEGTORAD )*m; //NOTE: anges in radians and negated 
		
			//POINT_CHANGED_BY_IVO 
			//myangles = m.TransformPoint(myangles);
			myangles = GetTransposed44(m) * (myangles);
	
		}

		myangles*=20.f;

		m_pDummyAttTarget->SetPos(mypos+myangles);
		pOperand->m_vDEBUG_VECTOR = mypos+myangles;
		//m_pDummyAttTarget->SetName("DUMMY");
		pOperand->m_bLooseAttention = true;
		pOperand->m_pLooseAttentionTarget = m_pDummyAttTarget;

	}
	else
	{

		Vec3d mypos = pOperand->GetPos();
		myDir = pOperand->GetAngles();
		myDir=ConvertToRadAngles(myDir);
		Vec3d otherpos = m_pDummyAttTarget->GetPos();
		otherDir = m_pDummyAttTarget->GetPos();
		otherDir-=mypos;
		otherDir.Normalize();


		float f = myDir.Dot(otherDir);
		if (f > 0.98f) 
		{

				if (m_pDummyAttTarget)
				{
					GetAISystem()->RemoveDummyObject(m_pDummyAttTarget);
					m_pDummyAttTarget = 0;
				}
				pOperand->m_bLooseAttention = false;
				pOperand->m_pLooseAttentionTarget = 0;
				return true;
		}

	/*	m_fLastDot = f;

		
		float zcross = myDir.x *otherDir.y - myDir.y*otherDir.x;
	//	if (f < 0)
	//		zcross-=f;
		pOperand->m_State.fValue =  (zcross) * pOperand->GetParameters().m_fResponsiveness * pOperand->m_fTimePassed;
		
		if (zcross < 0)
			pOperand->m_State.turnright = true;
		else 
			pOperand->m_State.turnleft = true;


		Vec3d vertCorrection = otherDir;
		Ang3	ang;
		ang=ConvertVectorToCameraAnglesSnap180( otherDir );
		vertCorrection=ConvertVectorToCameraAngles(vertCorrection);

		if (vertCorrection.x > 90.f)
			vertCorrection.x = 90.f;
		if (vertCorrection.x < -90.f)
			vertCorrection.x = 360+vertCorrection.x;
		pOperand->m_State.fValueAux = Snap_s180(ang.x-pOperand->GetAngles().x )*0.1f;
		*/
		
	}

	return false;
}


COPLookAround::~COPLookAround()
{
}


bool COPLookAround::Execute(CPipeUser *pOperand)
{
	
	//CAIObject *pObject = pOperand->m_pAttentionTarget;
	Vec3d myDir;
	Vec3d otherDir;

	if (!m_pDummyAttTarget)
	{
		// operand doesn't have a target... create a random new one
		m_pDummyAttTarget = GetAISystem()->CreateDummyObject();
		
		// lets place it at a random spot around the operand
		Vec3d mypos = pOperand->GetPos();;
		Vec3d myangles = pOperand->GetAngles();
		Vec3d offset;
		float rho, fi;
		if (m_fMaximumAngle> 0)
			fi = (((float)(rand() & 255)) / 255.f ) * m_fMaximumAngle; // random nr betwee 0 and maxangle
		else
			fi = (((float)(rand() & 255)) / 255.f ) * 6.28f; // random nr betwee 0 and 2pi
		
		rho = 20.f;
		fi-=(fi/2.f);
		fi+= (myangles.z*(3.14f/180.f));
		
		// offset.x and .y are guaranteed to be 2 floats (or doubles, doesn't matter) following
		// each other. CosSin will put the cosine into x and sine into y. It's run time is 
		// exactly the same as calculating just cosine or sine alone:
		//CosSin (fi,&offset.x); offset.x *= rho, offset.y *= rho;
		
		offset.x = rho * (float) cry_cosf(fi);
		offset.y = rho * (float) cry_sinf(fi);
		offset.z = 0;

		m_pDummyAttTarget->SetPos(mypos+offset);
		m_pDummyAttTarget->SetName("DUMMY");
		pOperand->m_bLooseAttention = true;
		pOperand->m_pLooseAttentionTarget = m_pDummyAttTarget;
	}
	else
	{
		Vec3d mypos = pOperand->GetPos();
		myDir = pOperand->GetAngles();
		myDir=ConvertToRadAngles(myDir);
		otherDir = m_pDummyAttTarget->GetPos();
		otherDir-=mypos;
		otherDir.Normalize();

		float f = myDir.Dot(otherDir);
		if (fabs(f-m_fLastDot) < 0.001) 
		{

				if (m_pDummyAttTarget)
				{
					GetAISystem()->RemoveDummyObject(m_pDummyAttTarget);
					m_pDummyAttTarget = 0;
				}
				pOperand->m_bLooseAttention = false;
				pOperand->m_pLooseAttentionTarget = 0;
				return true;
		}

		/*
		m_fLastDot = f;

		float zcross = myDir.x *otherDir.y - myDir.y*otherDir.x;
	//	pOperand->m_State.fValue =  (-zcross) * (acos(f)/pOperand->GetParameters().m_fResponsiveness);
		pOperand->m_State.fValue =  (-zcross) * pOperand->GetParameters().m_fResponsiveness* GetAISystem()->m_pSystem->GetITimer()->GetFrameTime();
		assert(-1e+9 < pOperand->m_State.fValue && pOperand->m_State.fValue < 1e+9);
		
		
		if (zcross > 0)
		{
			pOperand->m_State.turnright = true;
		}
		else
		{
			pOperand->m_State.turnleft = true;
		}
		*/
	}

	return false;
}

int COPLookAround::XMLExport(const char *pBuffer)
{
	strcat((char*)pBuffer,"<lookaround type=\"Goal\" Description=\"Instructs the agent to look away from its target in the specified bounds, or 360 deg if bounds not specified.\">\n");
	strcat((char*)pBuffer,"	<Bounds type=\"float\" value=\"0.0\" mandatory=\"NO\" units=\"radians\"/>\n");
	strcat((char*)pBuffer,"</lookaround>\n");

	return strlen(pBuffer);
}


void COPLookAround::Reset(CPipeUser *pOperand)
{
	m_pDummyAttTarget = 0;
	m_fLastDot = 0;
}



bool COPPathFind::Execute(CPipeUser *pOperand)
{
	if (!m_bWaitingForResult)
	{
		if (!m_pTarget)
		{

			// check whether the name given was a pathname
			if (m_sPathName.size()>0)
			{
				if (GetAISystem()->GetDesignerPath(m_sPathName.c_str(),pOperand->m_lstPath))
					return true; // it was, so just store it in the operand
			}

			// it target not specified, use last op result
			if  (!pOperand->m_pLastOpResult)
				return true;	// no last op result, return...

			Vec3d oppos= pOperand->m_pLastOpResult->GetPos();
			pOperand->RequestPathTo(oppos);
			m_bWaitingForResult = true;

			if (pOperand->m_nPathDecision == PATHFINDER_NOPATH)
				return true;

			return false;
		}
		else
		{
			// else just issue request to pathfind to target
			Vec3d oppos=m_pTarget->GetPos();;
			
			pOperand->RequestPathTo(oppos);
			m_bWaitingForResult = true;

			if (pOperand->m_nPathDecision == PATHFINDER_NOPATH)
				return true;

			return false;
		}
	}
	else
	{
		if (pOperand->m_nPathDecision)
		{
			m_bWaitingForResult = false;
			return true;
		}
			
	}

	return false;
}


int COPPathFind::XMLExport(const char *pBuffer)
{
	strcat((char*)pBuffer,"<pathfind type=\"Goal\" Description=\"Makes a request to generate path to destination targetname, pathname, or last operation target.\">\n");
	strcat((char*)pBuffer,"	<Target type=\"string\" value=\"\" mandatory=\"NO\" />\n");
	strcat((char*)pBuffer,"</pathfind>\n");

	return strlen(pBuffer);
}


bool COPLocate::Execute(CPipeUser *pOperand)
{
	if (m_sName.empty())
	{
		pOperand->SetLastOpResult((CAIObject*)GetAISystem()->GetNearestObjectOfType(pOperand->GetPos(),m_nObjectType,50));
		return true;
	}

	if (m_sName == "player")
	{
		// get the player
		pOperand->SetLastOpResult(GetAISystem()->GetPlayer());
		return true;
	}
	else if (m_sName == "beacon")
	{
		CAIObject *pBeacon = GetAISystem()->GetBeacon(pOperand->GetParameters().m_nGroup);
		if (pBeacon)
			pOperand->SetLastOpResult(pBeacon);
		return true;
	}
	else if (m_sName == "formation")
	{
		CAIObject *pFormationPoint = GetAISystem()->GetFormationPoint(pOperand);
		if (pFormationPoint)
			pOperand->SetLastOpResult(pFormationPoint);
		else
		{
			// lets send a NoFormationPoint event
			pOperand->SetSignal(1,"OnNoFormationPoint");
		}
		return true;
	}
	else if (m_sName == "atttarget")
	{
		pOperand->SetLastOpResult(pOperand->m_pAttentionTarget);
		return true;
	}
	else if (m_sName == "hidepoint")
	{
		int nbid;
		IVisArea *iva;

		Vec3d pos = pOperand->FindHidePoint(20,HM_RANDOM,GetAISystem()->CheckInside(pOperand->GetPos(),nbid,iva));
		if (!IsEquivalent(pos,pOperand->GetPos(),0.01f))
		{
			CAIObject *pHidePoint = (CAIObject*) GetAISystem()->CreateAIObject(AIOBJECT_HIDEPOINT,0);
			
			pHidePoint->SetPos(pos);
			pHidePoint->SetEyeHeight(pOperand->GetEyeHeight());
			pHidePoint->SetName("HIDEPOINT");
			// NOTE: This is not a leak. If anyone wants to change the lastopresult of this operand
			// he will have to release the old one first (because it it hidepoint)
			pOperand->SetLastOpResult(pHidePoint);
		}
		return true;
	}

	pOperand->m_pLastOpResult = GetAISystem()->GetAIObjectByName(m_sName.c_str());
	return true;
}

int COPLocate::XMLExport(const char *pBuffer)
{
	strcat((char*)pBuffer,"<locate type=\"Goal\" Description=\"Pushes as last operation target desired point or entity defined by its name\">\n");
	strcat((char*)pBuffer,"	<String type=\"string\" value=\"\" mandatory=\"YES\" />\n");
	strcat((char*)pBuffer,"</locate>\n");

	return strlen(pBuffer);
}



bool COPTrace::Execute(CPipeUser *pOperand)
{
	CAISystem *pSystem = GetAISystem();

	if (!m_pNavTarget)
	{
		// lets create the next nav target

		if (pOperand->m_lstPath.empty())
		{
			pOperand->m_bDirectionalNavigation = false;
			return true;  // finished
		}
	
		Vec3d vPos = (*pOperand->m_lstPath.begin());
		vPos.z+=0.3f;
	

		// shoot ray vertically down from this position and place target there.
		IPhysicalWorld *pWorld = pSystem->GetPhysicalWorld();

		bool bInside= false;
		int nBuilding;
		IVisArea *pArea;
		if (GetAISystem()->CheckInside(pOperand->GetPos(),nBuilding,pArea))
		{
			bInside = true;
			if (pSystem->CrowdControl(pOperand,vPos))
				return false;
		}


		ray_hit hit;
		int rayresult = pWorld->RayWorldIntersection(vectorf(vPos),vectorf(0,0,-40),ent_terrain|ent_static, rwi_stop_at_pierceable  | geom_colltype_player<<rwi_colltype_bit ,&hit,1);
		if (rayresult)
		{
			if ((fabs(hit.pt.z - vPos.z) < 4.f) || (!bInside))
				vPos = hit.pt;
		}


	 
		pOperand->m_lstPath.pop_front();
		m_vNextTarget = vPos-pOperand->GetPos();
		m_vNextTarget.z = 0;
		m_vNextTarget.Normalize();
		CreateDummyFromNode(vPos,GetAISystem());
		pOperand->m_AvoidingCrowd = false;
		m_pLastPosition = vPos;
		

		m_fTotalDistance=0;
		ListPositions::iterator li,linext;
		for (li=pOperand->m_lstPath.begin();li!=pOperand->m_lstPath.end();li++)
		{
			linext = li;
			linext++;
			if (linext==pOperand->m_lstPath.end())
				break;
			m_fTotalDistance+=((*li)-(*linext)).GetLength();
		}
		if (li!=pOperand->m_lstPath.end())
			m_fTotalDistance+=((*li)-vPos).GetLength();

		pOperand->m_pReservedNavPoint = m_pNavTarget;



	}

	

	if (bExactFollow)
		pOperand->m_bDirectionalNavigation = true;

	// unstuck the puppet if he didn't move in three updates
	Vec3d OpPos = pOperand->GetPos();
	if (!pOperand->m_bMovementSupressed)
	{
		int nBuilding;
		IVisArea *pArea;
		if (!GetAISystem()->CheckInside(pOperand->GetPos(),nBuilding,pArea))
			m_pLastPosition.z = OpPos.z;
		if (IsEquivalent(m_pLastPosition,OpPos,0.1f))
		{
			m_nTicker++;
			int	stuckTickLimit = 15;
			if(pOperand->GetType() == AIOBJECT_VEHICLE)
				stuckTickLimit = 15;
			if (m_nTicker == stuckTickLimit)
			{
				Reset(pOperand);
				pOperand->m_lstPath.clear();
				pOperand->m_bDirectionalNavigation = false;
				return true;
			}
		}
		else
			m_nTicker = 0;

	}


	
	
	
	m_pLastPosition = OpPos;

	// navigate puppet to the new navigation target
	{
		Vec3d mypos = OpPos;
		Vec3d myang = pOperand->GetAngles();
		myang=ConvertToRadAngles(myang);
		Vec3d navpos = m_pNavTarget->GetPos();




		int nBuilding;
		IVisArea *pArea;
		
 		///GraphNode *pThisNode = pSystem->GetGraph()->GetEnclosing(mypos,pOperand->m_pLastNode);
		if (!GetAISystem()->CheckInside(mypos,nBuilding,pArea))
			navpos.z = mypos.z;
		else
			mypos.z-=pOperand->GetEyeHeight();
		float fCurrentDistance = (navpos-mypos).GetLength();

		if (m_fDistance > m_fTotalDistance+fCurrentDistance)
		{
			Reset(pOperand);
			pOperand->m_lstPath.clear();
			pOperand->m_bDirectionalNavigation = false;
			return true;
		}

		float	fApproachDist=0.45f;
		bool	bKeepGoing = (fCurrentDistance > fApproachDist);
		Vec3d movement_dir = GetNormalized(navpos-mypos);
		if (movement_dir.Dot(m_vNextTarget) < 0)
		{
			bKeepGoing = false;
		}

		if(pOperand->GetType() == AIOBJECT_VEHICLE)
		{
			if( pOperand->m_pAttentionTarget )
			{
				float dist = (pOperand->m_pAttentionTarget->GetPos() - mypos).len();
				if( m_fDistance > dist )	// we are close to destination point already
				{
					Reset(pOperand);
					pOperand->m_lstPath.clear();
					pOperand->m_bDirectionalNavigation = false;
					return true;
				}
			}

			// approach distance depends on current speed for vehicles - more speed - bigger distance
			fApproachDist = 9.0f;
			IVehicleProxy *proxy=NULL;
			if(pOperand->GetProxy()->QueryProxy(AIPROXY_VEHICLE, (void**)&proxy))
			{
				pe_status_dynamics  dSt;
				float	curVelocity;
				// steer ammount depends on current speed
				proxy->GetPhysics()->GetStatus(&dSt);
				curVelocity = dSt.v.len();
				float	curDist = ( curVelocity/15.0f )*fApproachDist;
				if(curDist >9.0f)
					curDist = fApproachDist;
				if(curDist <2.0f)
					curDist  = 2.0f;
				fApproachDist = curDist;

//GetAISystem()->m_pSystem->GetILog()->Log("\001 %.2f  %.2f ", fCurrentDistance, fApproachDist);

			}

			if(fCurrentDistance < fApproachDist)
//{
				bKeepGoing=false;
//GetAISystem()->m_pSystem->GetILog()->Log("\001 >>>>got there--------  ");
//}
			if(bKeepGoing && !pOperand->m_lstPath.empty() && fCurrentDistance<15.0f )
			{
				Vec3d vNextDir = (*pOperand->m_lstPath.begin())-mypos;
				Vec3d vCurDir = navpos-mypos;
				vCurDir.z = vNextDir.z = 0;
//				float dotZ = vCurDir.x*vNextDir.x + vCurDir.y*vNextDir.y;
//				if( dotZ<0 )
//					bKeepGoing=false;
				float dotZCur = vCurDir.x*myang.x + vCurDir.y*myang.y;
				float dotZNext = vNextDir.x*myang.x + vNextDir.y*myang.y;
				if( dotZCur>0 && dotZNext<0  )
//{
					bKeepGoing=false;
//GetAISystem()->m_pSystem->GetILog()->Log("\001 >>>>canseled   ");
//}
			}
		}
		else
		{
			//--------ONLY FOR NONVEHICLES
			// check if puppet has to pass through a vehicle to get to his target
			// in that case abort the trace
			if (pOperand->m_lstPath.empty())
			{
				if (GetAISystem()->ThroughVehicle(OpPos,m_pNavTarget->GetPos()))
				{
					Reset(pOperand);
					pOperand->m_lstPath.clear();
					pOperand->m_bDirectionalNavigation = false;
					return true;
				}
			}
		}

		if ( bKeepGoing )
		{
			
			if(pOperand->GetType() == AIOBJECT_VEHICLE)
			{
//				pOperand->m_State.vTargetPos = pOperand->m_pAttentionTarget->GetPos();
//				pOperand->m_State.vMoveDir = GetNormalized((pOperand->m_State.vTargetPos-mypos));

				pOperand->m_State.vTargetPos = m_pNavTarget->GetPos();
				pOperand->m_State.vMoveDir = GetNormalized((navpos-mypos));
			}
			else
			{
				pOperand->m_State.vMoveDir = GetNormalized((navpos-mypos));
			}
		}
		else
		{
			// we reached this navpoint
			if (pOperand->m_pAttentionTarget == m_pNavTarget)
					pOperand->SetAttentionTarget(0);
			GetAISystem()->RemoveDummyObject(m_pNavTarget);
			m_pNavTarget = 0;
			pOperand->m_pReservedNavPoint = 0;
			pOperand->m_bDirectionalNavigation = false;
			if (m_bSingleStep)
				return true;
			else
				return Execute(pOperand);
		}

	}
	
	return false;
}

void COPTrace::CreateDummyFromNode(const Vec3d &pos, CAISystem *pSystem)
{

	m_pNavTarget = pSystem->CreateDummyObject();
	m_pNavTarget->SetEyeHeight(0.f);
	m_pNavTarget->SetPos(pos);


}

int COPTrace::XMLExport(const char *pBuffer)
{
	strcat((char*)pBuffer,"<trace type=\"Goal\" Description=\"Instructs the agent walk the generated or received path, with the desired precision\">\n");
	strcat((char*)pBuffer,"	<NoTarget type=\"bool\" value=\"false\" mandatory=\"YES\" />\n");
	strcat((char*)pBuffer,"</trace>\n");

	return strlen(pBuffer);
}



void COPTrace::Reset(CPipeUser *pOperand)
{
	GetAISystem()->RemoveDummyObject(m_pNavTarget);
	m_pNavTarget = 0;
}


bool COPIgnoreAll::Execute(CPipeUser *pOperand)
{
	pOperand->m_bUpdateInternal = !m_bParam;
	return true;
}

int COPIgnoreAll::XMLExport(const char *pBuffer)
{
	strcat((char*)pBuffer,"<ignoreall type=\"Goal\" Description=\"Makes agent start/stop (false/true) evaluating threats.\">\n");
	strcat((char*)pBuffer,"	<EvaluateThreats type=\"bool\" value=\"false\" mandatory=\"YES\" />\n");
	strcat((char*)pBuffer,"</ignoreall>\n");

	return strlen(pBuffer);
}



bool COPSignal::Execute(CPipeUser *pOperand)
{

	if (m_bSent)
	{
		m_bSent = false;
		return true;
	}

 	if (!m_cFilter)	// if you are sending to yourself
	{
		pOperand->SetSignal(m_nSignalID,m_sSignal.c_str());
		m_bSent = true;
		return false;
	}

	switch (m_cFilter)
	{
		case SIGNALFILTER_LASTOP:
			// signal to last operation target
			if (pOperand->m_pLastOpResult)
				pOperand->m_pLastOpResult->SetSignal(m_nSignalID,m_sSignal.c_str(),pOperand->GetAssociation());
			break;
		case SIGNALFILTER_TARGET:
			m_pTarget->SetSignal(m_nSignalID, m_sSignal.c_str(),pOperand->GetAssociation());
			break;
/*		case SIGNALFILTER_READIBILITY:
			pOperand->m_State.nAuxSignal = 1;
			pOperand->m_State.szAuxSignalText = m_sSignal.c_str();
			break;
			*/
		default:
			// signal to species, group or anyone within comm range
			GetAISystem()->SendSignal(m_cFilter,m_nSignalID,m_sSignal.c_str(),pOperand);
			break;

	}

	m_bSent = true;
	return false; 
}

int COPSignal::XMLExport(const char *pBuffer)
{
	strcat((char*)pBuffer,"<signal type=\"Goal\" Description=\"Generates a desired integer signal to owner and/or his group or species.\">\n");
	strcat((char*)pBuffer,"	<SignalID type=\"int\" value=\"0.0\" mandatory=\"YES\" />\n");
	strcat((char*)pBuffer,"	<SignalFilter type=\"int\" value=\"0.0\" mandatory=\"YES\" />\n");
	strcat((char*)pBuffer,"</signal>\n");

	return strlen(pBuffer);
}


bool COPDeValue::Execute(CPipeUser *pOperand)
{
	if (pOperand->m_pAttentionTarget)
		pOperand->Devalue(pOperand->m_pAttentionTarget,bDevaluePuppets);
	return true;
}

int COPDeValue::XMLExport(const char *pBuffer)
{
	strcat((char*)pBuffer,"<devalue type=\"Goal\" Description=\"Makes attention target very uninteresting. It will become interesting again later.\">\n");
	strcat((char*)pBuffer,"</devalue>\n");

	return strlen(pBuffer);
}


bool COPForget::Execute(CPipeUser *pOperand)
{
	pOperand->Forget(pOperand->m_pAttentionTarget);
	return true;
}

int COPForget::XMLExport(const char *pBuffer)
{
	strcat((char*)pBuffer,"<forget type=\"Goal\" Description=\"Instantly forgets the memorized attention target\">\n");
	strcat((char*)pBuffer,"</forget>\n");

	return strlen(pBuffer);
}


bool COPHide::Execute(CPipeUser *pOperand)
{
	CAISystem *pSystem = GetAISystem();
	pOperand->m_bHiding = true;
	
	if (!m_pHideTarget)
	{
			m_bEndEffect = false;
			pOperand->m_lstPath.clear();
			m_bAttTarget = true;
			if (!pOperand->m_pAttentionTarget)
			{
				m_bAttTarget = false;
				if (!pOperand->m_pLastOpResult)
				{
					pOperand->m_bLastHideResult = false;
					pOperand->m_bHiding = false;
					return true;
				}
			}
			// lets create the place where we will hide
	
			int nBuilding;
			IVisArea *pArea;
			//GraphNode *pNode = pSystem->GetGraph()->GetEnclosing(pOperand->GetPos());
			if (!GetAISystem()->CheckInside(pOperand->GetPos(),nBuilding,pArea))
			//if (pNode->nBuildingID == -1)
			{
					
					Vec3d vHidePos = pOperand->FindHidePoint(m_fSearchDistance, m_nEvaluationMethod,false);
					if (!pOperand->m_bLastHideResult || GetAISystem()->ThroughVehicle(pOperand->GetPos(),vHidePos))
					{
						pOperand->m_bHiding = false;
						return true;
					}

					m_vHidePos = vHidePos;
					Vec3d vHideDir;
					if (m_bAttTarget)
						vHideDir = vHidePos - pOperand->m_pAttentionTarget->GetPos();
					else
						vHideDir = vHidePos - pOperand->m_pLastOpResult->GetPos();
					vHideDir.z = 0;
					vHideDir.Normalize();
					
					IPhysicalWorld *pWorld = GetAISystem()->GetPhysicalWorld();
					ray_hit hit;   
					int rayresult = pWorld->RayWorldIntersection(vectorf(m_vHidePos),vectorf(vHideDir*50.f),ent_static,rwi_stop_at_pierceable,&hit,1);
					if (rayresult)					
					{
						Vec3d nml(hit.n.x,hit.n.y,hit.n.z);
						if (nml.Dot(vHideDir) < 0)
							vHidePos = Vec3d(hit.pt.x,hit.pt.y,hit.pt.z) + vHideDir;	
						else
							vHidePos = m_vHidePos + vHideDir*3.f;
					}
					else
						vHidePos+=(vHideDir*3.f);

		///			if (GetAISystem()->BehindForbidden(pOperand->GetPos(),vHidePos))
		//				return true;

					m_vLastPos(0,0,0);
					pOperand->m_vLastHidePos = m_vHidePos;
					
					m_pHideTarget = pSystem->CreateDummyObject();
					m_pHideTarget->SetEyeHeight(pOperand->GetEyeHeight());
					m_pHideTarget->SetPos(vHidePos);
					return false;
			}
			else
			{
					// inside
					Vec3d vHidePos = pOperand->FindHidePoint(m_fSearchDistance, m_nEvaluationMethod,true);
					if (!pOperand->m_bLastHideResult)
					{
						pOperand->m_bHiding = false;
						return true;
					}
	
					pOperand->m_vLastHidePos = vHidePos;
					m_pHideTarget = pSystem->CreateDummyObject();
					m_pHideTarget->SetEyeHeight(0);
					m_pHideTarget->SetPos(vHidePos);
					pOperand->m_nPathDecision = PATHFINDER_STILLTRACING;
					return false;
			}

	}
	else
	{
		
			if (!m_pPathFindDirective)
			{
				// request the path
				m_pPathFindDirective = new COPPathFind("",m_pHideTarget);
				return false;
			}
			else
			{
				if (!m_pTraceDirective)
				{
					if (m_pPathFindDirective->Execute(pOperand))
					{
						if (pOperand->m_nPathDecision == PATHFINDER_PATHFOUND)
						{
							m_pTraceDirective = new COPTrace(m_bLookAtHide);
						}
						else
						{
							Reset(pOperand);
							if (IsBadHiding(pOperand))
								pOperand->SetSignal(1,"OnBadHideSpot");
							return true;
						}
					}
				}
				else
				{
				/*	if (pOperand->m_pAttentionTarget)
					{
						GraphNode *pTgNode = pSystem->GetGraph()->GetEnclosing(pOperand->m_pAttentionTarget->GetPos());
						if (!pOperand->m_lstPath.empty())
						{
							//if (pTgNode->data.m_pos == pOperand->m_lstPath.front())
							if (IsEquivalent(pTgNode->data.m_pos,pOperand->m_lstPath.front()))
								pOperand->m_lstPath.pop_front();
						}
					}
					*/
					if (m_pTraceDirective->Execute(pOperand))
					{
						Reset(pOperand);
						if (IsBadHiding(pOperand))
							pOperand->SetSignal(1,"OnBadHideSpot");
						pOperand->m_bHiding = false;
						return true;
					}
					else
					{
						if (m_pHideTarget && !m_bEndEffect)
						{
								// we are on our last approach
								if ( GetLengthSquared(pOperand->GetPos()-m_pHideTarget->GetPos()) < 9.f)
								{
									// you are less than 3 meters from your hide target
									Vec3d vViewDir = ConvertToRadAngles(pOperand->GetAngles());
									Vec3d vHideDir = GetNormalized(m_pHideTarget->GetPos() - pOperand->GetPos());
									
									if (vViewDir.Dot(vHideDir) > 0.8f) 
									{
									//	pOperand->SetSignal(1,"HIDE_END_EFFECT");
										m_bEndEffect = true;
									}
								}
						}
					}

					return false;

				}
			}
	}
	return false;
}


bool COPHide::IsBadHiding(CPipeUser *pOperand)
{

	IAIObject *pTarget = pOperand->GetAttentionTarget();

	if (!pTarget)
		return false;

	IPhysicalWorld *pWorld = GetAISystem()->GetPhysicalWorld();

	Vec3d ai_pos = pOperand->GetPos();
	Vec3d target_pos = pTarget->GetPos();
	ray_hit hit;
	int rayresult = pWorld->RayWorldIntersection(target_pos,ai_pos-target_pos,ent_terrain|ent_static, rwi_stop_at_pierceable,&hit,1);
	if (rayresult)
	{
		// check possible leaning direction
		if (GetLengthSquared(hit.pt-ai_pos)<9.f)
		{
			Vec3d dir = ai_pos-target_pos;
			float zcross =  dir.y*hit.n.x - dir.x*hit.n.y;
			if (zcross < 0)
				pOperand->SetSignal(1,"OnRightLean");
			else
				pOperand->SetSignal(1,"OnLeftLean");
		}
		return false;
	}

	// try lowering yourself 1 meter
	ai_pos.z-=1.f;
	rayresult = pWorld->RayWorldIntersection(vectorf(ai_pos),vectorf(target_pos-ai_pos),ent_terrain|ent_static, rwi_stop_at_pierceable,&hit,1);
	if (rayresult)
	{
		// also switch bodypos automagically
		if (!pOperand->GetParameters().m_bSpecial)
			pOperand->SetSignal(1,"OnLowHideSpot");
		return false;
	}

	return true;
}


void COPHide::Reset(CPipeUser *pOperand)
{ 

  GetAISystem()->RemoveDummyObject(m_pHideTarget);

	if (m_pHideTarget)
		m_pHideTarget=0;

	if (m_pPathFindDirective)
		delete m_pPathFindDirective;

	m_pPathFindDirective = 0;

	if (m_pTraceDirective) 
		delete m_pTraceDirective;

	m_pTraceDirective = 0;

	pOperand->m_bHiding = false;

}

int COPHide::XMLExport(const char *pBuffer)
{
	strcat((char*)pBuffer,"<hide type=\"Goal\" Description=\"Evaluates closest hide place and hides there\">\n");
	strcat((char*)pBuffer,"</hide>\n");

	return strlen(pBuffer);
}



bool COPStick::Execute(CPipeUser *pOperand)
{
	CAIObject *pTarget = pOperand->m_pAttentionTarget;
	CAISystem *pSystem = GetAISystem();
	Vec3d mypos,targetpos;

	pOperand->m_State.fStickDist = m_fDistance;
	return true;
/*
	if (m_fDistance<0.0f)
		return true;

	if (!pTarget) 
	{
		if (pOperand->m_pLastOpResult)
			pTarget = pOperand->m_pLastOpResult;
		else
			return true;
	}


	mypos = pOperand->GetPos();
	targetpos = pTarget->GetPos();

	mypos-=targetpos;

	float dist = mypos.GetLength();

	GraphNode *pThisNode = pSystem->GetGraph()->GetEnclosing(pOperand->GetPos(),pOperand->m_pLastNode);
	GraphNode *pTargetNode = pSystem->GetGraph()->GetEnclosing(pTarget->GetPos(),pTarget->m_pLastNode);

	
	pOperand->m_pLastNode = pThisNode;
	pTarget->m_pLastNode = pTargetNode;

	dist -= m_fDistance;

	if (dist > 5.f)
	{
		pOperand->m_State.vMoveDir = GetNormalized((targetpos - pOperand->GetPos()));
		
	}
	else if (dist < -5.f)
	{
		pOperand->m_State.vMoveDir = GetNormalized((pOperand->GetPos()-targetpos));
//		pOperand->m_State.vMoveDir.Normalize();
	}
	else
		return true;

	if (pThisNode!=pTargetNode)
	{
		// steer if outdoors
		if (pThisNode->nBuildingID==-1)
			pOperand->Steer(pTarget->GetPos(),pThisNode);
	}

	pOperand->m_State.vMoveDir.Normalize();

	return false;	
*/
}

int COPStick::XMLExport(const char *pBuffer)
{
	strcat((char*)pBuffer,"<stick type=\"Goal\" Description=\"Makes agent hold his distance from his attention target.\">\n");
	strcat((char*)pBuffer,"	<Distance type=\"float\" value=\"3.0\" mandatory=\"YES\" />\n");
	strcat((char*)pBuffer,"</stick>\n");

	return strlen(pBuffer);
}



bool COPForm::Execute(CPipeUser *pOperand)
{
	pOperand->CreateFormation(m_sName.c_str());
	return true;
}

bool COPClear::Execute(CPipeUser *pOperand)
{
	return true;
}



bool COPJumpCmd::Execute(CPipeUser *pOperand)
{
	CAISystem *pSystem = GetAISystem();
	
	if (!m_pJumpTarget)
	{
			bool bAnticipate = false;
			CAIObject *pAnticipationTarget = 0;
			// lets create the place where we will jump
			Vec3d vJumpPos;

			if (m_fSearchDistance<1.f)
			{
				if (pOperand->m_pLastOpResult)
				{
					vJumpPos = pOperand->m_pLastOpResult->GetPos();
					if ( (pOperand->m_pLastOpResult->GetType()==AIOBJECT_PLAYER) ||
							(pOperand->m_pLastOpResult->GetType()==AIOBJECT_PUPPET) )
					{
						bAnticipate = true;
						pAnticipationTarget = pOperand->m_pLastOpResult;
					}
				}
				else
					return true;
			}
			else
			{
				int nBuildingID;
				IVisArea *pArea; 

				if (!pSystem->CheckInside(pOperand->GetPos(),nBuildingID,pArea))
				{
					vJumpPos = pOperand->FindHidePoint(m_fSearchDistance, m_nEvaluationMethod,false,!m_bJustCalculate);
					if (!pOperand->m_bLastHideResult)
					{
						pOperand->m_State.fJumpDuration = -1;
						return true;
					}

					m_vJumpPos = vJumpPos;

					IPhysicalWorld *pWorld = GetAISystem()->GetPhysicalWorld();
					IPhysicalEntity **pColliders;
					int colliders = pWorld->GetEntitiesInBox(vJumpPos-Vec3d(1.f,1.f,1.f),vJumpPos+Vec3d(1.f,1.f,1.f),pColliders,ent_static);
					if (colliders)					
					{
						pe_status_pos ppos;
						pColliders[0]->GetStatus(&ppos);
						if (ppos.BBox[1].z < 5.f)
							m_vJumpPos.z+=ppos.BBox[1].z;

					}

				}
				else
				{
					// inside
					vJumpPos = pOperand->FindHidePoint(m_fSearchDistance, m_nEvaluationMethod,true,!m_bJustCalculate);
					if (!pOperand->m_bLastHideResult)
					{
						pOperand->m_State.fJumpDuration = -1;
						return true;
					}

				}

			}

        
		Vec3d op_pos = pOperand->GetPos();
		

		if (bAnticipate)
		{
			if (pAnticipationTarget->m_bMoving)
			{
				Vec3d ant_dir = GetNormalized(pAnticipationTarget->GetPos()-pAnticipationTarget->m_vLastPosition);
				vJumpPos+=(ant_dir*4.f); 
			}
		}


		Vec3d jmpPos = vJumpPos;

		// shoot a ray to the jump position
		IPhysicalWorld *pWorld = GetAISystem()->GetPhysicalWorld();
		ray_hit hit;
		int rayresult = pWorld->RayWorldIntersection(op_pos,jmpPos-op_pos,ent_static, rwi_stop_at_pierceable,&hit,1);
		if (rayresult)
		{
			pOperand->m_State.fJumpDuration = 0;
			return true;
		}

		// land jump point on the nearest ground
		Vec3d vVertical(0,0,-100);
		rayresult = pWorld->RayWorldIntersection(jmpPos,vVertical,ent_all, rwi_stop_at_pierceable,&hit,1);
		if (rayresult)
			vJumpPos.z = hit.pt.z;


		

		//jmpPos.z+=pOperand->GetEyeHeight();
		
		
		 
		op_pos.z -=pOperand->GetEyeHeight();

	    Vec3d vJump =vJumpPos - op_pos;
		float d = vJump.GetLength();
		Vec3d vJumpProjected = vJump;
		vJumpProjected.z = 0;
		float dx = vJumpProjected.GetLength();
		float dy = vJump.z;
		
		float fGravMult = pOperand->GetParameters().m_fGravityMultiplier;
		float fActualGravity = 9.81f*fGravMult;


		// calculate duration and velocity of jump here
		// first velocity
		float vel;
		// calculate angles
		float beta = asin(dy/d);
		float gamma = m_fJumpAngleInRadians+beta;

		// calculate coefficients for the quadratic formula
		float sbeta,cbeta,sgamma,cgamma,tgamma;
		sbeta = sin(beta);
		cbeta = cos(beta);
        sgamma = sin(gamma);
		cgamma = cos(gamma);
		tgamma = sgamma/cgamma;
		float A = fActualGravity * dx* dx;
		float B = dx*sin(2*gamma) - dy*cgamma*cgamma;

		if (B<0.0001f) 
			return true;
				 
		vel = sqrt(A/B);
		// now calculate the time duration
		if ( fabs(vel*cgamma) < 0.01f)
			return true;
		float time = dx/(vel*cgamma);

		pOperand->m_State.jump = true;
 
		vJump/=d;	// normalize the direction vector
		if (!m_bJustCalculate)
		{
			
			//pOperand->m_State.vJumpDirection = vJump+Vec3d(0,0,1);
			Vec3d axis = vJump.Cross(Vec3d(0,0,1));
			
			//pOperand->m_State.vJumpDirection = Vec3d(0,-cgamma,sgamma);
			pOperand->m_State.vJumpDirection = vJump.rotated(axis,m_fJumpAngleInRadians);

			//pOperand->m_State.vJumpDirection *= cry_sqrtf(fGravMult*9.81f*d)*0.70710f;
			pOperand->m_State.vJumpDirection *= vel;
		//	pOperand->m_State.vMoveDir = vJump/d;
			m_pJumpTarget = pSystem->CreateDummyObject();
			m_pJumpTarget->SetEyeHeight(pOperand->GetEyeHeight());
			m_pJumpTarget->SetPos(vJumpPos);

		}
		else
		{
			pOperand->m_State.vJumpDirection.Set(0,0,0);
			pOperand->m_State.vMoveDir.Set(0,0,0);
		}                                

		int nDirectionOfJump = 1;
		Vec3d ang = pOperand->GetAngles();
		ang=ConvertToRadAngles(ang);
		ang.z = 0;
		ang.normalize();

		Vec3d normDir = vJumpPos-op_pos;
		normDir.z = 0;
		normDir.normalize(); 
		float fForwBack = ang.Dot(normDir);
		if (fabs(fForwBack) < 0.85f)
		{
			// its more left right than forward back
			float crossz = ang.y*normDir.x - ang.x*normDir.y;
			if (crossz > 0)
				nDirectionOfJump = 3;
			else
				nDirectionOfJump = 4;
		}
		else
		{
			if (fForwBack>0)
				nDirectionOfJump = 2;
		}
	
		pOperand->m_State.nJumpDirection = (float)(nDirectionOfJump);
	//	pOperand->m_State.fJumpDuration = cry_sqrtf(d/(fGravMult*9.81f))*1.2f;
		pOperand->m_State.fJumpDuration = time;
		m_fTimeLeft = pOperand->m_State.fJumpDuration;
		m_fLastUpdateTime = pSystem->m_pSystem->GetITimer()->GetCurrTime();

		return m_bJustCalculate;
	}
	else
	{
		//Vec3d mypos = pOperand->GetPos();
		//mypos.z = m_pJumpTarget->GetPos().z;

		float fCurrTime = pSystem->m_pSystem->GetITimer()->GetCurrTime();

		if (m_fTimeLeft<0)
		{
			pSystem->RemoveDummyObject(m_pJumpTarget);
			m_pJumpTarget = 0;
			return true;
		}

		m_fTimeLeft-=(fCurrTime-m_fLastUpdateTime);
		m_fLastUpdateTime = fCurrTime;

	}
	return false;
}

void COPJumpCmd::Reset(CPipeUser *pOperand)
{ 
	if (m_pJumpTarget)
		m_pJumpTarget=0;
}

int COPJumpCmd::XMLExport(const char *pBuffer)
{
	strcat((char*)pBuffer,"<hide type=\"Goal\" Description=\"Evaluates closest hide place and hides there\">\n");
	strcat((char*)pBuffer,"</hide>\n");

	return strlen(pBuffer);
}




COPHeliAdv::COPHeliAdv()
{
}

COPHeliAdv::~COPHeliAdv()
{
}

bool COPHeliAdv::Execute(CPipeUser *pOperand)
{

	if(pOperand->GetType() != AIOBJECT_VEHICLE)		// only for heli
		return true;
	if( pOperand->m_pAttentionTarget == NULL)		// no attention target - no attack
		return true;

	IVehicleProxy *proxy=NULL;

	if(pOperand->GetProxy()->QueryProxy(AIPROXY_VEHICLE, (void**)&proxy))
	{
		pOperand->m_State.vTargetPos = pOperand->m_pAttentionTarget->GetPos();
		pOperand->m_State.fValueAux = pOperand->GetEyeHeight();
		Vec3 nextPos = proxy->HeliAttackAdvance( pOperand->m_State );

		CAIObject *pHidePoint = (CAIObject*) GetAISystem()->CreateAIObject(AIOBJECT_HIDEPOINT,0);
		pHidePoint->SetPos(nextPos);
		pHidePoint->SetEyeHeight(pOperand->GetEyeHeight());

		// NOTE: This is not a leak. If anyone wants to change the lastopresult of this operand
		// he will have to release the old one first (because it it hidepoint)
		pOperand->SetLastOpResult(pHidePoint);
	}
	return true;
}

void COPHeliAdv::Reset(CPipeUser *pOperand )
{

}



int COPHeliAdv::XMLExport(const char *pBuffer)
{
	strcat((char*)pBuffer,"<hide type=\"Goal\" Description=\"Finds next position for helicopter attack\">\n");
	strcat((char*)pBuffer,"</heliadv>\n");

	return strlen(pBuffer);
}




