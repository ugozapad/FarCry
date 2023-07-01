// GoalOp.h: interface for the CGoalOp class.
//
//////////////////////////////////////////////////////////////////////


#if !defined(AFX_GOALOP_H__1CD1CEF7_0DC6_4C6C_BCF1_EDE36179F7E7__INCLUDED_)
#define AFX_GOALOP_H__1CD1CEF7_0DC6_4C6C_BCF1_EDE36179F7E7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CPuppet;
class CAIObject;
struct IAIObject;
struct ITimer;
struct GraphNode;
//<<FIXME>> remove later
class CAISystem;

//ADDED FOR PIPE USER
class CPipeUser;
//-------------------------

#include <string>

class CGoalOp  
{
public:
	CGoalOp();
	virtual ~CGoalOp();

	virtual bool Execute(CPipeUser *pOperand) {return true;}
	virtual int XMLExport( const char *pBuffer) {return 0;}
	virtual void Reset(CPipeUser *pOperand) {}
};

////////////////////////////////////////////////////////////
//
//				ACQUIRE TARGET - acquires desired target, even if it is outside of view
//
////////////////////////////////////////////////////////

class COPAcqTarget: public CGoalOp
{
	
public:
	COPAcqTarget(CAIObject *pTarget);
	~COPAcqTarget();

	CAIObject *m_pTarget;

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer);
};


////////////////////////////////////////////////////////////
//
//				APPROACH - makes agent approach to "distance" from current att target
//
////////////////////////////////////////////////////////
class COPPathFind;
class COPTrace;
class COPApproach: public CGoalOp
{

	float m_fLastDistance;
	CAIObject *m_pTempTarget;
	float m_fInitialDistance;
	bool	m_bPercent;
	bool	m_bUseLastOpResalt;
	char	m_nTicks;
	COPTrace		*m_pTraceDirective;
	COPPathFind		*m_pPathfindDirective;

	float	m_fTime;
	
public:
	COPApproach(float distance, bool percent = false, bool useLastOpResalt = false );
	~COPApproach();

	float m_fDistance;

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer);
	void Reset(CPipeUser *pOperand);
};

////////////////////////////////////////////////////////////
//
//				BACKOFF - makes agent back off to "distance" from current att target
//
////////////////////////////////////////////////////////
class COPBackoff: public CGoalOp
{
	bool m_bPercent;
	bool m_bBackingOff;
	Vec3d m_vSnapShot_of_Position;

public:
	COPBackoff(float distance, bool percent = false);
	~COPBackoff();

	float m_fDistance;

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer);
};

////////////////////////////////////////////////////////////
//
//				TIMEOUT - counts down a timer...
//
////////////////////////////////////////////////////////
class COPTimeout: public CGoalOp
{

	ITimer	*m_pTimer;
	float		m_fCount;
	float		m_fLastTime;
public:
	COPTimeout(float interval, ITimer *pTimer, float interval_end = 0);
	~COPTimeout();

	float m_fInterval;	
	float m_fIntervalCurrent;	
	float m_fIntervalEnd;	

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer);
};

////////////////////////////////////////////////////////////
//
//				STRAFE makes agent strafe left or right (1,-1)... 0 stops strafing
//
////////////////////////////////////////////////////////
class COPStrafe: public CGoalOp
{
	
public:
	COPStrafe(float distance);
	~COPStrafe();

	float m_fDistance,m_fRemainingDistance,m_fLastRemainingDistance;
	bool m_bPositionSnapshotTaken;
	Vec3d m_vTargetPos;
	Vec3d m_vStartPos;

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer){return 0;}
};

////////////////////////////////////////////////////////////
//
//				FIRECMD - 1 allowes agent to fire, 0 forbids agent to fire
//
////////////////////////////////////////////////////////
class COPFireCmd: public CGoalOp
{
	
public:
	COPFireCmd(bool AllowedToFire, bool bSmartFire);
	~COPFireCmd();

	bool m_bAllowedToFire;
	bool m_bSmartFire;

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer);
};

////////////////////////////////////////////////////////////
//
//				BODYCMD - controls agents stance (0- stand, 1-crouch, 2-prone)
//
////////////////////////////////////////////////////////
class COPBodyCmd: public CGoalOp
{
public:
	COPBodyCmd(int bodystate);
	~COPBodyCmd(){}

	int m_nBodyState;

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer);
};

////////////////////////////////////////////////////////////
//
//				RUNCMD - makes the agent run if 1, walk if 0
//
////////////////////////////////////////////////////////
class COPRunCmd: public CGoalOp
{
public:
	COPRunCmd(bool running);
	~COPRunCmd() {}

	bool m_bRunning;

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer);
};

/*
////////////////////////////////////////////////////////////
//
//				JUMPCMD - makes the agent jump
//
////////////////////////////////////////////////////////
class COPJumpCmd: public CGoalOp
{
public:
	COPJumpCmd(){}
	~COPJumpCmd(){}

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer);
};
*/


////////////////////////////////////////////////////////////
//
//				LOOKAROUND - looks in a random direction
//
////////////////////////////////////////////////////////
class COPLookAround : public CGoalOp
{
	CAIObject *m_pDummyAttTarget;
	float m_fLastDot;
	float m_fMaximumAngle;
public:
	void Reset(CPipeUser *pOperand);
	COPLookAround(float maximum) {m_pDummyAttTarget=0; m_fMaximumAngle = maximum;}
	~COPLookAround();

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer);
};


////////////////////////////////////////////////////////////
//
//				PATHFIND - generates a path to desired aiobject
//
////////////////////////////////////////////////////////
class COPPathFind : public CGoalOp
{
	IAIObject *m_pTarget;
	string m_sPathName;
	bool m_bWaitingForResult;
public:
	COPPathFind(const char *szName, IAIObject *pTarget = 0) { m_pTarget = pTarget; m_bWaitingForResult =false;m_sPathName = szName; }
	~COPPathFind() {}

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer);
};


////////////////////////////////////////////////////////////
//
//				LOCATE - locates an aiobject in the map
//
////////////////////////////////////////////////////////
class COPLocate : public CGoalOp
{
	string m_sName;
	unsigned int m_nObjectType;
public:
	COPLocate(const char *szName, unsigned int ot = 0) { if (szName) m_sName = szName;m_nObjectType = ot;}
	~COPLocate() {}

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer);
};

////////////////////////////////////////////////////////////
//
//				TRACE - makes agent follow generated path... does nothing if no path generated
//
////////////////////////////////////////////////////////
class COPTrace : public CGoalOp
{
	bool bExactFollow;
	Vec3d	m_pLastPosition;
	float m_fDistance;
	float m_fTotalDistance;
	bool	m_bSingleStep;
	unsigned char m_nTicker;
	Vec3d m_vNextTarget;

public:

	CAIObject *m_pNavTarget;

	void Reset(CPipeUser *pOperand);
	COPTrace( bool bExact, float fDistance = 0, bool bSingleStep = false) { m_pNavTarget = 0; bExactFollow = bExact;m_fDistance = fDistance;m_bSingleStep = bSingleStep;}
	~COPTrace() {}

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer);
protected:
	void CreateDummyFromNode(const Vec3d &pos, CAISystem *pSystem);
};

////////////////////////////////////////////////////////////
//
//				IGNOREALL - 1, puppet does not reevaluate threats, 0 evaluates again
//
////////////////////////////////////////////////////////
class COPIgnoreAll : public CGoalOp
{
	bool	m_bParam;
public:
	COPIgnoreAll(bool param) { m_bParam = param;}
	~COPIgnoreAll() {}

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer);
};


////////////////////////////////////////////////////////////
//
//				SIGNAL - send a signal to himself or other agents
//
////////////////////////////////////////////////////////
class COPSignal : public CGoalOp
{
	int m_nSignalID;
	string m_sSignal;
	unsigned char m_cFilter;
	CAIObject *m_pTarget;
	bool m_bSent;
public:
	COPSignal(int param, const char *szSignal, unsigned char cFilter) { m_bSent = false;m_nSignalID = param;m_pTarget=0;m_cFilter = cFilter;m_sSignal = szSignal;}
	~COPSignal() {}

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer);
};


////////////////////////////////////////////////////////////
//
//				DEVALUE - devalues current attention target 
//
////////////////////////////////////////////////////////
class COPDeValue : public CGoalOp
{
	bool bDevaluePuppets;
public:
	COPDeValue(int nPuppetsAlso = 0) { bDevaluePuppets = (nPuppetsAlso!=0);}
	~COPDeValue() {}

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer);
};

////////////////////////////////////////////////////////////
//
//				FORGET - makes agent forget the current attention target, if it can be found in memory
//
////////////////////////////////////////////////////////
class COPForget : public CGoalOp
{

public:
	COPForget() {}
	~COPForget() {}

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer);
};


////////////////////////////////////////////////////////////
//
//			HIDE - makes agent find closest hiding place and then hide there
//
////////////////////////////////////////////////////////
class COPHide : public CGoalOp
{
	CAIObject *m_pHideTarget;
	COPApproach *m_pApproach;
	Vec3d m_vHidePos;
	Vec3d m_vLastPos;
	float m_fSearchDistance;
	int		m_nEvaluationMethod;
	COPPathFind *m_pPathFindDirective;
	COPTrace *m_pTraceDirective;
	bool m_bLookAtHide;
	char m_nTicks;
	bool m_bAttTarget;
	bool m_bEndEffect;

public:
	COPHide(float distance, int method, bool bExact) 
	{
		m_fSearchDistance=distance; m_nEvaluationMethod = method; m_pHideTarget=0;
		m_pPathFindDirective = 0;
		m_pTraceDirective = 0;
		m_bLookAtHide = bExact;
		m_nTicks = 0;
	}
	~COPHide() {}

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer);
	void Reset(CPipeUser *pOperand);
	bool IsBadHiding(CPipeUser *pOperand);
};


////////////////////////////////////////////////////////////
//
//			STICK - the agent keeps constant distance to his target
//
////////////////////////////////////////////////////////
class COPStick : public CGoalOp
{
	float m_fDistance;
	bool m_bGroup;
public:
	COPStick(float dist,bool bGroup) { m_fDistance=dist;m_bGroup = bGroup;}
	~COPStick() {}

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer);
};


////////////////////////////////////////////////////////////
//
//			FORM - this agent creates desired formation
//
////////////////////////////////////////////////////////
class COPForm : public CGoalOp
{
	string m_sName;
	
public:
	COPForm(const char *name) { m_sName = name;}
	~COPForm() {}

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer){return 0;}
};


////////////////////////////////////////////////////////////
//
//			CLEAR - clears the actions for the operand puppet
//
////////////////////////////////////////////////////////
class COPClear : public CGoalOp
{
	string m_sName;
	
public:
	COPClear() {}
	~COPClear() {}

	bool Execute(CPipeUser *pOperand);

#if !defined(LINUX)
	int XMLExport(const char *pBuffer);
#endif
};


////////////////////////////////////////////////////////////
//
//			CLEAR - clears the actions for the operand puppet
//
////////////////////////////////////////////////////////
class COPLookAt : public CGoalOp
{
	float m_fStartAngle;
	float m_fEndAngle;
	CAIObject *m_pDummyAttTarget;
	float m_fLastDot;
	
public:
	COPLookAt(float startangle, float endangle) {m_fStartAngle = startangle; m_fEndAngle = endangle;m_pDummyAttTarget=0;}
	~COPLookAt() {}

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer){return 0;}
};


////////////////////////////////////////////////////////////
//
//			JUMPCMD - makes apgent find closest hiding place and then do bug jump to hide there
//
////////////////////////////////////////////////////////
class COPJumpCmd : public CGoalOp
{
	CAIObject *m_pJumpTarget;
	Vec3d m_vJumpPos;
	Vec3d m_vLastPos;
	float m_fSearchDistance;
	int		m_nEvaluationMethod;
	bool m_bJustCalculate;
	float m_fTimeLeft;
	float m_fLastUpdateTime;
	float m_fJumpAngleInRadians;

public:
	COPJumpCmd(float distance, int method, bool bExact, float fJumpAngle) 
	{
		m_fSearchDistance=distance; m_nEvaluationMethod = method; m_pJumpTarget=0;
		m_bJustCalculate = bExact;
		m_fJumpAngleInRadians = fJumpAngle*3.1415926f/180.f;
	}
	~COPJumpCmd() {}

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer);
	void Reset(CPipeUser *pOperand);
};



////////////////////////////////////////////////////////////
//
//				HELLYADV - helicopter chooses the next point for attack - puts it into LastOpResult
//
////////////////////////////////////////////////////////
class COPHeliAdv: public CGoalOp
{
	
public:
	COPHeliAdv();
	~COPHeliAdv();

	float m_fDistance;

	bool Execute(CPipeUser *pOperand);
	int XMLExport(const char *pBuffer);
	void Reset(CPipeUser *pOperand);
};





#endif // !defined(AFX_GOALOP_H__1CD1CEF7_0DC6_4C6C_BCF1_EDE36179F7E7__INCLUDED_)
