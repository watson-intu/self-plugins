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


#include "RaspiSpeechGesture.h"
#include "sensors/AudioData.h"
#include "skills/SkillManager.h"
#include "utils/ThreadPool.h"

#include "SelfInstance.h"

#include <stdlib.h>


REG_OVERRIDE_SERIALIZABLE(SpeechGesture, RaspiSpeechGesture);
REG_SERIALIZABLE(RaspiSpeechGesture);
RTTI_IMPL( RaspiSpeechGesture, SpeechGesture );

bool RaspiSpeechGesture::Start()
{
	if (! SpeechGesture::Start() )
		return false;

	ITextToSpeech * pTTS = SelfInstance::GetInstance()->FindService<ITextToSpeech>();
	if ( pTTS == NULL )
	{
		Log::Error( "RaspiSpeechGesture", "TextToSpeech service is missing." );
		return false;
	}

	pTTS->GetVoices( DELEGATE( RaspiSpeechGesture, OnVoices, Voices *, this ) );
	return true;
}

void RaspiSpeechGesture::OnVoices( Voices * a_pVoices )
{
	m_pVoices = a_pVoices;
	if( m_pVoices )
		Log::Debug("RaspiSpeechGesture", "TTS returned %d available voices", m_pVoices->m_Voices.size());
}

bool RaspiSpeechGesture::CanExecute(const ParamsMap & a_Params)
{
	if (!a_Params.ValidPath("text"))
		return false;
	return true;
}

bool RaspiSpeechGesture::Execute(GestureDelegate a_Callback, const ParamsMap & a_Params)
{
	if ( PushRequest( a_Callback, a_Params ) )
		StartSpeech();

	return true;
}

void RaspiSpeechGesture::StartSpeech()
{
	bool bSuccess = false;
	Request * pReq = ActiveRequest();

	if ( pReq )
	{
		const std::string & text = pReq->m_Params["text"].asString();
		const std::string & gender = pReq->m_Params["gender"].asString();
		const std::string & language = pReq->m_Params["language"].asString();

		std::string voice = "en-US_MichaelVoice";
		if ( m_pVoices )
		{
			for(size_t i=0;i<m_pVoices->m_Voices.size();++i)
			{
				Voice & v = m_pVoices->m_Voices[i];
				if ( v.m_Language == language && v.m_Gender == gender )
				{
					voice = v.m_Name;
					break;
				}
			}
		}

		ITextToSpeech * pTextToSpeech = SelfInstance::GetInstance()->FindService<ITextToSpeech>();
		if ( pTextToSpeech != NULL )
		{
			// call the service to get the audio data for playing ..
			pTextToSpeech->SetVoice(voice);
			pTextToSpeech->ToSound( text, DELEGATE( RaspiSpeechGesture, OnSpeechData, Sound *, this ) );
			bSuccess = true;
		}
		else
		{
			Log::Error( "RaspiSpeechGesture", "No TTS service available." );
		}
	}
	else
	{
		Log::Debug("RaspiSpeechGesture", "Caught Active Request == NULL");
	}
}

bool RaspiSpeechGesture::Abort()
{
	if ( HaveRequests() )
	{
		Log::Debug("RaspiSpeechGesture", "Abort() invoked.");

		PopAllRequests();

		SelfInstance::GetInstance()->GetSensorManager()->ResumeSensorType(AudioData::GetStaticRTTI().GetName() );
		return true;
	}

	return false;
}

void RaspiSpeechGesture::OnSpeechData( Sound * a_pSound )
{
	if ( a_pSound )
	{
		SelfInstance::GetInstance()->GetSensorManager()->PauseSensorType(AudioData::GetStaticRTTI().GetName() );

		a_pSound->SaveToFile("tmp.wav");
		system("aplay tmp.wav");

		ThreadPool::Instance()->InvokeOnMain( VOID_DELEGATE( RaspiSpeechGesture, OnSpeechDone, this ) );
	}
	else
	{
		Log::Error("RaspiSpeechGesture", "NULL response from TTS");
	}
}

void RaspiSpeechGesture::OnSpeechDone()
{
	SelfInstance::GetInstance()->GetSensorManager()->ResumeSensorType(AudioData::GetStaticRTTI().GetName());

	Request * pReq = ActiveRequest();
	if ( pReq != NULL && pReq->m_Callback.IsValid() )
		pReq->m_Callback( this );

	// start the next speech if we have any..
	if ( PopRequest() )
		StartSpeech();
}

