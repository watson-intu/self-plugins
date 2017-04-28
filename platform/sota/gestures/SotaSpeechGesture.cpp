/**
* Copyright 2017 IBM Corp. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/


#include "SotaSpeechGesture.h"
#include "sensors/AudioData.h"
#include "services/ITextToSpeech.h"
#include "skills/SkillManager.h"
#include "utils/ThreadPool.h"
#include "utils/Sound.h"

#include "SelfInstance.h"

REG_OVERRIDE_SERIALIZABLE(SpeechGesture, SotaSpeechGesture);
REG_SERIALIZABLE(SotaSpeechGesture);
RTTI_IMPL(SotaSpeechGesture, SpeechGesture);

bool SotaSpeechGesture::Start()
{
	if (!SpeechGesture::Start())
		return false;

	ITextToSpeech  * pTTS = SelfInstance::GetInstance()->FindService<ITextToSpeech>();
	if (pTTS == NULL)
	{
		Log::Error("SotaSpeechGesture", "ITextToSpeech service is missing.");
		return false;
	}

	pTTS->GetVoices(DELEGATE(SotaSpeechGesture, OnVoices, Voices *, this));
	return true;
}

void SotaSpeechGesture::OnVoices(Voices * a_pVoices)
{
	m_pVoices = a_pVoices;
	if (ActiveRequest() != NULL)
		StartSpeech();
}

bool SotaSpeechGesture::Execute(GestureDelegate a_Callback, const ParamsMap & a_Params)
{
	if (PushRequest(a_Callback, a_Params))
		StartSpeech();

	return true;
}

void SotaSpeechGesture::StartSpeech()
{
	if (m_pVoices != NULL)
	{
		bool bSuccess = false;

		Request * pReq = ActiveRequest();

		const std::string & text = pReq->m_Params["text"].asString();
		const std::string & gender = pReq->m_Params["gender"].asString();
		const std::string & language = pReq->m_Params["language"].asString();

		std::string voice = "en-US_MichaelVoice";
		for (size_t i = 0; i < m_pVoices->m_Voices.size(); ++i)
		{
			Voice & v = m_pVoices->m_Voices[i];
			if (v.m_Language == language && v.m_Gender == gender)
			{
				voice = v.m_Name;
				break;
			}
		}

		ITextToSpeech * pTTS = SelfInstance::GetInstance()->FindService<ITextToSpeech>();
		if (pTTS != NULL)
		{
			// call the service to get the audio data for playing ..
			pTTS->SetVoice(voice);
			pTTS->ToSound(text, DELEGATE(SotaSpeechGesture, OnSpeechData, Sound *, this));
			bSuccess = true;
		}
		else
			Log::Error("SotaSpeechGesture", "No ITextToSpeech service available.");

		if (!bSuccess)
			OnSpeechDone();
	}

}

bool SotaSpeechGesture::Abort()
{
	if ( HaveRequests() )
	{
		Log::Debug("SotaSpeechGesture", "Abort() invoked.");

		PopAllRequests();

		SelfInstance::GetInstance()->GetSensorManager()->ResumeSensorType(AudioData::GetStaticRTTI().GetName() );
		return true;
	}
	
	return false;
}

void SotaSpeechGesture::OnSpeechData(Sound * a_pSound)
{
	// play the provided WAV file..
	if (a_pSound != NULL)
	{
		SelfInstance::GetInstance()->GetSensorManager()->PauseSensorType(AudioData::GetStaticRTTI().GetName() );
		ThreadPool::Instance()->InvokeOnThread<Sound *>(DELEGATE(SotaSpeechGesture, OnPlaySpeech, Sound *, this), a_pSound);
	}
	else
	{
		Log::Error("SotaSpeechGesture", "Failed to load WAV data.");
		OnSpeechDone();
	}
}

void SotaSpeechGesture::OnPlaySpeech(Sound * a_pSound)
{
	std::string tmpFile( Config::Instance()->GetInstanceDataPath() + "tmp.wav" );
	a_pSound->SaveToFile( tmpFile );
	delete a_pSound;

	if ( system( StringUtil::Format( "aplay %s", tmpFile.c_str() ).c_str() ) != 0 )
		Log::Error( "LinuxSpeechGesture", "Failed to play wav file." );

	ThreadPool::Instance()->InvokeOnMain( VOID_DELEGATE( SotaSpeechGesture, OnSpeechDone, this ) );
}

void SotaSpeechGesture::OnSpeechDone()
{
	SelfInstance::GetInstance()->GetSensorManager()->ResumeSensorType(AudioData::GetStaticRTTI().GetName());

	// start the next speech if we have any..
	if (PopRequest())
		StartSpeech();
}
