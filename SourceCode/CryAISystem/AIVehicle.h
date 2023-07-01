#ifndef _AI_VEHICLE_
#define _AI_VEHICLE_

//#include "aiobject.h"
#include "Puppet.h"

#include "CAISystem.h"
#include "PipeUser.h"
//#include "AgentParams.h"
//#include "IAgent.h"
//#include "GoalPipe.h"
//#include "Graph.h"
//#include <list>
//#include <map>
//#include <vector>


class CPuppet;

class CAIVehicle :
//	public CPipeUser, IVehicle
	public CPuppet
{
public:
	CAIVehicle(void);
	~CAIVehicle(void);

	void Update();
	void Steer(const Vec3d & vTargetPos, GraphNode * pNode);
//	void UpdateVehicleInternalState();
	void Navigate(CAIObject *pTarget);
	void Event(unsigned short eType, SAIEVENT *pEvent);

	void Reset(void);
	void ParseParameters(const AIObjectParameters &params);
	void OnObjectRemoved(CAIObject *pObject);

	bool CanBeConvertedTo(unsigned short type, void **pConverted);

	void	SetParameters(AgentParameters & sParams);
	AgentParameters GetPuppetParameters() { return GetParameters();}
	void SetPuppetParameters(AgentParameters &pParams) { SetParameters(pParams);}

	void SetVehicleType(unsigned short type) { m_VehicleType = type; }
	unsigned short GetVehicleType( ) { return m_VehicleType; }

	void UpdateThread();

	virtual IUnknownProxy* GetProxy() { return m_pProxy; };
	void Bind(IAIObject* bind);
	void Unbind( );
//	IAIObject* GetBound( )  { return m_Gunner; }
//	void	SetGunner( IAIObject *pGunner );				//CPuppet	*gunner);
//	void Event(unsigned short eType, SAIEVENT *pEvent);

	IVehicleProxy	*m_pProxy;

	CAIObject		*m_Threat;

private:

	unsigned short m_VehicleType;

	CPuppet	*m_Gunner;

public:
	void AlertPuppets(void);
	void Save(CStream & stm);
	void Load(CStream & stm);
};

#endif