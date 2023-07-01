// GoalPipe.cpp: implementation of the CGoalPipe class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CAISystem.h"
#include "GoalPipe.h"
#include "GoalOp.h"
#include "PipeUser.h"

#include <ISystem.h>
#include <ITimer.h>

#if defined(WIN32) && defined(_DEBUG) 
#include <crtdbg.h> 
#define DEBUG_NEW_NORMAL_CLIENTBLOCK(file, line) new(_NORMAL_BLOCK, file, line) 
#define new DEBUG_NEW_NORMAL_CLIENTBLOCK( __FILE__, __LINE__) 
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGoalPipe::CGoalPipe(const string &name, CAISystem *pAISystem)
{
	m_sName = name;
	m_pAISystem = pAISystem;
	m_nPosition = 0;
	m_pSubPipe = 0; 
	m_pArgument = 0;
}

CGoalPipe::~CGoalPipe()
{
	if (!m_qGoalPipe.empty())
	{
//		GoalQueue::iterator gi;
/*		for (gi=m_qGoalPipe.begin();gi!=m_qGoalPipe.end();gi++)
		{
			QGoal goal = (*gi);
			if (goal.pGoalOp)
				delete goal.pGoalOp;
		}
		*/
		m_qGoalPipe.clear();
	}

	if (m_pSubPipe)
	{
		delete m_pSubPipe;
		m_pSubPipe = 0;
	}
}

void CGoalPipe::PushGoal(const string &Name, bool bBlocking, GoalParameters &params)
{
	QGoal newgoal;

	if (Name == AIOP_ACQUIRETARGET)
	{
		newgoal.pGoalOp = new COPAcqTarget((CAIObject *) params.m_pTarget);
	}
	else if (Name == AIOP_APPROACH)
	{
		newgoal.pGoalOp = new COPApproach(params.fValue, params.nValue!=0, params.bValue);
	}
	else if (Name == AIOP_BACKOFF)
	{
		newgoal.pGoalOp = new COPBackoff(params.fValue, params.nValue !=0);
	}
	else if (Name == AIOP_FIRECMD)
	{
		bool allowed = (params.fValue > 0);
		bool smart = !(params.fValue > 1);
		newgoal.pGoalOp = new COPFireCmd(allowed,smart);
	}
	else if (Name == AIOP_BODYPOS)
	{
		int thepos = (int) params.fValue;
		newgoal.pGoalOp = new COPBodyCmd(thepos);
	}
	else if (Name == AIOP_STRAFE)
	{
		 newgoal.pGoalOp = new COPStrafe(params.fValue);	
	}
	else if (Name == AIOP_TIMEOUT)
	{	
		if (!m_pAISystem->m_pSystem)
			CryError("[AISYSTEM ERROR] Pushing goals without a valid System instance");
		if (!m_pAISystem->m_pSystem->GetITimer())
			CryError("[AISYSTEM ERROR] Pushing goals without a valid Timer instance");

		newgoal.pGoalOp = new COPTimeout(params.fValue, m_pAISystem->m_pSystem->GetITimer(),params.fValueAux);
	}
	else if (Name == AIOP_RUN)
	{
		newgoal.pGoalOp = new COPRunCmd((params.fValue>0));
	}
/*	else if (Name == AIOP_JUMP)
	{
		newgoal.pGoalOp = new COPJumpCmd();
	}*/
	else if (Name == AIOP_LOOKAROUND)
	{
		newgoal.pGoalOp = new COPLookAround(params.fValue);
	}
	else if (Name == AIOP_LOCATE)
	{
		newgoal.pGoalOp = new COPLocate(params.szString.c_str(), params.nValue);
	}
	else if (Name == AIOP_PATHFIND)
	{
		newgoal.pGoalOp = new COPPathFind(params.szString.c_str(),params.m_pTarget);
	}
	else if (Name == AIOP_TRACE)
	{
		bool bParam = (params.nValue > 0);
		newgoal.pGoalOp = new COPTrace(bParam,0,(params.fValue>0));
	}
	else if (Name == AIOP_IGNOREALL)
	{
		bool bParam = (params.fValue > 0);
		newgoal.pGoalOp = new COPIgnoreAll(bParam);
	}
	else if (Name == AIOP_SIGNAL)
	{
		int nParam = (int) params.fValue;
		unsigned char cFilter = (unsigned char) params.nValue;
		newgoal.pGoalOp = new COPSignal(nParam,params.szString.c_str(),cFilter);
	}
	else if (Name == AIOP_DEVALUE)
	{
		newgoal.pGoalOp = new COPDeValue(params.fValue);
	}
	else if (Name == AIOP_FORGET)
	{
		newgoal.pGoalOp = new COPForget();
	}
	else if (Name == AIOP_HIDE)
	{
		newgoal.pGoalOp = new COPHide(params.fValue,params.nValue,params.bValue);
	}
	else if (Name == AIOP_JUMP)
	{
		newgoal.pGoalOp = new COPJumpCmd(params.fValue,params.nValue,params.bValue,params.fValueAux);
	}
	else if (Name == AIOP_STICK)
	{
		newgoal.pGoalOp = new COPStick(params.fValue,false);
	}
	else if (Name == AIOP_FORM)
	{
    newgoal.pGoalOp = new COPForm(params.szString.c_str());
	}
	else if (Name == AIOP_CLEAR)
	{
		newgoal.pGoalOp = new COPDeValue();
	}
	else if (Name == AIOP_LOOP)
	{
		newgoal.pGoalOp = new COPDeValue();
		bBlocking = false;
	}
	else if (Name == AIOP_LOOKAT)
	{
		newgoal.pGoalOp = new COPLookAt(params.fValue, (float) params.nValue);
	}
	else if (Name == AIOP_HELIADV)
	{
		newgoal.pGoalOp = new COPHeliAdv();
	}


	newgoal.params = params;
	newgoal.name = Name.c_str(); // FIX: refcounted STL with static libs (e.g. on AMD64 compiler) will crash without .c_str()
	newgoal.bBlocking = bBlocking;
	m_qGoalPipe.push_back(newgoal);
}


GoalPointer CGoalPipe::PopGoal(bool &blocking, string &name, GoalParameters &params, CPipeUser *pOperand)
{

	// if we are processing a subpipe
	if (m_pSubPipe)
	{
		GoalPointer anymore = m_pSubPipe->PopGoal(blocking, name,params,pOperand);
		if (!anymore)
		{
			if (m_pSubPipe)
				delete m_pSubPipe;
			
			m_pSubPipe = 0; // this subpipe is finished

			if (m_pArgument)
				pOperand->SetLastOpResult(m_pArgument);

		}
		else
			return anymore;
	}

	if (m_nPosition < m_qGoalPipe.size() )
	{
		QGoal current = m_qGoalPipe[m_nPosition++];

		CGoalPipe *pPipe;
		if (pPipe = m_pAISystem->IsGoalPipe(current.name))
		{
			// this goal is a subpipe of goals, get that one until it is finished
			m_pSubPipe = pPipe;
			return m_pSubPipe->PopGoal(blocking,name,params,pOperand);
		}
		else
		{
			// this is an atomic goal, just return it
			blocking = current.bBlocking;
			name = current.name;
			params = current.params;
			// goal succesfuly retrieved

			return current.pGoalOp;
		}
	}

	// we have reached the end of this goal pipe
	// reset position and let the world know we are done
	Reset();
	return 0;
}

CGoalPipe * CGoalPipe::Clone()
{

	CGoalPipe *pClone = new CGoalPipe(m_sName, m_pAISystem);

	// copy goal queue
	GoalQueue::iterator gi;

	for (gi=m_qGoalPipe.begin();gi!=m_qGoalPipe.end();gi++)
	{
		QGoal gl = (*gi);
		pClone->PushGoal(gl.name,gl.bBlocking,gl.params);
	}

	return pClone;
}

void CGoalPipe::Reset()
{
	m_nPosition = 0;
	if (m_pSubPipe)
		delete m_pSubPipe;
	m_pSubPipe = 0;
}

// Makes the IP of this pipe jump to the desired position
void CGoalPipe::Jump(int position)
{
	if (position<0)
		position--;
	if (m_nPosition)
		m_nPosition+=position;
}

bool CGoalPipe::IsInSubpipe(void)
{
	return (m_pSubPipe != 0);
}

CGoalPipe * CGoalPipe::GetSubpipe(void)
{
	return m_pSubPipe;
}

void CGoalPipe::SetSubpipe(CGoalPipe * pPipe)
{
	m_pSubPipe = pPipe;
}
