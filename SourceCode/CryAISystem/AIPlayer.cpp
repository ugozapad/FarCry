#include "stdafx.h"
#include "aiplayer.h"

CAIPlayer::CAIPlayer(void)
{
	m_bSendPerceptionResetNotification = false;
}

CAIPlayer::~CAIPlayer(void)
{
	if (m_pProxy)
	{
		m_pProxy->Release();
		m_pProxy = 0;
	}
}

void CAIPlayer::ParseParameters(const AIObjectParameters & params)
{
	m_Parameters = params.m_sParamStruct;
	SetEyeHeight(params.fEyeHeight);
	
	m_pProxy = params.pProxy;
}

bool CAIPlayer::CanBeConvertedTo(unsigned short type, void ** pConverted)
{
	if (type == AIOBJECT_PLAYER)
	{
		*pConverted = (CAIPlayer *) this;
		return true;
	}

	return false;
}

void CAIPlayer::RegisterPerception(float fValue)
{
	if (fValue > m_fMaxPerception) 
		SetPerception(fValue);
}

void CAIPlayer::SetPerception(float fValue)
{
	m_fMaxPerception = fValue;

}

float CAIPlayer::GetPerception(void)
{
	if (m_bSendPerceptionResetNotification)
	{
		if (m_fLastPerceptionSnapshot < 0.01f)
		{
			SetSignal(1,"PERCEPTION_RESET",0);
			m_bSendPerceptionResetNotification = false;
			m_pProxy->Update(&m_State);
		}
	}

	return m_fLastPerceptionSnapshot;
}

void CAIPlayer::SnapshotPerception(void)
{
	m_fLastPerceptionSnapshot = m_fMaxPerception;
	if (m_fLastPerceptionSnapshot > 0.01f)
		m_bSendPerceptionResetNotification = true;
}
