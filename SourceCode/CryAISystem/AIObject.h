// AIObject.h: interface for the CAIObject class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_AIOBJECT_H__ED373C3A_BCCB_48BE_A2C8_B53177D331D7__INCLUDED_)
#define AFX_AIOBJECT_H__ED373C3A_BCCB_48BE_A2C8_B53177D331D7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IAgent.h"
#include <string>

struct GraphNode;
class CAISystem;


/*! Basic ai object class. Defines a framework that all puppets and points of interest
	later follow.
*/
class CAIObject : public IAIObject
{

	IUnknownProxy *m_pProxy;
	
	

protected:

	unsigned short	m_nObjectType;
	Vec3						m_vPosition;
	
	Vec3						m_vOrientation;
	void *					m_pAssociation;
	float						m_fEyeHeight;
	float						m_fRadius;

	CAISystem			 *m_pAISystem;

	string			m_sName;

typedef std::vector<CAIObject *> AIBINDLIST;
typedef std::vector<CAIObject *>::iterator AIBINDLISTiterator;
	AIBINDLIST					m_lstBindings;
	bool						m_bIsBoind;
	Vec3						m_vBoundPosition;

	void	UpdateHierarchy();

public:
	void SetAISystem(CAISystem *pSystem);
	void GetLastPosition(Vec3 &pos);
	void IsEnabled(bool enabled);
	char * GetName();
	void SetName( const char *pName);
	
	void SetEyeHeight(float height);
	void SetAssociation(void *pAssociation);
	void * GetAssociation();
	void SetType(unsigned short type);
	unsigned short GetType();
	const Vec3 &GetPos( );
	void SetPos(const Vec3 &pos, bool bKeepEyeHeight=true);
	const Vec3 &GetAngles() { return m_vOrientation; }
	void SetAngles(const Vec3 &angles);
	void Release() {delete this;}

	virtual void ParseParameters( const AIObjectParameters &params);
	virtual void Update();
	virtual void Event(unsigned short eType, SAIEVENT *pEvent) {}
	virtual bool CanBeConvertedTo(unsigned short type, void **pConverted);
	virtual void OnObjectRemoved(CAIObject *pObject ) {}
	virtual void NeedsPathOutdoor( bool bNeeds, bool bForce=false ) {
					m_bNeedsPathOutdoor = bNeeds; 
					m_bForceTargetPos = bForce; }
	virtual bool IfNeedsPathOutdoor( ) { return m_bNeedsPathOutdoor; }



	CAIObject();
	virtual ~CAIObject();

	SOBJECTSTATE		m_State;
	bool						m_bEnabled;
	bool						m_bSleeping;
	bool						m_bCanReceiveSignals;
	GraphNode*			m_pLastNode;
	bool						m_bMoving;
	float						m_DEBUGFLOAT;
	bool						m_bDEBUGDRAWBALLS;
	bool						m_bForceTargetPos;			// used for vehicles
	bool						m_bNeedsPathOutdoor;		// used for vehicles
	bool						m_bNeedsPathIndoor;			//
	float				m_fPassRadius;		
	bool	m_bCloaked;
	Vec3						m_vLastPosition;

	virtual void Reset(void);
	float GetEyeHeight(void);
	// returns the state of this object
	SOBJECTSTATE * GetState(void);
	void SetSignal(int nSignalID, const char * szText, void *pSender=0);

	virtual void Bind(IAIObject* bind) { }
	virtual void Unbind( ) { }
	
	void EDITOR_DrawRanges(bool bEnable);

//	virtual IAIObject* GetBound( )		{ return 0; }
	virtual IUnknownProxy* GetProxy() { return m_pProxy; };

//	void CAIObject::SetEyeHeight(float height);

	void	CreateBoundObject( unsigned short type, const Vec3& vBindPos, const Vec3& vBindAngl);
	void	SetPosBound(const Vec3 &pos);
	const Vec3 &GetPosBound( );
	void SetRadius(float fRadius);
	bool IsMoving() { return m_bMoving;}

	float GetRadius(void)
	{
		return m_fRadius;
	}
	virtual void Save(CStream & stm);
	virtual void Load(CStream & stm);
	virtual void Load_PATCH_1(CStream & stm) { Load(stm); }
};

#endif // !defined(AFX_AIOBJECT_H__ED373C3A_BCCB_48BE_A2C8_B53177D331D7__INCLUDED_)
