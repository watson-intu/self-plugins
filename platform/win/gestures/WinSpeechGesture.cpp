/**
* Copyright 2016 IBM Corp. All Rights Reserved.
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


#include "WinSpeechGesture.h"
#include "sensors/AudioData.h"
#include "services/ITextToSpeech.h"
#include "skills/SkillManager.h"
#include "utils/ThreadPool.h"
#include "utils/Sound.h"

#include "SelfInstance.h"

#include <mmsystem.h>
#include <dsound.h>
#include <dinput.h>
#include <dxerr.h>

// HACK: to make an older dxerr.lib work with VS 2015
#pragma warning(disable:4996)
int (WINAPIV * __vsnprintf)(char *, size_t, const char*, va_list) = _vsnprintf;
#pragma comment( lib, "dxerr.lib" )

LPDIRECTSOUND8  WinSpeechGesture::sm_pDirectSound = NULL;

REG_OVERRIDE_SERIALIZABLE(SpeechGesture, WinSpeechGesture);
REG_SERIALIZABLE(WinSpeechGesture);
RTTI_IMPL( WinSpeechGesture, SpeechGesture );

bool WinSpeechGesture::Start()
{
	if (! SpeechGesture::Start() )
		return false;

	ITextToSpeech * pTTS = Config::Instance()->FindService<ITextToSpeech>();
	if ( pTTS == NULL )
	{
		Log::Error( "DSSPeechGesture", "ITextToSpeech service is missing." );
		return false;
	}

	if (!InitDS())
		return false;

	pTTS->GetVoices( DELEGATE( WinSpeechGesture, OnVoices, Voices *, this ) );
	return true;
}

bool WinSpeechGesture::Stop()
{
	PopAllRequests();
	// wait for our active buffer to stop..
	while( m_bStreamPlaying )
	{
		ThreadPool::Instance()->ProcessMainThread();
		boost::this_thread::yield();
	}

	return true;
}

bool WinSpeechGesture::InitDS()
{
	if (sm_pDirectSound == NULL)
	{
		DirectSoundCreate8(NULL, &sm_pDirectSound, NULL);
		if (sm_pDirectSound == NULL)
		{
			Log::Error("WinSpeechGesture", "No speakers detected, cannot initialize DirectSoundX!");
			return false;
		}
		HRESULT hr = sm_pDirectSound->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY);
		if (!SUCCEEDED(hr))
		{
			Log::Error("WinSpeechGesture", "SetCooperativeLevel failed: %S", DXGetErrorString(hr));
			return false;
		}
	}

	return true;
}

bool WinSpeechGesture::Execute(GestureDelegate a_Callback, const ParamsMap & a_Params)
{
	if ( PushRequest( a_Callback, a_Params ) && m_bVoiceReady )
		StartSpeech();

	return true;
}

void WinSpeechGesture::StartSpeech()
{
	bool bSuccess = false;

	Request * pReq = ActiveRequest();

	std::string text = pReq->m_Params["text"].asString();
	if (text[0] != 0)
	{
		std::string voice = "en-US_AllisonVoice";
		if (m_pVoices != NULL)
		{
			const std::string & gender = pReq->m_Params["gender"].asString();
			const std::string & language = pReq->m_Params["language"].asString();

			for (size_t i = 0; i < m_pVoices->m_Voices.size(); ++i)
			{
				Voice & v = m_pVoices->m_Voices[i];
				if (v.m_Language == language && v.m_Gender == gender)
				{
					voice = v.m_Name;
					break;
				}
			}
		}

		// Expressive TTS only works with en-US_AllisonVoice for now, 
		// so confirming gender is set to female before applying tags
		// TODO: Move this into a data file that is loaded so this isn't hard-coded
		if (voice == "en-US_AllisonVoice")
		{
			if (pReq->m_Params["emotion"].asString().compare("Apology") == 0)
				text = "<express-as type=\"Apology\">" + text + "</express-as>";
			else if (pReq->m_Params["emotion"].asString().compare("GoodNews") == 0)
				text = "<express-as type=\"GoodNews\">" + text + "</express-as>";
		}

		ITextToSpeech * pTTS = Config::Instance()->FindService<ITextToSpeech>();
		if (pTTS != NULL)
		{
			m_StreamedSound.ResetLoadFromStream();
			m_bStreamDone = false;
			m_bAbortStream = false;
			m_bError = false;

			// call the service to get the audio data for playing ..
			pTTS->SetVoice(voice);
			pTTS->ToSound(text, DELEGATE(WinSpeechGesture, OnSpeechData, std::string *, this) );
			bSuccess = true;

		}
		else
			Log::Error("WinSpeechGesture", "No ITextToSpeech service available.");
	}
	else
		Log::Warning("DSSPeechGesture", "Empty text passed into SpeechGesture.");

	if (!bSuccess)
		OnSpeechDone();
}

bool WinSpeechGesture::Abort()
{
	m_bAbortStream = true;
	return true;
}

void WinSpeechGesture::OnVoices( Voices * a_pVoices )
{
	m_pVoices = a_pVoices;
	m_bVoiceReady = true;

	if ( ActiveRequest() != NULL )
		StartSpeech();
}

void WinSpeechGesture::OnSpeechData( std::string * a_pSound )
{
	Request * pReq = ActiveRequest();

	m_StreamLock.lock();

	if ( a_pSound != NULL )
	{
		if (! m_bStreamPlaying )
		{
			m_bStreamPlaying = true;
			SelfInstance::GetInstance()->GetSensorManager()->PauseSensorType( "AudioData" );
			ThreadPool::Instance()->InvokeOnThread( VOID_DELEGATE( WinSpeechGesture, PlayStreamedSound, this ) );
		}

		// handle moving data from the sound into the actual playing buffers.
		m_StreamedSound.LoadFromStream( *a_pSound );
		delete a_pSound;
	}
	else 
	{
		m_bStreamDone = true;

		// if the thread is not running, then we need to invoke OnSpeechDone() ourselves
		if (! m_bStreamPlaying )
		{
			m_bError = true;
			ThreadPool::Instance()->InvokeOnMain( VOID_DELEGATE( WinSpeechGesture, OnSpeechDone, this ) );
		}
	}

	m_StreamLock.unlock();
}

const float AUDIO_BUFFER_TIME = 0.5f;			// we want our buffer time to be 500 ms worth of audio data..

void WinSpeechGesture::PlayStreamedSound()
{
	// wait for enough audio data to initialize our buffers..
	bool bSoundReady = false;
	while(! bSoundReady && !m_bAbortStream )
	{
		boost::this_thread::sleep(boost::posix_time::milliseconds(5));

		m_StreamLock.lock();
		if ( m_StreamedSound.GetTime() >= AUDIO_BUFFER_TIME || m_bStreamDone )
			bSoundReady = true;
		m_StreamLock.unlock();
	}

	m_StreamLock.lock();
	if ( bSoundReady && !m_bAbortStream && m_StreamedSound.GetTime() > 0.0f && sm_pDirectSound != NULL )
	{
		// Set up WAV format structure. 
		WAVEFORMATEX wfx; 
		memset(&wfx, 0, sizeof(WAVEFORMATEX)); 
		wfx.wFormatTag = WAVE_FORMAT_PCM; 
		wfx.nChannels = m_StreamedSound.GetChannels();
		wfx.nSamplesPerSec = m_StreamedSound.GetRate();
		wfx.wBitsPerSample = m_StreamedSound.GetBits();
		wfx.nBlockAlign = (wfx.wBitsPerSample / 8) * wfx.nChannels;
		wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign; 

		// Set up DSBUFFERDESC structure. 
		DSBUFFERDESC dsbdesc; 
		memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); 
		dsbdesc.dwSize = sizeof(DSBUFFERDESC); 
		dsbdesc.dwFlags = DSBCAPS_GLOBALFOCUS; 
		dsbdesc.dwBufferBytes = (DWORD)(AUDIO_BUFFER_TIME * wfx.nAvgBytesPerSec);
		dsbdesc.lpwfxFormat = &wfx; 

		DWORD midPoint = dsbdesc.dwBufferBytes / 2;

		// Create buffer. 
		QIRef<LPDIRECTSOUNDBUFFER> buffer;
		HRESULT hr = sm_pDirectSound->CreateSoundBuffer(&dsbdesc, &buffer, NULL);
		if (SUCCEEDED(hr)) 
		{
			void * pLock = NULL; DWORD lockBytes = 0;
			hr = buffer->Lock( 0, dsbdesc.dwBufferBytes, &pLock, &lockBytes, NULL, NULL, 0 );
			if ( SUCCEEDED(hr) )
			{
				DWORD bytesPlayed = 0;
				DWORD nextWritePos = 0;
				DWORD bytesCopied = lockBytes;

				if ( bytesCopied > m_StreamedSound.GetWaveData().size() )
					bytesCopied = m_StreamedSound.GetWaveData().size();
				memcpy( pLock, m_StreamedSound.GetWaveData().data(), bytesCopied );
				if ( bytesCopied < lockBytes )
					memset( (BYTE *)pLock + bytesCopied, 0, lockBytes - bytesCopied );
				buffer->Unlock( pLock, lockBytes, NULL, 0 );

				buffer->SetCurrentPosition( 0 );
				buffer->SetVolume( DSBVOLUME_MAX );
				hr = buffer->Play( 0, 0, DSBPLAY_LOOPING );
				if (! SUCCEEDED( hr ) )
					Log::Error( "WinSpeechGesture", "Play failed: %S", DXGetErrorString( hr ) );

				while( bytesPlayed < m_StreamedSound.GetWaveData().size() )
				{
					m_StreamLock.unlock();
					boost::this_thread::sleep(boost::posix_time::milliseconds(50));
					m_StreamLock.lock();

					DWORD currentPos = 0;
					hr = buffer->GetCurrentPosition( &currentPos, NULL );
					if (! SUCCEEDED( hr ) )
						break;

					if ( (nextWritePos < midPoint && currentPos > midPoint) 
						|| (nextWritePos == midPoint && currentPos < midPoint) )
					{
						bytesPlayed += midPoint;

						hr = buffer->Lock( nextWritePos, midPoint, &pLock, &lockBytes, NULL, NULL, 0 );
						if (! SUCCEEDED( hr ) )
						{
							Log::Error( "WinSpeechGesture", "Failed to lock sound buffer." );
							break;
						}

						DWORD bytesAvailable = m_StreamedSound.GetWaveData().size() - bytesCopied;
						DWORD copyBytes = lockBytes;
						if ( copyBytes > bytesAvailable )
							copyBytes = bytesAvailable;
						if ( copyBytes > 0 )
							memcpy( pLock, m_StreamedSound.GetWaveData().data() + bytesCopied, copyBytes );
						if ( copyBytes < lockBytes )
							memset( (BYTE *)pLock + copyBytes, 0, lockBytes - copyBytes );
						buffer->Unlock( pLock, lockBytes, NULL, 0 );

						bytesCopied += copyBytes;
						if ( nextWritePos == 0 )
							nextWritePos = midPoint;
						else
							nextWritePos = 0;
					}

				}

				buffer->Stop();
			}
		}
		else
		{
			Log::Error( "WinSpeechGesture", "Failed to make sound buffer." );
		}
	}
	m_StreamLock.unlock();

	ThreadPool::Instance()->InvokeOnMain( VOID_DELEGATE( WinSpeechGesture, OnSpeechDone, this ) );
}

void WinSpeechGesture::OnSpeechDone()
{
	if ( m_bStreamPlaying )
	{
		SelfInstance::GetInstance()->GetSensorManager()->ResumeSensorType( "AudioData" );
		m_bStreamPlaying = false;
	}

	// start the next speech if we have any..
	if ( PopRequest( m_bError ) )
		StartSpeech();
}

