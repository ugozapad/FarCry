#include "stdafx.h"
#include "aiautobalance.h"
#include "caisystem.h"
#include <irenderer.h>
#include <isystem.h>
#include <itimer.h>
#include <iconsole.h>

CAIAutoBalance::CAIAutoBalance(void)
{
	m_nAllowedDeaths = 0;
	m_nPlayerDeaths = 0;

	m_fAvgEnemyLifetime = 0;
	m_fAggressionMult = 1.f;
	m_fHealthMult = 1.f;
	m_fAccuracyMult = 1.f;

	m_nNumShotsFired = 0;
	m_fMaxClampValue = 2.f;
	
	m_fLastPlayerDeathTime = GetAISystem()->m_pSystem->GetITimer()->GetCurrTime();
}

CAIAutoBalance::~CAIAutoBalance(void)
{
}

void CAIAutoBalance::RegisterPlayerDeath()
{
	m_nPlayerDeaths++;
	m_Stats.nTotalPlayerDeaths++;

	float fCurrentTime = GetAISystem()->m_pSystem->GetITimer()->GetCurrTime();
	m_Stats.fAVGPlayerLifetime+=fCurrentTime-m_fLastPlayerDeathTime;
	m_fLastPlayerDeathTime=fCurrentTime;
	if (m_Stats.nTotalPlayerDeaths>1)
		m_Stats.fAVGPlayerLifetime/=2.f;

	AdjustDifficulty();
	m_vEnemyLifetimes.clear();
	m_fAvgEnemyLifetime = 0;
	m_nNumShotsFired = 0;
}

void CAIAutoBalance::Checkpoint()
{
	m_vEnemyLifetimes.clear();
	m_nAllowedDeaths = 0;
	m_nPlayerDeaths = 0;
	m_fAvgEnemyLifetime = 0;
	
	if (!m_Stats.nCheckpointsHit)
	{
		m_Stats.nTotalEnemiesInLevel = GetAISystem()->GetNumberOfObjects(AIOBJECT_PUPPET);
		m_fTotalTimeStart = GetAISystem()->m_pSystem->GetITimer()->GetCurrTime();
	}
	
	m_Stats.nCheckpointsHit++;

	

	m_fStartingAccuracy = m_fAccuracyMult;
	m_fStartingAggresion= m_fAggressionMult;
	m_fStartingHealth = m_fHealthMult;
}

void CAIAutoBalance::SetAllowedDeathCount(int nDeaths)
{
	m_nAllowedDeaths = nDeaths;
}

void CAIAutoBalance::RegisterEnemyLifetime(float fLifeInSeconds)
{
	m_Stats.nEnemiesKilled++;
	m_Stats.fAVGEnemyLifetime+=fLifeInSeconds;
	if (m_Stats.nEnemiesKilled>1)
		m_Stats.fAVGEnemyLifetime/=2.f;

	if (fLifeInSeconds < 0.01f)
		m_Stats.nSilentKills++;

	if (m_vEnemyLifetimes.size()==5)
	{
		// if more than 10 enemies killed without dying, reset average lifetime

		m_vEnemyLifetimes.clear();
		m_vEnemyLifetimes.push_back(m_fAvgEnemyLifetime);
	}

	m_vEnemyLifetimes.push_back(fLifeInSeconds);

	m_fAvgEnemyLifetime=0;
	VectorOfFloats::iterator vi,viend = m_vEnemyLifetimes.end();
	for (vi=m_vEnemyLifetimes.begin();vi!=viend;++vi)
	{
		m_fAvgEnemyLifetime+=(*vi);
	}
	m_fAvgEnemyLifetime/=m_vEnemyLifetimes.size();
	if (m_fAvgEnemyLifetime>20.f)
		m_fAvgEnemyLifetime=20.f;

	AdjustDifficulty();
}

void CAIAutoBalance::DebugDraw(IRenderer * pRenderer)
{

	pRenderer->TextToScreen(0,8,"ALLOWED DEATHS: %d",m_nAllowedDeaths);
	pRenderer->TextToScreen(0,10,"PLAYER DEATHS: %d",m_nPlayerDeaths);
	pRenderer->TextToScreen(0,12,"Average enemy lifetime: %.3f",m_fAvgEnemyLifetime);

	pRenderer->TextToScreen(0,16,"----------------- CURRENT BALANCE VALUES ---------------------");
	pRenderer->TextToScreen(0,18,"CURRENT ACCURACY: %.3f ",m_fAccuracyMult);
	//pRenderer->TextToScreen(0,18,"ENEMY ACCURACY: %.3f",m_fAccuracyMult);
	pRenderer->TextToScreen(0,20,"CURRENT AGGRESION: %.3f ",m_fAggressionMult);
	//pRenderer->TextToScreen(0,20,"ENEMY AGGRESION: %.3f",m_fAggressionMult);
	pRenderer->TextToScreen(0,22,"ENEMY HEALTH: %.3f",m_fHealthMult);
	pRenderer->TextToScreen(0,25,"STEALTH-O-METER SPEED: %.3f",GetAISystem()->m_cvSOM_Speed->GetFVal());
	pRenderer->TextToScreen(0,27,"ACCURACY INCREASE: %.3f",1);

	if (GetAISystem()->m_bCollectingAllowed)
        pRenderer->TextToScreen(0,30,"NOW COLLECTING");
	else
		pRenderer->TextToScreen(0,30,"--- STOPPED COLLECTING ----");

	GetAISystem()->DrawPuppetAutobalanceValues(pRenderer);

	VectorOfFloats::iterator vi,viend = m_vEnemyLifetimes.end();
	int i=0;
	for (vi=m_vEnemyLifetimes.begin();vi!=viend;++vi,i++)
	{
		pRenderer->TextToScreen(0,40.0f+i*2,"ENEMY LIFETIME: %.3f",(*vi));
	}

}

void CAIAutoBalance::SetMultipliers(float fAccuracy, float fAggression, float fHealth)
{
	m_fStartingAccuracy = fAccuracy;
	m_fStartingAggresion= fAggression;
	m_fStartingHealth = fHealth;

	m_fAccuracyMult = fAccuracy;
	m_fAggressionMult = fAggression;
	m_fHealthMult = fHealth;
}

void CAIAutoBalance::GetMultipliers(float & fAccuracy, float & fAggression, float & fHealth)
{
	fAccuracy = m_fAccuracyMult;
	fAggression = m_fAggressionMult;
	fHealth = m_fHealthMult;
}

void CAIAutoBalance::AdjustDifficulty(bool bCalcDeath)
{
	float fDeathMultiplier = 1.f - (float)m_nPlayerDeaths/(float)(m_nAllowedDeaths-1);

	float fAGG = fDeathMultiplier;
	Clamp(fAGG);
	float fACC = fDeathMultiplier + (1.f - fDeathMultiplier) * 0.25f;
	Clamp(fACC);
//	float fHEA = fDeathMultiplier + (1.f - fDeathMultiplier) * 0.5f;
//	Clamp(fHEA);

	float fLifetimeMod = (10.f-m_fAvgEnemyLifetime)/10.f;// * m_fMaxClampValue;

	//m_fAggressionMult	=	m_fStartingAggresion*fAGG*fLifetimeMod;
	m_fAggressionMult	=	fAGG*fLifetimeMod;
//	m_fHealthMult		=	m_fStartingHealth*fHEA*fLifetimeMod;
	//m_fAccuracyMult		=	m_fStartingAccuracy * fACC*fLifetimeMod;
	m_fAccuracyMult		=	fACC*fLifetimeMod;

	CalcMinimum();

	GetAISystem()->ApplyDifficulty(m_fAccuracyMult,m_fAggressionMult,1.f);

}

void CAIAutoBalance::Clamp(float & fVal)
{
	if (fVal < 0.1f)
		fVal = 0.1f;
	if (fVal > m_fMaxClampValue)
		fVal = m_fMaxClampValue;
}

void CAIAutoBalance::RegisterPlayerFire(int nShots)
{
	m_Stats.nShotsFires+=nShots;
	m_nNumShotsFired += nShots;
	m_fHitPercentage = (float)m_nNumShotsHit/(float)m_nNumShotsFired;
}

void CAIAutoBalance::RegisterPlayerHit()
{
	m_Stats.nShotsHit++;
	m_nNumShotsHit++;
	m_fHitPercentage = (float)m_nNumShotsHit/(float)m_nNumShotsFired;

	GetAISystem()->m_bCollectingAllowed = true;
	GetAISystem()->m_fAutoBalanceCollectingTimeout = 0.f;

}

void CAIAutoBalance::CalcMinimum(void)
{
	float fMinFloatMultiplier = m_fStartingAccuracy;
	if (m_fStartingAggresion < fMinFloatMultiplier)
		fMinFloatMultiplier = m_fStartingAggresion;
	if (m_fStartingHealth < fMinFloatMultiplier)
		fMinFloatMultiplier = m_fStartingHealth;

	m_fMaxClampValue = 2.f / fMinFloatMultiplier;
}

void CAIAutoBalance::GetAutobalanceStats(AIBalanceStats & stats)
{
	if (stats.bFinal)
	{
		m_Stats.fTotalTimeSeconds = GetAISystem()->m_pSystem->GetITimer()->GetCurrTime() - m_fTotalTimeStart;
	}
	stats = m_Stats;
	stats.nAllowedDeaths = m_nAllowedDeaths;
	
}

void CAIAutoBalance::RegisterVehicleDestroyed(void)
{
	m_Stats.nVehiclesDestroyed++;
}
