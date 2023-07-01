#ifndef _AGENTPARAMS_H_
#define _AGENTPARAMS_H_


typedef struct AgentParameters
{
	//-------------
	// ranges
	//----------------
	// sensors:
	// sight
	float	m_fSightRange;		// how far can the agent see
	float m_fHorizontalFov;
	float m_fVerticalFov;
	float m_fAccuracy;
	float m_fResponsiveness;

	float m_fMaxHealth;

	// sound
	float m_fSoundRange;		// how far can agent hear

	// behaviour
	float m_fAttackRange;
	float m_fCommRange;

	//-----------
	// indices
	//-------------
	float m_fAggression;
	float m_fDefensiveness;
	float m_fMorale;
	float m_fPanic;
	float m_fCohesion;
	float m_fPersistence;

	//-----------
	// hostility data
	//------------
	float m_fSpeciesHostility;
	float m_fGroupHostility;

	//-------------
	// grouping data
	//--------------
	int		m_nSpecies;
	int		m_nGroup;

	bool  m_bIgnoreTargets;

	AgentParameters()
	{
			m_fSightRange=0;
			m_fHorizontalFov=0;
			m_fVerticalFov=0;
			m_fAccuracy=0;
			m_fResponsiveness=0;

			m_fMaxHealth=0;
			m_fSoundRange=0;
			m_fAttackRange=0;
			m_fCommRange=0;

			m_fAggression=0;
			m_fDefensiveness=0;
			m_fMorale=0;
			m_fPanic=0;
			m_fCohesion=0;
			m_fPersistence=0;

			m_fSpeciesHostility=0;
			m_fGroupHostility=0;

			m_nSpecies=0;
			m_nGroup=0;

			m_bIgnoreTargets=false;
	}


} AgentParameters;


#endif _AGENTPARAMS_H_

