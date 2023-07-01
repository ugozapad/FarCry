#include "stdafx.h"
#include <IPhysics.h>

#if !defined(LINUX)
#include <assert.h>
#endif

//#include <malloc.h>
#include <Cry_Math.h>
#include <ISystem.h>

#include <string>

#include <stdio.h>
#include <map>

//#include <Cry_Camera.h>
#include <IRenderer.h>
#include "CAISystem.h"
#include "CTriangulator.h"

#include <IEntitySystem.h>
#include <IScriptSystem.h>
#include <I3DEngine.h>
#include <ILog.h>

// important stuff
#include "graph.h"
#include <ITimer.h>
#include <IConsole.h>

#include "Puppet.h"
#include "AIVehicle.h"
#include "aiattribute.h"
#include "GoalPipe.h"
#include "GoalOp.h"
#include "AIPlayer.h"
#include <algorithm>
#include "PipeUser.h"
#include "aiautobalance.h"


#if defined(WIN32) && defined(_DEBUG) 
#include <crtdbg.h> 
#define DEBUG_NEW_NORMAL_CLIENTBLOCK(file, line) new(_NORMAL_BLOCK, file, line) 
#define new DEBUG_NEW_NORMAL_CLIENTBLOCK( __FILE__, __LINE__) 
#endif


const float	g_POINT_DIST_PRESITION = 0.0000000015f;
//const float epsilon = 0.00001f;
const float epsilon = 0.001f;



CAISystem::CAISystem(ISystem *pSystem)
: m_nNumPuppets(0),
m_pTheSkip(NULL)
{
	m_fLastPathfindTimeStart = 0.f;
	m_bRepopulateUpdateList = true;
	m_nNumRaysShot = 0;
	m_bCollectingAllowed = true;
	m_fAutoBalanceCollectingTimeout =0.f;
	m_pAutoBalance = 0;
	m_nNumBuildings = 0;
	m_nTickCount = 0;
	m_pTriangulator = 0;
	m_pSystem = pSystem;
	DEBUG_object = NULL;
	m_pGraph = 0;
	m_pHideGraph = 0;
	m_pCurrentRequest = 0;
	m_bInitialized = false;
	m_pLastHidePlace = 0;
		
	m_cvSoundPerception = 0;
	m_cvCalcIndoorGraph=NULL;
	m_cvCalcIndoorGraph=pSystem->GetIConsole()->CreateVariable("ai_createindoorgraph","0",VF_CHEAT);
	m_cvTriangulate=pSystem->GetIConsole()->CreateVariable("ai_triangulate","0",0);

	// is not cheat protected because it changes during game, depending on your settings
	m_cvAIUpdateInterval= pSystem->GetIConsole()->CreateVariable("ai_update_interval","0.1",0,
		"In seconds the amount of time between two full updates for AI  \n"
		"Usage: ai_update_interval <number>\n"
		"Default is 0.1. Number is time in seconds\n");

	// is not cheat protected because it changes during game, depending on your settings
	m_cvAIVisRaysPerFrame= pSystem->GetIConsole()->CreateVariable("ai_max_vis_rays_per_frame","100",0,
		"Maximum allowed visibility rays per frame - the rest are all assumed to succeed \n"
		"Usage: ai_max_vis_rays_per_frame <number>\n"
		"Default is 100. \n");

	m_cvRunAccuracyMultiplier = pSystem->GetIConsole()->CreateVariable("ai_autobalance","0",0,
		"Set to 1 to enable autobalancing.");
	m_cvTargetMovingAccuracyMultiplier = pSystem->GetIConsole()->CreateVariable("ai_allow_accuracy_increase","0",VF_SAVEGAME,
		"Set to 1 to enable AI accuracy increase when target is standing still.");
	m_cvLateralMovementAccuracyDecay = pSystem->GetIConsole()->CreateVariable("ai_allow_accuracy_decrease","1",VF_SAVEGAME,
		"Set to 1 to enable AI accuracy decrease when target is moving lateraly.");

	m_cvSOM_Speed = pSystem->GetIConsole()->CreateVariable("ai_SOM_SPEED","1.5",VF_SAVEGAME,
		"Multiplier for the speed of increase of the Stealth-O-Meter\n"
		"Usage: ai_SOM_SPEED 1.5\n"
		"Default is 1.5. A lower value causes the Stealth-O-Meter\n"
		"to climb more slowly when the player is seen.");	

	m_cvAllowedTimeForPathfinding = pSystem->GetIConsole()->CreateVariable("ai_pathfind_time_limit","2",0,
		"Specifies how many seconds an individual AI can hold the pathfinder blocked\n"
		"Usage: ai_pathfind_time_limit 1.5\n"
		"Default is 2. A lower value will result in more path requests that end in NOPATH -\n"
		"although the path may actually exist.");	
}


CAISystem::~CAISystem()
{
	ShutDown();
}

bool CAISystem::Init(ISystem *pSystem, const char *szLevel, const char *szMission)
{
	
	ShutDown();
	m_pAutoBalance = new CAIAutoBalance;
	f7=0;
	f8=0;
	m_nTickCount = 0;
	m_nNumBuildings = 0;
	m_mapBuildingMap.clear();
	m_pWorld = pSystem->GetIPhysicalWorld();
	m_pSystem = pSystem;
	
	pSystem->GetILog()->Log("\003 [AISYSTEM] Initialization started.");

	IConsole *pConsole = m_pSystem->GetIConsole();

	m_cvViewField  = pConsole->CreateVariable("ai_viewfield","0",VF_CHEAT,
		"Toggles the vision pyramid of the AI agent.\n"
		"Usage: ai_viewfield [0/1]\n"
		"Default is 0 (off). ai_debugdraw must be enabled before\n"
		"this tool can be used.");
	m_cvAgentStats  = pConsole->CreateVariable("ai_agentstats","1",VF_CHEAT,
		"Toggles agent statistics, such as current goalpipe, command and target.\n"
		"Usage: ai_agentstats [0/1]\n"
		"Default is 1 (on). Works with ai_debugdraw enabled.");
	m_cvDrawBalls  = pConsole->CreateVariable("ai_highlight_agent_position","0",VF_CHEAT,
		"Highlights the positions of AI agents.\n"
		"Usage: ai_highlight_agent_position [0/1]\n"
		"Default is 0 (off). When set to 1, white balls are drawn\n"
		"around the heads of the AI agents.");
	m_cvDebugDraw  = pConsole->CreateVariable("ai_debugdraw","0",VF_CHEAT,
		"Toggles the AI debugging view.\n"
		"Usage: ai_debugdraw [0/1]\n"
		"Default is 0 (off). ai_debugdraw displays AI rays and targets \n"
		"and enables the view for other AI debugging tools.");
	m_cvBadTriangleRecursionDepth  = pConsole->CreateVariable("ai_bad_triangle_recursion_depth","3",VF_CHEAT,
		"This variable is not used.\n");
	m_cvAiSystem = pConsole->CreateVariable("ai_systemupdate","1",VF_CHEAT,
		"Toggles the regular AI system update.\n"
		"Usage: ai_systemupdate [0/1]\n"
		"Default is 1 (on). Set to 0 to disable ai system updating.");
	m_cvSoundPerception = pConsole->CreateVariable("ai_soundperception","1",VF_CHEAT,
		"Toggles AI sound perception.\n"
		"Usage: ai_soundperception [0/1]\n"
		"Default is 1 (on). Used to prevent AI from hearing sounds for\n"
		"debugging purposes. Works with ai_debugdraw enabled.");
	m_cvIgnorePlayer = pConsole->CreateVariable("ai_ignoreplayer","0",VF_CHEAT,
		"Makes AI ignore the player.\n"
		"Usage: ai_ignoreplayer [0/1]\n"
		"Default is 0 (off). Set to 1 to make AI ignore the player.\n"
		"Used with ai_debugdraw enabled.");
	m_cvDrawPlayerNode	= pConsole->CreateVariable("ai_drawplayernode","0",VF_CHEAT,
		"Toggles visibility of player position on AI triangulation.\n"
		"Usage: ai_drawplayernode [0/1]\n"
		"Default is 0. Set to 1 to show the current triangle on terrain level\n"
	    "and closest vertex to player.");
	m_cvDrawPath				= pConsole->CreateVariable("ai_drawpath","0",VF_CHEAT,
		"Draws the generated paths of the AI agents.\n"
		"Usage: ai_drawpath [0/1]\n"
		"Default is 0 (off). Set to 1 to draw the AI paths.");
	m_cvAllTime		= pConsole->CreateVariable("ai_alltime","0",VF_CHEAT,
		"Displays the update times of all agents, in milliseconds.\n"
		"Usage: ai_alltime [0/1]\n"
		"Default is 0 (off). Times all agents and displays the time used updating\n"
		"each of them. The name is colour coded to represent the update time.\n"
		"	Green: less than 1 ms (ok)\n"
		"	White: 1 ms to 5 ms\n"
		"	Red: more than 5 ms\n"
		"You must enable ai_debugdraw before you can use this tool.");
	m_cvDrawHide		= pConsole->CreateVariable("ai_hidedraw","0",VF_CHEAT,
		"Toggles the triangulation display.\n"
		"Usage: ai_hidedraw [0/1]\n"
		"Default is 0 (off). Set to 1 to display the triangulation\n"
		"which the AI uses to hide (objects made 'hideable' affect\n"
		"AI triangulation). When ai_hidedraw is off, the normal\n"
		"obstacle triangulation is displayed instead.\n"
		"Used with ai_debugdraw and other AI path tools.");
	m_cvProfileGoals		= pConsole->CreateVariable("ai_profilegoals","0",VF_CHEAT,
		"Toggles timing of AI goal execution.\n"
		"Usage: ai_profilegoals [0/1]\n"
		"Default is 0 (off). Records the time used for each AI goal (like\n"
		"approach, run or pathfind) to execute. The longest execution time\n"
		"is displayed on screen. Used with ai_debugdraw enabled.");
	m_cvBeautifyPath = pConsole->CreateVariable("ai_beautify_path","1",VF_CHEAT,
		"Toggles AI optimisation of the generated path.\n"
		"Usage: ai_beautify_path [0/1]\n"
		"Default is 1 (on). Optimisation is on by default. Set to 0 to\n"
		"disable path optimisation (AI uses non-optimised path).");
	m_cvPercentSound = pConsole->CreateVariable("ai_percent_sound","0",VF_CHEAT,
		"This variable is not used.\n");
	m_cvBallSightRangeReliable= pConsole->CreateVariable("ai_ball_sightrange_reliable","0",VF_CHEAT,
		"Draws a ball representing the AI sight range.\n"
		"Usage: ai_ball_sightrange_reliable [0/1]\n"
		"Default is 0 (off). Set to 1 to draw a ball, centered on\n"
		"the agent's head, with radius equal to it's sightrange.");
	m_cvBallSightRangeTotal= pConsole->CreateVariable("ai_ball_sightrange","0",VF_CHEAT,
		"Draws a ball representing the AI sight range.\n"
		"Usage: ai_ball_sightrange [0/1]\n"
		"Default is 0 (off). Set to 1 to draw a ball, centered on\n"
		"the agent's head, with radius equal to it's sight range.");
	m_cvBallAttackRange= pConsole->CreateVariable("ai_ball_attackrange","0",VF_CHEAT,
		"Draws a ball representing the AI attack range.\n"
		"Usage: ai_ball_attackrange [0/1]\n"
		"Default is 0 (off). Set to 1 to draw a ball, centered on\n"
		"the agent's head, with radius equal to it's attack range.");
	m_cvBallSoundRange= pConsole->CreateVariable("ai_ball_soundrange","0",VF_CHEAT,
		"Draws a ball representing the AI sound range.\n"
		"Usage: ai_ball_soundrange [0/1]\n"
		"Default is 0 (off). Set to 1 to draw a ball, centered on\n"
		"the agent's head, with radius equal to it's sound range.");
	m_cvBallCommunicationRange= pConsole->CreateVariable("ai_ball_communicationrange","0",VF_CHEAT,
		"Draws a ball representing the AI communication range.\n"
		"Usage: ai_ball_communicationrange [0/1]\n"
		"Default is 0 (off). Set to 1 to draw a ball, centered on the\n"
		"agent's head, with radius equal to it's communication range.");
	m_cvOptimizeGraph = pConsole->CreateVariable("ai_optimize_graph","1",VF_CHEAT,
		"Toggles optimisation of the triangulation graph.\n"
		"Usage: ai_optimize_graph [0/1]\n"
		"Default is 1 (on). Optimises the AI triangulation graph by\n"
		"removing degenerate triangles. Set to 0 to skip optimisation.");
	m_cvUpdateProxy = pConsole->CreateVariable("ai_update_proxy","1",VF_CHEAT,
		"Toggles update of AI proxy (model).\n"
		"Usage: ai_update_proxy [0/1]\n"
		"Default is 1 (on). Updates proxy (AI representation in game)\n"
		"set to 0 to disable proxy updating.");

	m_cvDrawAnchors = pConsole->CreateVariable("ai_draw_anchors","0",VF_CHEAT,
		"Toggles anchor view for debugging AI.\n"
		"Usage: ai_draw_anchors [0/1]\n"
		"Default is 0 (off). Indicates the AI anchors by drawing\n"
		"dark blue balls at their positions.");	
	m_cvAreaInfo = pConsole->CreateVariable("ai_area_info","0",VF_CHEAT,
		"Toggles AI area information about the player's position.\n"
		"Usage: ai_area_info [0/1]\n"
		"Default is 0 (off). Shows AI area navigation information\n"
		"including the name of the building the player is in,\n"
		"the entrypoints and their links.");	
	m_cvStatsTarget = pConsole->CreateVariable("ai_stats_target","none",VF_CHEAT,
		"Focus debugging information on a specific AI\n"
		"Usage: ai_stats_target AIName\n"
		"Default is 'none'. AIName is the name of the AI\n"
		"on which to focus.");

	m_cvSampleFrequency = pConsole->CreateVariable("ai_sample_freq","10",VF_CHEAT,
		"Number of full updates before a check is performed if the target is moving lateraly \n"
		"Usage: ai_sample_freq <number>\n"
		"Default is 10. number is the number of updates\n");

	m_cvShowGroup = pConsole->CreateVariable("ai_show_group","-1",VF_CHEAT,
		"Displays the members in the same group by highlighting them with green balls \n"
		"Usage: ai_show_group <groupid>\n"
		"Default is -1. \n");




	fLastUpdateTime = m_pSystem->GetITimer()->GetCurrTime();
	m_lstWaitingToBeUpdated.reserve(100);
	m_lstAlreadyUpdated.reserve(100);
	//m_lstWaitingToBeUpdated.clear();
	//m_lstAlreadyUpdated.clear();

	 m_bInitialized = true;

	FormationDescriptor fdesc;
	fdesc.sName = "woodwalk";
	fdesc.vOffsets.push_back(Vec3d(1,-1,0));
	fdesc.vOffsets.push_back(Vec3d(-1,-1,0));
	fdesc.vOffsets.push_back(Vec3d(0.5,1,0));
	m_mapFormationDescriptors.insert(DescriptorMap::iterator::value_type(fdesc.sName, fdesc));
	fdesc.vOffsets.clear();
	fdesc.sName = "clearingline";
	fdesc.vOffsets.push_back(Vec3d(2,-3,0));
	fdesc.vOffsets.push_back(Vec3d(-2,-3,0));
	fdesc.vOffsets.push_back(Vec3d(0,-3,0));
	m_mapFormationDescriptors.insert(DescriptorMap::iterator::value_type(fdesc.sName, fdesc));
	fdesc.vOffsets.clear();
	fdesc.sName = "test_attack";
	fdesc.vOffsets.push_back(Vec3d(3,-3,0));
	fdesc.vOffsets.push_back(Vec3d(-3,-3,0));
	fdesc.vOffsets.push_back(Vec3d(0,-5,0));
	m_mapFormationDescriptors.insert(DescriptorMap::iterator::value_type(fdesc.sName, fdesc));
	fdesc.vOffsets.clear();
	fdesc.sName = "clearinglineback";
	fdesc.vOffsets.push_back(Vec3d(2,3,0));
	fdesc.vOffsets.push_back(Vec3d(-2,3,0));
	fdesc.vOffsets.push_back(Vec3d(0,3,0));
	m_mapFormationDescriptors.insert(DescriptorMap::iterator::value_type(fdesc.sName, fdesc));
	fdesc.vOffsets.clear();
	fdesc.sName = "wedge";
	fdesc.vOffsets.push_back(Vec3d(-2,0,0));
	fdesc.vOffsets.push_back(Vec3d(2,0,0));
	fdesc.vOffsets.push_back(Vec3d(0,2,0));
	fdesc.vOffsets.push_back(Vec3d(0,-2,0));
	fdesc.vOffsets.push_back(Vec3d(4,2,0));
	fdesc.vOffsets.push_back(Vec3d(-4,2,0));
	m_mapFormationDescriptors.insert(DescriptorMap::iterator::value_type(fdesc.sName, fdesc));
	fdesc.vOffsets.clear();
	fdesc.sName = "mutant_form";
	fdesc.vOffsets.push_back(Vec3d(-1,0,0));
	fdesc.vOffsets.push_back(Vec3d(1,0,0));
	fdesc.vOffsets.push_back(Vec3d(-1,-1,0));
	fdesc.vOffsets.push_back(Vec3d(1,-1,0));
	fdesc.vOffsets.push_back(Vec3d(0,-1,0));
	m_mapFormationDescriptors.insert(DescriptorMap::iterator::value_type(fdesc.sName, fdesc));
	pSystem->GetILog()->Log("\003 [AISYSTEM] Initialization finished.");
	return true;
}

void CAISystem::Update()
{
	if (!m_bInitialized)
		return;

	FUNCTION_PROFILER( m_pSystem,PROFILE_AI );

	if (m_bRepopulateUpdateList)
	{
		m_lstAlreadyUpdated.resize(0);
		m_lstWaitingToBeUpdated.resize(0);
//		m_lstAlreadyUpdated.clear();
//		m_lstWaitingToBeUpdated.clear();
		m_bRepopulateUpdateList = false;
	}

	

	if (!m_cvAiSystem->GetIVal()) 
	{
		// dry update both lists
		ListPuppets::iterator li = m_lstAlreadyUpdated.begin(),liend = m_lstAlreadyUpdated.end();
		while (li!=liend)
		{
			(*li)->SetAttentionTarget(0);
			(*li)->m_State.Reset();	
			SingleDryUpdate((*li));
			++li;
		}
		li = m_lstWaitingToBeUpdated.begin(),liend = m_lstWaitingToBeUpdated.end();
		while (li!=liend)
		{
			(*li)->SetAttentionTarget(0);
			(*li)->m_State.Reset();	
			SingleDryUpdate((*li));
			++li;
		}
		return;
	}

	// Autobalancing timer
	if (m_fAutoBalanceCollectingTimeout < 1.f)
	{
		m_fAutoBalanceCollectingTimeout+=m_pSystem->GetITimer()->GetFrameTime();
	}
	else
		m_bCollectingAllowed = false;

	//<< FIXME >> this should be removed
	if (!m_pGraph)
	{
		m_pGraph = new CGraph(this);
	}

	if (!m_pHideGraph)
	{
		m_pHideGraph = new CGraph(this);
	}
	// ------------- upto here

	if (m_lstAlreadyUpdated.empty())
	{
		AIObjects::iterator aio = m_Objects.find(AIOBJECT_PUPPET);
		for (;aio!=m_Objects.end();aio++)
		{
			if (aio->first != AIOBJECT_PUPPET)
				break;
			CPuppet *pPuppet = 0;
			if ((aio->second)->CanBeConvertedTo(AIOBJECT_CPUPPET,(void**) &pPuppet))
				m_lstAlreadyUpdated.push_back(pPuppet);
		}
		aio = m_Objects.find(AIOBJECT_VEHICLE);
		for (;aio!=m_Objects.end();aio++)
		{
			if (aio->first != AIOBJECT_VEHICLE)
				break;
			CPuppet *pPuppet = 0;
			if ((aio->second)->CanBeConvertedTo(AIOBJECT_CPUPPET,(void**) &pPuppet))
				m_lstAlreadyUpdated.push_back(pPuppet);
		}
	}


	// trace any paths that were requested
	{
	UpdatePathFinder();
	}
	m_pSystem->GetITimer()->MeasureTime("AIPathfinder");

	{
	if (!m_mapAuxSignalsFired.empty())
	{
		MapSignalStrings::iterator mss = m_mapAuxSignalsFired.begin();
		while (mss!=m_mapAuxSignalsFired.end())
		{
			(mss->second).fTimeout-= m_pSystem->GetITimer()->GetFrameTime();
			if ((mss->second).fTimeout < 0)
			{
				MapSignalStrings::iterator mss_to_erase = mss;
				++mss;
				m_mapAuxSignalsFired.erase(mss_to_erase);
			}
			else
				++mss;
		}
	}
	}



	float currTime = m_pSystem->GetITimer()->GetCurrTime();

	// check how many puppets to update this time
	int nPuppetsToUpdate = (int) (((float)m_nNumPuppets * (currTime - fLastUpdateTime))/m_cvAIUpdateInterval->GetFVal());
	m_nRaysThisUpdateFrame = 0;

	if (nPuppetsToUpdate)
	{
		fLastUpdateTime = currTime;

		if (m_lstWaitingToBeUpdated.empty())
		{
			// get all puppets in this list
			m_lstWaitingToBeUpdated.swap(m_lstAlreadyUpdated);
			// reset perception on player

			CAIPlayer *pThePlayer=0;
			if ( GetPlayer() && GetPlayer()->CanBeConvertedTo(AIOBJECT_PLAYER, (void**) &pThePlayer) )
			{
				pThePlayer->SnapshotPerception();
				pThePlayer->SetPerception(0);
			}
			f3=f6;
			f6=0;
			m_nNumRaysShot = 0;
		}

	}

	// dry update all already updated
	if (!m_lstAlreadyUpdated.empty())
	{
		ListPuppets::iterator li = m_lstAlreadyUpdated.begin(),liend = m_lstAlreadyUpdated.end();
		while (li!=liend)
		{
			SingleDryUpdate((*li));
			++li;
		}
	}

	{
	bool bRunOutOfRays = false;
	while (nPuppetsToUpdate-- && !m_lstWaitingToBeUpdated.empty() && !bRunOutOfRays)
	{
		CPuppet *pPuppet = m_lstWaitingToBeUpdated.back();
		m_lstWaitingToBeUpdated.pop_back();
		
		if (pPuppet->m_bEnabled)
		{
			f6++;
			bRunOutOfRays = SingleFullUpdate(pPuppet);
		}
		
		m_lstAlreadyUpdated.push_back(pPuppet);
	}
	}

	// now dry update all the rest that are waiting
	if (!m_lstWaitingToBeUpdated.empty())
	{
	ListPuppets::iterator li = m_lstWaitingToBeUpdated.begin(),liend =m_lstWaitingToBeUpdated.end() ;
		while (li!=liend)
		{
			SingleDryUpdate((*li));
			++li;
		}
	}

	
//	fLastUpdateTime = currTime;
	m_nTickCount++;
	

	f4 = 0;
	f1=0;	
	f5=0;
	f2=0;
	
	if (m_cvDebugDraw->GetIVal())
		f4 += m_pSystem->GetITimer()->MeasureTime("endupdate");
	 
}







void CAISystem::ShutDown()
{

	Reset();

	if (!m_lstDummies.empty())
	{
		ListAIObjects::iterator li;
		for (li=m_lstDummies.begin();li!=m_lstDummies.end();li++)
		{
			CAIObject *pObject = (*li);
			delete pObject;
		}

		m_lstDummies.clear();
	}


	if (m_pTriangulator)
	{
		delete m_pTriangulator;
		m_pTriangulator = 0;
	}
	

	if (!m_mapGoals.empty())
	{
		GoalMap::iterator gi;
		for (gi=m_mapGoals.begin();gi!=m_mapGoals.end();gi++)
			delete gi->second;

		m_mapGoals.clear();
	}

	

	if (!m_Objects.empty())
	{
		AIObjects::iterator ai;
		for (ai=m_Objects.begin();ai!=m_Objects.end();ai++)
		{
			CAIObject *pObject = ai->second;
			pObject->Release();
		}

		m_Objects.clear();
	}

	if (m_pGraph)
	{
		delete m_pGraph;
		m_pGraph = 0;
	}

	if (m_pHideGraph)
	{
		delete m_pHideGraph;
		m_pHideGraph = 0;
	}

	if (m_pAutoBalance)
	{
		delete m_pAutoBalance;
		m_pAutoBalance = 0;
	}
		
	m_mapSpecies.clear();
	m_mapGroups.clear();
	m_mapBeacons.clear();

	m_mapSpecialAreas.clear();
	m_mapDesignerPaths.clear();
	m_mapForbiddenAreas.clear();
	m_mapOcclusionPlanes.clear();

	
}


void CAISystem::DebugDraw(IRenderer *pRenderer)
{
	if (!m_bInitialized) return;


//return;
	if (!pRenderer) 
		return;

	//pRenderer->ResetToDefault();

	if (m_pAutoBalance && (m_cvDebugDraw->GetIVal()==10))
		m_pAutoBalance->DebugDraw(pRenderer);


	DebugDrawVehicle(pRenderer);
	DebugDrawAlter(pRenderer);
	DebugDrawDirections(pRenderer);

	pRenderer->SetState(0);
	pRenderer->SetColorOp(eCO_MODULATE, eCO_MODULATE, eCA_Texture|(eCA_Constant<<3), eCA_Texture|(eCA_Constant<<3));
	pRenderer->SetWhiteTexture();


	m_pGraph->m_lstTrapNodes.clear();
	if (m_cvDebugDraw->GetIVal()==100)
	{

		
		m_pGraph->ClearMarks();
		GraphNode *pCurrent = m_pGraph->GetEnclosing(GetPlayer()->GetPos());
		m_pGraph->FindTrapNodes(pCurrent,m_cvBadTriangleRecursionDepth->GetIVal());
		m_pGraph->ClearMarks();

		ListNodes::iterator li,liend = m_pGraph->m_lstTrapNodes.end();
		for (li=m_pGraph->m_lstTrapNodes.begin();li!=liend;++li)
		{
			GraphNode *pThisNode = (*li);
			VectorOfLinks::iterator vi; 
			for (vi=pThisNode->link.begin();vi!=pThisNode->link.end();vi++)
			{
				GraphLink gl = (*vi);
				GraphNode *pConnectedNode=gl.pLink; 
				if (pThisNode->nBuildingID==-1)
				{
					pRenderer->DrawLabel((*vi).vEdgeCenter,1,"%.2f",(*vi).fMaxRadius);
				}

				pRenderer->SetMaterialColor(1,0,0,1);
				pRenderer->DrawBall(pThisNode->data.m_pos,1);
				pRenderer->SetMaterialColor(1,1,1,1);
				Vec3d v0=m_VertexList.GetVertex(pThisNode->vertex[gl.nStartIndex]).vPos;
				Vec3d v1=m_VertexList.GetVertex(pThisNode->vertex[gl.nEndIndex]).vPos;

				v0.z = m_pSystem->GetI3DEngine()->GetTerrainElevation(v0.x,v0.y);
				v1.z = m_pSystem->GetI3DEngine()->GetTerrainElevation(v1.x,v1.y);
				pRenderer->DrawLine(v0, v1);
				
				pRenderer->TextToScreenColor(100,100,1,0,0,1,"BAD TRIANGLE FOUND (%.3f,%.3f,%.3f)",pThisNode->data.m_pos.x,pThisNode->data.m_pos.y,pThisNode->data.m_pos.z);
			} //vi
		}

	}



	if (m_cvDebugDraw->GetIVal()!=1) 
		return;

	if (!m_cvAiSystem->GetIVal())
	{
		pRenderer->TextToScreen(1,3,"!!!!!!!AI SYSTEM UPDATE IS OFF!!!!!!!");
		//return;
	}

	if (m_cvShowGroup->GetIVal()>=0)
	{
		pRenderer->TextToScreen(1,3,"GROUP MEMBERS: %d ",m_mapGroups.count(m_cvShowGroup->GetIVal()));
		AIObjects::iterator gri = m_mapGroups.find(m_cvShowGroup->GetIVal());
		for (;gri!=m_mapGroups.end();++gri)
		{
			if (gri->first != m_cvShowGroup->GetIVal())
				break;
			pRenderer->DrawBall((gri->second)->GetPos(),3);
			pRenderer->DrawBall((gri->second)->GetPos(),3);
		}
		return;
	}

	if (m_pCurrentRequest)
	{
		pRenderer->SetMaterialColor(0,1,0,1);
		pRenderer->DrawBall(m_pCurrentRequest->pRequester->GetPos(),4);
	}

	m_pSystem->GetITimer()->MeasureTime("AIDbgDraw0");

		CandidateMap::iterator cii= m_pGraph->m_lstVisited.begin();
	for (cii;cii!=m_pGraph->m_lstVisited.end();cii++)
		
	{
		GraphNode *pCurrentNode =cii->second;
			// draw the triangle
		if (pCurrentNode->vertex.size() >= 3)
		{
			Vec3d one,two,three;
			one = m_VertexList.GetVertex(pCurrentNode->vertex[0]).vPos; one.z=21.f;
			two = m_VertexList.GetVertex(pCurrentNode->vertex[1]).vPos; two.z=21.f;
			three = m_VertexList.GetVertex(pCurrentNode->vertex[2]).vPos; three.z=21.f;

			if (m_cvDrawPlayerNode->GetIVal() > 1)
			{
				one.z = m_cvDrawPlayerNode->GetFVal();
				two.z = m_cvDrawPlayerNode->GetFVal();
				three.z = m_cvDrawPlayerNode->GetFVal();
			}
			else
			{
				I3DEngine *pEngine = m_pSystem->GetI3DEngine();
				one.z = pEngine->GetTerrainElevation(one.x,one.y);
				two.z = pEngine->GetTerrainElevation(two.x,two.y);
				three.z = pEngine->GetTerrainElevation(three.x,three.y);
			}



			if (GetLengthSquared((one-two)) > 100)
					pRenderer->SetMaterialColor(1.0,1.0,0,1.0);
			pRenderer->DrawLine(one,two);
			pRenderer->SetMaterialColor(1.0,0,0,1.0);
			if (GetLengthSquared((two-three)) > 100)
				pRenderer->SetMaterialColor(1.0,1.0,0,1.0);
			pRenderer->DrawLine(two,three);
			pRenderer->SetMaterialColor(1.0,0,0,1.0);
			if (GetLengthSquared((three-one)) > 100)
				pRenderer->SetMaterialColor(1.0,1.0,0,1.0);
			pRenderer->DrawLine(three,one);
		};
	}





	if (!m_lstVisibilityRays.empty())
		{		ListPositions::iterator li;
			for (li=m_lstVisibilityRays.begin();li!=m_lstVisibilityRays.end();)
			{
				Vec3d pos1 = (*li);
				li++;
				if (li==m_lstVisibilityRays.end())
					break;
				Vec3d pos2 = (*li);
				li++;
				pRenderer->SetMaterialColor(1,0,1,1);
				pRenderer->DrawLine(pos1,pos2);
				pRenderer->SetMaterialColor(1,1,1,1);
			}
		}

	
	if (m_cvDrawAnchors->GetIVal())
	{
		AIObjects::iterator anchors = m_Objects.begin();
		for (;anchors != m_Objects.end();++anchors)
		{
			if (anchors->first == m_cvDrawAnchors->GetIVal())
			{
				pRenderer->SetMaterialColor(0,0,0.5,1);
				pRenderer->DrawBall((anchors->second)->GetPos(),0.7f);
				pRenderer->DrawLabel((anchors->second)->GetPos(),1,(anchors->second)->GetName());
			}
		}
	}

	if (m_cvIgnorePlayer->GetIVal())
		pRenderer->TextToScreen(0,0,"****THE PLAYER IS IGNORED FROM ALL PUPPETS****");
	
	pRenderer->TextToScreenColor(0,8,0.6f,0.6f,0.6f,1.f,"- raycasting ----------------------------------------------");
	pRenderer->TextToScreen(0,10,"SHOT %d RAYS FOR ALL PUPPETS (OVER ALL FRAMES)",m_nNumRaysShot);
	if (m_nRaysThisUpdateFrame >= m_cvAIVisRaysPerFrame->GetIVal())
		pRenderer->TextToScreenColor(0,12,1.f,0.0f,0.0f,1.f,"SHOT %d RAYS LAST FRAME (HIT MAXIMUM)",m_nRaysThisUpdateFrame);
	else
		pRenderer->TextToScreenColor(0,12,1.f,1.0f,1.0f,1.f,"SHOT %d RAYS LAST FRAME ",m_nRaysThisUpdateFrame);
	

	pRenderer->TextToScreenColor(0,16,0.6f,0.6f,0.6f,1.f,"- update timing & pathfinding -----------------------------");
	pRenderer->TextToScreen(0,18,"UPDATED %d PUPPETS IN %.3f MILLISECONDS",(int)f3,f4);
	pRenderer->TextToScreen(0,20,"CURRENTLY %d path requests waiting",m_lstPathQueue.size());
	if (m_pCurrentRequest)
	{
		pRenderer->TextToScreen(1,22,"PATHFINDER HOG:> %s, (%.3f,%.3f,%.3f)",m_pCurrentRequest->pRequester->GetName(),m_pCurrentRequest->pRequester->GetPos().x,
								m_pCurrentRequest->pRequester->GetPos().y,
								m_pCurrentRequest->pRequester->GetPos().z);
		pRenderer->SetMaterialColor(0,1,0,1);
		pRenderer->DrawBall(m_pCurrentRequest->endpos,0.5);

	}
	else
		pRenderer->TextToScreen(1,22,"PATHFINDER HOG:>");
	pRenderer->TextToScreen(0,26,"AVERAGE ITERATIONS PER GET ENCLOSING %.3f",f7);



	if (m_cvAgentStats->GetIVal())
	{

		if (!m_mapDEBUGTiming.empty())
		{
			TimingMap::iterator ti;
			int starty=0;
			float sum = 0;
			for (ti=m_mapDEBUGTiming.begin();ti!=m_mapDEBUGTiming.end();ti++)
			{
				if (ti->second > 5.f)
					pRenderer->TextToScreenColor(61,starty,1.f,0.f,0.f,1.f,"%s: %.3f",(ti->first).c_str(),ti->second);
				else if (ti->second > 2.f)
					pRenderer->TextToScreenColor(61,starty,1.f,1.f,0.f,1.f,"%s: %.3f",(ti->first).c_str(),ti->second);
				else if (ti->second > 1.f)
					pRenderer->TextToScreenColor(61,starty,1.f,1.f,1.f,1.f,"%s: %.3f",(ti->first).c_str(),ti->second);
				else
					pRenderer->TextToScreenColor(61,starty,0.f,1.f,0.f,1.f,"%s: %.3f",(ti->first).c_str(),ti->second);

				sum+=ti->second;
				starty+=2;
			}
			pRenderer->TextToScreen((float)1,(float)starty,"TOTAL: %.3f",sum);
		}

		if (!m_mapDEBUGTimingGOALS.empty())
		{
			TimingMap::iterator ti;
			int starty=0;
			float sum = 0;
			for (ti=m_mapDEBUGTimingGOALS.begin();ti!=m_mapDEBUGTimingGOALS.end();ti++)
			{
				pRenderer->TextToScreen((float)1,(float)starty,"%s: %.3f",(ti->first).c_str(),ti->second);
				sum+=ti->second;
				starty+=2;
			}
		}
	}
	
	// find if there are any puppets
	int cnt=0;
	AIObjects::iterator ai;
	if ((ai = m_Objects.find(AIOBJECT_PUPPET)) != m_Objects.end())
	{
		for (;ai!=m_Objects.end();)
		{
			if (ai->first != AIOBJECT_PUPPET)
				break;
			cnt++;
			CPuppet *pPuppet = (CPuppet*) ai->second;

			if (!pPuppet->m_bEnabled)
			{
				ai++;
				continue;
			}


			if (pPuppet->m_pReservedNavPoint)
				pRenderer->DrawBall(pPuppet->m_pReservedNavPoint->GetPos(),1);
			///pRenderer->DrawBall(pPuppet->m_vLastHidePos,1);
		
			// Draw vision field for this puppet
			//------------------------- 
			Vec3d pos,angles;
			pos = pPuppet->GetPos();
			angles = pPuppet->GetAngles();
			Vec3d lookdir;
			//lookdir=ConvertToRadAngles(lookdir);
			angles.Normalize();
			lookdir = pos+(angles*50.f); 
			//lookdir.z = pos.z;
			pRenderer->DrawLine(pos,lookdir);
			AgentParameters params;
			pPuppet->GetAgentParams(params);

			// draw last hiding place
			if (pPuppet->m_pDEBUGLastHideSpot)
			{
				pRenderer->SetMaterialColor(1,0,0,1);
				pRenderer->DrawBall(pPuppet->m_pDEBUGLastHideSpot->GetPos(),1);
			}

			if (!pPuppet->m_bLastHideResult)
			{
				Vec3d pzpos = pos;
				pzpos.z+=0.8f;
				float fColor[4] = {1,0,0,1};
				pRenderer->DrawLabelEx(pzpos,4,fColor,false,false,"?");
			}

			if (!pPuppet->m_bCanReceiveSignals)
			{
				Vec3d pzpos = pos;
				pzpos.z+=0.5f;
				pRenderer->DrawLabel(pzpos,1,"IGNORAMUS");
			}

			if (!pPuppet->m_bUpdateInternal)
			{
				Vec3d pzpos = pos;
				pzpos.z+=0.5f;
				pRenderer->DrawLabel(pzpos,1,"SENSORY_BLIND");
			}

			if (m_cvViewField->GetIVal())
			{
			
				Vec3d minv,maxv;
				Vec3d minh,maxh;
				minv = angles;
				maxv = angles;
				minh = angles;
				maxh = angles;

				minh.z = angles.z - (params.m_fHorizontalFov/2.f);
				maxh.z = angles.z + (params.m_fHorizontalFov/2.f);


				maxv=ConvertToRadAngles(maxv);
				maxv.Normalize();
				maxv = maxv*params.m_fSightRange;
				maxv += pos;
				minv=ConvertToRadAngles(minv);
				minv.Normalize();
				minv = minv*params.m_fSightRange;
				minv +=pos;

				maxh=ConvertToRadAngles(maxh);
				maxh.Normalize();
				maxh = maxh*params.m_fSightRange;
				maxh += pos;
				minh=ConvertToRadAngles(minh);
				minh.Normalize();
				minh = minh*params.m_fSightRange;
				minh +=pos;

				Vec3d sq1,sq2,sq3,sq4;
				sq1 = sq2 = minh;
				sq1.z = minv.z;
				sq2.z = maxv.z;
				sq3 = sq4 = maxh;
				sq3.z = minv.z;
				sq4.z = maxv.z;

				pRenderer->DrawLine(pos,sq1);
				pRenderer->DrawLine(pos,sq2);
				pRenderer->DrawLine(pos,sq3);
				pRenderer->DrawLine(pos,sq4);

				pRenderer->DrawLine(sq1,sq2);
				pRenderer->DrawLine(sq2,sq4);
				pRenderer->DrawLine(sq3,sq4);
				pRenderer->DrawLine(sq3,sq1);
			}

			if (m_cvAgentStats->GetIVal())
			{
				MemoryMap::iterator vi;
				pRenderer->DrawLabel(pos,1,pPuppet->GetName());


				if (pPuppet->m_pAttentionTarget)
				{
					pos.z-=0.3f;
					pRenderer->DrawLabel(pPuppet->m_pAttentionTarget->GetPos(),5,pPuppet->m_pAttentionTarget->GetName());
				}	

				if (pPuppet->GetCurrentGoalPipe())
				{
					pos.z-=0.3f;
					pRenderer->DrawLabel(pos,1,pPuppet->GetCurrentGoalPipe()->m_sName.c_str());
				}
				pos.z-=0.3f;
				pRenderer->DrawLabel(pos,1,pPuppet->m_sDEBUG_GOAL.c_str());
				pos.z-=0.3f;


				pRenderer->DrawLabel(pos,1,"%.2f",pPuppet->m_fCos);
				for (vi=pPuppet->m_mapMemory.begin();vi!=pPuppet->m_mapMemory.end();vi++)
				{
					Vec3d pos = (vi->second).vLastKnownPosition;
					pRenderer->DrawLabel(pos,1,"%s: I:%.2f",(vi->first)->GetName(),(vi->second).fIntensity);
					pos.z-=1.f;
					pos.z -=0.3f;
				}

			}

			DevaluedMap::iterator di;
			for (di=pPuppet->m_mapDevaluedPoints.begin();di!=pPuppet->m_mapDevaluedPoints.end();di++)
			{
				Vec3d pos = (di->first)->GetPos();
				
				pRenderer->DrawLabel(pos,2,"dev:%.2f",(di->second));

			}

			if (pPuppet->m_pFormation)
			{
					pPuppet->m_pFormation->Draw(pRenderer);
			}

			{
				CAIObject *pBeacon = GetBeacon(pPuppet->GetParameters().m_nGroup);
				if (pBeacon)
				{
					pRenderer->SetMaterialColor(1.f,0.f,0.f,1.f);
					pRenderer->DrawBall(pBeacon->GetPos(),0.5);
				}
			}

			//pRenderer->SetBlendMode();
			//pRenderer->EnableBlend(true);
      pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
			

			if (m_cvBallSightRangeReliable->GetIVal())
			{
				pRenderer->SetMaterialColor(1.0f,0.f,0.f,0.2f);
				pRenderer->DrawBall(pPuppet->GetPos(), pPuppet->GetParameters().m_fSightRange / 2.f);
			}

			if (m_cvBallSightRangeTotal->GetIVal())
			{
				pRenderer->SetMaterialColor(1.0f,1.f,0.f,0.2f);
				pRenderer->DrawBall(pPuppet->GetPos(), pPuppet->GetParameters().m_fSightRange);
			}

			if (m_cvBallAttackRange->GetIVal())
			{
				pRenderer->SetMaterialColor(0.0f,0.f,1.0f,0.2f);
				pRenderer->DrawBall(pPuppet->GetPos(), pPuppet->GetParameters().m_fAttackRange);
			}
						
			if (m_cvBallSoundRange->GetIVal())
			{
				pRenderer->SetMaterialColor(0.0f,1.0f,0.0f,0.2f);
				pRenderer->DrawBall(pPuppet->GetPos(), pPuppet->GetParameters().m_fSoundRange);
			}

			if (m_cvBallCommunicationRange->GetIVal())
			{
				pRenderer->SetMaterialColor(1.0f,1.0f,1.0f,0.2f);
				pRenderer->DrawBall(pPuppet->GetPos(), pPuppet->GetParameters().m_fCommRange);
			}

			//pRenderer->EnableBlend(false);
      pRenderer->SetState(0);

			if (m_cvDrawPath->GetIVal())
			{
				//if (m_pGraph)
					//m_pGraph->DrawPath(pRenderer);

				GraphNode *pPuppetNode = m_pGraph->GetEnclosing(pPuppet->GetPos(), pPuppet->m_pLastNode);
				pPuppet->m_pLastNode = pPuppetNode;

				float color = 1.0f;
				pRenderer->SetMaterialColor(0,0,color,1.0);
				pRenderer->DrawBall(pPuppet->m_vDEBUG_VECTOR,1);
				if (!pPuppet->m_lstPath.empty())	
				{
					ListPositions::iterator li,linext;
					li = pPuppet->m_lstPath.begin();
					linext = li;
					linext++;
					while (linext!=pPuppet->m_lstPath.end())
					{
						color-=0.1f;
						pRenderer->SetMaterialColor(0,0,color,1.0);
						pRenderer->DrawLine((*li), (*linext));
						li=linext;
						linext++;
					}
				}
			
			}
			if (m_cvDrawBalls->GetIVal())
				pRenderer->DrawBall(pPuppet->GetPos(),0.3f);
			ai++;
		}
		
	}

	if (GetPlayer())
	{

		if (m_cvDrawBalls->GetIVal())
			pRenderer->DrawBall(GetPlayer()->GetPos(),0.3f);


		pRenderer->TextToScreenColor(0,30,0.6f,0.6f,0.6f,1.f,"- player related ------------------------------------------");

		CAIPlayer *pPlayer = 0;
		if (GetPlayer()->CanBeConvertedTo(AIOBJECT_PLAYER,(void**) &pPlayer))
			pRenderer->TextToScreen(0,32,"STEALTH-O-METER VALUE: %.2f",pPlayer->GetPerception());

		if (GetPlayer()->m_bMoving)
			pRenderer->TextToScreen(0,34,"PLAYER IS MOVING");
		else
			pRenderer->TextToScreen(0,34,"PLAYER HAS STOPPED");


		if (m_cvAreaInfo->GetIVal())
		{
			GraphNode *pPlayerNode = m_pGraph->GetEnclosing(GetPlayer()->GetPos(),GetPlayer()->m_pLastNode);
			GetPlayer()->m_pLastNode = pPlayerNode;
			pRenderer->TextToScreenColor(0,40,0.3f,0.3f,0.3f,1.f,"- area related --------------------------------------------");
			pRenderer->TextToScreen(0,42,"You are now in building %d",pPlayerNode->nBuildingID);
			pRenderer->TextToScreen(0,44,"It has %d entrances",m_pGraph->m_mapEntrances.count(pPlayerNode->nBuildingID));
			EntranceMap::iterator ei = m_pGraph->m_mapEntrances.find(pPlayerNode->nBuildingID);
			int i=0;
			while (ei!=m_pGraph->m_mapEntrances.end())
			{
				if (ei->first != pPlayerNode->nBuildingID)
					break;
				i++;

				pRenderer->TextToScreen(2.f,46.f+2.f*i,"Entrance %d has %d links",i,(ei->second)->link.size());
				VectorOfLinks::iterator vli = (ei->second)->link.begin();
				while (vli!=(ei->second)->link.end())
				{
					pRenderer->DrawLine( (ei->second)->data.m_pos, (*vli).pLink->data.m_pos );
					vli++;
				}
				ei++;
			}
		}

	
	}
 
	if (GetPlayer() && m_cvDrawPlayerNode->GetIVal())
	{
				I3DEngine *pEngine = m_pSystem->GetI3DEngine();
				CGraph *pGraph;
				if (m_cvDrawHide->GetIVal())
					pGraph = m_pHideGraph;
				else
					pGraph = m_pGraph;

/*
				if(m_pGraph->m_DEBUG_outlineListL.size()>2)
				{
					ListPositions::iterator outlItr=m_pGraph->m_DEBUG_outlineListL.begin();
					Vec3d vOutlPrev=*outlItr;
					vOutlPrev.z = m_cvDrawPlayerNodeFlat->GetIVal();
					pRenderer->SetMaterialColor(1.0,.4,.4,1.0);
					for(++outlItr; outlItr!=m_pGraph->m_DEBUG_outlineListL.end(); ++outlItr)
					{
						Vec3d vOutl=*outlItr;
						vOutl.z = m_cvDrawPlayerNodeFlat->GetIVal();
						pRenderer->DrawLine( vOutlPrev, vOutl );
						pRenderer->DrawBall( vOutlPrev, .1 );
						vOutlPrev = vOutl;
					}
				}

				if(m_pGraph->m_DEBUG_outlineListR.size()>2)
				{
					ListPositions::iterator outlItr=m_pGraph->m_DEBUG_outlineListR.begin();
					Vec3d vOutlPrev=*outlItr;
					vOutlPrev.z = m_cvDrawPlayerNodeFlat->GetIVal();
					pRenderer->SetMaterialColor(.2f,1.0f,.2f,1.0f);

					for(++outlItr; outlItr!=m_pGraph->m_DEBUG_outlineListR.end(); ++outlItr)
					{
						Vec3d vOutl=*outlItr;
						if(vOutl.z<-100)
							pRenderer->SetMaterialColor(.2f,.2f,1.0f,1.0f);
						else
							pRenderer->SetMaterialColor(.2f,1.0f,.2f,1.0f);
						vOutl.z = m_cvDrawPlayerNodeFlat->GetIVal();
						pRenderer->DrawLine( vOutlPrev, vOutl );
						pRenderer->DrawBall( vOutlPrev, .1 );
						vOutlPrev = vOutl;
					}
				}
//*/
				pRenderer->SetMaterialColor(1.0,1,1,1.0);
				
				Vec3d ppos = GetPlayer()->GetPos();
				ppos.z-=GetPlayer()->GetEyeHeight();
				GraphNode *pPlayerNode = pGraph->GetEnclosing(ppos,GetPlayer()->m_pLastNode);
				GetPlayer()->m_pLastNode = pPlayerNode;
				VectorOfLinks::iterator vi; 
				for (vi=pPlayerNode->link.begin();vi!=pPlayerNode->link.end();vi++)
				{
					GraphNode *pConnectedNode=(*vi).pLink; 
					if (pPlayerNode->nBuildingID==-1)
					{
						if ((*vi).fMaxRadius<0) 
							pRenderer->SetMaterialColor(1,0,0,1);
						else
							pRenderer->SetMaterialColor(1,1,1,1);

						pRenderer->DrawLabel((*vi).vEdgeCenter,1,"%.2f",(*vi).fMaxRadius);
					}
					if ((*vi).fMaxRadius<0)
						pRenderer->SetMaterialColor(0,1,0,1);
					else
						pRenderer->SetMaterialColor(1,1,1,1);
					if (m_cvDrawPlayerNode->GetIVal()==1)
						pRenderer->DrawLine(pPlayerNode->data.m_pos,pConnectedNode->data.m_pos);
					else
					{
						Vec3d v0=pPlayerNode->data.m_pos;
						Vec3d v1=pConnectedNode->data.m_pos;
						v0.z=v1.z=m_cvDrawPlayerNode->GetFVal();
						pRenderer->DrawLine(v0, v1);
					}
				} //vi
				if( pPlayerNode->data.fSlope == 1111 )
					pRenderer->SetMaterialColor(.0,1.0,0,1.0);
				else
					pRenderer->SetMaterialColor(1.0,0,0,1.0);
				pRenderer->DrawBall(pPlayerNode->data.m_pos,0.1f);
				
				
				if (pPlayerNode->nBuildingID == -1)
				{
					GraphNode *pCurrentNode =pPlayerNode;
					// draw the triangle
					if (pCurrentNode->vertex.size() >= 3)
					{
						Vec3d one,two,three;
						one = m_VertexList.GetVertex(pCurrentNode->vertex[0]).vPos; //one.z+=1.f;
						two = m_VertexList.GetVertex(pCurrentNode->vertex[1]).vPos; //two.z+=1.f;
						three = m_VertexList.GetVertex(pCurrentNode->vertex[2]).vPos;// three.z+=1.f;
						if (m_cvDrawPlayerNode->GetIVal()>1)
						{
							one.z = m_cvDrawPlayerNode->GetFVal();
							two.z = m_cvDrawPlayerNode->GetFVal();
							three.z = m_cvDrawPlayerNode->GetFVal();
						}
						else
						{
							one.z = pEngine->GetTerrainElevation(one.x,one.y);
							two.z = pEngine->GetTerrainElevation(two.x,two.y);
							three.z = pEngine->GetTerrainElevation(three.x,three.y);
						}

						if (GetLengthSquared((one-two)) > 100)
								pRenderer->SetMaterialColor(1.0,1.0,0,1.0);
						pRenderer->DrawLine(one,two);
						pRenderer->SetMaterialColor(1.0,0,0,1.0);
						if (GetLengthSquared((two-three)) > 100)
							pRenderer->SetMaterialColor(1.0,1.0,0,1.0);
						pRenderer->DrawLine(two,three);
						pRenderer->SetMaterialColor(1.0,0,0,1.0);
						if (GetLengthSquared((three-one)) > 100)
							pRenderer->SetMaterialColor(1.0,1.0,0,1.0);
						pRenderer->DrawLine(three,one);
					}
					else if (!pCurrentNode->vertex.empty())
					{
						ObstacleIndexVector::iterator oi;
						  for (oi=pCurrentNode->vertex.begin();oi!=pCurrentNode->vertex.end();oi++)
						{
							pRenderer->SetMaterialColor(0.5f,0.5f,0.5f,1.f);
							pRenderer->DrawBall(m_VertexList.GetVertex((*oi)).vPos,0.3f);
						}
					}

				}
				else
				{
					GraphNode *pCurrentNode =pPlayerNode;

					for(int i=0;i<(int)pCurrentNode->vertex.size();i++)
					{
						pRenderer->SetMaterialColor(0,1,1,1.0);
						pRenderer->DrawLine(pCurrentNode->data.m_pos,m_VertexList.GetVertex(pCurrentNode->vertex[i]).vPos);
						pRenderer->DrawBall(m_VertexList.GetVertex(pCurrentNode->vertex[i]).vPos, 0.3f);
					}

				}

				// draw forbidden areas
				if (!m_mapForbiddenAreas.empty())
				{
					DesignerPathMap::iterator fi;
					for (fi=m_mapForbiddenAreas.begin();fi!=m_mapForbiddenAreas.end();fi++)
					{
						ListPositions lst = fi->second;						
						pRenderer->SetMaterialColor(0,1,0,1.0);
						ListPositions::iterator li,linext;
						for (li=lst.begin();li!=lst.end();li++)
						{
							linext = li;
							linext++;
							if (linext==lst.end())
								linext = lst.begin();
						
							if (m_cvDrawPlayerNode->GetIVal()>1)
							{
								Vec3d v0=(*li);
								Vec3d v1=(*linext);
								v0.z=v1.z=m_cvDrawPlayerNode->GetFVal();
								pRenderer->DrawLine(v0,v1);
							}
							else
								pRenderer->DrawLine((*li),(*linext));
						}
					}
				}
	}
	

	if (stricmp(m_cvStatsTarget->GetString(),"none"))
	{
		CAIObject *pTargetObject = GetAIObjectByName(m_cvStatsTarget->GetString());
		if (pTargetObject)
		{
			CPuppet *pTargetPuppet = 0;
			if (pTargetObject->CanBeConvertedTo(AIOBJECT_CPUPPET,(void**)&pTargetPuppet))
			{
				pRenderer->TextToScreenColor(0,60,0.3f,0.3f,0.3f,1.f,"- observing %s --------------------------------------------",pTargetPuppet->GetName());

				CGoalPipe *pPipe = pTargetPuppet->GetCurrentGoalPipe();
				if (pPipe)
				{
					pRenderer->TextToScreen(0,62,"Goalpipe: %s",pTargetPuppet->GetCurrentGoalPipe()->m_sName.c_str());
					int i=0;
					CGoalPipe *pSubPipe = pPipe;
					while (pSubPipe->IsInSubpipe())
					{
						pSubPipe = pSubPipe->GetSubpipe();
						char str[1024];
						memset(str,32,sizeof(char)*1024);
						str[i] = '+';
						strcpy(&str[i+1],pSubPipe->m_sName.c_str());
						pRenderer->TextToScreen(0.f,64.f+2.f*i,str);
						i++;
					}
				}


				switch (pTargetPuppet->m_State.bodystate)
				{
					case BODYPOS_STAND:
						pRenderer->TextToScreen(0,72,"STANCE: STAND COMBAT");
					break;
					case BODYPOS_CROUCH:
						pRenderer->TextToScreen(0,72,"STANCE: CROUCH");
					break;
					case BODYPOS_PRONE:
						pRenderer->TextToScreen(0,72,"STANCE: PRONE");
					break;
					case BODYPOS_RELAX:
						pRenderer->TextToScreen(0,72,"STANCE: STAND RELAXED");
					break;
					case BODYPOS_STEALTH:
						pRenderer->TextToScreen(0,72,"STANCE: STEALTH");
					break;
					
				}

				if (pTargetPuppet->m_State.aimLook) 
					pRenderer->TextToScreen(0,70,"AIMLOOK");

				if (pTargetPuppet->m_State.aimLook) 
					pRenderer->TextToScreen(0,68,"RESPONSIVENESS (NEW/OLD): (%.2f/%.2f)",pTargetPuppet->GetParameters().m_fResponsiveness,pTargetPuppet->GetParameters().m_fResponsiveness*7.5f/400.f);

				
				if (pTargetPuppet->GetAttentionTarget())
				{
					pRenderer->TextToScreen(0,74,"Attention target name: %s",pTargetPuppet->GetAttentionTarget()->GetName());
				}
				else
					pRenderer->TextToScreen(0,74,"No attention target");

				pRenderer->TextToScreen(0,76,"Angles now at: (%.3f,%.3f,%.3f)",pTargetPuppet->GetAngles().x,pTargetPuppet->GetAngles().y,pTargetPuppet->GetAngles().z);

				if (!pTargetPuppet->m_State.vSignals.empty())
				{
					int i=0;
					
					pRenderer->TextToScreen(0,78,"Pending signals:");
					std::vector<AISIGNAL>::iterator sig,iend=pTargetPuppet->m_State.vSignals.end();
					for (sig=pTargetPuppet->m_State.vSignals.begin();sig!=iend;++sig,i++)
						pRenderer->TextToScreen(0.f,80.f+2.f*i,(*sig).strText);
				}

				pRenderer->TextToScreen(50,62,"GR.MEMBERS:%d GROUPID:%d",m_mapGroups.count(pTargetPuppet->GetParameters().m_nGroup),pTargetPuppet->GetParameters().m_nGroup);

				if (pTargetPuppet->GetProxy())
					pTargetPuppet->GetProxy()->DebugDraw(pRenderer);
			}
		}
	}


  pRenderer->SetState(GS_DEPTHWRITE);

	m_pSystem->GetITimer()->MeasureTime("AIDbgDraw1");
}

void CAISystem::DebugDrawDirections(IRenderer *pRenderer)
{
	if (m_cvDebugDraw->GetIVal()!=4) return;

	if (!pRenderer) return;
	//pRenderer->ResetToDefault();
	//pRenderer->EnableDepthWrites(false);
  pRenderer->SetState(0);


	// find if there are any puppets
	int cnt=0;
	AIObjects::iterator ai;
	if ((ai = m_Objects.find(AIOBJECT_PUPPET)) != m_Objects.end())
	{
		for (;ai!=m_Objects.end();)
		{
			if (ai->first != AIOBJECT_PUPPET)
				break;
			cnt++;
			CPuppet *pPuppet = (CPuppet*) ai->second;
		
			// Draw vision field for this puppet
			//------------------------- 
			Vec3d pos,angles;
			pos = pPuppet->GetPos();
			angles = pPuppet->GetAngles();
			Vec3d lookdir = angles;
			lookdir=ConvertToRadAngles(lookdir);

			lookdir = pos + (  GetNormalized(pPuppet->m_State.vFireDir) * 5.f); 

			//lookdir.z = pos.z;
			pRenderer->DrawLine(pos,lookdir);

			ai++;
//			break;
		}
		
	}


	//pRenderer->EnableDepthWrites(true);
  pRenderer->SetState(GS_DEPTHWRITE);
}




void CAISystem::DebugDrawAlter(IRenderer *pRenderer)
{
	if (m_cvDebugDraw->GetIVal()!=3) return;

	if (!pRenderer) return;
	//pRenderer->ResetToDefault();
	//pRenderer->EnableDepthWrites(false);
  pRenderer->SetState(0);
	
	// find if there are any puppets
	int cnt=0;
	AIObjects::iterator ai;
	if ((ai = m_Objects.find(AIOBJECT_PUPPET)) != m_Objects.end())
	{
//		while (ai->first == AIOBJECT_PUPPET)
		{
			cnt++;
			CPuppet *pPuppet = (CPuppet*) ai->second;

			if (!pPuppet->m_bEnabled)
			{
				ai++;
				return;
			}

		
			// Draw vision field for this puppet
			//------------------------- 
			Vec3d pos,angles;
			pos = pPuppet->GetPos();
			angles = pPuppet->GetAngles();
			Vec3d lookdir = angles;
			lookdir=ConvertToRadAngles(lookdir);
			lookdir = pos+(pPuppet->m_State.vFireDir*50.f); 
			//lookdir.z = pos.z;
			pRenderer->DrawLine(pos,lookdir);
			AgentParameters params;
			pPuppet->GetAgentParams(params);

			// draw last hiding place
			if (pPuppet->m_pDEBUGLastHideSpot)
			{
				pRenderer->SetMaterialColor(1,0,0,1);
				pRenderer->DrawBall(pPuppet->m_pDEBUGLastHideSpot->GetPos(),1);
			}

			if (m_cvViewField->GetIVal())
			{
			
				Vec3d minv,maxv;
				Vec3d minh,maxh;
				minv = angles;
				maxv = angles;
				minh = angles;
				maxh = angles;

				minh.z = angles.z - (params.m_fHorizontalFov/2.f);
				maxh.z = angles.z + (params.m_fHorizontalFov/2.f);


				maxv=ConvertToRadAngles(maxv);
				maxv.Normalize();
				maxv = maxv*params.m_fSightRange;
				maxv += pos;
				minv=ConvertToRadAngles(minv);
				minv.Normalize();
				minv = minv*params.m_fSightRange;
				minv +=pos;

				maxh=ConvertToRadAngles(maxh);
				maxh.Normalize();
				maxh = maxh*params.m_fSightRange;
				maxh += pos;
				minh=ConvertToRadAngles(minh);
				minh.Normalize();
				minh = minh*params.m_fSightRange;
				minh +=pos;

				Vec3d sq1,sq2,sq3,sq4;
				sq1 = sq2 = minh;
				sq1.z = minv.z;
				sq2.z = maxv.z;
				sq3 = sq4 = maxh;
				sq3.z = minv.z;
				sq4.z = maxv.z;

				pRenderer->DrawLine(pos,sq1);
				pRenderer->DrawLine(pos,sq2);
				pRenderer->DrawLine(pos,sq3);
				pRenderer->DrawLine(pos,sq4);

				pRenderer->DrawLine(sq1,sq2);
				pRenderer->DrawLine(sq2,sq4);
				pRenderer->DrawLine(sq3,sq4);
				pRenderer->DrawLine(sq3,sq1);
			}

			if (m_cvAgentStats->GetIVal())
			{
				MemoryMap::iterator vi;
				//if (pPuppet->m_bVisible)
//					pRenderer->DrawLabel(pos,1,pPuppet->GetName());

				if (pPuppet->m_pAttentionTarget)
				{
					pos.z-=0.3f;
			//		if (pPuppet->m_pAttentionTarget->GetType() == AIOBJECT_DUMMY)
			//			pRenderer->DrawLabel(pos,1,"DUMMY!!");
					pRenderer->DrawLabel(pPuppet->m_pAttentionTarget->GetPos(),5,pPuppet->m_pAttentionTarget->GetName());
				}	

				if (pPuppet->GetCurrentGoalPipe())
				{
					pos.z-=0.3f;
//					pRenderer->DrawLabel(pos,1,pPuppet->GetCurrentGoalPipe()->m_sName.c_str());

pRenderer->TextToScreen(30,27,"pipe >> %s",pPuppet->GetCurrentGoalPipe()->m_sName.c_str());
				}
pRenderer->TextToScreen(30,37,"goal >> %s",pPuppet->m_sDEBUG_GOAL.c_str());
			//	pRenderer->DrawLabel(pos,1,pPuppet->GetName());

		
			//	m_pGraph->GetEnclosing(pPuppet->GetPos());
			//	pRenderer->DrawBall(m_pGraph->GetCurrent()->data.m_pos,0.5);
			}

			DevaluedMap::iterator di;
			for (di=pPuppet->m_mapDevaluedPoints.begin();di!=pPuppet->m_mapDevaluedPoints.end();di++)
			{
				Vec3d pos = (di->first)->GetPos();
				
				pRenderer->DrawLabel(pos,2,"dev:%.2f",(di->second));

			}

			if (pPuppet->m_pFormation)
			{
					pPuppet->m_pFormation->Draw(pRenderer);
			}

			{
				CAIObject *pBeacon = GetBeacon(pPuppet->GetParameters().m_nGroup);
				if (pBeacon)
				{
					pRenderer->SetMaterialColor(1.f,0.f,0.f,1.f);
					pRenderer->DrawBall(pBeacon->GetPos(),0.5);
				}
			}

			//pRenderer->SetBlendMode();
			//pRenderer->EnableBlend(true);
      pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
			

			if (m_cvBallSightRangeReliable->GetIVal())
			{
				pRenderer->SetMaterialColor(1.0f,0.f,0.f,0.2f);
				pRenderer->DrawBall(pPuppet->GetPos(), pPuppet->GetParameters().m_fSightRange / 2.f);
			}

			if (m_cvBallSightRangeTotal->GetIVal())
			{
				pRenderer->SetMaterialColor(1.0f,1.f,0.f,0.2f);
				pRenderer->DrawBall(pPuppet->GetPos(), pPuppet->GetParameters().m_fSightRange);
			}

			if (m_cvBallAttackRange->GetIVal())
			{
				pRenderer->SetMaterialColor(0.0f,0.f,1.0f,0.2f);
				pRenderer->DrawBall(pPuppet->GetPos(), pPuppet->GetParameters().m_fAttackRange);
			}
						
			if (m_cvBallSoundRange->GetIVal())
			{
				pRenderer->SetMaterialColor(0.0f,1.0f,0.0f,0.2f);
				pRenderer->DrawBall(pPuppet->GetPos(), pPuppet->GetParameters().m_fSoundRange);
			}

			if (m_cvBallCommunicationRange->GetIVal())
			{
				pRenderer->SetMaterialColor(1.0f,1.0f,1.0f,0.2f);
				pRenderer->DrawBall(pPuppet->GetPos(), pPuppet->GetParameters().m_fCommRange);
			}

			//pRenderer->EnableBlend(false);
      pRenderer->SetState(0);


			if (m_cvDrawPath->GetIVal())
			{

				GraphNode *pPuppetNode = m_pGraph->GetEnclosing(pPuppet->GetPos(), pPuppet->m_pLastNode);
				pPuppet->m_pLastNode = pPuppetNode;

				pRenderer->SetMaterialColor(0,1.0f,0,1.0);
				pRenderer->DrawBall(pPuppetNode->data.m_pos,1);
				pRenderer->SetMaterialColor(0,0,1.0,1.0);
				pRenderer->DrawBall(pPuppet->m_vDEBUG_VECTOR,1);
				if (!pPuppet->m_lstPath.empty())	
				{
					ListPositions::iterator li,linext;
					li = pPuppet->m_lstPath.begin();
					linext = li;
					linext++;
					while (linext!=pPuppet->m_lstPath.end())
					{
						pRenderer->DrawLine((*li), (*linext));
						li=linext;
						linext++;
					}
				}
			
			}
			ai++;
		}
		
	}

 
	if (m_pLastHidePlace)
	{
		Vec3d oo = m_pLastHidePlace->GetPos();;
		
		pRenderer->DrawBall(oo,0.3f);
	}
	
	//pRenderer->EnableDepthWrites(true);
  pRenderer->SetState(GS_DEPTHWRITE);

	m_pSystem->GetITimer()->MeasureTime("AIDbgDraw1");
}


void CAISystem::DebugDrawVehicle(IRenderer *pRenderer)
{
	if (m_cvDebugDraw->GetIVal()!=2) return;

	if (!pRenderer) return;
	//pRenderer->ResetToDefault();
  pRenderer->SetState(GS_DEPTHWRITE);

//	pRenderer->TextToScreen(30,17,"SHOT %d RAYS IN %.3f MILLISECONDS",(int)f2,f1);

	
	// find if there are any puppets
	int cnt=0;
	AIObjects::iterator ai;
	if ((ai = m_Objects.find(AIOBJECT_VEHICLE)) != m_Objects.end())
	{
		for (;ai!=m_Objects.end();)
		{
			if (ai->first != AIOBJECT_VEHICLE)
				break;
			cnt++;
			CAIVehicle *pVehicle = (CAIVehicle*) ai->second;

			if (!pVehicle->m_bEnabled)
			{
				ai++;
				continue;
			}

			// Draw entity direction
			//------------------------- 
			Vec3d pos,angles;
			pos = pVehicle->GetPos();
			pos.z+=1;
			angles = pVehicle->GetAngles();
			Vec3d lookdir = angles;
			lookdir = pVehicle->m_State.vMoveDir; 
			lookdir.Normalize();
			lookdir = lookdir*10 + pos;
			lookdir.z = pos.z;
			pRenderer->DrawLine(pos,lookdir);


			// Draw velocity
			//------------------------- 
			Vec3d curVel;
		// Create a new status object.  The fields are initialized for us
			pe_status_dynamics status;
			// Get a pointer to the physics engine
			IPhysicalEntity *physEnt = pVehicle->GetProxy()->GetPhysics();
			// Get new player status from physics engine
			if (physEnt && physEnt->GetStatus(&status))
			{
				// Get our current velocity, default will be (0,0,0)
				curVel = (Vec3d)status.v;
				curVel = curVel*3 + pos;
				curVel.z = pos.z;
				pRenderer->SetMaterialColor(1.0f,.3f,1.0f,.50f);
				pRenderer->DrawLine(pos,curVel);
			}

			// Draw controlling  
			//-------------------------------
			if( pVehicle->m_State.DEBUG_controlDirection==1 )
			{
				lookdir.x = status.v.y; 
				lookdir.y = -status.v.x; 
			}
			else if( pVehicle->m_State.DEBUG_controlDirection==2 )
			{
				lookdir.x = -status.v.y; 
				lookdir.y = status.v.x; 
			}
			if(pVehicle->m_State.DEBUG_controlDirection )
			{
				lookdir = lookdir*2 + pos;
				lookdir.z = pos.z;
				pRenderer->SetMaterialColor(.10f,.3f,1.0f,1);
				pRenderer->DrawLine(pos,lookdir);
			}


			// draw threat
			if(pVehicle->m_Threat)
			{
//				pRenderer->SetMaterialColor(1,1,0,1);
				pRenderer->SetMaterialColor(1.0f,.3f,0.0f,1.0f);
				pRenderer->DrawBall(pVehicle->m_Threat->GetPos(),1);

				pRenderer->TextToScreen(30,17,"--- THREATENED ---");
			}
			// draw atention target
			if(pVehicle->m_pAttentionTarget)
			{
//				pRenderer->SetMaterialColor(1,1,0,0);
				pRenderer->SetMaterialColor(0.0f,1.0f,.7f,1.0f);
				pRenderer->DrawBall(pVehicle->m_pAttentionTarget->GetPos(),1);
			}

			// draw Target
			{
//				pRenderer->SetMaterialColor(1,1,0,1);
				pRenderer->SetMaterialColor(1.0f,.3f,1.0f,.2f);
				pRenderer->DrawBall(pVehicle->m_State.vTargetPos+Vec3d(0,0,1),1);
			}

			if (pVehicle->GetCurrentGoalPipe())
			{
pRenderer->TextToScreen(30,27,"pipe >> %s",pVehicle->GetCurrentGoalPipe()->m_sName.c_str());
			}
pRenderer->TextToScreen(30,37,"goal >> %s",pVehicle->m_sDEBUG_GOAL.c_str());


			if (m_cvDrawPath->GetIVal())
			{
				//if (m_pGraph)
					//m_pGraph->DrawPath(pRenderer);

				GraphNode *pPuppetNode = m_pGraph->GetEnclosing(pVehicle->GetPos(),pVehicle->m_pLastNode);
				pVehicle->m_pLastNode = pPuppetNode;

				//pRenderer->SetMaterialColor(0,1.0f,0,1.0);
				//pRenderer->DrawBall(pPuppetNode->data.m_pos,1);
				float color = 1.0f;
				pRenderer->SetMaterialColor(0,0,color,1.0);
//				pRenderer->DrawBall(pPuppet->m_vDEBUG_VECTOR,1);
				if (!pVehicle->m_lstPath.empty())	
				{
					ListPositions::iterator li,linext;
					li = pVehicle->m_lstPath.begin();
					linext = li;
					linext++;
					while (linext!=pVehicle->m_lstPath.end())
					{
						color-=0.1f;
						pRenderer->SetMaterialColor(0,0,color,1.0);
						pRenderer->DrawLine((*li), (*linext));
						li=linext;
						linext++;
					}
				}
			
			}


			if (m_cvDrawPlayerNode->GetIVal())
			{
				CGraph *pGraph;
				if (m_cvDrawHide->GetIVal())
					pGraph = m_pHideGraph;
				else
					pGraph = m_pGraph;

				pRenderer->SetMaterialColor(1.0,1,1,1.0);
				pGraph->GetEnclosing(pVehicle->GetPos(),pVehicle->m_pLastNode);
				pVehicle->m_pLastNode = pGraph->GetCurrent();
				VectorOfLinks::iterator vi; 
				for (vi=pGraph->GetCurrent()->link.begin();vi!=pGraph->GetCurrent()->link.end();vi++)
				{
					GraphNode *pConnectedNode=(*vi).pLink;
						pRenderer->DrawLine(pGraph->GetCurrent()->data.m_pos,pConnectedNode->data.m_pos);
				} //vi
				pRenderer->SetMaterialColor(1.0,0,0,1.0);
				pRenderer->DrawBall(pGraph->GetCurrent()->data.m_pos,0.5);
				if (pGraph->GetCurrent()->nBuildingID == -1)
				{
					GraphNode *pCurrentNode =pGraph->GetCurrent();
					// draw the triangle
					if (pCurrentNode->vertex.size() >= 3)
					{
						Vec3d one,two,three;
						one = m_VertexList.GetVertex(pCurrentNode->vertex[0]).vPos; one.z=21.f;
						two = m_VertexList.GetVertex(pCurrentNode->vertex[1]).vPos; two.z=21.f;
						three = m_VertexList.GetVertex(pCurrentNode->vertex[2]).vPos; three.z=21.f;

						if (m_cvDrawPlayerNode->GetIVal() > 1)
						{
							one.z = m_cvDrawPlayerNode->GetFVal();
							two.z = m_cvDrawPlayerNode->GetFVal();
							three.z = m_cvDrawPlayerNode->GetFVal();
						}
						else
						{
							I3DEngine *pEngine = m_pSystem->GetI3DEngine();
							one.z = pEngine->GetTerrainElevation(one.x,one.y);
							two.z = pEngine->GetTerrainElevation(two.x,two.y);
							three.z = pEngine->GetTerrainElevation(three.x,three.y);
						}



						if (GetLengthSquared((one-two)) > 100)
								pRenderer->SetMaterialColor(1.0,1.0,0,1.0);
						pRenderer->DrawLine(one,two);
						pRenderer->SetMaterialColor(1.0,0,0,1.0);
						if (GetLengthSquared((two-three)) > 100)
							pRenderer->SetMaterialColor(1.0,1.0,0,1.0);
						pRenderer->DrawLine(two,three);
						pRenderer->SetMaterialColor(1.0,0,0,1.0);
						if (GetLengthSquared((three-one)) > 100)
							pRenderer->SetMaterialColor(1.0,1.0,0,1.0);
						pRenderer->DrawLine(three,one);
					}
					else if (!pCurrentNode->vertex.empty())
					{
						ObstacleIndexVector::iterator oi;
					  for (oi=pCurrentNode->vertex.begin();oi!=pCurrentNode->vertex.end();oi++)
						{
							pRenderer->SetMaterialColor(0.5f,0.5f,0.5f,1.f);
							 pRenderer->DrawBall(m_VertexList.GetVertex((*oi)).vPos,0.3f);
						}
					}

				}
				else
				{
					GraphNode *pCurrentNode =pGraph->GetCurrent();

					for(int i=0;i<(int)pCurrentNode->vertex.size();i++)
					{
						pRenderer->SetMaterialColor(0,1,1,1.0);
						pRenderer->DrawBall(m_VertexList.GetVertex(pCurrentNode->vertex[i]).vPos, 0.3f);
					}
				}
			}

			ai++;
		}
	}

	
}



IPhysicalWorld * CAISystem::GetPhysicalWorld()
{
	return m_pWorld;
}

CGraph * CAISystem::GetGraph()
{
	return m_pGraph;
}

IAIObject *CAISystem::CreateAIObject(unsigned short type, void *pAssociation)
{
	if (!m_bInitialized)
	{
		m_pSystem->GetILog()->LogError("CAISystem::CreateAIObject called on an uninitialized AI system.");
		return 0;
	}

	CAIObject *pObject;
	

	switch (type)
	{
		case AIOBJECT_PUPPET:
				pObject = new CPuppet();
				break;

		case AIOBJECT_ATTRIBUTE:
				pObject = new CAIAttribute();
				pObject->SetEyeHeight(0);
				break;
			
		case AIOBJECT_BOAT:
				type = AIOBJECT_VEHICLE;
				pObject = new CAIVehicle();
				((CAIVehicle*)pObject)->SetVehicleType( AIVEHICLE_BOAT );
//				pObject->m_bNeedsPathOutdoor = false;
				break;
		case AIOBJECT_CAR:
				type = AIOBJECT_VEHICLE;
				pObject = new CAIVehicle();
				((CAIVehicle*)pObject)->SetVehicleType( AIVEHICLE_CAR );
				pObject->m_bNeedsPathIndoor = false;
				break;
		case AIOBJECT_HELICOPTER:
				type = AIOBJECT_VEHICLE;
				pObject = new CAIVehicle();
				((CAIVehicle*)pObject)->SetVehicleType( AIVEHICLE_HELICOPTER );
				pObject->m_bNeedsPathOutdoor = false;
				pObject->m_bNeedsPathIndoor = false;
				break;
		case AIOBJECT_PLAYER:
			{
				pObject = new CAIPlayer(); // just a dummy for the player	
				pObject->m_bEnabled = true;
			}
				break;
		case AIOBJECT_WAYPOINT:
		case AIOBJECT_HIDEPOINT:
		case AIOBJECT_SNDSUPRESSOR:
				pObject = new CAIObject();
				pObject->SetEyeHeight(0);
				break;
		default:
				// try to create an object of user defined type
				pObject = new CAIObject();
				pObject->SetEyeHeight(0);
				break;
	}

	pObject->SetType(type);
	pObject->SetAISystem(this);
	pObject->SetAssociation(pAssociation);
	// insert object into map under key type
	// this is a multimap
	m_Objects.insert(AIObjects::iterator::value_type(type,pObject));
	int nNumPuppets = m_Objects.count(AIOBJECT_PUPPET);
	nNumPuppets += m_Objects.count(AIOBJECT_VEHICLE);

	if (nNumPuppets>m_nNumPuppets)
		m_bRepopulateUpdateList = true;
	m_nNumPuppets = nNumPuppets;
	return pObject;
}

IGoalPipe *CAISystem::CreateGoalPipe(const char *pName)
{
	if (m_mapGoals.find(pName) == m_mapGoals.end())
	{
		CGoalPipe *pNewPipe = new CGoalPipe(pName,this);
		m_mapGoals.insert(GoalMap::iterator::value_type(pName,pNewPipe));

		return pNewPipe;
	}
	else
	{
    // destroy old and create new
		GoalMap::iterator gi =m_mapGoals.find(pName);
		delete (gi->second);
		m_mapGoals.erase(gi);

		// re-enter
		return CreateGoalPipe(pName);
	}
}

IGoalPipe *CAISystem::OpenGoalPipe(const char *pName)
{
	GoalMap::iterator gi;

	if ((gi = m_mapGoals.find(pName)) != m_mapGoals.end())
	{
		return gi->second;
	}
	else
		return 0;
}


CGoalPipe *CAISystem::IsGoalPipe(const string &name)
{
	GoalMap::iterator gi;

	if ((gi = m_mapGoals.find(name)) != m_mapGoals.end())
		return (gi->second)->Clone();
	
	return 0;
}

CAIObject * CAISystem::GetPlayer()
{
	AIObjects::iterator ai;
	if ((ai = m_Objects.find(AIOBJECT_PLAYER)) != m_Objects.end())
		return ai->second;
	return 0;
}

IAIObject * CAISystem::GetAIObjectByName(unsigned short type, const char *pName)
{
	AIObjects::iterator ai;

	if (m_Objects.empty()) return 0;

	if (!type) 
		return (IAIObject *) GetAIObjectByName(pName);


	if ((ai = m_Objects.find(type)) != m_Objects.end())

	for (;ai!=m_Objects.end();)
	{
		if (ai->first != type)
			break;
		CAIObject *pObject = ai->second;

		if (string(pName) == string(pObject->GetName()))
			return (IAIObject *) pObject;
		ai++;
	}

	return 0;

}


void CAISystem::PathFind(const PathFindRequest &request)
{

	// all previous paths for this requester should be cancelled
	if (!m_lstPathQueue.empty())
	{
		PathQueue::iterator iend = m_lstPathQueue.end();
		for (PathQueue::iterator pi=m_lstPathQueue.begin();pi!=iend;)
		{
			if ( (*pi)->pRequester == request.pRequester)
				pi= m_lstPathQueue.erase(pi);
			else
				++pi;
		}
	}


	PathFindRequest *pNewRequest = new PathFindRequest;
	pNewRequest->pStart = request.pStart;
	pNewRequest->pEnd = request.pEnd;
	pNewRequest->pRequester = request.pRequester;
	pNewRequest->bSuccess = false;
	pNewRequest->endpos = request.endpos;
	pNewRequest->startpos = request.startpos;
	pNewRequest->m_nSelectedHeuristic = request.m_nSelectedHeuristic;
	if( pNewRequest->pRequester->GetType() ==  AIOBJECT_VEHICLE)
	{
		pNewRequest->m_nSelectedHeuristic = AIHEURISTIC_VEHICLE;
		m_lstPathQueue.push_front(pNewRequest);
	}
	else
		m_lstPathQueue.push_back(pNewRequest);
}

void CAISystem::UpdatePathFinder()
{
	FUNCTION_PROFILER( m_pSystem,PROFILE_AI );

	int nIterationsLeft = PATHFINDER_ITERATIONS;
	
	if (!m_pCurrentRequest)
		m_nPathfinderResult = PATHFINDER_POPNEWREQUEST;

	if (m_pCurrentRequest)
	{
		float fCurrentTime = m_pSystem->GetITimer()->GetCurrTime();
		if ((fCurrentTime - m_fLastPathfindTimeStart)>m_cvAllowedTimeForPathfinding->GetFVal()) 
		{
			m_nPathfinderResult = PATHFINDER_NOPATH;
			m_pGraph->Reset();
		}
	}
 
	while (nIterationsLeft>0)
	{
		switch (m_nPathfinderResult)
		{
			case PATHFINDER_CLEANING_GRAPH:
				m_nPathfinderResult = m_pGraph->WalkAStar(m_pCurrentRequest->pStart,m_pCurrentRequest->pEnd,nIterationsLeft);
				break;

			case PATHFINDER_STILLTRACING:
				m_nPathfinderResult = m_pGraph->ContinueAStar(m_pCurrentRequest->pEnd,nIterationsLeft);
				break;

			case PATHFINDER_WALKINGBACK:
				m_nPathfinderResult = m_pGraph->WalkBack(m_pCurrentRequest->pEnd, m_pCurrentRequest->pStart,nIterationsLeft);
				break;

			case PATHFINDER_BEAUTIFYINGPATH:
//				if(m_pCurrentRequest->pRequester->GetType()==AIOBJECT_VEHICLE)
//					m_nPathfinderResult = m_pGraph->BeautifyPathCar(nIterationsLeft,m_pCurrentRequest->pRequester->GetPos(),m_pCurrentRequest->endpos);
//					m_nPathfinderResult = m_pGraph->BeautifyPath(nIterationsLeft,m_pCurrentRequest->pRequester->GetPos(),m_pCurrentRequest->endpos);
//				else
					m_nPathfinderResult = m_pGraph->BeautifyPath(nIterationsLeft,m_pCurrentRequest->pRequester->GetPos(),m_pCurrentRequest->endpos);
				break;

			case PATHFINDER_PATHFOUND:
				{
					SAIEVENT event;
					event.bPathFound = true;
					event.vPosition = m_pCurrentRequest->endpos;
					m_pCurrentRequest->pRequester->Event(AIEVENT_ONPATHDECISION,&event);
					delete m_pCurrentRequest;
					m_pCurrentRequest = 0;
					m_nPathfinderResult = PATHFINDER_POPNEWREQUEST;
				}

				break;

			case PATHFINDER_NOPATH:
				{
					SAIEVENT event;
					event.bPathFound = false;
					m_pCurrentRequest->pRequester->Event(AIEVENT_ONPATHDECISION,&event);
					delete m_pCurrentRequest;
					m_pCurrentRequest = 0;
					m_nPathfinderResult = PATHFINDER_POPNEWREQUEST;
				}
				break;
			case PATHFINDER_POPNEWREQUEST:
				{
					if (!m_pCurrentRequest)
						{
							// get request if any waiting in queue
							m_nPathfinderResult = 0;
				
							if (m_lstPathQueue.empty()) return;

							m_pCurrentRequest = (*m_lstPathQueue.begin());
							m_lstPathQueue.pop_front();

							m_pGraph->SetRequester( m_pCurrentRequest->pRequester );
							m_pGraph->SetCurrentHeuristic(m_pCurrentRequest->m_nSelectedHeuristic);

							m_pGraph->m_fDistance = (m_pCurrentRequest->endpos-m_pCurrentRequest->startpos).GetLength();
							m_pGraph->m_vRealPathfinderEnd = m_pCurrentRequest->endpos;
							m_nPathfinderResult = m_pGraph->WalkAStar(m_pCurrentRequest->pStart,m_pCurrentRequest->pEnd,nIterationsLeft);

							m_fLastPathfindTimeStart = m_pSystem->GetITimer()->GetCurrTime();
						}
				}
				break;
		}//end switch
	}// end while
}

void CAISystem::TracePath(const Vec3d &start, const Vec3d &end, CAIObject *pRequester)
{


	
	PathFindRequest pfr;
	pfr.pStart = m_pGraph->GetEnclosing(start,pRequester->m_pLastNode);
	pRequester->m_pLastNode = pfr.pStart;
	pfr.pEnd = m_pGraph->GetEnclosing(end,pRequester->m_pLastNode);
	pfr.startpos = start;
	pfr.endpos = end;

	if ( ExitNodeImpossible(pfr.pStart,pRequester->m_fPassRadius) )
	{
		Vec3d pos = pfr.pStart->data.m_pos;
		m_pSystem->GetILog()->LogToFile("\001 Node at position (%.3f,%.3f,%.3f) was rejected as path start.",pos.x,pos.y,pos.z);
		SAIEVENT sai;
		sai.bPathFound = false;
		pRequester->Event(AIEVENT_ONPATHDECISION,&sai);
		return;
	}

	if (ExitNodeImpossible(pfr.pEnd,pRequester->m_fPassRadius))
	{
		Vec3d pos = pfr.pEnd->data.m_pos;
		m_pSystem->GetILog()->LogToFile("\001 Node at position (%.3f,%.3f,%.3f) was rejected as path destination.",pos.x,pos.y,pos.z);
		SAIEVENT sai;
		sai.bPathFound = false;
		pRequester->Event(AIEVENT_ONPATHDECISION,&sai);
		return;
	}

 	Vec3d updStart = start;
	Vec3d updEnd = end;
	

	// check whether this path will ever cross a forbidden area
	{
			string strForb= "";
			EntranceMap::iterator iend = m_pGraph->m_mapEntrances.end();
			EntranceMap::iterator ex_end = m_pGraph->m_mapExits.end();
			if (pfr.pStart->nBuildingID>=0)
			{
				if (pfr.pEnd->nBuildingID>=0)
				{
					if (pfr.pEnd->nBuildingID != pfr.pStart->nBuildingID)
					// both inside. check entrances
					for (EntranceMap::iterator fi=m_pGraph->m_mapEntrances.find(pfr.pStart->nBuildingID);fi!=iend;++fi)
					{
						if (fi->first != pfr.pStart->nBuildingID)
							break;
						for (EntranceMap::iterator si=m_pGraph->m_mapEntrances.find(pfr.pEnd->nBuildingID);si!=iend;++si)
						{
							if (si->first != pfr.pEnd->nBuildingID)
								break;
							
							if (BehindForbidden((fi->second)->data.m_pos,(si->second)->data.m_pos,strForb))
								return;
							else 
								break;
						}
					}
				}
				else
				{
					// end outside start inside
					GraphNode *pClosestEntrance = 0;
					float mindist = 200000.f;
					EntranceMap::iterator fi;
					for (fi=m_pGraph->m_mapEntrances.find(pfr.pStart->nBuildingID);fi!=iend;++fi)
					{
						if (fi->first != pfr.pStart->nBuildingID)
							break;
						GraphNode *pThisEntrance = fi->second;

						float curr_dist = (pThisEntrance->data.m_pos - end).len2();
						if (curr_dist<mindist)
						{
							mindist = curr_dist;
							pClosestEntrance = fi->second;
						}
					}
				
					// check exits
					for (fi=m_pGraph->m_mapExits.find(pfr.pStart->nBuildingID);fi!=ex_end;++fi)
					{
						if (fi->first != pfr.pStart->nBuildingID)
							break;

						GraphNode *pThisExit = fi->second;

						float curr_dist = (pThisExit->data.m_pos - end).len2();
						if (curr_dist<mindist)
						{
							mindist = curr_dist;
							pClosestEntrance = fi->second;
						}

					}

					if (pClosestEntrance)
					{
						if (BehindForbidden(pClosestEntrance->data.m_pos,end,strForb))
							updStart = pClosestEntrance->data.m_pos;
					}
				}
			}
			else
			{
				if (pfr.pEnd->nBuildingID>=0)
				{
					// start outside end inside
					GraphNode *pClosestEntrance = 0;
					float mindist = 200000.f;
					for (EntranceMap::iterator si=m_pGraph->m_mapEntrances.find(pfr.pEnd->nBuildingID);si!=iend;++si)
					{
						if (si->first != pfr.pEnd->nBuildingID)
							break;
 
						GraphNode *pThisEntrance = si->second;

						float curr_dist = (pThisEntrance->data.m_pos - end).len2();
						if (curr_dist<mindist)
						{
							mindist = curr_dist;
							pClosestEntrance = si->second;
						}
					}

					if (pClosestEntrance)
					{
						if (BehindForbidden(start,pClosestEntrance->data.m_pos,strForb))
							updEnd = pClosestEntrance->data.m_pos;

					}
					else
						return;

				}
				else
				{
					// both outside
					BehindForbidden(start,end,strForb);
				}
			}

		if (strForb!="")
		{
		
			Vec3d result = IntersectPolygon(updStart,updEnd, (m_mapForbiddenAreas.find(strForb))->second);
			Vec3d req_pos = pRequester->GetPos();
			req_pos.z = result.z;
			if ( GetLength(req_pos-result) < 2.f)
			{
				SAIEVENT sai;
				sai.bPathFound = false;
				pRequester->Event(AIEVENT_ONPATHDECISION,&sai);
				return;
			}
			result += GetNormalized(updStart-updEnd);
			pfr.pEnd = m_pGraph->GetEnclosing(result);
			updEnd = result;
			pfr.endpos = updEnd;

		}
	}

	pfr.pRequester = pRequester;

	
	
	PathFind(pfr);
}


CAIObject * CAISystem::GetAIObjectByName(const char *pName)
{
	AIObjects::iterator ai;

	ai = m_Objects.begin();
	while (ai!=m_Objects.end())
	{
		CAIObject *pObject = ai->second;

		if (string(pName) == string(pObject->GetName()))
			return pObject;
		ai++;
	}

	return 0;
}



void CAISystem::RemoveDummyObject(CAIObject *pObject)
{
	if (std::find(m_lstDummies.begin(), m_lstDummies.end(), pObject) == m_lstDummies.end())
		return;
	m_lstDummies.remove(pObject);
	

	AIObjects::iterator ai;
	// tell all puppets that this object is invalid
	if ( (ai=m_Objects.find(AIOBJECT_PUPPET))!= m_Objects.end() )
	{
		for (;ai!=m_Objects.end();)
		{
			if (ai->first != AIOBJECT_PUPPET)
				break;
			(ai->second)->OnObjectRemoved(pObject);
			ai++;
		}
	}

	// tell all vehicles that this object is invalid
	if ( (ai=m_Objects.find(AIOBJECT_VEHICLE))!= m_Objects.end() )
	{
		for (;ai!=m_Objects.end();)
		{
			if (ai->first != AIOBJECT_VEHICLE)
				break;
			(ai->second)->OnObjectRemoved(pObject);
			ai++;
		}
	}


	// make sure we dont delete any beacon
	BeaconMap::iterator bi;
	for (bi=m_mapBeacons.begin();bi!=m_mapBeacons.end();bi++)
	{
		if ((bi->second).pBeacon == pObject)
		{
			m_mapBeacons.erase(bi);
			break;
		}
	}

	delete pObject;
}

CAIObject * CAISystem::CreateDummyObject()
{
	CAIObject *pObject = new CAIObject();
	pObject->SetType(AIOBJECT_DUMMY);
	pObject->SetEyeHeight(0);
	pObject->SetAssociation(0);

	m_lstDummies.push_back(pObject);

	return pObject;
}

void CAISystem::RemoveObject(IAIObject *pObject)
{
	AIObjects::iterator ai;
	CAIObject *pAIObject = (CAIObject*)pObject;

	if (!pObject)
		return;

	if(pObject->GetProxy() && pObject->GetProxy()->GetPhysics() == m_pTheSkip )
		m_pTheSkip = NULL;

	if ( (ai=m_Objects.find(pObject->GetType())) != m_Objects.end())
	{
		 for (;ai!=m_Objects.end();)
 		 {
			 if (ai->first != pObject->GetType())
				 break;
			 if (ai->second == pObject)
			 {
				 pAIObject = ai->second;
				 m_Objects.erase(ai);
				 break;
			 }
				 
			 ai++;
		 }
	}

	RemoveObjectFromAllOfType(AIOBJECT_PUPPET,pAIObject);
	RemoveObjectFromAllOfType(AIOBJECT_VEHICLE,pAIObject);
	RemoveObjectFromAllOfType(AIOBJECT_ATTRIBUTE,pAIObject);
	
	AIObjects::iterator oi;
	for (oi=m_mapSpecies.begin();oi!=m_mapSpecies.end();oi++)
	{
		if (oi->second == pAIObject)
		{
			m_mapSpecies.erase(oi);
			break;
		}
	}

	for (oi=m_mapGroups.begin();oi!=m_mapGroups.end();oi++)
	{
		if (oi->second == pAIObject)
		{
			m_mapGroups.erase(oi);
			break;
		}
	}


	//remove this object from any pending paths generated for him
	CPuppet *pPuppet = 0;
	if (pObject->CanBeConvertedTo(AIOBJECT_CPUPPET,(void**)&pPuppet))
		CancelAnyPathsFor(pPuppet);


	ListPuppets::iterator delItr=std::find(m_lstWaitingToBeUpdated.begin(),m_lstWaitingToBeUpdated.end(), pAIObject);
	while(delItr!=m_lstWaitingToBeUpdated.end())
	{
		m_lstWaitingToBeUpdated.erase( delItr );
		delItr=std::find(m_lstWaitingToBeUpdated.begin(),m_lstWaitingToBeUpdated.end(), pAIObject);
	}

	delItr=std::find(m_lstAlreadyUpdated.begin(),m_lstAlreadyUpdated.end(), pAIObject);
	while (delItr!=m_lstAlreadyUpdated.end())
	{
		m_lstAlreadyUpdated.erase( delItr );
		delItr=std::find(m_lstAlreadyUpdated.begin(),m_lstAlreadyUpdated.end(), pAIObject);
	}

	// check if this object owned any beacons and remove them if so
	if (!m_mapBeacons.empty())
	{
		BeaconMap::iterator bi,biend = m_mapBeacons.end();
		for (bi=m_mapBeacons.begin();bi!=biend;)
		{
			if ((bi->second).pOwner == pAIObject)
			{
				BeaconMap::iterator eraseme = bi;
				++bi;
				RemoveDummyObject((eraseme->second).pBeacon);
			}
			else
				++bi;
		}
	}

	// finally release the object itself
	pObject->Release();
	m_nNumPuppets = m_Objects.count(AIOBJECT_PUPPET);
	m_nNumPuppets += m_Objects.count(AIOBJECT_VEHICLE);
}


// tells if the sound is hearable by the puppet
//////////////////////////////////////////////////////////////////////////
bool	CAISystem::IsSoundHearable(CPuppet *pPuppet,const Vec3d &vSoundPos,float fSoundRadius)
{
	int nBuildingID1=-1, nBuildingID2=-1;
	IVisArea *pArea1=0, *pArea2=0;
	//GraphNode *pNode1=GetGraph()->GetEnclosing(pPuppet->GetPos());
	CheckInside(pPuppet->GetPos(),nBuildingID1,pArea1);
	CheckInside(vSoundPos,nBuildingID2,pArea2);
	if ((nBuildingID1<0) && (nBuildingID2<0))
			return (true); // there is no sound occlusion in outdoor

	if (nBuildingID1!=nBuildingID2)
	{
		if (nBuildingID1<0)
		{
			if (pArea2)
			{
				if (pArea2->IsConnectedToOutdoor())
				{
					// shoot a ray to figure out if this sound will be heard
//					ray_hit hit;
//					int rayresult = m_pWorld->RayWorldIntersection(vectorf(vSoundPos),vSoundPos-pPuppet->GetPos(),ent_static, 0,&hit,1);
//					if (rayresult)					
//						return false;
//					else
						return true;
				}
				return false;
			}
		}
		else if (nBuildingID2<0)
		{
			if (pArea1)
			{
				if (pArea1->IsConnectedToOutdoor())
				{
					// shoot a ray to figure out if this sound will be heard
					ray_hit hit;
					int rayresult = m_pWorld->RayWorldIntersection(vectorf(vSoundPos),vSoundPos-pPuppet->GetPos(),ent_static, 0,&hit,1);
					if (rayresult)					
						return false;
					else
						return true;
				}
				return false;
			}
		}

		if (pArea1 || pArea2)	//only if in real indoors
			return (false); // if in 2 different buildings we cannot hear the sound for sure
	}

	// make the real indoor sound occlusion check (doors, sectors etc.)
	//if (pArea1 && pArea2)
		//return (m_pSystem->GetI3DEngine()->IsVisAreasConnected(pArea1,pArea2,1,false));
	//else
		return true;
	//return (m_pIndoor->IsSoundPotentiallyHearable(pNode1->nBuildingID,pNode1->nSector,pNode2->nSector));
}

//////////////////////////////////////////////////////////////////////////
void CAISystem::SoundEvent(int soundid, const Vec3d &pos, float fRadius, float fThreat, float fInterest,IAIObject *pSender)
{
	if (m_cvSoundPerception)
	{
		if ( !m_cvSoundPerception->GetIVal() )
			return;
	}
	else
		return;

	if (fRadius < 0.000001)
		return;

	CPuppet *pSenderPuppet=0;
	CAIPlayer *pSenderPlayer=0;
	pSender->CanBeConvertedTo(AIOBJECT_CPUPPET,(void**) &pSenderPuppet);
	pSender->CanBeConvertedTo(AIOBJECT_PLAYER,(void**) &pSenderPlayer);
	Vec3d position = pos;
	
	// go trough all the puppets and register a sound event for those that have it in radius
	AIObjects::iterator ai;

	if ( (ai = m_Objects.find(AIOBJECT_PUPPET)) != m_Objects.end() )
	{
		// go trough all the sound supressors and reduse radius if in effective radius of sepressor
		SupressSoundEvent(position,fRadius);
		AIObjects::iterator spr;
/*		if ( (spr = m_Objects.find(AIOBJECT_SNDSUPRESSOR)) != m_Objects.end() )
		{
			for (;spr!=m_Objects.end();)
			{
				if (spr->first != AIOBJECT_SNDSUPRESSOR)
					break;
				(spr->second)->Supress(position, fRadius);
				spr++;
			}
*/		
			if( fRadius < .5 )		//	sound event radius too small - it was REALLY supressed. Don't propagete sound
				return;
//		}
		

		for (;ai!=m_Objects.end();++ai)
		{ 
			if (ai->first != AIOBJECT_PUPPET)
				break;

			if (ai->second == pSender)
				continue;

			CPuppet *pPuppet=0;
			if ( (ai->second)->CanBeConvertedTo(AIOBJECT_CPUPPET, (void **) &pPuppet))
			{
				
				if (pSenderPuppet)
				{
					if (pPuppet->GetParameters().m_nSpecies == pSenderPuppet->GetParameters().m_nSpecies)
					{
						if (fThreat>fInterest)
						{
							if (pSenderPuppet->GetAttentionTarget())
							{
								if  ( (pSenderPuppet->GetAttentionTarget()->GetType()!=AIOBJECT_PLAYER) &&
									(pSenderPuppet->GetAttentionTarget()->GetType()!=AIOBJECT_PUPPET) )
									break;
								else
									position=pSenderPuppet->GetAttentionTarget()->GetPos();
							}
						}
						else 
						 break;
					}

					if (pPuppet->GetParameters().m_nGroup == pSenderPuppet->GetParameters().m_nGroup)
						continue;


					if (pPuppet->m_bEnabled && pPuppet->PointAudible(position,fRadius) && IsSoundHearable(pPuppet,position,fRadius))
					{
						float dist = (position-pPuppet->GetPos()).GetLength();
						float totalDist = fRadius+pPuppet->GetPuppetParameters().m_fSoundRange;
						SAIEVENT event;
						event.nDeltaHealth = soundid; // used just for convinience in the DeltaHealth integer variable
						event.fInterest = fInterest * (1-dist/totalDist) * 10.f;
						event.fThreat = fThreat * (1-dist/totalDist);
						event.vPosition = position;
						event.pSeen = pSenderPuppet;	// used for convinience to pass the owner of this event
						pPuppet->Event(AIEVENT_ONSOUNDEVENT, &event);
					}
				}
				
				if (pSenderPlayer && (pPuppet->GetParameters().m_nGroup != pSenderPlayer->m_Parameters.m_nGroup))
				{
					if (pPuppet->m_bEnabled && pPuppet->PointAudible(position,fRadius) && IsSoundHearable(pPuppet,position,fRadius))
					{
						float dist = (position-pPuppet->GetPos()).GetLength();
						float totalDist = fRadius+pPuppet->GetPuppetParameters().m_fSoundRange;
						SAIEVENT event;
						event.nDeltaHealth = soundid; // used just for convinience in the DeltaHealth integer variable
						event.fInterest = fInterest*(1-dist/totalDist)*10.f;
						event.fThreat = fThreat * (1-dist/totalDist);
						event.vPosition = position;
						event.pSeen = pSenderPlayer; // used for convinience to pass the owner of this event
						pPuppet->Event(AIEVENT_ONSOUNDEVENT, &event);
					}
				}
			}
	
		}
	}
}




// Sends a signal using the desired filter to the desired agents
void CAISystem::SendSignal(unsigned char cFilter, int nSignalId,const char *szText, IAIObject *pSenderObject)
{

	CPipeUser *pSender=0;
  if (!pSenderObject->CanBeConvertedTo(AIOBJECT_CPIPEUSER, (void**) &pSender))
		return;

	float fRange = pSender->GetParameters().m_fCommRange;
	fRange*=pSender->GetParameters().m_fCommRange;
	Vec3d pos = pSender->GetPos();

	//m_pSystem->GetILog()->Log("\001 Enemy %s sending signal %s",pSender->GetName(),szText);

	switch (cFilter)
	{

		case 0:
		{
			pSender->SetSignal(nSignalId, szText,pSender->GetAssociation());
			break;
		}
		case SIGNALFILTER_READIBILITY:
		{
	
			CPipeUser *pUser = 0;
			if (pSender->CanBeConvertedTo(AIOBJECT_CPIPEUSER,(void**) &pUser))
			{
				if (pUser->m_bHiding)
					return;
			}
	
			unsigned int groupid = pSender->GetParameters().m_nGroup;
			MapSignalStrings::iterator mi = m_mapAuxSignalsFired.find(groupid);
			if (mi != m_mapAuxSignalsFired.end())
			{
				for (;mi!=m_mapAuxSignalsFired.end();)
				{
					if (mi->first != groupid)
						break;
					if ((mi->second).strMessage == szText)
						return;
					mi++;
				}
			}

			AuxSignalDesc asd;
			asd.fTimeout = 3.f;
			asd.strMessage = string(szText);
			m_mapAuxSignalsFired.insert(MapSignalStrings::iterator::value_type(groupid,asd));
			
			pSender->m_State.nAuxSignal = 1;
			pSender->m_State.szAuxSignalText = szText;
	
			break;
		}
		case SIGNALFILTER_HALFOFGROUP:
			{
				int groupid = pSender->GetParameters().m_nGroup;
				AIObjects::iterator ai;
		
				if ( (ai=m_mapGroups.find(groupid)) != m_mapGroups.end() )
				{
					int groupmembers=m_mapGroups.count(groupid);
					groupmembers/=2;

					for (;ai!=m_mapGroups.end();)
					{
						if ( (ai->first != groupid) || (!groupmembers--) )
							break;

						if (ai->second != pSenderObject)
						{
							(ai->second)->SetSignal(nSignalId,szText,pSender->GetAssociation());
						}
						else
							groupmembers++;	// dont take into account sender
						ai++;
					}
				}
			}
			break;
		case SIGNALFILTER_NEARESTGROUP:
			{
				int groupid = pSender->GetParameters().m_nGroup;
				AIObjects::iterator ai;
				CAIObject *pNearest= 0;
				float mindist = 2000;
				if ( (ai=m_mapGroups.find(groupid)) != m_mapGroups.end() )
				{
					for (;ai!=m_mapGroups.end();)
					{
						if (ai->first != groupid)
							break;
						if (ai->second != pSenderObject)
						{
							float dist = (ai->second->GetPos() - pSender->GetPos()).GetLength();
							if (dist < mindist)
							{
								pNearest = ai->second;
								mindist = (ai->second->GetPos() - pSender->GetPos()).GetLength();
							}
						}
						ai++;
					}

					if (pNearest)
					{
						pNearest->SetSignal(nSignalId,szText,pSender->GetAssociation());
					}
				}
			}
			break;
			 
		case SIGNALFILTER_NEARESTINCOMM:
			{
				int groupid = pSender->GetParameters().m_nGroup;
				AIObjects::iterator ai;
				CAIObject *pNearest= 0;
				float mindist = pSender->GetParameters().m_fCommRange;
				if ( (ai=m_mapGroups.find(groupid)) != m_mapGroups.end() )
				{
					for (;ai!=m_mapGroups.end();)
					{
						if (ai->first != groupid)
							break;
						if (ai->second != pSenderObject)
						{
							float dist = (ai->second->GetPos() - pSender->GetPos()).GetLength();
							if (dist < mindist)
							{
								pNearest = ai->second;
								mindist = (ai->second->GetPos() - pSender->GetPos()).GetLength();
							}
						}
						ai++;
					}

					if (pNearest)
					{
						pNearest->SetSignal(nSignalId,szText,pSender->GetAssociation());
					}
				}
			}
			break;
		case SIGNALFILTER_SUPERGROUP:
			{
				int groupid = pSender->GetParameters().m_nGroup;
				AIObjects::iterator ai;
				if ( (ai=m_mapGroups.find(groupid)) != m_mapGroups.end() )
				{
					for (;ai!=m_mapGroups.end();)
					{
						if (ai->first != groupid)
							break;
						(ai->second)->SetSignal(nSignalId, szText,pSender->GetAssociation());
						ai++;
					}
				}
			}
			break;
		case SIGNALFILTER_SUPERSPECIES:
			{
				int speciesid = pSender->GetParameters().m_nSpecies;
				AIObjects::iterator ai;

				if ( (ai=m_mapSpecies.find(speciesid)) != m_mapSpecies.end() )
				{
					for (;ai!=m_mapSpecies.end();)
					{
						if (ai->first != speciesid)
							break;
						(ai->second)->SetSignal(nSignalId, szText,pSender->GetAssociation());
						ai++;
					}
				}
			}
			break;

		case SIGNALFILTER_GROUPONLY:
			{
				int groupid = pSender->GetParameters().m_nGroup;
				AIObjects::iterator ai;
				if ( (ai=m_mapGroups.find(groupid)) != m_mapGroups.end() )
				{
					for (;ai!=m_mapGroups.end();)
					{
						if (ai->first != groupid)
							break;

						CAIObject *pObject = ai->second;
						Vec3d mypos = pObject->GetPos();
						mypos -=pos;

						if (GetLengthSquared(mypos) < fRange) 
						{
								pObject->SetSignal(nSignalId, szText,pSender->GetAssociation());
						}

						ai++;
					}
				}
			}
			break;
		case SIGNALFILTER_SPECIESONLY:
			{
				int speciesid = pSender->GetParameters().m_nSpecies;
				AIObjects::iterator ai;

				if ( (ai=m_mapSpecies.find(speciesid)) != m_mapSpecies.end() )
				{
					for (;ai!=m_mapSpecies.end();)
					{
						if (ai->first != speciesid)
							break;
						CAIObject *pObject = ai->second;
						Vec3d mypos = pObject->GetPos();
						mypos -=pos;

						if (GetLengthSquared(mypos) < fRange)
						{
							pObject->SetSignal(nSignalId, szText,pSender->GetAssociation());
						}

						ai++;
					}
				}
			}
			break;
		case SIGNALFILTER_ANYONEINCOMM:
			{
				// send to puppets and aware objects in the communications range of the sender
				AIObjects::iterator ai;
				// first look for all the puppets
				if ( (ai=m_Objects.find(AIOBJECT_PUPPET)) != m_Objects.end() )
				{
					for (;ai!=m_Objects.end();)
					{
						if (ai->first != AIOBJECT_PUPPET)
							break;
						CAIObject *pObject = ai->second;
						Vec3d mypos = pObject->GetPos();
						mypos -=pos;

						if (GetLengthSquared(mypos) < fRange)
						{
								pObject->SetSignal(nSignalId, szText,pSender->GetAssociation());
						}

						ai++;
					}
				}

				// now look for aware objects
				if ( (ai=m_Objects.find(AIOBJECT_AWARE)) != m_Objects.end() )
				{
					for (;ai!=m_Objects.end();)
					{
						if (ai->first != AIOBJECT_AWARE)
							break;
						CAIObject *pObject = ai->second;
						Vec3d mypos = pObject->GetPos();
						mypos -=pos;

						if (GetLengthSquared(mypos) < fRange) 
						{
							pObject->SetSignal(nSignalId, szText,pSender->GetAssociation());
						}

						ai++;
					}
				}
			}
			break;
	}

	
	
}

// adds an object to a group
void CAISystem::AddToGroup(CAIObject * pObject, unsigned short nGroup)
{
	AIObjects::iterator gi;

	if ( (gi=m_mapGroups.find(nGroup)) != m_mapGroups.end())
	{
		// check whether it was added before
		while ( (gi!=m_mapGroups.end()) &&  (gi->first == nGroup) )
		{
			if (gi->second == pObject)
				return;
			gi++;
		}
	}

	// it has not been added, add it now
	m_mapGroups.insert(AIObjects::iterator::value_type(nGroup,pObject));
}

// adds an object to a species
void CAISystem::AddToSpecies(CAIObject * pObject, unsigned short nSpecies)
{
	AIObjects::iterator gi;

	if ( (gi=m_mapSpecies.find(nSpecies)) != m_mapSpecies.end())
	{
		// check whether it was added before
		while ((gi!=m_mapSpecies.end()) && (gi->first == nSpecies) )
		{
			if (gi->second == pObject)
				return;
			gi++;
		}
	}

	// it has not been added, add it now
	m_mapSpecies.insert(AIObjects::iterator::value_type(nSpecies,pObject));
}

// creates a formation and associates it with a group of agents
CFormation *CAISystem::CreateFormation(int nGroupID, const char * szFormationName)
{
	FormationMap::iterator fi;
	fi = m_mapActiveFormations.find(nGroupID);
	if (fi==m_mapActiveFormations.end())
	{
		DescriptorMap::iterator di;
		di = m_mapFormationDescriptors.find(szFormationName);
		if (di!=m_mapFormationDescriptors.end())
		{
			CFormation *pFormation = new CFormation(this);
			pFormation->Create(di->second);
			m_mapActiveFormations.insert(FormationMap::iterator::value_type(nGroupID,pFormation));
			return pFormation;
		}
	}
	else
		return (fi->second);
	return 0;
}

// retrieves the next available formation point if a formation exists for the group of the requester
CAIObject * CAISystem::GetFormationPoint(CPipeUser * pRequester)
{
	int groupid = pRequester->GetParameters().m_nGroup;
	FormationMap::iterator fi;
	fi = m_mapActiveFormations.find(groupid);
	if (fi!=m_mapActiveFormations.end())
	{
		return (fi->second)->GetFormationPoint((CAIObject *)pRequester);
	}
	
	return NULL;
}


// Resets all agent states to initial
void CAISystem::Reset(void)
{
	if (!m_bInitialized)
		return;

	m_pTheSkip = NULL;

	if (!m_pGraph)
		return;

	if (m_Objects.empty())
		return;

	m_mapDEBUGTiming.clear();

	AIObjects::iterator ai;

	for (ai=m_Objects.begin();ai!=m_Objects.end();ai++)
		(ai->second)->Reset();

	m_lstWaitingToBeUpdated.clear();
	m_lstAlreadyUpdated.clear();
	
	while (!m_lstPathQueue.empty())
	{
			PathFindRequest *pReq = m_lstPathQueue.front();
			m_lstPathQueue.pop_front();
			delete pReq;
	}

	if (m_pCurrentRequest)
	{
		delete m_pCurrentRequest;
		m_pCurrentRequest = 0;
		
	}

	m_nPathfinderResult = PATHFINDER_POPNEWREQUEST;
	m_pGraph->ClearMarks();
	m_pGraph->ClearTagsNow();
	m_pGraph->Reset();
	

	if (!m_mapActiveFormations.empty())
	{
		FormationMap::iterator fi;
		for (fi=m_mapActiveFormations.begin();fi!=m_mapActiveFormations.end();fi++)
				delete (fi->second);
	}

	m_mapActiveFormations.clear();

	m_mapAuxSignalsFired.clear();

	if (!m_mapBeacons.empty())
	{
		BeaconMap::iterator iend = m_mapBeacons.end();
		BeaconMap::iterator binext;
		for (BeaconMap::iterator bi=m_mapBeacons.begin();bi!=iend;)
		{
			binext = bi;
			binext++;
			RemoveDummyObject((bi->second).pBeacon);
			bi = binext;
		}
		m_mapBeacons.clear();
	}
	

}

// copies a designer path into provided list if a path of such name is found
bool CAISystem::GetDesignerPath(const char * szName, ListPositions & lstPath)
{

	DesignerPathMap::iterator di;
	di = m_mapDesignerPaths.find(szName);

	if (di!= m_mapDesignerPaths.end())
	{
		lstPath.insert(lstPath.begin(),(di->second).begin(),(di->second).end());
		return true;
	}
	return false;
}

// adds a point to a designer path specified by the name.. If path non existant, one is created 
void CAISystem::AddPointToPath(const Vec3d & pos, const char * szPathName, EnumAreaType eAreaType)
{ 
	if (eAreaType==AREATYPE_PATH)
	{
		DesignerPathMap::iterator di;
		di = m_mapDesignerPaths.find(szPathName);
		if (di!= m_mapDesignerPaths.end())
			(di->second).push_back(pos);
		else
			AIWarning( "[AIWARNING] Tried to add a point to path %s, but path not created",szPathName);
	}
	else if (eAreaType==AREATYPE_FORBIDDEN)
	{
		DesignerPathMap::iterator di;
		di = m_mapForbiddenAreas.find(szPathName);

		if (di!= m_mapForbiddenAreas.end())
			(di->second).push_back(pos);
		else
			AIWarning( "[AIWARNING] Tried to add a point to forbidden area %s, but it has not been created",szPathName);
	}
	else if (eAreaType == AREATYPE_NAVIGATIONMODIFIER)
	{
		SpecialAreaMap::iterator di;
		di = m_mapSpecialAreas.find(szPathName);
		
		if (di!= m_mapSpecialAreas.end())
		{
			SpecialArea& sa = di->second;

			if (sa.fHeight>0.0001f)
			{
				if ( pos.z < sa.fMinZ )
					sa.fMinZ = pos.z ;

				if ( pos.z + sa.fHeight > sa.fMaxZ )
					sa.fMaxZ = pos.z + sa.fHeight;
			}

			(di->second).lstPolygon.push_back(pos);
		}
		else
			AIWarning( "[AIWARNING] Tried to add a point to navigation modifier %s, but it has not been created",szPathName);
	}
	else if (eAreaType == AREATYPE_OCCLUSION_PLANE)
	{
		DesignerPathMap::iterator di;
		di = m_mapOcclusionPlanes.find(szPathName);

		if (di!= m_mapOcclusionPlanes.end())
			(di->second).push_back(pos);
		else
			AIWarning( "[AIWARNING] Tried to add a point to occlusion area %s, but it has not been created",szPathName);

	}
	
}

// gets how many agents are in a specified group
int CAISystem::GetGroupCount(int nGroupID)
{
	AIObjects::iterator gi;
	return m_mapGroups.count(nGroupID);
}

// removes specified object from group
void CAISystem::RemoveFromGroup(int nGroupID, CAIObject * pObject)
{
	AIObjects::iterator gi;
	
	for (gi = m_mapGroups.find(nGroupID);gi!=m_mapGroups.end();)
	{
		if (gi->first!=nGroupID)
			break;

		if ( gi->second == pObject)
		{
			m_mapGroups.erase(gi);
			break;
		}
		gi++;
	}

}

// parses ai information into file
void CAISystem::ParseIntoFile(const char * szFileName, CGraph *pGraph, bool bForbidden)
{

	for (unsigned int index=0;index<(int)m_vTriangles.size();index++)
	{
		Tri *tri = m_vTriangles[index];
	// make vertices know which triangles contain them
			m_vVertices[tri->v[0]].m_lstTris.push_back(index);
			m_vVertices[tri->v[1]].m_lstTris.push_back(index);
			m_vVertices[tri->v[2]].m_lstTris.push_back(index);
	}

	Vec3d tbbmin, tbbmax;
	tbbmin(m_pTriangulator->m_vtxBBoxMin.x,m_pTriangulator->m_vtxBBoxMin.y,m_pTriangulator->m_vtxBBoxMin.z);
	tbbmax(m_pTriangulator->m_vtxBBoxMax.x,m_pTriangulator->m_vtxBBoxMax.y,m_pTriangulator->m_vtxBBoxMax.z);

	pGraph->SetBBox(tbbmin,tbbmax);

	//I3DEngine *pEngine = m_pSystem->GetI3DEngine();
	unsigned int cnt=0;
	std::vector<Tri*>::iterator ti;
	//for ( unsigned int i=0;i<m_vTriangles.size();i++)
		
	for (ti=m_vTriangles.begin();ti!=m_vTriangles.end();ti++,cnt++)
	{
//		Tri *tri = m_vTriangles[i];
		Tri *tri = (*ti);

		if (!tri->outsider)
		{
			// create node for this tri
			//GraphNode *pNew = new GraphNode;
			GraphNode *pNew = pGraph->CreateNewNode(true);
			tri->outsider = pNew;
		}


		Vtx *v1,*v2,*v3;
		v1 = &m_vVertices[tri->v[0]];
		v2 = &m_vVertices[tri->v[1]];
		v3 = &m_vVertices[tri->v[2]];

		GraphNode *pNode = (GraphNode *)tri->outsider;

		// add the triagle vertices... for outdoors only
		ObstacleData odata;
		if (pGraph == m_pHideGraph)
			odata.vPos = Vec3d(v1->x,v1->y,v1->z);
		else
			odata.vPos = Vec3d(v1->x,v1->y,0);

		pNode->vertex.push_back(m_VertexList.AddVertex(odata));
		//pNode->vertex.push_back(odata);
		if (pGraph == m_pHideGraph)
			odata.vPos = Vec3d(v2->x,v2->y,v2->z);
		else
			odata.vPos = Vec3d(v2->x,v2->y,0);
		pNode->vertex.push_back(m_VertexList.AddVertex(odata));
		//pNode->vertex.push_back(odata);
		if (pGraph == m_pHideGraph)
			odata.vPos = Vec3d(v3->x,v3->y,v3->z);
		else
			odata.vPos = Vec3d(v3->x,v3->y,0);

		pNode->vertex.push_back(m_VertexList.AddVertex(odata));
		//pNode->vertex.push_back(odata);

		m_pGraph->FillGraphNodeData(pNode);
	
		

		// test first edge
		std::vector<int>::iterator u,v;
		for (u = v1->m_lstTris.begin();u!=v1->m_lstTris.end();u++)
			for (v = v2->m_lstTris.begin();v!=v2->m_lstTris.end();v++)
			{
				int au,av;
				au = (*u);
				av = (*v);
				if (au==av && au!=cnt)
				{
					Tri *other = m_vTriangles[au];
					if (!other->outsider)
					{
						// create node for this tri
						GraphNode *pNew = pGraph->CreateNewNode(true);//new GraphNode;
						other->outsider = pNew;
					}
					pGraph->Connect((GraphNode *)tri->outsider, (GraphNode *) other->outsider);
					pGraph->ResolveLinkData((GraphNode *)tri->outsider, (GraphNode *) other->outsider);
				}
			}
			
		for (u = v2->m_lstTris.begin();u!=v2->m_lstTris.end();u++)
			for (v = v3->m_lstTris.begin();v!=v3->m_lstTris.end();v++)
			{
				int au,av;
				au = (*u);
				av = (*v);
				if (au==av && au!=cnt)
				{
					Tri *other = m_vTriangles[au];
					if (!other->outsider)
					{
						// create node for this tri
						GraphNode *pNew = pGraph->CreateNewNode(true);//new GraphNode;
						other->outsider = pNew;
					}
					pGraph->Connect((GraphNode *)tri->outsider, (GraphNode *) other->outsider);
					pGraph->ResolveLinkData((GraphNode *)tri->outsider, (GraphNode *) other->outsider);
				}
			}

		for (u = v3->m_lstTris.begin();u!=v3->m_lstTris.end();u++)
			for (v = v1->m_lstTris.begin();v!=v1->m_lstTris.end();v++)
			{
				int au,av;
				au = (*u);
				av = (*v);
				if (au==av && au!=cnt)
				{
					Tri *other = m_vTriangles[au];
					if (!other->outsider)
					{
						// create node for this tri
						GraphNode *pNew = pGraph->CreateNewNode(true); //new GraphNode;
						other->outsider = pNew;
					}
					pGraph->Connect((GraphNode *)tri->outsider, (GraphNode *) other->outsider);
					pGraph->ResolveLinkData((GraphNode *)tri->outsider, (GraphNode *) other->outsider);
				}
			}

		
		
	}
 
	for (ti=m_vTriangles.begin();ti!=m_vTriangles.end();ti++)
	{
			GraphNode *pCurrent = (GraphNode*) (*ti)->outsider;
			VectorOfLinks::iterator vi;
			for (vi=pCurrent->link.begin();vi!=pCurrent->link.end();vi++)
				m_pGraph->ResolveLinkData(pCurrent,(*vi).pLink);
	}

	


	if (bForbidden)
	{
		m_pSystem->GetILog()->Log("\003[AISYSTEM] Checking forbidden area validity.");
		if (ForbiddenAreasOverlap())
			return;

		m_pSystem->GetILog()->Log("\003[AISYSTEM] Adding forbidden areas.");
		AddForbiddenAreas();

		m_pSystem->GetILog()->Log("\003[AISYSTEM] Calculating pass radiuses.");
		CalculatePassRadiuses();
	}

	m_pSystem->GetILog()->Log("\003[AISYSTEM] Now writing to %s.",szFileName);
	pGraph->WriteToFile(szFileName);

}

//////////////////////////////////////////////////////////////////////////
// Given the list and the vector, inserts the whole list in front of
// the vector's elements, widening the vector accordingly
// THis is cross-32/64 compilable function
static void PushFront (std::vector<Tri*>& arrTri, const std::list<Tri*>& lstTri)
{
#ifdef WIN64
	// workaround for non-compliant 64bit compiler
	// NOTE: MS STL actually saves the list size, so size() doesn't iterate through the whole list
	size_t nPrevSize = arrTri.size();
	size_t nListSize = lstTri.size();
	arrTri.resize(nPrevSize+nListSize);
	size_t i;
	for (i = 0; i < nPrevSize; ++i)
		arrTri[nPrevSize+nListSize-1-i] = arrTri[nPrevSize-1-i];
	std::list<Tri*>::const_iterator itList = lstTri.begin();
	for (i = 0; i < nListSize; ++i, ++itList)
	{
		assert (itList != lstTri.end());
		arrTri[i] = *itList;
	}

	assert (itList == lstTri.end());
#else
	arrTri.insert(arrTri.begin(),lstTri.begin(),lstTri.end());
#endif
}

CGraph * CAISystem::GetHideGraph(void)
{
	return m_pHideGraph;
}

// // loads the triangulation for this level and mission
void CAISystem::LoadTriangulation(const char * szLevel, const char * szMission)
{

	if (!szLevel || !szMission)
		return;
 
	char fileName[255],fileNameHide[255];
  sprintf(fileName, "%s/net%s.bai",szLevel,szMission);
	sprintf(fileNameHide, "%s/hide%s.bai",szLevel,szMission);

	while (!m_lstPathQueue.empty())
	{
		AIWarning( "GRAPHRELOADWARNING: Agents were waiting to generate paths, but graph changed. May cause incorrect behaviour.");
		PathFindRequest *pRequest = m_lstPathQueue.front();
		delete pRequest;
		m_lstPathQueue.pop_front();
	}

	if (m_pGraph) 
	{
		delete m_pGraph;
		m_pGraph = 0;
	}

	if (m_pHideGraph)
	{
		delete m_pHideGraph;
		m_pHideGraph = 0;
	}

	m_VertexList.Clear();


	m_pGraph = new CGraph(this);
	m_pHideGraph = new CGraph(this);

	if (m_cvTriangulate->GetIVal())
	{
		m_pSystem->GetILog()->Log("\003[AISYSTEM] Triangulation started.");
		vectorf min,max;
		min.Set(0,0,0);
		max.Set(2048,2048,256);

		if (m_pTriangulator) 
			delete m_pTriangulator;
		m_pTriangulator = new CTriangulator();
		CTriangulator *pHideTriangulator = new CTriangulator();

		// get only static physical entities (trees, buildings etc...)
		int count = m_pWorld->GetEntitiesInBox(min,max,m_pObstacles,ent_static |ent_ignore_noncolliding);

		pHideTriangulator->m_vVertices.reserve(count);
		m_pTriangulator->m_vVertices.reserve(count);

		if (!pHideTriangulator->PrepForTriangulation())
			return;
		if (!m_pTriangulator->PrepForTriangulation())
			return;

		float fTSize = (float) m_pSystem->GetI3DEngine()->GetTerrainSize();

		m_pTriangulator->AddVertex(0,0,0,0);
		m_pTriangulator->AddVertex(fTSize,0,0,0);
		//m_pTriangulator->AddVertex(fTSize/2,0,0,0);
		m_pTriangulator->AddVertex(fTSize,fTSize,0,0);
		m_pTriangulator->AddVertex(0,fTSize,0,0);

		pHideTriangulator->AddVertex(0,0,0,0);
		pHideTriangulator->AddVertex(fTSize,0,0,0);
		//pHideTriangulator->AddVertex(fTSize/2,0,0,0);
		pHideTriangulator->AddVertex(fTSize,fTSize,0,0);
		pHideTriangulator->AddVertex(0,fTSize,0,0);


		

		I3DEngine *pEngine = m_pSystem->GetI3DEngine();
		 
		
		for (int i=0;i<count;i++)
		{
			IPhysicalEntity *pCurrent = m_pObstacles[i];
			pe_status_pos status;
			pCurrent->GetStatus(&status);
			Vec3d calc_pos = status.pos;
			//calc_pos.x += (status.BBox[1].x - status.BBox[0].x)/2.f;
			//calc_pos.y += (status.BBox[1].y - status.BBox[0].y)/2.f;
			//calc_pos.z += (status.BBox[1].z - status.BBox[0].z)/2.f;
			
			// if too flat and too close to the terrain
			float zpos = pEngine->GetTerrainElevation(calc_pos.x,calc_pos.y);
			float ftop =calc_pos.z + status.BBox[1].z;
			
			if (zpos>ftop)	// skip underground stuff
				continue;
			if (fabs(ftop - zpos)<0.4f)	// skip stuff too close to the terrain
				continue;
			 
			IVisArea *pArea;
			int buildingID;
			if (CheckInside(calc_pos,buildingID,pArea))
				continue;

			if (OnForbiddenEdge(calc_pos))
				continue;
			
			pe_params_foreign_data pfd;
			pCurrent->GetParams(&pfd);

			if (pfd.iForeignFlags & PFF_HIDABLE)
				pHideTriangulator->AddVertex(calc_pos.x,calc_pos.y,calc_pos.z,(void*)-1);

			if (!pCurrent->GetForeignData() && !(pfd.iForeignFlags & PFF_EXCLUDE_FROM_STATIC))
				m_pTriangulator->AddVertex(calc_pos.x,calc_pos.y,0,(void*) -1);

		}

		if (!m_pTriangulator->TriangulateNew())
			return;
		if (!pHideTriangulator->TriangulateNew())
			return;

		m_pTriangulator->FinalizeTriangulation();
		pHideTriangulator->FinalizeTriangulation();
		
		TARRAY lstTriangles = m_pTriangulator->GetTriangles();
		m_vVertices = m_pTriangulator->GetVertices();
		m_vTriangles.clear();
		PushFront(m_vTriangles,lstTriangles);

		m_pSystem->GetILog()->Log("\003[AISYSTEM] Creation of graph and parsing into file.");
		ParseIntoFile(fileName,m_pGraph,true);

		if (!m_vTriangles.empty())
		{
			std::vector<Tri *>::iterator ti;
			for (ti=m_vTriangles.begin();ti!=m_vTriangles.end();ti++)
			{
				Tri *tri = (*ti);
				delete tri;
			}
			m_vTriangles.clear();
		}

		m_vVertices.clear();

		lstTriangles = pHideTriangulator->GetTriangles();
		m_vVertices = pHideTriangulator->GetVertices();
		m_vTriangles.clear();
		PushFront(m_vTriangles, lstTriangles);

		m_cvCalcIndoorGraph->Set(0);
		m_pSystem->GetILog()->Log("\003[AISYSTEM] Creation of hide graph and parsing into file.");
		ParseIntoFile(fileNameHide,m_pHideGraph);

		if (!m_vTriangles.empty())
		{
			std::vector<Tri *>::iterator ti;
			for (ti=m_vTriangles.begin();ti!=m_vTriangles.end();ti++)
			{
				Tri *tri = (*ti);
				delete tri;
			}
			m_vTriangles.clear();
		}

		m_vVertices.clear();

		delete pHideTriangulator;


	}
	else
	{
		m_pGraph->ReadFromFile(fileName);
		m_pHideGraph->ReadFromFile(fileNameHide);
	}

 
 m_nNumBuildings = 0;

 m_pSystem->GetILog()->Log("\003[AISYSTEM] Triangulation finished.");

}

// deletes designer created path
void CAISystem::DeletePath(const char * szName)
{
	DesignerPathMap::iterator di;
	di = m_mapDesignerPaths.find(szName);

	if (di!= m_mapDesignerPaths.end())
		m_mapDesignerPaths.erase(di);

	di = m_mapForbiddenAreas.find(szName);

	if (di!= m_mapForbiddenAreas.end())
		m_mapForbiddenAreas.erase(di);

	di = m_mapOcclusionPlanes.find(szName);

	if (di!= m_mapOcclusionPlanes.end())
		m_mapOcclusionPlanes.erase(di);

	SpecialAreaMap::iterator si;
	si = m_mapSpecialAreas.find(szName);

	if (si!= m_mapSpecialAreas.end())
	{
		if ( (si->second).nBuildingID>=0)
			m_BuildingIDManager.FreeId((si->second).nBuildingID);
		m_mapSpecialAreas.erase(si);
	}

}


void CAISystem::ReleaseFormation(int nGroupID)
{
	FormationMap::iterator fi;
	fi = m_mapActiveFormations.find(nGroupID);
	if (fi!=m_mapActiveFormations.end())
		m_mapActiveFormations.erase(fi);
}

void CAISystem::FreeFormationPoint(int nGroupID, CAIObject * pLastHolder)
{
	FormationMap::iterator fi;
	fi = m_mapActiveFormations.find(nGroupID);
	if (fi!=m_mapActiveFormations.end())
		(fi->second)->FreeFormationPoint(pLastHolder);
}



bool CAISystem::NoFriendsInWay(CPuppet * pShooter, const Vec3d & vDirection)
{
	AIObjects::iterator ai;
	float dist_to_target = (pShooter->GetAttentionTarget()->GetPos() - pShooter->GetPos()).len2();
	
	
	for (ai = m_mapGroups.find(pShooter->GetParameters().m_nGroup);ai!= m_mapGroups.end();)
	{
		if (ai->first != pShooter->GetParameters().m_nGroup)
			break;
		CAIObject *pObject = ai->second;
		CPuppet *pPuppet;
		if ( !pObject->CanBeConvertedTo(AIOBJECT_CVEHICLE,(void**) &pPuppet) &&
			pObject->CanBeConvertedTo(AIOBJECT_CPUPPET,(void**) &pPuppet) )
			if (pPuppet != pShooter )
			{
				Vec3d dir = pPuppet->GetPos() - pShooter->GetPos();
				float friend_dist = dir.len2();
				Vec3d shootdir = vDirection;
				shootdir.Normalize();
				dir.Normalize();
				if ((dir.Dot(shootdir) > 0.98) && (friend_dist < dist_to_target))
				{
					pShooter->SetSignal(1,"OnFriendInWay");
					return false;
				}
			}
		ai++;
	}

	// also no vehicles in way
/*	AIObjects::iterator aiend = m_Objects.end();
	for (ai=m_Objects.find(AIOBJECT_VEHICLE);ai!=aiend;++ai)
	{
		if (ai->first !=AIOBJECT_VEHICLE) 
			break;
		CAIObject *pVehicle = ai->second;
		Vec3d dir = pVehicle->GetPos() - pShooter->GetPos();
		float friend_dist = dir.len2();
		Vec3d shootdir = vDirection;
		shootdir.Normalize();
		dir.Normalize();
		if ((dir.Dot(shootdir) > 0.96) && (friend_dist < dist_to_target))
			return false;
	}
	*/
	return true;
}

IGraph * CAISystem::GetNodeGraph(void)
{
	return (IGraph*) m_pGraph;
}

void CAISystem::FlushSystem(void)
{
	m_lstWaitingToBeUpdated.clear();
	m_lstAlreadyUpdated.clear();


	// clear pathfinder
	if (m_pCurrentRequest)
		delete m_pCurrentRequest;
	m_pCurrentRequest =0;

	while(!m_lstPathQueue.empty())
	{
		PathFindRequest *pRequest = (*m_lstPathQueue.begin());
		m_lstPathQueue.pop_front();

		delete pRequest;
	}

	// clear any paths in the puppets
	AIObjects::iterator ai;
	for (ai = m_Objects.find(AIOBJECT_PUPPET);ai!=m_Objects.end();)
	{
		if (ai->first != AIOBJECT_PUPPET) 
			break;
		CAIObject *pObject = ai->second;
		CPuppet *pPuppet;
		if (pObject->CanBeConvertedTo(AIOBJECT_CPUPPET,(void**) &pPuppet))
			pPuppet->Reset();
		ai++;
	} 

	for (ai = m_Objects.begin();ai!=m_Objects.end();ai++)
	{
		CAIObject *pObject = ai->second;
		pObject->Reset();
	} 


	// clear building id's
	IntIntMultiMap::iterator iim = m_mapBuildingMap.begin();
	for(;iim!=m_mapBuildingMap.end();++iim)
	{
		m_BuildingIDManager.FreeId(iim->second);
	}

	// clear areas id's
	SpecialAreaMap::iterator si = m_mapSpecialAreas.begin();
	while (si!=m_mapSpecialAreas.end())
	{
		m_BuildingIDManager.FreeId((si->second).nBuildingID);
		(si->second).nBuildingID = -1;
		si++;
	}


	m_mapBuildingMap.clear();

}



float CAISystem::GetPerceptionValue(IAIObject * pObject)
{
	if (!pObject)
		return 0; 

	CAIPlayer *pPlayer =0;
	if ( pObject->CanBeConvertedTo(AIOBJECT_PLAYER, (void **) &pPlayer) )
		return pPlayer->GetPerception();
	return 0;
}

int CAISystem::GetAITickCount(void)
{
	return m_nTickCount;
}

void CAISystem::SendAnonimousSignal(int nSignalID, const char * szText, const Vec3d & vPos, float fRadius, IAIObject *pObject)
{
	AIObjects::iterator ai;
	fRadius*=fRadius;

	// find sending puppet (we need this to know which group to skip when we send the signal)
	int groupid = -1;
	if (pObject)
	{
		CPuppet *pPuppet;
		if (pObject->CanBeConvertedTo(AIOBJECT_CPUPPET, (void**) &pPuppet))
		{
			groupid = pPuppet->GetParameters().m_nGroup;
		}
	}

	// go trough all the puppets
	for (ai=m_Objects.find(AIOBJECT_PUPPET);ai!=m_Objects.end();)
	{
		if (ai->first != AIOBJECT_PUPPET)
			break;
		CPuppet *pPuppet;
		if ( (ai->second)->CanBeConvertedTo(AIOBJECT_CPUPPET,(void**)&pPuppet) ) 
		{
			Vec3d mypos = pPuppet->GetPos();
			mypos -=vPos;
			if ( (GetLengthSquared(mypos) < fRadius) && (pPuppet->GetParameters().m_nGroup != groupid) )
			{
				if (pObject)
					pPuppet->SetSignal(nSignalID, szText,pObject->GetAssociation());
				else
					pPuppet->SetSignal(nSignalID, szText);
			}
		}
		ai++;
	}

	// go trough all the vehicles
	for (ai=m_Objects.find(AIOBJECT_VEHICLE);ai!=m_Objects.end();)
	{
		if (ai->first != AIOBJECT_VEHICLE)
			break;
		CAIVehicle *pVehicle;
		if ( (ai->second)->CanBeConvertedTo(AIOBJECT_CVEHICLE,(void**)&pVehicle) ) 
		{
			Vec3d mypos = pVehicle->GetPos();
			mypos -=vPos;
			if ( (GetLengthSquared(mypos) < fRadius) && (pVehicle->GetParameters().m_nGroup != groupid) )
			{
				if (pObject)
					pVehicle->SetSignal(nSignalID, szText,pObject->GetAssociation());
				else
					pVehicle->SetSignal(nSignalID, szText);
			}
		}
		ai++;
	}
}

void CAISystem::ReleaseFormationPoint(CAIObject * pReserved)
{
	if (m_mapActiveFormations.empty())
		return;

	FormationMap::iterator fi;
	for (fi=m_mapActiveFormations.begin();fi!=m_mapActiveFormations.end();fi++)
		(fi->second)->FreeFormationPoint(pReserved);

}

IAIObject * CAISystem::GetNearestObjectOfType(const Vec3d &pos, unsigned int nTypeID, float fRadius, IAIObject* pSkip)
{
	AIObjects::iterator ai;
	ai = m_Objects.find(nTypeID);

	float	fRadiusSQR = fRadius*fRadius;

	if (ai == m_Objects.end()) 
    return NULL;

	IAIObject *pRet = 0;
	float mindist = 100000000;

	for (;ai!=m_Objects.end();ai++)
	{
		if (ai->first != nTypeID)
			break;
		if( pSkip == ai->second )
			continue;
		Vec3d ob_pos = (ai->second)->GetPos();
		float fActivationRadius = (ai->second)->GetRadius();
		fActivationRadius*=fActivationRadius;
		float f = GetLengthSquared((ob_pos-pos));
		if ( (f<mindist) && (f<fRadiusSQR))
		{
			if ((fActivationRadius>0) && (f>fActivationRadius))
				continue;

			pRet = ai->second;
			mindist = f;
		}
		
	}

	return pRet;
}

void CAISystem::UpdateBeacon(unsigned short nGroupID, const Vec3d & vPos, CAIObject *pOwner)
{
	Vec3d pos = vPos;
	ray_hit hit;
	int rayresult = m_pWorld->RayWorldIntersection(vectorf(pos),vectorf(0,0,-20),ent_terrain|ent_static, rwi_stop_at_pierceable,&hit,1);
	if (rayresult)					
		pos.Set(hit.pt.x,hit.pt.y,hit.pt.z);

	BeaconMap::iterator bi;
	bi = m_mapBeacons.find(nGroupID);
	if ( bi== m_mapBeacons.end())
	{
		BeaconStruct bs;
		CAIObject *pObject = CreateDummyObject();
		pObject->SetPos(pos);
		pObject->SetEyeHeight(0);
		pObject->SetType(AIOBJECT_WAYPOINT);
		pObject->SetName("BEACON");
		bs.pBeacon = pObject;
		bs.pOwner = pOwner;
		m_mapBeacons.insert(BeaconMap::iterator::value_type(nGroupID,bs));
	}
	else
	{
		// beacon found, update its position
		(bi->second).pBeacon->SetPos(pos);
		(bi->second).pOwner = pOwner; 

	}
}

CAIObject * CAISystem::GetBeacon(unsigned short nGroupID)
{
	BeaconMap::iterator bi;
	bi = m_mapBeacons.find(nGroupID);
	if ( bi== m_mapBeacons.end())
		return NULL;

	return (bi->second).pBeacon;
}

void CAISystem::CancelAnyPathsFor(CPuppet * pRequester)
{
		if (m_pCurrentRequest)
		{
			if (m_pCurrentRequest->pRequester == pRequester)
			{
				delete m_pCurrentRequest;
				m_pCurrentRequest = 0;
				m_pGraph->Reset();
			}
		}

		PathQueue::iterator pi;
		for (pi=m_lstPathQueue.begin();pi!=m_lstPathQueue.end();)
		{
			if ((*pi)->pRequester == pRequester)
				pi = m_lstPathQueue.erase(pi);
			else
				pi++;
		}
}

void CAISystem::SetAssesmentMultiplier(unsigned short type, float fMultiplier)
{
	MapMultipliers::iterator mi;
	
	mi = m_mapMultipliers.find(type);
	if (mi== m_mapMultipliers.end())
		m_mapMultipliers.insert(MapMultipliers::iterator::value_type(type,fMultiplier));
	else
		mi->second = fMultiplier;

	if (std::find(m_lstVisible.begin(),m_lstVisible.end(),type) == m_lstVisible.end())
		m_lstVisible.push_front(type);
}




IAIObject * CAISystem::GetNearestToObject(IAIObject * pRef, unsigned short nType, float fRadius)
{
	AIObjects::iterator ai;
	ai = m_Objects.find(nType);

	float	fRadiusSQR = fRadius*fRadius;

	if (ai == m_Objects.end()) 
		return NULL;

	CPipeUser *pPipeUser = 0;
	CAIObject *pAttTarget = 0;
	if (pRef->CanBeConvertedTo(AIOBJECT_CPIPEUSER,(void**) &pPipeUser))
			pAttTarget = pPipeUser->m_pAttentionTarget;
	

	IAIObject *pRet = 0;
	float maxdot = -1;
	float maxdist = 10000000;
	Vec3d pos = pRef->GetPos();
	for (;ai!=m_Objects.end();ai++)
	{
		if (ai->first != nType)
			break;
		Vec3d ob_pos = (ai->second)->GetPos();
		float f = GetLengthSquared((ob_pos-pos));
		float fdot = 1;
		float fFrontDot=-1;
		if (pAttTarget)
		{
			Vec3d dir = pAttTarget->GetPos() - ob_pos;
			dir.Normalize();

			Matrix44 m;
			m.SetIdentity();
			//m.RotateMatrix_fix((ai->second)->GetAngles());
			m=Matrix44::CreateRotationZYX( - (ai->second)->GetAngles() * gf_DEGTORAD )*m; //NOTE: angles in radians and negated 
		  //POINT_CHANGED_BY_IVO 
			//Vec3d orie = m.TransformPoint(Vec3d(0,-1,0));
			Vec3d orie = GetTransposed44(m) * Vec3d(0,-1,0);

			fdot = dir.Dot(orie);

			Vec3d my_dir = GetNormalized(ob_pos - pos);
			fFrontDot = my_dir.Dot(GetNormalized(pAttTarget->GetPos()-pos));
		}

		if (fFrontDot<0.001f)
			continue;

		CPuppet *pPuppet = 0;
		if (pRef->CanBeConvertedTo(AIOBJECT_CPUPPET,(void**)&pPuppet))
		{
			if (pPuppet->m_mapDevaluedPoints.find(ai->second) != pPuppet->m_mapDevaluedPoints.end())
				continue;
		}
		

		if ( (f<fRadiusSQR) && (fdot>maxdot))
		{
			if ((f < maxdist))
			{
				pRet = ai->second;
				maxdot = fdot;
			}
		}
		
	}

	if (pRet)
	{
		int my_group = pPipeUser->GetParameters().m_nGroup;
		AIObjects::iterator gri = m_mapGroups.find(my_group);
		for (;gri!=m_mapGroups.end();gri++)
		{
			if (gri->first != my_group)
				break;

			CPuppet *pGroupMember;
			if ((gri->second)->CanBeConvertedTo(AIOBJECT_CPUPPET,(void**)&pGroupMember))
				pGroupMember->Devalue((CAIObject*)pRet,false);
		}

	}


	return pRet; 
} 

void CAISystem::AddForbiddenAreas(void)
{ 
	m_lstNewNodes.clear();
	m_lstOldNodes.clear(); 
	int	areaCounter=0;
	int scipOptimizeCounter=0;

	if (!m_mapForbiddenAreas.empty())
	{
		areaCounter++;
		for (DesignerPathMap::iterator di=m_mapForbiddenAreas.begin();di!=m_mapForbiddenAreas.end();di++)
		{
  
			int cutCounter=0;
			m_pSystem->GetILog()->Log("[AISYSTEM] Now processing %s",di->first.c_str());
			
			ListPositions lstPos = di->second;
			for (ListPositions::iterator li = lstPos.begin();li!=lstPos.end(); li++)
//ListPositions::iterator li = lstPos.begin();
			{
				ListPositions::iterator linext;
				Vec3d vStart = (*li);

				m_lstNewNodes.clear();
				m_lstOldNodes.clear();
				cutCounter++;

 
//				m_NewCutsMap.clear();
				m_NewCutsVector.clear();
				// create edge  
				linext = li;
				linext++;
		        if (linext == lstPos.end())	
				{
					linext = lstPos.begin();
				}
				Vec3d vEnd = (*linext);

				vEnd.z = vStart.z = 0;
				f5 = 0;

 
//				GraphNode *pStartNode = m_pGraph->GetEnclosing(vStart+0.0001f*(GetNormalized(vEnd-vStart)),0,true);
//				while (pStartNode = RefineTriangle(pStartNode,vStart,vEnd));


				ListNodes nodes2refine;	
				CreatePossibleCutList( vStart, vEnd, nodes2refine );

				bool bDontOptimize = (m_cvOptimizeGraph->GetIVal()==0);
				for(ListNodes::iterator nI=nodes2refine.begin();nI!=nodes2refine.end() && !bDontOptimize;++nI)
				{
					GraphNode *pNode = (*nI);
					for(unsigned int vIdx=0; vIdx<pNode->vertex.size() && !bDontOptimize; ++vIdx)
					{
						Vec3d vtx = m_VertexList.GetVertex(pNode->vertex[vIdx]).vPos; 
						float dist = PointLineDistance(vStart,vEnd, vtx);
						if( dist>0.0f &&  dist<0.01f)
						{
							bDontOptimize=true;
							++scipOptimizeCounter;						
						}
					}
				}


				for(ListNodes::iterator nI=nodes2refine.begin();nI!=nodes2refine.end();nI++)
					RefineTriangle((*nI),vStart,vEnd);


				if( m_NewCutsVector.size()==1 && m_lstNewNodes.size()<5 )	// nothing was really changed
					continue;

				m_pGraph->ClearTagsNow();

				// delete old triangles
				ListNodes::iterator di;
				for (di=m_lstOldNodes.begin();di!=m_lstOldNodes.end();di++)
					m_pGraph->Disconnect((*di));


				m_pGraph->ConnectNodes(m_lstNewNodes);


				// if optimization is on - do optimize
//				if(m_cvOptimizeGraph->GetIVal())
				if( !bDontOptimize )
				{

					int	segCounter=0;
					int	newSegCounter=0;

					ListNodes lstTemp;
					lstTemp.insert(lstTemp.begin(),m_lstNewNodes.begin(),m_lstNewNodes.end());

					// add to newList all the neighbores
					for (ListNodes::iterator node=lstTemp.begin();node!=lstTemp.end();node++)
					{
						GraphNode *pNode = (*node);
						VectorOfLinks::iterator ilink;
						for (ilink=pNode->link.begin();ilink!=pNode->link.end();ilink++)
							if (std::find(m_lstNewNodes.begin(),m_lstNewNodes.end(),(*ilink).pLink)==m_lstNewNodes.end())
								m_lstNewNodes.push_back((*ilink).pLink);
					}
					// check newList for duplication, mark all links as NewNotOnCut (set all passRadiuses to -10)
					for (ListNodes::iterator it=m_lstNewNodes.begin();it!=m_lstNewNodes.end();it++)
					{
					GraphNode *pCurrent = (*it);

						ListNodes::iterator itnext = it;
						if (++itnext != m_lstNewNodes.end())
						{
							ListNodes::iterator li;
							li = std::find(itnext,m_lstNewNodes.end(),(*it));
							assert(li==m_lstNewNodes.end());		// duplicated entry in list
						}

						for (VectorOfLinks::iterator il=pCurrent->link.begin();il!=pCurrent->link.end();il++)
							(*il).fMaxRadius = -10.f;
					}
					// find and mark all new links on cut
					for(NewCutsVector::iterator cItr=m_NewCutsVector.begin(); cItr!=m_NewCutsVector.end(); cItr++)
					{
						CutEdgeIdx curCut = (*cItr);
						GraphNode *pNode = FindMarkNodeBy2Vertex( curCut.idx1, curCut.idx2, NULL );
						assert( pNode );	// cut not found!!!!
						// find second node - on other side of cut
	//					pNode = FindMarkNodeBy2Vertex( curCut.idx1, curCut.idx2, pNode );
	//					assert( pNode );	// second cut not found!!!!
						newSegCounter++;
					}

				//	if (f5!=segCounter)
					//	int a=5;

				//m_pGraph->DbgCheckList(m_lstNewNodes);

//					if(m_cvOptimizeGraph->GetIVal())
					m_pGraph->Rearrange(m_lstNewNodes, vStart, vEnd);
				}
			}
		}
	}
}




GraphNode *CAISystem::RefineTriangle(GraphNode * pNode,const Vec3d &start, const Vec3d &end)
{
	
	int	newNodesCount = m_lstNewNodes.size();

	GraphNode *pNextNode=0;
	Vec3d D0,P0;

	Vec3d newStart = start;
	float maximum_s = -epsilon;
	float aux_maximum_s = -epsilon;

	// parametric equation of new edge
	P0 = start;
	D0 = end-start;

	Vec3d vCut1,vCut2;

	int FirstCutStart=-1, FirstCutEnd=-1;
	int SecondCutStart=-1, SecondCutEnd=-1;
	int StartCutStart=-1, StartCutEnd=-1;
	int EndCutStart=-1, EndCutEnd=-1;
	int TouchStart=-1, TouchEnd=-1;
	
	unsigned int index,next_index;
	for (index=0;index<pNode->vertex.size();index++)
	{
		Vec3d P1,D1;
		next_index = index+1;
		if (next_index == pNode->vertex.size())		// make sure we wrap around correctly
			next_index = 0;

		//get the triangle edge
		P1=m_VertexList.GetVertex(pNode->vertex[index]).vPos;
		D1=m_VertexList.GetVertex(pNode->vertex[next_index]).vPos - P1;

		// calculate intersection parameters
		float s=-1;	// parameter on new edge
		float t=-1;	// parameter on triangle edge
		

		

		{
			Vec3d delta = P1-P0;
			float crossD = D0.x*D1.y-D0.y*D1.x;
			float crossDelta1 = delta.x*D1.y-delta.y*D1.x;
			float crossDelta2 = delta.x*D0.y-delta.y*D0.x;

			if ( fabs(crossD) > epsilon )
			{
				// intersection
				s=crossDelta1/crossD;
				t=crossDelta2/crossD;
			}
			else
			{
				TouchStart =index;
				TouchEnd = next_index;
			}
			
		}
		

		// for any reasonable calculation both s and t must be between 0 and 1
		// everything else is a non desired situation
 		if ((t>-epsilon) && (t<1.f+epsilon) )
		{
			// calculate the point of intersection
			Vec3d result = P0 + D0*s;
			if (s>maximum_s)
				maximum_s = s;

			// s < 0	
			if (s < -epsilon)
			{
				// a clean start triangle
				StartCutStart = index;		
				StartCutEnd	  = next_index;		
			}

			// s == 0  or s == 1
			if ( (fabs(s) < epsilon) || ((s>1.f-epsilon) && (s<1.f+epsilon)) )
			{
				// the start coincides with a triangle vertex or lies on a triangle side
				if ( (t>epsilon) && (t<1.f-epsilon) )
				{
					// the start lies on a triangle side
					if (FirstCutStart<0)
					{
						FirstCutStart = index;
						FirstCutEnd   = next_index;
						vCut1 = result;
					}
					else
					{
						SecondCutStart= index;
						SecondCutEnd	= next_index;
						vCut2 = result;
					}
				} 
				// if its in the triangle vertex then just skip
			}


			// s between 0 and 1
			if ( (s>epsilon) && (s<1.f-epsilon))
			{
				// a normal cut or new edge coincides with a vertex on the triangle
				if ( (fabs(t) < epsilon) || (t>1.f-epsilon) )
				{
					//the edge coincides with a triangle vertex
					// skip this case
					int a=5;
				}
				else
				{
					// a normal cut
					if (FirstCutStart<0)
					{
						FirstCutStart = index;
						FirstCutEnd   = next_index;
						vCut1 = result;
					}
					else
					{
						SecondCutStart= index;
						SecondCutEnd	= next_index;
						vCut2 = result;
					}
				}
			}

			// s bigger then 1
			if ( s>1.f+epsilon )
			{
				// a clear end situation
				EndCutStart = index;
				EndCutEnd		= next_index;
			}

		}

		aux_maximum_s=s;

	} // end for

	ObstacleData od1,od2;

	// now create the new triangles
	if (StartCutStart >= 0)
	{
		// start is in this triangle
		// triangle: start vertex and edge that it cuts
		od1.vPos = start; 
		od2.vPos=end;
		CreateNewTriangle(od1,m_VertexList.GetVertex(pNode->vertex[StartCutStart]),m_VertexList.GetVertex(pNode->vertex[StartCutEnd]));

		int notStart	= 3-(StartCutStart+StartCutEnd);

		if (EndCutStart >= 0)
		{
			// and end also
			// triangle: end vertex and edge that it cuts
			CreateNewTriangle(od2,m_VertexList.GetVertex(pNode->vertex[EndCutStart]),m_VertexList.GetVertex(pNode->vertex[EndCutEnd]));
			// triangle: start vertex end vertex and both end vertices
			CreateNewTriangle(od1,od2,m_VertexList.GetVertex(pNode->vertex[EndCutStart]), true);
//			m_NewCutsMap.insert(NewCutsMap::value_type( m_lstNewNodes.back(), CutEdgeIdx(0,1)) );
			f5++;
			CreateNewTriangle(od1,od2,m_VertexList.GetVertex(pNode->vertex[EndCutEnd]), true);
//			m_NewCutsMap.insert(NewCutsMap::value_type( m_lstNewNodes.back(), CutEdgeIdx(0,1)) );
			f5++;
			AddTheCut( m_VertexList.FindVertex(od1), m_VertexList.FindVertex(od2) );

			int notEnd		= 3-(EndCutStart+EndCutEnd);
			CreateNewTriangle(od1,m_VertexList.GetVertex(pNode->vertex[notStart]),m_VertexList.GetVertex(pNode->vertex[notEnd]));
		}
		else
		{
			if (FirstCutStart >= 0)
			{
				// simple start-cut case
				od2.vPos = vCut1;
				// triangle: start vertex, cut pos and both cut vertices
				f5++;
				CreateNewTriangle(od1,od2,m_VertexList.GetVertex(pNode->vertex[FirstCutStart]), true);
//				m_NewCutsMap.insert(NewCutsMap::value_type( m_lstNewNodes.back(), CutEdgeIdx(0,1)) );
				f5++;
				CreateNewTriangle(od1,od2,m_VertexList.GetVertex(pNode->vertex[FirstCutEnd]), true);
//				m_NewCutsMap.insert(NewCutsMap::value_type( m_lstNewNodes.back(), CutEdgeIdx(0,1)) );
				AddTheCut( m_VertexList.FindVertex(od1), m_VertexList.FindVertex(od2) );
				// find index of vertex that does not lie on cut edge
				int notCut = 3-(FirstCutEnd+FirstCutStart);
				// triangle: start vertex, notcut and notstart
				CreateNewTriangle(od1,m_VertexList.GetVertex(pNode->vertex[notStart]),m_VertexList.GetVertex(pNode->vertex[notCut]));
			}
			else
			{
				// nasty: start ok but end or cut into a vertex
				// two more triangles
				if (od1.vPos != m_VertexList.GetVertex(pNode->vertex[notStart]).vPos)
				{
					// od1 and notStart
					f5++;
					CreateNewTriangle(od1,m_VertexList.GetVertex(pNode->vertex[StartCutStart]),m_VertexList.GetVertex(pNode->vertex[notStart]), true);
//					m_NewCutsMap.insert(NewCutsMap::value_type( m_lstNewNodes.back(), CutEdgeIdx(0,2)) );
					f5++;
					CreateNewTriangle(od1,m_VertexList.GetVertex(pNode->vertex[StartCutEnd]),m_VertexList.GetVertex(pNode->vertex[notStart]), true);
//					m_NewCutsMap.insert(NewCutsMap::value_type( m_lstNewNodes.back(), CutEdgeIdx(0,2)) );
					AddTheCut( m_VertexList.FindVertex(od1), pNode->vertex[notStart] );
				}
			}
		}
	}
	else
	{

		// start is not in this triangle
		od1.vPos = end; 
		od2.vPos = vCut1;

		if (EndCutStart >= 0)
		{
			// but end is
			CreateNewTriangle(od1,m_VertexList.GetVertex(pNode->vertex[EndCutStart]),m_VertexList.GetVertex(pNode->vertex[EndCutEnd]));
			int notEnd = 3 - (EndCutStart + EndCutEnd);
			if (FirstCutStart >= 0)
			{
				// simple cut-end case
				f5++;
				CreateNewTriangle(od1,od2,m_VertexList.GetVertex(pNode->vertex[FirstCutStart]), true);
//				m_NewCutsMap.insert(NewCutsMap::value_type( m_lstNewNodes.back(), CutEdgeIdx(0,1)) );
				f5++;
				CreateNewTriangle(od1,od2,m_VertexList.GetVertex(pNode->vertex[FirstCutEnd]), true);
//				m_NewCutsMap.insert(NewCutsMap::value_type( m_lstNewNodes.back(), CutEdgeIdx(0,1)) );
				AddTheCut( m_VertexList.FindVertex(od1), m_VertexList.FindVertex(od2) );
				int notCut = 3 - (FirstCutStart + FirstCutEnd);
				CreateNewTriangle(od1,m_VertexList.GetVertex(pNode->vertex[notEnd]),m_VertexList.GetVertex(pNode->vertex[notCut]));
			}
			else
			{
				// od1 and notEnd
				// end ok but cut in vertex
				CreateNewTriangle(od1,m_VertexList.GetVertex(pNode->vertex[notEnd]),m_VertexList.GetVertex(pNode->vertex[EndCutStart]), true);
//				m_NewCutsMap.insert(NewCutsMap::value_type( m_lstNewNodes.back(), CutEdgeIdx(0,1)) );
				f5++;
				CreateNewTriangle(od1,m_VertexList.GetVertex(pNode->vertex[notEnd]),m_VertexList.GetVertex(pNode->vertex[EndCutEnd]), true);
//				m_NewCutsMap.insert(NewCutsMap::value_type( m_lstNewNodes.back(), CutEdgeIdx(0,1)) );
				AddTheCut( m_VertexList.FindVertex(od1), pNode->vertex[notEnd] );
				f5++;
			}
		}
		else
		{
			od1.vPos = vCut1;
			od2.vPos = vCut2;
			// this triangle contains no start and no end
			if (FirstCutStart >= 0)
			{
				int notCut1 = 3-(FirstCutStart+FirstCutEnd);
				if (SecondCutStart >= 0)
				{
					// simple cut-cut case
					// find shared vertex
					int SHARED;
					if (FirstCutStart == SecondCutStart)
						SHARED = FirstCutStart;
					else
					{
						if (FirstCutStart == SecondCutEnd)
							SHARED = FirstCutStart;
						else
							SHARED = FirstCutEnd;
					}
				
					int notCut2 = 3-(SecondCutStart+SecondCutEnd);

					f5++;
					CreateNewTriangle(od1,od2,m_VertexList.GetVertex(pNode->vertex[SHARED]), true);
//					m_NewCutsMap.insert(NewCutsMap::value_type( m_lstNewNodes.back(), CutEdgeIdx(0,1)) );
					f5++;
					CreateNewTriangle(od1,od2,m_VertexList.GetVertex(pNode->vertex[notCut1]), true);
//					m_NewCutsMap.insert(NewCutsMap::value_type( m_lstNewNodes.back(), CutEdgeIdx(0,1)) );
					AddTheCut(m_VertexList.FindVertex(od1), m_VertexList.FindVertex(od2));
					CreateNewTriangle(od1,m_VertexList.GetVertex(pNode->vertex[notCut1]),m_VertexList.GetVertex(pNode->vertex[notCut2]));
				}
				else
				{
					// od and NotCut
					// a cut is ok but otherwise the other edge hits a vertex
					f5++;
					CreateNewTriangle(od1,m_VertexList.GetVertex(pNode->vertex[FirstCutStart]),m_VertexList.GetVertex(pNode->vertex[notCut1]), true);
//					m_NewCutsMap.insert(NewCutsMap::value_type( m_lstNewNodes.back(), CutEdgeIdx(0,2)) );
					f5++;
					CreateNewTriangle(od1,m_VertexList.GetVertex(pNode->vertex[FirstCutEnd]),m_VertexList.GetVertex(pNode->vertex[notCut1]), true);
//					m_NewCutsMap.insert(NewCutsMap::value_type( m_lstNewNodes.back(), CutEdgeIdx(0,2)) );
					AddTheCut(m_VertexList.FindVertex(od1), pNode->vertex[notCut1]);
				}
			}
			else
			{
				// nasty case when nothing was cut... possibly the edge touches a triangle
				// skip - no new triangles added.
				if (TouchStart>=0)
				{
					float d1 = PointLineDistance(start,end,m_VertexList.GetVertex(pNode->vertex[TouchStart]).vPos);
					float d2 = PointLineDistance(start,end,m_VertexList.GetVertex(pNode->vertex[TouchEnd]).vPos);
//					CryWarning("!TOUCHED EDGE. Distances are %.6f and %.6f and aux_s %.6f and maximum_s:%.6f",d1,d2,aux_maximum_s,maximum_s);
					if ((d1<epsilon && d1<epsilon) &&
						(aux_maximum_s>epsilon && aux_maximum_s<1-epsilon))
					{
						// an edge has been touched
					//	CryWarning("!Nasty case. Plugged %.4f into maximum_s.",aux_maximum_s);
						maximum_s = aux_maximum_s;
					}
					else
						maximum_s = -1;
	
					AddTheCut(pNode->vertex[TouchStart], pNode->vertex[TouchEnd]);
					// we don't create new triangle - just add it to new nodes list
					m_lstNewNodes.push_back(pNode);
				}
				else
				{
//					CryWarning("!TOUCHED VERTEX. SKIP! Aux_s %.6f and maximum :%.6f",aux_maximum_s,maximum_s);
///					if (aux_maximum_s>epsilon && aux_maximum_s<1-epsilon)
//						maximum_s = aux_maximum_s;
//					else
						maximum_s = 0.01f;
				}
			}
		}
	}

	bool bAddedTriangles = (newNodesCount != m_lstNewNodes.size());
	if ( bAddedTriangles ) 
	{
		// now push old triangle for disconnection later
		m_lstOldNodes.push_back(pNode);
		VectorOfLinks::iterator vi;
		for (vi=pNode->link.begin();vi!=pNode->link.end();vi++)
		{
			GraphNode *pLink = (*vi).pLink;
			if( pLink->tag )	// this one is in candidatesForCutting list (nodes2refine)
				continue;
			if ((std::find(m_lstNewNodes.begin(),m_lstNewNodes.end(),pLink) == m_lstNewNodes.end())) // only push if its not the next triangle for refinement
				m_lstNewNodes.push_back(pLink);
		}
	}


	
	return pNextNode;
}







IAIObject * CAISystem::GetNearestObjectOfType(IAIObject * pObject, unsigned int nTypeID, float fRadius, int nOption)
{
	
	Vec3d pos = pObject->GetPos();
	AIObjects::iterator ai;
	ai = m_Objects.find(nTypeID);

	if (ai == m_Objects.end()) 
		return NULL;

	float	fRadiusSQR = fRadius*fRadius;

	CPuppet *pPuppet = 0;
	if (!pObject->CanBeConvertedTo(AIOBJECT_CPUPPET,(void**)&pPuppet))
		if(!pObject->CanBeConvertedTo(AIOBJECT_CVEHICLE,(void**)&pPuppet))
		return NULL;



	IAIObject *pRet = 0;
	float mindist = 100000000;
	for (;ai!=m_Objects.end();ai++)
	{
		if (ai->first != nTypeID)
			break;

		if (ai->second == pObject)
			continue;

		Vec3d ob_pos = (ai->second)->GetPos();
		float f = GetLengthSquared((ob_pos-pos));
		float fActivationRadius = (ai->second)->GetRadius();
		fActivationRadius*=fActivationRadius;
		if ( (f<mindist) && (f<fRadiusSQR)) 
			 if (pPuppet->m_mapDevaluedPoints.find(ai->second) == pPuppet->m_mapDevaluedPoints.end())
			 {
					if ((fActivationRadius>0) && (f>fActivationRadius))
						continue;

					if (nOption & AIFAF_VISIBLE_FROM_REQUESTER)
					{
						ray_hit rh;
						int colliders = m_pWorld->RayWorldIntersection(vectorf(ob_pos),vectorf(pos-ob_pos),ent_static|ent_terrain, rwi_stop_at_pierceable,&rh,1);
						if (colliders)
							continue;
					}

					if (nOption & AIFAF_VISIBLE_TARGET)
					{
						if (pPuppet->m_pAttentionTarget)
						{
							CAIObject *pRealTarget = pPuppet->GetMemoryOwner(pPuppet->m_pAttentionTarget);
							if (!pRealTarget)
								pRealTarget = pPuppet->m_pAttentionTarget;
							ray_hit rh;
							int colliders = m_pWorld->RayWorldIntersection(vectorf(ob_pos),vectorf(pRealTarget->GetPos()-ob_pos),ent_static|ent_terrain, rwi_stop_at_pierceable,&rh,1);
							if (colliders)
								continue;
						}
					}

					pRet = ai->second;
					mindist = f;
			}
	}



	if (pRet)
	{

		int my_group = pPuppet->GetParameters().m_nGroup;
		AIObjects::iterator gri = m_mapGroups.find(my_group);
		for (;gri!=m_mapGroups.end();gri++)
		{
			if (gri->first != my_group)
				break;

			//if (gri->second == pObject)
			//	continue;

			CPuppet *pGroupMember;
			if ((gri->second)->CanBeConvertedTo(AIOBJECT_CPUPPET,(void**)&pGroupMember))
				pGroupMember->Devalue((CAIObject*)pRet,false);
		}

	}
 


	return pRet;
}

void CAISystem::CalculatePassRadiuses()
{
//fixme - remove
//return;


	I3DEngine *p3DEngine = m_pSystem->GetI3DEngine();
	m_lstOldNodes.clear();
    m_lstOldNodes.push_back(m_pGraph->m_pSafeFirst);

	int numnodes = 0;
	
	while (!m_lstOldNodes.empty())
	{
		GraphNode *pCurrent = m_lstOldNodes.front();
		m_lstOldNodes.pop_front();

		m_pGraph->MarkNode(pCurrent);

		if (pCurrent->nBuildingID < 0)
		{
			// outdoors we calculate
			if ((pCurrent->vertex.size() == 3)) 
			{
				// find max radius that can stand in this node
				// find max passing radius between this node and all neighboors
				VectorOfLinks::iterator li;
				for(li=pCurrent->link.begin();li!=pCurrent->link.end();li++)
				{

					if ((*li).pLink == m_pGraph->m_pSafeFirst)
						continue;

					if ((*li).pLink->nBuildingID >=0 )
					{
						(*li).fMaxRadius = 100;
						continue;
					}

					if ( ((*li).nStartIndex>2) || ((*li).nStartIndex<0)) 
						m_pSystem->GetILog()->LogToConsole("\001 found bad triangle index!!");

					if ( ((*li).nEndIndex>2) || ((*li).nEndIndex<0)) 
						m_pSystem->GetILog()->LogToConsole("\001 found bad triangle index!!");


					if (IsForbidden( m_VertexList.GetVertex(pCurrent->vertex[(*li).nStartIndex]).vPos, m_VertexList.GetVertex(pCurrent->vertex[(*li).nEndIndex]).vPos))
					{
						(*li).vEdgeCenter = Vec3d(0,0,0);
						(*li).fMaxRadius = -1;
					}
					else
					{
						Vec3d vStart = m_VertexList.GetVertex(pCurrent->vertex[(*li).nStartIndex]).vPos;
						Vec3d vEnd = m_VertexList.GetVertex(pCurrent->vertex[(*li).nEndIndex]).vPos;
						vStart.z = p3DEngine->GetTerrainElevation(vStart.x, vStart.y);
						vEnd.z = p3DEngine->GetTerrainElevation(vEnd.x, vEnd.y);

						Vec3d bboxsize;
						bboxsize.Set(1.f,1.f,1.f);
						IPhysicalEntity *pEndPhys=0;
						IPhysicalEntity *pStartPhys=0;

						IPhysicalEntity **pEntityList;
						int nCount = m_pWorld->GetEntitiesInBox(vStart-bboxsize,vStart+bboxsize,pEntityList,ent_static|ent_ignore_noncolliding);
						if (nCount>0) 
						{
							int i=0;
							while (i<nCount)
							{
								pe_status_pos ppos;
								pEntityList[i]->GetStatus(&ppos);
								ppos.pos.z = vStart.z;
								if (IsEquivalent(ppos.pos, vStart,0.001f))
								{
									pStartPhys = pEntityList[i];
									break;
								}
								i++;
							}
						}

						nCount = m_pWorld->GetEntitiesInBox(vEnd-bboxsize,vEnd+bboxsize,pEntityList,ent_static|ent_ignore_noncolliding);
						if (nCount>0) 
						{
							int i=0;
							while (i<nCount)
							{
								pe_status_pos ppos;
								pEntityList[i]->GetStatus(&ppos);
								ppos.pos.z = vEnd.z;
								if (IsEquivalent(ppos.pos, vEnd,0.001f))
								{
									pEndPhys = pEntityList[i];
									break;
								}
								i++;
							}
						}

						Vec3d vStartCut = vStart;
						Vec3d vEndCut = vEnd;

						ray_hit se_hit;
						if (pEndPhys)
						{
							Vec3d vModStart = vStart+4.f*GetNormalized(vStart-vEnd);
							if (m_pWorld->CollideEntityWithBeam(pEndPhys,vModStart,(vEnd-vModStart),1.f,&se_hit))
								vEndCut = se_hit.pt;		
						}

						if (pStartPhys)
						{	
							Vec3d vModEnd = vEnd+4.f*GetNormalized(vEnd-vStart);
							if (m_pWorld->CollideEntityWithBeam(pStartPhys,vModEnd,(vStart-vModEnd),1.f,&se_hit))
								vStartCut = se_hit.pt;
						}

	/*
						ray_hit rh;
						int colliders = m_pWorld->RayWorldIntersection(vectorf(vStart),vectorf(vEnd-vStart),ent_static, rwi_stop_at_pierceable  | geom_colltype_player<<rwi_colltype_bit,&rh,1);
                        while (colliders)
						{

//							pe_status_pos status;
//								rh.pCollider->GetStatus(&status);
//							// if too flat and too close to the terrain
//							float zpos = p3DEngine->GetTerrainElevation(status.pos.x,status.pos.y);
//							if (fabs( (status.pos.z + status.BBox[1].z) - zpos)<0.4f)
//							{
//								vStartTrace = Vec3d(rh.pt.x,rh.pt.y,rh.pt.z);
//								colliders = m_pWorld->RayWorldIntersection(vectorf(vStartTrace),vectorf(vEndTrace-vStartTrace),ent_static, 0,&rh,1);
//								continue;
//							}

							Vec3d normal = Vec3d(rh.n.x,rh.n.y,rh.n.z);
							if ( normal.Dot(vEnd-vStart) > 0.00001f )
							{
								if (IsEquivalent(vStart,rh.pt,0.01f))
									break;

								vStart = Vec3d(rh.pt.x,rh.pt.y,rh.pt.z);
								if (!IsEquivalent(vStart,vEnd,0.001f))
								{
									vStart+= GetNormalized(vEnd-vStart)*0.01f;
									colliders = m_pWorld->RayWorldIntersection(vectorf(vStart),vectorf(vEnd-vStart),ent_static, rwi_stop_at_pierceable  | geom_colltype_player<<rwi_colltype_bit,&rh,1);

									pe_status_pos status;
										rh.pCollider->GetStatus(&status);
									// if too flat and too close to the terrain
									float zpos = p3DEngine->GetTerrainElevation(status.pos.x,status.pos.y);
									if (!(fabs( (status.pos.z + status.BBox[1].z) - zpos)<0.4f))
										vStartCut = vStart;
								}
								else
									break;
							}
							else
								break;
						}

						if (!IsEquivalent(vStart,vEnd,0.1f))
							colliders = m_pWorld->RayWorldIntersection(vectorf(vEnd),vectorf(vStart-vEnd),ent_static, rwi_stop_at_pierceable  | geom_colltype_player<<rwi_colltype_bit,&rh,1);
						else
							colliders = 0;

						while (colliders)
						{
							Vec3d normal = Vec3d(rh.n.x,rh.n.y,rh.n.z);
							if ( normal.Dot(vStart-vEnd) > 0.00001f )
							{
								if (IsEquivalent(vEnd,rh.pt,0.01f))
									break;

								vEnd = Vec3d(rh.pt.x,rh.pt.y,rh.pt.z);
								if (!IsEquivalent(vStart,vEnd,0.001f))
								{
									vEnd+= GetNormalized(vStart-vEnd)*0.01f;
									colliders = m_pWorld->RayWorldIntersection(vectorf(vEnd),vectorf(vStart-vEnd),ent_static, rwi_stop_at_pierceable  | geom_colltype_player<<rwi_colltype_bit,&rh,1);

									pe_status_pos status;
										rh.pCollider->GetStatus(&status);
									// if too flat and too close to the terrain
									float zpos = p3DEngine->GetTerrainElevation(status.pos.x,status.pos.y);
									if (!(fabs( (status.pos.z + status.BBox[1].z) - zpos)<0.4f))
										vEndCut = vEnd;

								}
								else
									break;
							}
							else
								break;
						}

	*/					
//						(*li).vEdgeCenter = (vStart+vEnd)/2.f;
//						(*li).fMaxRadius = ((*li).vEdgeCenter - vStart).GetLength();
						Vec3d NormStart=vStartCut, NormEnd=vEndCut;
						NormStart.z =0;
						NormEnd.z = 0;
						(*li).vEdgeCenter = (vStartCut+vEndCut)/2.f;
						Vec3d radiusCenter = (NormStart+NormEnd)/2.f;

						(*li).fMaxRadius = (radiusCenter-NormStart).GetLength();

					}
					
				}
			}
		}
		else
		{
			// all building links have maximum pass radiuses
			VectorOfLinks::iterator li;
			for(li=pCurrent->link.begin();li!=pCurrent->link.end();li++)
			{ 
				if ((*li).pLink->nBuildingID >=0)
					(*li).fMaxRadius = 100;
			}
		}

		// add unmarked links to nodelist
		VectorOfLinks::iterator vli;
		for (vli=pCurrent->link.begin();vli!=pCurrent->link.end();vli++)
		{
			if ( !(*vli).pLink->mark )
			{
				if (std::find(m_lstOldNodes.begin(),m_lstOldNodes.end(),vli->pLink) == m_lstOldNodes.end())
					m_lstOldNodes.push_back((*vli).pLink);
			}
		}
	}

	m_pGraph->ClearMarks();
}

void CAISystem::CreateNewTriangle(const ObstacleData & od1, const ObstacleData & od2, const ObstacleData & od3, bool tag)
{

int	vIdx1 = m_VertexList.AddVertex(od1);
int	vIdx2 = m_VertexList.AddVertex(od2);
int	vIdx3 = m_VertexList.AddVertex(od3);

	if( vIdx1==vIdx2 || vIdx1==vIdx3 || vIdx2==vIdx3 )
	{	
		// it's degenrate
		return;
	}

	GraphNode* pNewNode = m_pGraph->CreateNewNode(true);
//	if ((od1.vPos==od2.vPos) || (od1.vPos==od3.vPos) || (od2.vPos==od3.vPos))
//		int a=5;
	
	pNewNode->vertex.push_back(vIdx1);
	//pNewNode->vertex.push_back(od1);
	pNewNode->vertex.push_back(vIdx2);
	//pNewNode->vertex.push_back(od2);
	pNewNode->vertex.push_back(vIdx3);
	//pNewNode->vertex.push_back(od3);

	m_pGraph->FillGraphNodeData(pNewNode);
	m_lstNewNodes.push_back(pNewNode);
	if(tag)
		m_pGraph->TagNode( pNewNode );

	return;


/*
	GraphNode* pNewNode = m_pGraph->CreateNewNode(true);
	if ((od1.vPos==od2.vPos) || (od1.vPos==od3.vPos) || (od2.vPos==od3.vPos))
		int a=5;
	
	pNewNode->vertex.push_back(m_VertexList.AddVertex(od1));
	//pNewNode->vertex.push_back(od1);
	pNewNode->vertex.push_back(m_VertexList.AddVertex(od2));
	//pNewNode->vertex.push_back(od2);
	pNewNode->vertex.push_back(m_VertexList.AddVertex(od3));
	//pNewNode->vertex.push_back(od3);

	m_pGraph->FillGraphNodeData(pNewNode);
	m_lstNewNodes.push_back(pNewNode);
	if(tag)
		m_pGraph->TagNode( pNewNode );
*/
}

//////////////////////////////////////////////////////////////////////////


bool CAISystem::DEBUG_LISTCORRUPT(ListNodes & lstNodes)
{
	ListNodes::iterator li;
	for (li=lstNodes.begin();li!=lstNodes.end();li++)
	{
		if ((*li)->link.size() != 3) 
			return true;
	}
	return false;
}

bool CAISystem::IsForbidden(const Vec3d & start, const Vec3d & end)
{
	if (m_mapForbiddenAreas.empty())
		return false;

	DesignerPathMap::iterator di;
	for (di=m_mapForbiddenAreas.begin();di!=m_mapForbiddenAreas.end();di++)
	{
		ListPositions lstPoints = di->second;
		if (lstPoints.empty())
			continue;

		ListPositions::iterator li,linext;
		for (li=lstPoints.begin();li!=lstPoints.end();li++)
		{
			linext = li;
			linext++;
			if (linext == lstPoints.end())
				linext = lstPoints.begin();

			Vec3d prev = (*li);
			prev.z = 0;
			Vec3d next = (*linext);
			next.z = 0;

			if  ((prev==start && next==end) || (prev==end)&&(next==start))
				return true;

			Vec3d s,e;
			s = start; e=end;
			s.z=e.z=0;
			if ( PointsOnLine( prev,next,s,e )  )
				return true;

				
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAISystem::CheckInside(const Vec3d & pos, int & nBuildingID, IVisArea *&pAreaID, bool bSkipSpecialAreas)
{
	if (!m_mapSpecialAreas.empty()  && !bSkipSpecialAreas)
	{
		SpecialAreaMap::iterator si;
		for (si=m_mapSpecialAreas.begin();si!=m_mapSpecialAreas.end();si++)
		{
			SpecialArea &sa = si->second;
			ListPositions::iterator linext;

			if (PointInsidePolygon(sa.lstPolygon, pos))//&& 
				//(pos.z > sa.fMinZ) &&
				//(pos.z < sa.fMaxZ) )
			{
				if ((sa.fHeight>0.00001f) &&
					((pos.z < sa.fMinZ) ||
					(pos.z > sa.fMaxZ)) )
					continue;

				if (sa.nBuildingID<0)
				{
					nBuildingID =m_BuildingIDManager.GetId();
					(si->second).nBuildingID = nBuildingID;
				}
				else
					nBuildingID = sa.nBuildingID;

				I3DEngine *p3dEngine=m_pSystem->GetI3DEngine();
				pAreaID = p3dEngine->GetVisAreaFromPos(pos);;
				return true;
			}
		}
	}
	

	{
 		I3DEngine *p3dEngine=m_pSystem->GetI3DEngine();
		IVisArea *pArea=p3dEngine->GetVisAreaFromPos(pos);
		if (pArea)
		{
			IntIntMultiMap::iterator imi;
			imi = m_mapBuildingMap.find((INT_PTR)pArea);		//AMD Port
			if (imi!=m_mapBuildingMap.end())
			{
				nBuildingID=imi->second;
				pAreaID=pArea;
			}
			else
			{
				
				std::list<IVisArea*>::iterator si;
				lstSectors.push_back(pArea);
				m_nNumBuildings = m_BuildingIDManager.GetId();
				while (!lstSectors.empty())
				{
					IVisArea *pCurrent = lstSectors.front();
					lstSectors.pop_front();

					if (m_mapBuildingMap.find((INT_PTR)pCurrent) == m_mapBuildingMap.end())									//AMD Port
					{
						
						m_mapBuildingMap.insert(IntIntMultiMap::iterator::value_type((INT_PTR)pCurrent,m_nNumBuildings));	//AMD Port

						
						int nr =pCurrent->GetVisAreaConnections(&m_pAreaList[0],100);
	#ifdef WIN32
						if (100 < nr)
							CryError("[AISYSTEM] More than 100 connestions from a Vis area. Lets change the function.");
	#endif

						for (int i=0;i<nr;i++)
						{
						if (m_mapBuildingMap.find((INT_PTR)m_pAreaList[i]) == m_mapBuildingMap.end())						//AMD Port
							lstSectors.push_back(m_pAreaList[i]);
						}
					}
				}
				
				nBuildingID = m_nNumBuildings;
				pAreaID = pArea;
			}
			return (true);
		}
	}
	return false;
}




bool CAISystem::PointInsidePolygon(ListPositions & lstPolygon, const Vec3d & pos)
{
	ListPositions::iterator linext;
	int  wndCount = 0;    

	// loop through all edges of the polygon
	for (ListPositions::iterator li = lstPolygon.begin();li!=lstPolygon.end();li++)
	{
		linext = li;
		linext++;
		if (linext == lstPolygon.end())
			linext = lstPolygon.begin();
	
		if ((*li).y <= pos.y) 
		{        
			if ((*linext).y > pos.y)      
				if ( (((*linext).x - (*li).x) * (pos.y - (*li).y) - (pos.x - (*li).x) * ((*linext).y - (*li).y)) > 0) 
						wndCount++;           
		}
		else 
		{                       
			if ((*linext).y <= pos.y)     
				if ( (((*linext).x - (*li).x) * (pos.y - (*li).y) - (pos.x - (*li).x) * ((*linext).y - (*li).y)) < 0)  
					wndCount--;
		}
	}
	
	if (wndCount)
		return true;

	return false;
}


//
//--------------------------------------------------------------------------------------
float CAISystem::PointLineDistance(const Vec3d & vLineStart, const Vec3d & vLineEnd, const Vec3d & vPoint)
{
	Vec3d dir;
	Vec3d point_vector;

	if ((vPoint-vLineStart).len2()<(vPoint-vLineEnd).len2())
	{
		dir = vLineStart-vLineEnd;
		point_vector = vPoint - vLineEnd;
	}
	else
	{
		dir = vLineEnd-vLineStart;
	    point_vector = vPoint - vLineStart;
	}	

	dir.Normalize();
	float t0 = dir.Dot(GetNormalized(point_vector));

	return (point_vector - t0*point_vector).GetLength();
	
}


//
//--------------------------------------------------------------------------------------
bool CAISystem::PointOnLine(const Vec3d & vLineStart, const Vec3d & vLineEnd, const Vec3d & vPoint, float fPrecision)
{
Vec3d vLine;
Vec3d vDir;



	if( (vPoint-vLineStart).len2()>(vPoint-vLineEnd).len2() )
	{
		vLine = vLineEnd-vLineStart;
		vDir = vPoint-vLineStart;
	}
	else
	{
		vLine = vLineStart-vLineEnd;
		vDir = vPoint-vLineEnd;
	}

	float	cosine = vLine.normalized().Dot( vDir.normalized() );

	if(cosine<0.0f)
		return false;

	Vec3d vProjection = vDir*cosine;

	if( vProjection.len2() > vLine.len2() )
		return false;			// of segment

	float dist = (vDir*(1.0f-cosine)).len2();

	if (fPrecision>0.f)
	{
		if( dist>fPrecision )
			return false;	// too far from line
	}
	else
	{
		if( dist>g_POINT_DIST_PRESITION )
			return false;	// too far from line
	}

	return true;
}

//
//--------------------------------------------------------------------------------------
bool CAISystem::PointsOnLine(const Vec3d & vLineStart_, const Vec3d & vLineEnd_, const Vec3d & vPoint1_, const Vec3d & vPoint2_)
{

Vec3d vLineStart = vLineStart_;
Vec3d vLineEnd = vLineEnd_;
Vec3d vPoint1 = vPoint1_;
Vec3d vPoint2 = vPoint2_;

vLineStart.z = vLineEnd.z = vPoint1.z = vPoint2.z = 0.0f;

	if ((IsEquivalent(vPoint1,vLineStart,0.001f) && IsEquivalent(vPoint2,vLineEnd,0.001f)) ||
		(IsEquivalent(vPoint2,vLineStart,0.001f) && IsEquivalent(vPoint1,vLineEnd,0.001f)))
		return true;

	if	(IsEquivalent(vPoint1,vLineStart,0.001f) && PointOnLine(vLineStart, vLineEnd, vPoint2) )
		return true;

	if	(IsEquivalent(vPoint1,vLineEnd,0.001f) && PointOnLine(vLineStart, vLineEnd, vPoint2) )
		return true;


	if	(IsEquivalent(vPoint2,vLineStart,0.001f) && PointOnLine(vLineStart, vLineEnd, vPoint1) )
		return true;

	if	(IsEquivalent(vPoint2,vLineEnd,0.001f) && PointOnLine(vLineStart, vLineEnd, vPoint1) )
		return true;

	if	(PointOnLine(vLineStart, vLineEnd, vPoint1) && PointOnLine(vLineStart, vLineEnd, vPoint2) )
		return true;


	return false;
}



bool CAISystem::SegmentsIntersect(const Vec3d & vSegmentAStart, const Vec3d & vSegmentADir, const Vec3d & vSegmentBStart, const Vec3d & vSegmentBDir, float & fCutA, float & fCutB)
{

	Vec3d delta = vSegmentBStart-vSegmentAStart;
	float crossD = vSegmentADir.x*vSegmentBDir.y-vSegmentADir.y*vSegmentBDir.x;
	float crossDelta1 = delta.x*vSegmentBDir.y-delta.y*vSegmentBDir.x;
	float crossDelta2 = delta.x*vSegmentADir.y-delta.y*vSegmentADir.x;

	if ( fabs(crossD) > epsilon )
	{
		// intersection
		fCutA=crossDelta1/crossD;
		fCutB=crossDelta2/crossD;
	}
	else
		return false;

	float cm2 = cm_epsilon*cm_epsilon;

	if (fCutA > (1.f+epsilon))
	{
		Vec3d vActualDir = vSegmentADir*(fCutA-1.f);
		if (vActualDir.len2()>cm2)
			return false;
	}
	else if (fCutA < (0.f-epsilon))
	{
		Vec3d vActualDir = vSegmentADir*(-fCutA);
		if (vActualDir.len2()>cm2)
			return false;
	}

	if (fCutB > (1.f+epsilon))
	{
		Vec3d vActualDir = vSegmentBDir*(fCutB-1.f);
		if (vActualDir.len2()>cm2)
			return false;
	}
	else if (fCutB < (0.f-epsilon))
	{
		Vec3d vActualDir = vSegmentBDir*(-fCutB);
		if (vActualDir.len2()>cm2)
			return false;
	}

	return true;
}

bool CAISystem::TriangleLineIntersection(GraphNode * pNode, const Vec3d & vStart, const Vec3d & vEnd)
{
	Vec3d P0 = vStart;
	Vec3d D0 = vEnd - vStart;
	unsigned int index,next_index;
	for (index=0;index<pNode->vertex.size();index++)
	{
		Vec3d P1,D1;
		next_index = index+1;
		if (next_index == pNode->vertex.size())		// make sure we wrap around correctly
			next_index = 0;

		//get the triangle edge
		P1=m_VertexList.GetVertex(pNode->vertex[index]).vPos;
		D1=m_VertexList.GetVertex(pNode->vertex[next_index]).vPos - P1;

		float s=-1,t=-1;
		SegmentsIntersect(P0,D0,P1,D1,s,t);

		if ((s>0.f-epsilon) && (s<1.f+epsilon) && (t>0.f+epsilon) && (t<1.f-epsilon))
			return true;

	}
	return false;
}

bool CAISystem::SegmentInTriangle(GraphNode * pNode, const Vec3d & vStart, const Vec3d & vEnd)
{
	if(m_pGraph->PointInTriangle( (vStart+vEnd)*.5f, pNode))
		return true;
//	if(m_pGraph->PointInTriangle( vEnd, pNode))
//		return true;
	return TriangleLineIntersection(pNode, vStart, vEnd);
}



bool CAISystem::CreatePath(const char * szPathName, EnumAreaType eAreaType, float fHeight)
{

	if (eAreaType==AREATYPE_PATH)
	{
		DesignerPathMap::iterator di;
		di = m_mapDesignerPaths.find(szPathName);

		ListPositions newpos;
		newpos.clear();
		if (di== m_mapDesignerPaths.end())
		{
			// insert new
			m_mapDesignerPaths.insert(DesignerPathMap::iterator::value_type(szPathName,newpos));
			di = m_mapDesignerPaths.find(szPathName);
		}
		else
			return false;
	}
	else if (eAreaType==AREATYPE_FORBIDDEN)
	{
		DesignerPathMap::iterator di;
		di = m_mapForbiddenAreas.find(szPathName);

		if (di== m_mapForbiddenAreas.end())
		{
			ListPositions newpos;
			newpos.clear();
			// insert new
			m_mapForbiddenAreas.insert(DesignerPathMap::iterator::value_type(szPathName,newpos));
			di = m_mapForbiddenAreas.find(szPathName);
		}
		else
			return false;
	}
	else if (eAreaType == AREATYPE_NAVIGATIONMODIFIER)
	{
		SpecialAreaMap::iterator di;
		di = m_mapSpecialAreas.find(szPathName);

		if (di== m_mapSpecialAreas.end())
		{
			SpecialArea sa;
			sa.lstPolygon.clear();
			sa.nBuildingID = -1;
			sa.fHeight = fHeight;

			// insert new
			m_mapSpecialAreas.insert(SpecialAreaMap::iterator::value_type(szPathName,sa));
		}
		else
			return false;
	}
	else if (eAreaType == AREATYPE_OCCLUSION_PLANE)
	{
		DesignerPathMap::iterator di;
		di = m_mapOcclusionPlanes.find(szPathName);

		if (di== m_mapOcclusionPlanes.end())
		{
			ListPositions newpos;
			newpos.clear();
			// insert new
			m_mapOcclusionPlanes.insert(DesignerPathMap::iterator::value_type(szPathName,newpos));
			di = m_mapOcclusionPlanes.find(szPathName);
		}
		else
			return false;
	}

	return true;
}

bool CAISystem::BehindForbidden(const Vec3d & vStart, const Vec3d & vEnd, string &forb)
{
	if (m_mapForbiddenAreas.empty())
		return false;

	DesignerPathMap::iterator fi = m_mapForbiddenAreas.begin();
	DesignerPathMap::iterator iend = m_mapForbiddenAreas.end();
	while (fi!=iend)
	{
		if (PointInsidePolygon(fi->second,vStart) != PointInsidePolygon(fi->second,vEnd))
		{
			forb = fi->first;
			return true;
		}
		++fi;
	}
	return false;
}

bool CAISystem::NoFriendInVicinity(const Vec3d & vPos, float fRadius , CPipeUser * pChecker)
{
	AIObjects::iterator ai;

	for (ai = m_mapGroups.find(pChecker->GetParameters().m_nGroup);ai!= m_mapGroups.end();)
	{
		if (ai->first != pChecker->GetParameters().m_nGroup)
			break;
		CAIObject *pObject = ai->second;
		CPipeUser *pPipeUser;
		if (pObject->CanBeConvertedTo(AIOBJECT_CPIPEUSER,(void**) &pPipeUser))
		{
			if (pPipeUser != pChecker)
			{
				Vec3d dir = pPipeUser->GetPos() - vPos;
				if (dir.GetLength()<fRadius)
					return false;
			}
		}
		ai++;
	}
	return true;
}

bool CAISystem::NoSameHidingPlace(CPipeUser * pHider, const Vec3d & vPos)
{
	AIObjects::iterator ai;

	for (ai = m_mapGroups.find(pHider->GetParameters().m_nGroup);ai!= m_mapGroups.end();)
	{
		if (ai->first != pHider->GetParameters().m_nGroup)
			break;
		CAIObject *pObject = ai->second;
		CPipeUser *pPipeUser;
		if (pObject->CanBeConvertedTo(AIOBJECT_CPIPEUSER,(void**) &pPipeUser))
		{
			if (pPipeUser != pHider)
			{
				Vec3d dir = pPipeUser->m_vLastHidePos - vPos;
				if (dir.GetLength()<2)
					return false;
			}
		}
		ai++;
	}
	return true;
}

bool CAISystem::CrowdControl(CPipeUser * pMain, const Vec3d &pos)
{
	AIObjects::iterator ai;

	for (ai=m_mapSpecies.find(pMain->GetParameters().m_nSpecies);ai!=m_mapSpecies.end();)
	{
		if (ai->first != pMain->GetParameters().m_nSpecies)
			break;
		CAIObject *pObject = ai->second;
		CPipeUser *pOther;
		if (pObject->CanBeConvertedTo(AIOBJECT_CPIPEUSER, (void**) &pOther))
		{
			if (pOther != pMain)
			{

				Vec3d vOtherPos = pOther->GetPos();
				Vec3d vMainPos = pMain->GetPos();

				if (pOther->m_pReservedNavPoint)
				{
					// the point I am going to is reserved..
					if ( (pOther->m_pReservedNavPoint->GetPos() - pos).len2() < 3.f )
						return true;
					return false;
				}
 
				
				if (((vOtherPos-pos).len2() < 3.f) && (!pOther->m_AvoidingCrowd))
				{

					Vec3d vNextPos = pos;
					if (pMain->m_lstPath.size() > 1)
					{
						vNextPos = (*++pMain->m_lstPath.begin());
					}
					GraphNode *pNode = m_pGraph->GetThirdNode(vOtherPos,vMainPos,vNextPos);
					if (pNode)
					{
						Vec3d vNewPos = pNode->data.m_pos;
						pOther->m_lstPath.push_front(vOtherPos);
						pOther->m_lstPath.push_front(vNewPos);
						pOther->m_AvoidingCrowd = true;
						return true;
					}
				}
					

			}
		}
		ai++;
	}

	return false;
}



void CAISystem::SingleDryUpdate(CPuppet * pObject)
{
	FUNCTION_PROFILER( m_pSystem,PROFILE_AI );
	pObject->m_bDryUpdate = true;
	if (pObject->m_bEnabled)
		pObject->Update();
}

bool CAISystem::SingleFullUpdate(CPuppet * pPuppet)
{
	FUNCTION_PROFILER( m_pSystem,PROFILE_AI );

		AIObjects::iterator ai;


		pPuppet->m_bCloseContact = false;

		if (pPuppet->m_bEnabled)
		{
			
				if (!m_lstVisible.empty())
				{
					FRAME_PROFILER( "AIPlayerVisibilityCheck", m_pSystem,PROFILE_AI);
					ListOfUnsignedShort::iterator li;
					for (li=m_lstVisible.begin();li!=m_lstVisible.end();li++)
					{
						ai = m_Objects.find((*li));
						if (ai!=m_Objects.end())
						{
							Vec3d ppos = pPuppet->GetPos();
							while ((ai != m_Objects.end()) && (ai->first == (*li)))
							{
								CAIObject *pTarget = ai->second;
								if (!pTarget->m_bEnabled)
								{
									ai++;
									continue;
								}

								float	dist = (ppos - pTarget->GetPos()).GetLength();

								CAIPlayer *pPlayer = 0;
								if (pTarget->CanBeConvertedTo(AIOBJECT_PLAYER,(void**)&pPlayer))
								{
									// for puppets of same species just report melee - dont shoot rays
									if (pPlayer->m_Parameters.m_nSpecies==pPuppet->GetParameters().m_nSpecies)
									{
										if (pTarget->m_bEnabled)
										{
											if( dist<pPuppet->GetParameters().m_fMeleeDistance)
												pPuppet->SetSignal(1, "OnCloseContact",pTarget->GetAssociation());		
										}
										ai++;
										continue;
									}
								}


								if (pPuppet->PointVisible(pTarget->GetPos()))
								{
									bool bFuzzy = false;
									ray_hit hit;
									f2++;

									int rwi = 0;
									if ((pTarget->GetPos()-ppos).GetLength()>f5)
										f5 = (pTarget->GetPos()-ppos).GetLength();
									if (m_cvDebugDraw->GetIVal())
										f4+=m_pSystem->GetITimer()->MeasureTime("");
									IPhysicalEntity *skip = pPuppet->GetProxy()->GetPhysics();
									if (pPuppet->m_bHaveLiveTarget)
									{
										ray_hit hit_aux[2];
										rwi = m_pWorld->RayWorldIntersection(vectorf(ppos),vectorf(pTarget->GetPos()-ppos),ent_terrain|ent_static|ent_sleeping_rigid, 
										0,&hit_aux[0],2, skip, m_pTheSkip);
										if (rwi)
										{
											if (hit_aux[0].dist < 0) 
											{
												float fBounce,fFriction;
												unsigned int flags = sf_pierceable_mask;
												m_pWorld->GetSurfaceParameters(hit_aux[1].surface_idx, fBounce,fFriction,flags);
												rwi = 0;
												if (flags<=13)
												{
													bFuzzy = true;
												}
											}
											hit = hit_aux[0];
										}
									}
									else
									{
										rwi = m_pWorld->RayWorldIntersection(vectorf(ppos),vectorf(pTarget->GetPos()-ppos),ent_terrain|ent_static|ent_sleeping_rigid, 
										13,&hit,1, skip, m_pTheSkip);
									}

									m_nNumRaysShot++;
									m_nRaysThisUpdateFrame++;

									if (rwi && !pPuppet->m_bHaveLiveTarget)
									{
										if ( hit.n.Dot(pTarget->GetPos()-ppos) > 0 )
										{
											rwi = m_pWorld->RayWorldIntersection(vectorf(hit.pt),vectorf(pTarget->GetPos()-hit.pt),ent_terrain|ent_static|ent_sleeping_rigid, 
												13,&hit,1, skip, m_pTheSkip);
											m_nNumRaysShot++;
											m_nRaysThisUpdateFrame++;
										}
									}
									else if (!rwi && !pPuppet->m_bHaveLiveTarget)
									{
										rwi = RayOcclusionPlaneIntersection(ppos,pTarget->GetPos());
									}

									if (m_cvDebugDraw->GetIVal())
									{
										float f = m_pSystem->GetITimer()->MeasureTime("");
										f1+=f;
										f4+=f;
									}

									if (!pPuppet->GetParameters().m_bSmartMelee && dist<pPuppet->GetParameters().m_fMeleeDistance)
										bFuzzy= false; 

									

									if (!rwi)				
									{
										// notify puppet that it sees something
										SAIEVENT event;
										CAIAttribute *pAttrib = 0;
										if (pTarget->CanBeConvertedTo(AIOBJECT_ATTRIBUTE,(void**)&pAttrib))
										{
											event.pSeen = pAttrib->GetPrincipalObject();
											if (!event.pSeen)
											{
												ai++;
												continue;
											}
											event.fThreat = 5.f; // 10 times more visible than normal
										}
										else
										{
											event.fThreat = 1.f; // normal visibility
											event.pSeen = pTarget;
										}
										

										event.bFuzzySight = bFuzzy;
                                        pPuppet->Event(AIEVENT_ONVISUALSTIMULUS,&event);

										if (pTarget->m_bEnabled && ((pTarget->GetType()==AIOBJECT_PLAYER) || (pTarget->GetType()==AIOBJECT_PUPPET) ))
										{
											if (pPuppet->GetAttentionTarget() == pTarget)
											{
												if( dist<pPuppet->GetParameters().m_fMeleeDistance)
													pPuppet->SetSignal(1, "OnCloseContact",pTarget->GetAssociation());		
											}
										}

									}
									else if (!pPuppet->GetParameters().m_bSmartMelee)
									{ 
											if (pTarget->m_bEnabled && ((pTarget->GetType()==AIOBJECT_PLAYER) || (pTarget->GetType()==AIOBJECT_PUPPET) ))
											{
												SAIEVENT event;
												event.fThreat = 1.f;
												event.bFuzzySight = false;
												if( dist<pPuppet->GetParameters().m_fMeleeDistance)
												{
													ray_hit hit;
													int rwi=0;
													IPhysicalEntity *skip = pPuppet->GetProxy()->GetPhysics();
													rwi = m_pWorld->RayWorldIntersection(vectorf(ppos),vectorf(pTarget->GetPos()-ppos),ent_terrain|ent_static|ent_sleeping_rigid, 
												rwi_stop_at_pierceable  | geom_colltype_player<<rwi_colltype_bit,&hit,1, skip, m_pTheSkip);
													if (!rwi)
													{
														event.pSeen = pTarget;
														pPuppet->Event(AIEVENT_ONVISUALSTIMULUS,&event);		
													}
												}
											}
									}

								} // if point visible


								ai++;

							}// while (ai->first == type)

						}// if type found

					}// for all types in visibility list

				}// lstVisible


				// check visibility between enemies after checking for visibility of player


				if (pPuppet->GetType() == AIOBJECT_VEHICLE)
					CheckVisibility( pPuppet, AIOBJECT_VEHICLE);
				else
					CheckVisibility( pPuppet, AIOBJECT_PUPPET);

		}




		//f3++;		
		//update the puppet
		float fTime;
		pPuppet->m_bDryUpdate = false;

		if (m_cvAllTime->GetIVal())
			m_pSystem->GetITimer()->MeasureTime("");

		pPuppet->Update();

		{
			FRAME_PROFILER( "AISFU:TimeMeasuringBlock", m_pSystem,PROFILE_AI);
		if (m_cvAllTime->GetIVal())
		{
			fTime =m_pSystem->GetITimer()->MeasureTime("");
			TimingMap::iterator ti = m_mapDEBUGTiming.find(pPuppet->GetName());
			if (ti!=m_mapDEBUGTiming.end())
				m_mapDEBUGTiming.erase(ti);
			m_mapDEBUGTiming.insert(TimingMap::iterator::value_type(pPuppet->GetName(),fTime));
		}

	if (m_cvDebugDraw->GetIVal())
		f4+=m_pSystem->GetITimer()->MeasureTime("");
		}

	if (m_nRaysThisUpdateFrame>m_cvAIVisRaysPerFrame->GetIVal())
			return true;

		return false;
}

void CAISystem::CheckVisibility(CPuppet * pPuppet, unsigned short typeToCheck)
{
	FUNCTION_PROFILER( m_pSystem,PROFILE_AI );
	AIObjects::iterator ai;

	ai=m_Objects.find(typeToCheck);

	if(ai == m_Objects.end())
		return;

	for (;ai!=m_Objects.end();)
	{
		if (ai->first != typeToCheck)
			break;



		CPuppet *pOtherPuppet = (CPuppet *) ai->second;

		if (!pOtherPuppet->m_bEnabled || pOtherPuppet->m_bCloaked)
		{
			ai++;
			continue;
		}

		Vec3d onepos,twopos;
		onepos = pPuppet->GetPos();
		twopos = pOtherPuppet->GetPos();

		bool bNotifyFirst = true;
		bool bNotifySecond = true;
		if (!pPuppet->PointVisible(twopos))
			bNotifyFirst = false;

		if (!pOtherPuppet->PointVisible(onepos))
			bNotifySecond = false;

		if (!bNotifySecond && !bNotifyFirst) 
		{
			ai++;
			continue;
		}


		if (pPuppet->GetParameters().m_nSpecies != pOtherPuppet->GetParameters().m_nSpecies)
		{
			ray_hit hit[2];
			hit[0].dist=-1.f;
			//ray_hit hit;
			//--------------- ACCURATE MEASURING
			float dist = (twopos-onepos).GetLength();
			if (dist>f5)
				f5 = dist;
			if (m_cvDebugDraw->GetIVal())
				f4+=m_pSystem->GetITimer()->MeasureTime("");
			f2++;
			int rayresult;
			IPhysicalEntity *skip = pPuppet->GetProxy()->GetPhysics();
			if (pPuppet->m_bHaveLiveTarget)
				rayresult = m_pWorld->RayWorldIntersection(vectorf(onepos),vectorf(twopos-onepos),ent_terrain|ent_static|ent_sleeping_rigid, 
				0,&hit[0],2, skip);
			else
				rayresult = m_pWorld->RayWorldIntersection(vectorf(onepos),vectorf(twopos-onepos),ent_terrain|ent_static|ent_sleeping_rigid, 
				13,&hit[0],2, skip);
			m_nNumRaysShot++;
			m_nRaysThisUpdateFrame++;

			if (m_cvDebugDraw->GetIVal())
			{
				float f =m_pSystem->GetITimer()->MeasureTime("");
				f1+=f;
				f4+=f;
			}

			//--------------- ACCURATE MEASURING
			if (!rayresult || (hit[0].dist<0))				
			{
				// notify puppet that it sees something
				if (bNotifyFirst && !pOtherPuppet->m_bCloaked)
				{
					SAIEVENT event;
					event.pSeen = pOtherPuppet;
					event.bFuzzySight = false;
					pPuppet->Event(AIEVENT_ONVISUALSTIMULUS,&event);
				}
				if (bNotifySecond && !pPuppet->m_bCloaked)
				{
					SAIEVENT event;
					event.pSeen = pPuppet;
					event.bFuzzySight = false;
					pOtherPuppet->Event(AIEVENT_ONVISUALSTIMULUS,&event);
				}

				if (pOtherPuppet->m_bEnabled && ((pOtherPuppet->GetType()==AIOBJECT_PLAYER) || (pOtherPuppet->GetType()==AIOBJECT_PUPPET) ))
				{
					if (pPuppet->GetAttentionTarget()==pOtherPuppet)
					{
						if( dist<pPuppet->GetParameters().m_fMeleeDistance)
							pPuppet->SetSignal(1, "OnCloseContact",pOtherPuppet->GetAssociation());		
					}
				}

			}



		}
		ai++;
	}
}

Vec3d CAISystem::IntersectPolygon(const Vec3d & start, const Vec3d & end, ListPositions & lstPolygon)
{
	Vec3d start_closest = end;
	float min_dist = (end-start).len2();
	ListPositions::iterator iend = lstPolygon.end();
	ListPositions::iterator li, linext;
	for (li=lstPolygon.begin();li!=iend;++li)
	{
		linext = li;
		++linext;
		if (linext == iend)
			linext = lstPolygon.begin();

		float s,t;

		if (SegmentsIntersect(start,end-start,(*li), (*linext)-(*li),s,t))
		{
			if (s<0.00001f || s>0.99999f || t<0.00001f || t>0.99999f)
				continue;
			Vec3d new_intersection = start + s*(end-start);
			float new_distance = (start-new_intersection).len2();
			if (new_distance < min_dist)
			{
				min_dist = new_distance;
				start_closest = new_intersection;
			}
		}
	}
	return start_closest;
}

bool CAISystem::BehindSpecialArea(const Vec3d & vStart, const Vec3d & vEnd, string & strSpecial)
{
	SpecialAreaMap::iterator iend = m_mapSpecialAreas.end();
	for (SpecialAreaMap::iterator si=m_mapSpecialAreas.begin();si!=iend;++si)
	{
		SpecialArea sa = si->second;
		ListPositions::iterator linext;

		if (PointInsidePolygon(sa.lstPolygon, vStart) != PointInsidePolygon(sa.lstPolygon, vEnd) )
		{
			strSpecial = si->first;
			return true;
		}
	}
	return false;
}

bool CAISystem::IntersectsForbidden(const Vec3d & vStart, const Vec3d & vEnd, Vec3d & vClosestPoint)
{
	if (m_mapForbiddenAreas.empty())
		return false;

	DesignerPathMap::iterator fi = m_mapForbiddenAreas.begin();
	DesignerPathMap::iterator iend = m_mapForbiddenAreas.end();
	while (fi!=iend)
	{
		Vec3d result = IntersectPolygon(vStart,vEnd,fi->second);
		if (!IsEquivalent(result,vEnd,0.001f))
		{
			vClosestPoint = result;
			return true;
		}
		++fi;
	}
	return false;
}

bool CAISystem::IntersectsSpecialArea(const Vec3d & vStart, const Vec3d & vEnd, Vec3d & vClosestPoint)
{
	if (m_mapSpecialAreas.empty())
		return false;

	SpecialAreaMap::iterator fi = m_mapSpecialAreas.begin();
	SpecialAreaMap::iterator iend = m_mapSpecialAreas.end();
	while (fi!=iend)
	{
		Vec3d result = IntersectPolygon(vStart,vEnd,fi->second.lstPolygon);
		if (!IsEquivalent(result,vEnd,0.001f))
		{
			vClosestPoint = result;
			return true;
		}
		++fi;
	}
	return false;
}

bool CAISystem::ForbiddenAreasOverlap(void)
{
	if (m_mapForbiddenAreas.empty())
		return false;


	DesignerPathMap::iterator fi,iend = m_mapForbiddenAreas.end();
	for (fi=m_mapForbiddenAreas.begin();fi!=iend;++fi)
	{
		ListPositions &lstPolygon = fi->second;
		ListPositions::iterator li,linext,liend = lstPolygon.end();
		for (li=lstPolygon.begin();li!=liend;)
		{
			linext = li;
			++linext;
			if (linext == liend)
				linext = lstPolygon.begin();

			if ( IsEquivalent((*li),(*linext),0.0001f) )
			{
				AIError("!Forbidden area %s contains one or more identical points (difference less than 0.0001 (100 nanometers).",(fi->first).c_str());
				AIError("!The system will remove excess points, but IT IS RECOMMENDED that you delete it and recreate it.");
				li=(fi->second).erase(li);
				continue;
			}

			Vec3d closest_point;
			if (IntersectsForbidden((*li),(*linext),closest_point))
			{
				AIError("!Forbidden area %s intersects with another forbidden area. Please fix and re-triangulate, grid is in unstable state.",(fi->first).c_str());
				return true;
			}

			++li;
		}
	}

	return false;
}


GraphNode*	CAISystem::FindMarkNodeBy2Vertex( int vIdx1, int vIdx2, GraphNode* exclude )
{
ListNodes::iterator nItr=m_lstNewNodes.begin();

	for(;nItr!=m_lstNewNodes.end(); nItr++)
	{
		GraphNode* curNode = (*nItr);
		if  ((curNode == exclude) || (curNode == m_pGraph->m_pSafeFirst))
			continue;
		VectorOfLinks::iterator ilink;
		for (ilink=curNode->link.begin();ilink!=curNode->link.end();ilink++)
			if( curNode->vertex[(*ilink).nStartIndex] == vIdx1 && curNode->vertex[(*ilink).nEndIndex] == vIdx2 ||
				curNode->vertex[(*ilink).nStartIndex] == vIdx2 && curNode->vertex[(*ilink).nEndIndex] == vIdx1 )
			{
//				segCounter++;
				(*ilink).fMaxRadius = -1.f;
				return curNode;
			}
			
	}
	return NULL;
}

void CAISystem::AddTheCut( int vIdx1, int vIdx2 )
{
	assert( vIdx1>=0 && vIdx2>=0 );
	if( vIdx1 == vIdx2 )
		return;
	m_NewCutsVector.push_back(CutEdgeIdx(vIdx1,vIdx2));
}


void	CAISystem::CreatePossibleCutList( const Vec3d & vStart, const Vec3d & vEnd, ListNodes & lstNodes )
{
float		offset=.5f;
GraphNode *pCurNode = m_pGraph->GetEnclosing(vStart+offset*(vEnd-vStart),0,true);
//m_pGraph->GetEnclosing(vStart+0.0001f*(GetNormalized(vEnd-vStart)),0,true);
ListNodes queueList;


	lstNodes.clear();
	queueList.clear();
	m_pGraph->ClearTagsNow();

	while( !SegmentInTriangle(pCurNode, vStart, vEnd) )
	{
		offset *=.5f;
		pCurNode = m_pGraph->GetEnclosing(vStart+offset*(vEnd-vStart),0,true);
		if( offset< .0001f )
		{
			// can't find  node on the edge
//			assert( 0 );
			return;
		}
	}
	m_pGraph->TagNode( pCurNode );
	queueList.push_back( pCurNode );

	while( !queueList.empty() )
	{
		pCurNode = queueList.front();
		queueList.pop_front();
		lstNodes.push_back( pCurNode );

		for( VectorOfLinks::iterator li=pCurNode->link.begin(); li!=pCurNode->link.end(); li++ )
		{
			GraphNode *pCandidate = (*li).pLink;
			if( pCandidate->tag )
				continue;
			m_pGraph->TagNode( pCandidate );
			if( !SegmentInTriangle(pCandidate, vStart, vEnd) )
				continue;
			queueList.push_back( pCandidate );
		}
	}
	m_pGraph->ClearTagsNow();
	for(ListNodes::iterator nI=lstNodes.begin();nI!=lstNodes.end();nI++)
		m_pGraph->TagNode( (*nI) );
	return;
}

const ObstacleData CAISystem::GetObstacle(int nIndex)
{
	return m_VertexList.GetVertex(nIndex);
}

// it removes all references to this object from all objects of the specified type
void CAISystem::RemoveObjectFromAllOfType(int nType, CAIObject* pRemovedObject)
{
	AIObjects::iterator ai;
	// tell all objects of nType that this object is considered invalid
	if ( (ai=m_Objects.find(nType))!= m_Objects.end() )
	{
		for (;ai!=m_Objects.end();ai++)
		{
			if ( ai->first != nType)
				break;
			(ai->second)->OnObjectRemoved(pRemovedObject);
		}
	}
}

bool CAISystem::OnForbiddenEdge(const Vec3d & pos)
{
	if (m_mapForbiddenAreas.empty())
		return false;

	DesignerPathMap::iterator di, diend=m_mapForbiddenAreas.end();
	for (di=m_mapForbiddenAreas.begin();di!=diend;di++)
	{
		ListPositions::iterator li,linext,liend = (di->second).end();
		for (li=(di->second).begin(); li!=liend;li++)
		{
			linext = li;
			linext++;
			if (linext == liend)
				linext = (di->second).begin();

			if (PointOnLine((*li),(*linext),pos,0.001f))
				return true;
		}
	}
	return false;
}

IAutoBalance * CAISystem::GetAutoBalanceInterface(void)
{
	return m_pAutoBalance;
}

int CAISystem::ApplyDifficulty(float fAccuracy, float fAggression, float fHealth)
{

	if (m_cvRunAccuracyMultiplier->GetIVal())
	{
		AIObjects::iterator ai=m_Objects.find(AIOBJECT_PUPPET),aiend = m_Objects.end();
		for (;ai!=aiend;++ai)
		{
			if (ai->first!=AIOBJECT_PUPPET)
				break;

			CPuppet *pPuppet = 0;
			if ( (ai->second)->CanBeConvertedTo(AIOBJECT_CPUPPET,(void**) &pPuppet) )
			{
				if (!pPuppet->m_bEnabled || pPuppet->m_bSleeping)
				{
					// this guy is dead, skip
					continue;
				}
				AgentParameters ap = pPuppet->GetParameters();
				
				//float newAgg = ap.m_fOriginalAggression*fAggression;
				float newAgg = fAggression;
				//float newAcc = ap.m_fOriginalAccuracy*fAccuracy;
				float newAcc = fAccuracy;
				if (newAgg>1.f)
					newAgg = 1.f;
				if (newAcc>1.f)
					newAcc = 1.f;

				if (newAgg<0.1f)
					newAgg=0.1f;
				if (newAcc<0.1f)
					newAcc=0.1f;

				ap.m_fAggression = 1.f - (newAgg);
				ap.m_fAccuracy = 1.f - (newAcc);
				
			//	pPuppet->GetProxy()->ApplyHealth(ap.m_fMaxHealth * fHealth);

				pPuppet->SetParameters(ap);
			}
		}

	}
	return 0;
}

bool CAISystem::ExitNodeImpossible(GraphNode * pNode, float fRadius)
{
	VectorOfLinks::iterator vli=pNode->link.begin(), vliend = pNode->link.end();
	for (;vli!=vliend;++vli)
	{
		GraphLink gl = (*vli);
		if (gl.fMaxRadius >= fRadius)
			return false;
	}

	return true;
}

void CAISystem::DrawPuppetAutobalanceValues(IRenderer * pRenderer)
{

	AIObjects::iterator ai;
	if ((ai = m_Objects.find(AIOBJECT_PUPPET)) != m_Objects.end())
	{
		for (;ai!=m_Objects.end();)
		{
			if (ai->first != AIOBJECT_PUPPET)
				break;

			CPuppet *pPuppet = (CPuppet*) ai->second;

			if (!pPuppet->m_bEnabled)
			{
				ai++;
				continue;
			}

			Vec3d puppetPos = pPuppet->GetPos();
			
			pRenderer->DrawLabel(puppetPos,1,"ACC:%.3f",1.f - pPuppet->GetParameters().m_fAccuracy);
			puppetPos.z-=0.3f;
			pRenderer->DrawLabel(puppetPos,1,"AGG:%.3f",1.f - pPuppet->GetParameters().m_fAggression);

			ai++;
		}
		
	}

}

void CAISystem::SetSpeciesThreatMultiplier(int nSpeciesID, float fMultiplier)
{
	// will use this multiplier any time a puppet perceives a target of these species
	MapMultipliers::iterator mi;
	
	mi = m_mapSpeciesThreatMultipliers.find(nSpeciesID);
	if (mi== m_mapSpeciesThreatMultipliers.end())
		m_mapSpeciesThreatMultipliers.insert(MapMultipliers::iterator::value_type(nSpeciesID,fMultiplier));
	else
		mi->second = fMultiplier;

}

void CAISystem::DumpStateOf(IAIObject * pObject)
{
	ILog* pLog = m_pSystem->GetILog();

	pLog->Log("\001|AIName: %s",pObject->GetName());
	CPuppet *pPuppet = 0;
	if (pObject->CanBeConvertedTo(AIOBJECT_CPUPPET,(void**)&pPuppet))
	{
		CGoalPipe *pPipe = pPuppet->GetCurrentGoalPipe();
		if (pPipe)
		{
			pLog->Log("\001|Current pipes: %s",pPipe->m_sName.c_str());
			while (pPipe->IsInSubpipe())
			{
				pPipe = pPipe->GetSubpipe();
				pLog->Log("\001|   subpipe: %s",pPipe->m_sName.c_str());
			}
		}
	}
}


void CAISystem::SupressSoundEvent(const Vec3 &pos, float &fAffectedRadius)
{
	AIObjects::iterator ai=m_Objects.find(AIOBJECT_SNDSUPRESSOR),aiend =m_Objects.end();
	for (;ai!=aiend;++ai)
	{
		if (ai->first != AIOBJECT_SNDSUPRESSOR)
			break;
	
		float fRadius = (ai->second)->GetRadius();
		float totalSuppressionRadius = fRadius*.3f;		//
		float dist = ((ai->second)->GetPos() - pos).GetLength();

		if( dist < totalSuppressionRadius )			// sound event is within supressor radius 
					fAffectedRadius= 0.0f;									// this sound can;t be heard - it's withing 
																					// complite silence radous of supressor

		else if( dist < fRadius )																			// sound event is within supressor radius 
					fAffectedRadius *= (dist-totalSuppressionRadius)/(fRadius - totalSuppressionRadius);		// supress sound -- reduse it's radius

	}
}	

int CAISystem::RayOcclusionPlaneIntersection(const Vec3d &start,const Vec3d &end)
{
	if (m_mapOcclusionPlanes.empty())
		return 0;

	int iRet = 0;

	DesignerPathMap::iterator di = m_mapOcclusionPlanes.begin(),diend = m_mapOcclusionPlanes.end();
	for (;di!=diend;++di)
	{
		if (!di->second.empty())
		{
			float fShapeHeight = ((di->second).front()).z;
			if ( (start.z<fShapeHeight) && (end.z<fShapeHeight))
				continue;
			if ( (start.z>fShapeHeight) && (end.z>fShapeHeight))
				continue;

			// find out where ray hits horizontal plane fShapeHeigh (with a nasty hack)
			Vec3d vIntersection;
			float t = (start.z-fShapeHeight)/(start.z-end.z);
			vIntersection = start+t*(end-start);


			// is it inside the polygon?
			if (PointInsidePolygon(di->second,vIntersection))
				return 1;
		}
	}

	return 0;
}

int CAISystem::GetNumberOfObjects(unsigned short type)
{
	return m_Objects.count(type);

}

bool CAISystem::ThroughVehicle(const Vec3d & start, const Vec3d & end)
{
		
	AIObjects::iterator ai,aiend = m_Objects.end();
	for (ai=m_Objects.find(AIOBJECT_VEHICLE);ai!=aiend;++ai)
	{
		if (ai->first !=AIOBJECT_VEHICLE) 
			break;
		CAIObject *pVehicle = ai->second;
		Vec3d veh_dir = pVehicle->GetPos() - start;
		Vec3d dir = end-start;
		float veh_dist = veh_dir.len2();
		float dist = dir.len2();
		dir.Normalize();
		veh_dir.Normalize();
		if ((dir.Dot(veh_dir) > 0.2) && (veh_dist < dist))
			return true;
	}
	return false;
}
