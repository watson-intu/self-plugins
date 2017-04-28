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


#include "RemoteMicrophone.h"
#include "SelfInstance.h"

#ifndef ANDROID_BUILD
REG_OVERRIDE_SERIALIZABLE(Microphone, RemoteMicrophone);
#endif
REG_SERIALIZABLE( RemoteMicrophone );
RTTI_IMPL(RemoteMicrophone, Microphone);

bool RemoteMicrophone::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance != NULL)
	{
		ITopics * pTopics = pInstance->GetTopics();
		pTopics->RegisterTopic("audio-input", "audio/L16;rate=16000");
		pTopics->Subscribe("audio-input", DELEGATE(RemoteMicrophone, OnRemoteAudio, const ITopics::Payload &, this));
	}

	return true;
}

bool RemoteMicrophone::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if (pInstance != NULL)
	{
		ITopics * pTopics = pInstance->GetTopics();
		pTopics->Unsubscribe("audio-input");
		pTopics->UnregisterTopic("audio-input");
	}
	return true;
}

void RemoteMicrophone::OnPause()
{
	m_Paused++;
}

void RemoteMicrophone::OnResume()
{
	m_Paused--;
}

void RemoteMicrophone::OnRemoteAudio( const ITopics::Payload & a_Payload )
{
	if (m_Paused <= 0 )
		SendData( new AudioData( a_Payload.m_Data, 16000, 1, 16 ) );
}

