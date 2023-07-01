#include "stdafx.h"
#include <crysizer.h>
#include "CAISystem.h"
#include "Puppet.h"
#include "AIVehicle.h"
#include "AIPlayer.h"
#include "GoalPipe.h"
#include "GoalOp.h"
#include "aiautobalance.h"




void CAISystem::GetMemoryStatistics(ICrySizer *pSizer)
{
	size_t size = 0;

	size = sizeof(*this);
	size += lstSectors.size() * sizeof(IVisArea*);
	size += m_mapBuildingMap.size()*sizeof(int)*2;	// account for key and value in int-int map
	if (m_pAutoBalance)
		size += sizeof(*m_pAutoBalance);

	size+= (m_lstWaitingToBeUpdated.size()+m_lstAlreadyUpdated.size())*sizeof(CAIObject*);
	size+= m_lstPathQueue.size()*(sizeof(PathFindRequest*)+sizeof(PathFindRequest));

	pSizer->AddObject( this, size); 

	

	{
		char str[255];
		sprintf(str,"%d AIObjects",m_Objects.size());
		SIZER_SUBCOMPONENT_NAME(pSizer,str);

		AIObjects::iterator curObj = m_Objects.begin();

		for(;curObj!=m_Objects.end();curObj++)
		{
			CAIObject *pObj = (CAIObject *) curObj->second;
			CPuppet		*pPuppet;
			CAIPlayer *pPlayer;
			CAIVehicle *pVehicle;

			size+=strlen(pObj->GetName());

			if( pObj->CanBeConvertedTo(AIOBJECT_CPUPPET, (void**)&pPuppet) )
			{
				size += pPuppet->MemStats();

//				pSizer->AddObject( pPuppet, sizeof( *pPuppet ) );
//				m_pProxy
//CFormation *m_pFormation;
			}
			else if( pObj->CanBeConvertedTo(AIOBJECT_PLAYER, (void**)&pPlayer) )
			{
				size += sizeof *pPlayer;
//				pSizer->AddObject( pPlayer, sizeof( *pPlayer ) );
			}
			else if( pObj->CanBeConvertedTo(AIOBJECT_CVEHICLE, (void**)&pVehicle) )
			{
				size += sizeof *pVehicle;
//				pSizer->AddObject( pVehicle, sizeof( *pVehicle ) );
			}
			else
			{
				size += sizeof *pObj;
//				pSizer->AddObject( pObj, sizeof( *pObj ) );
			}
		}
		pSizer->AddObject( &m_Objects, size );
	}
	
	//size = sizeof(Tri) *	m_vTriangles.size();
	pSizer->AddObject( &m_vTriangles, m_vTriangles.capacity());

	size = 0;
	for(std::vector<Vtx>::iterator vtx=m_vVertices.begin(); vtx!=m_vVertices.end(); vtx++)
	{
		size += sizeof(Vtx) + sizeof(int)*vtx->m_lstTris.size();
	}
	pSizer->AddObject( &m_vVertices, size);

	{
		SIZER_SUBCOMPONENT_NAME(pSizer,"Graphs");
		if(m_pGraph)
			pSizer->AddObject( m_pGraph, m_pGraph->MemStats());
		if(m_pHideGraph)
			pSizer->AddObject( m_pHideGraph, m_pHideGraph->MemStats());
	}

	{
		char str[255];
		sprintf(str,"%d GoalPipes",m_mapGoals.size());
		SIZER_SUBCOMPONENT_NAME(pSizer,"Goals");
		size = 0;
		GoalMap::iterator gItr=m_mapGoals.begin();
		for(;gItr!=m_mapGoals.end();gItr++)
		{
			size += (gItr->first).capacity();
			size += (gItr->second)->MemStats() + sizeof(*(gItr->second));
		}
		pSizer->AddObject( &m_mapGoals, size);
	}

	size = 0;
	DescriptorMap::iterator fItr=m_mapFormationDescriptors.begin();
	for(;fItr!=m_mapFormationDescriptors.end();fItr++)
	{
		size += (fItr->first).capacity();
		size += sizeof((fItr->second));
		size += (fItr->second).sName.capacity();
		size += (fItr->second).vOffsets.size()*sizeof((fItr->second).vOffsets[0]);
	}
	pSizer->AddObject( &m_mapFormationDescriptors, size);

	size=0;
	for(DesignerPathMap::iterator itr=m_mapDesignerPaths.begin(); itr!=m_mapDesignerPaths.end(); itr++)
	{
		size += (itr->first).capacity();
		size += (itr->second).size()*sizeof(Vec3d);
	}
	pSizer->AddObject( &m_mapDesignerPaths, size);

	size=0;
	for(DesignerPathMap::iterator itr=m_mapForbiddenAreas.begin(); itr!=m_mapForbiddenAreas.end(); itr++)
	{
		size += (itr->first).capacity();
		size += (itr->second).size()*sizeof(Vec3d);
	}
	pSizer->AddObject( &m_mapForbiddenAreas, size);

	size=0;
	for(SpecialAreaMap::iterator sit=m_mapSpecialAreas.begin(); sit!=m_mapSpecialAreas.end(); sit++)
	{
		size += (sit->first).capacity();
		size += sizeof(SpecialArea);
	}
	pSizer->AddObject( &m_mapSpecialAreas, size);


	size = m_mapSpecies.size()*(sizeof(unsigned short)+sizeof(CAIObject*));
	pSizer->AddObject( &m_mapSpecies, size );

	size = m_mapGroups.size()*(sizeof(unsigned short)+sizeof(CAIObject*));
	pSizer->AddObject( &m_mapGroups, size );

	{
		SIZER_SUBCOMPONENT_NAME(pSizer,"Triangle vertices");
		size = m_VertexList.GetSize()*sizeof(ObstacleData);
		pSizer->AddObject( &m_VertexList, size );

	}

	for(ListAIObjects::iterator curObj = m_lstDummies.begin();curObj!=m_lstDummies.end();curObj++)
	{
		CAIObject *pObj = (CAIObject *) (*curObj);
		size += sizeof *pObj;
		size+=strlen(pObj->GetName());
	}
	pSizer->AddObject( &m_lstDummies, size );



}


//
//----------------------------------------------------------------------------------
size_t CGraph::MemStats( )
{
size_t size=0;

	size = sizeof *this;
//	pSizer->AddObject( this, sizeof *this );

	//size += sizeof(GraphNode*)*m_lstTagTracker.size();
	size += m_lstTagTracker.capacity();
	//size += sizeof(GraphNode*)*m_lstMarkTracker.size();
	size += m_lstMarkTracker.capacity();
	size += sizeof(GraphNode*)*m_lstNodeStack.size();

	VectorOfLinks::iterator vi;
	ListNodes::iterator current;
	m_lstNodeStack.clear();
	m_lstNodeStack.push_front(m_pSafeFirst);
	current = m_lstNodeStack.begin();
	while( current!=m_lstNodeStack.end() )
	{
	GraphNode *pCurrentNode = (*current);
		// put all links into the node stack
		for (vi=pCurrentNode->link.begin();vi!=pCurrentNode->link.end();vi++)
		{
			GraphNode *pLink =(*vi).pLink;
			if (std::find(m_lstNodeStack.begin(),m_lstNodeStack.end(),pLink) == m_lstNodeStack.end())
				m_lstNodeStack.push_back(pLink);
		}
		current++;
	}
	

	while(!m_lstNodeStack.empty())
	{
		GraphNode *pCurrentNode = m_lstNodeStack.front();
		size += sizeof(*pCurrentNode);
		size += pCurrentNode->link.size()*sizeof(GraphLink);
		size += pCurrentNode->vertex.size()*sizeof(ObstacleData);
		m_lstNodeStack.pop_front();
	}

	size += m_vBuffer.size()*sizeof(NodeDescriptor);
//	NodeBuffer m_vBuffer;
	size += m_vLinks.size()*sizeof(int);
//		LinkBuffer m_vLinks;
	

	return size;
}

size_t CGoalPipe::MemStats()
{
	size_t size=sizeof(*this);

	GoalQueue::iterator itr = m_qGoalPipe.begin();

	for(; itr!=m_qGoalPipe.end(); itr++)
	{
		size += sizeof( QGoal );
		size += itr->name.capacity();
		size += sizeof (*(itr->pGoalOp));
		if(!itr->params.szString.empty())
			size += itr->params.szString.capacity();

/*
	QGoal *curGoal = itr;
		size += sizeof *curGoal;
		size += curGoal->name.capacity();
		size += sizeof (*curGoal->pGoalOp);
		size += strlen(curGoal->params.szString);
*/
	}
	size += m_sName.capacity();
	return size;
}

size_t CPuppet::MemStats()
{
	size_t size=sizeof(*this);

	if(m_pCurrentGoalPipe)
		size += m_pCurrentGoalPipe->MemStats();

/*
	GoalMap::iterator itr=m_mapAttacks.begin();
	for(; itr!=m_mapAttacks.end();itr++ )
	{
		size += (itr->first).capacity();
		size += sizeof(CGoalPipe *);
	}
	itr=m_mapRetreats.begin();
	for(; itr!=m_mapRetreats.end();itr++ )
	{
		size += (itr->first).capacity();
		size += sizeof(CGoalPipe *);
	}
	itr=m_mapWanders.begin();
	for(; itr!=m_mapWanders.end();itr++ )
	{
		size += (itr->first).capacity();
		size += sizeof(CGoalPipe *);
	}
	itr=m_mapIdles.begin();
	for(; itr!=m_mapIdles.end();itr++ )
	{
		size += (itr->first).capacity();
		size += sizeof(CGoalPipe *);
	}
*/
	if(m_mapVisibleAgents.size() < 1000)
	size += (sizeof(CAIObject*)+sizeof(VisionSD))*m_mapVisibleAgents.size();
	if(m_mapMemory.size()<1000)
	size += (sizeof(CAIObject*)+sizeof(MemoryRecord))*m_mapMemory.size();
	if(m_mapDevaluedPoints.size()<1000)
	size += (sizeof(CAIObject*)+sizeof(float))*m_mapDevaluedPoints.size();
	if(m_mapPotentialTargets.size()<1000)
	size += (sizeof(CAIObject*)+sizeof(float))*m_mapPotentialTargets.size();
	if(m_mapSoundEvents.size()<1000)
	size += (sizeof(CAIObject*)+sizeof(SoundSD))*m_mapSoundEvents.size();

	return size;
}
//
//----------------------------------------------------------------------------------

