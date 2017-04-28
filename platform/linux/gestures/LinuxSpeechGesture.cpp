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


#include "LinuxSpeechGesture.h"
#include "sensors/AudioData.h"
#include "skills/SkillManager.h"
#include "utils/ThreadPool.h"
#include "utils/StringUtil.h"
#include "services/ITextToSpeech.h"
#include "SelfInstance.h"

#include <stdlib.h>


REG_OVERRIDE_SERIALIZABLE(SpeechGesture, LinuxSpeechGesture);
REG_SERIALIZABLE(LinuxSpeechGesture);
RTTI_IMPL( LinuxSpeechGesture, SpeechGesture );

bool LinuxSpeechGesture::Start()
{
    if (! SpeechGesture::Start() )
        return false;

	ITextToSpeech * pTTS = SelfInstance::GetInstance()->FindService<ITextToSpeech>();
    if ( pTTS == NULL )
    {
        Log::Error( "LinuxSpeechGesture", "TextToSpeech service is missing." );
        return false;
    }

    pTTS->GetVoices( DELEGATE( LinuxSpeechGesture, OnVoices, Voices *, this ) );
    return true;
}

void LinuxSpeechGesture::OnVoices( Voices * a_pVoices )
{
    m_pVoices = a_pVoices;
    if( m_pVoices )
        Log::Debug("LinuxSpeechGesture", "TTS returned %d available voices", m_pVoices->m_Voices.size());
}

bool LinuxSpeechGesture::Execute(GestureDelegate a_Callback, const ParamsMap & a_Params)
{
    if ( PushRequest( a_Callback, a_Params ) )
        StartSpeech();

    return true;
}

void LinuxSpeechGesture::StartSpeech()
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
            pTextToSpeech->ToSound( text, DELEGATE( LinuxSpeechGesture, OnSpeechData, Sound *, this ) );
            bSuccess = true;
        }
        else
        {
            Log::Error( "LinuxSpeechGesture", "No TTS service available." );
        }
    }
    else
    {
        Log::Debug("LinuxSpeechGesture", "Caught Active Request == NULL");
    }
	if (! bSuccess )
		OnSpeechDone();
}

bool LinuxSpeechGesture::Abort()
{
    if ( HaveRequests() )
    {
        Log::Debug("LinuxSpeechGesture", "Abort() invoked.");

        PopAllRequests();

        SelfInstance::GetInstance()->GetSensorManager()->ResumeSensorType(AudioData::GetStaticRTTI().GetName() );
        return true;
    }

    return false;
}

void LinuxSpeechGesture::OnSpeechData( Sound * a_pSound )
{
    if ( a_pSound != NULL )
    {
        SelfInstance::GetInstance()->GetSensorManager()->PauseSensorType(AudioData::GetStaticRTTI().GetName() );
		ThreadPool::Instance()->InvokeOnThread<Sound *>( DELEGATE( LinuxSpeechGesture, OnPlaySpeech, Sound *, this), a_pSound );
    }
    else
    {
        Log::Error("LinuxSpeechGesture", "NULL response from TTS");
		OnSpeechDone();
	}
}

void LinuxSpeechGesture::OnPlaySpeech( Sound * a_pSound )
{
	std::string tmpFile( Config::Instance()->GetInstanceDataPath() + "tmp.wav" );
	a_pSound->SaveToFile( tmpFile );
	delete a_pSound;

	if ( system( StringUtil::Format( "aplay %s", tmpFile.c_str() ).c_str() ) != 0 )
		Log::Error( "LinuxSpeechGesture", "Failed to play wav file." );

	ThreadPool::Instance()->InvokeOnMain( VOID_DELEGATE( LinuxSpeechGesture, OnSpeechDone, this ) );
}

void LinuxSpeechGesture::OnSpeechDone()
{
    SelfInstance::GetInstance()->GetSensorManager()->ResumeSensorType(AudioData::GetStaticRTTI().GetName());

    // start the next speech if we have any..
    if ( PopRequest() )
        StartSpeech();
}

