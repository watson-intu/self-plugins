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


#ifndef DS_SPEECH_GESTURE_H
#define DS_SPEECH_GESTURE_H

#include "gestures/SpeechGesture.h"
#include "utils/Sound.h"
#include "utils/QIRef.h"

#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <mmsystem.h>
#include <dsound.h>

class Sound;
struct Voices;

//! This gesture wraps DirectSound so the local windows box can speak.
class WinSpeechGesture : public SpeechGesture
{ 
public:
	RTTI_DECL();

	//! Construction
	WinSpeechGesture() : 
		m_bVoiceReady( false ), 
		m_pVoices( NULL ), 
		m_bStreamPlaying( false ),
		m_bStreamDone( true ),
		m_bAbortStream( false ),
		m_bError( false )
	{}

	//! IGesture interface
	virtual bool Start();
	virtual bool Stop();
	virtual bool Execute( GestureDelegate a_Callback, const ParamsMap & a_Params );
	virtual bool Abort();

	static LPDIRECTSOUND8  sm_pDirectSound;
	static bool InitDS();

private:
	void StartSpeech();
	void OnVoices( Voices * a_pVoices );
	void OnSpeechData( std::string * a_pSound );
	void PlayStreamedSound();
	void OnSpeechDone();

	bool						m_bVoiceReady;
	Voices *					m_pVoices;			

	volatile bool				m_bStreamPlaying;
	volatile bool				m_bStreamDone;
	volatile bool				m_bAbortStream;
	volatile bool				m_bError;
	boost::mutex				m_StreamLock;
	Sound						m_StreamedSound;
};


#endif //SPEECH_GESTURE_H
