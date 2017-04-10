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

#include "MacSoundGesture.h"
#include "utils/ThreadPool.h"
#include "SelfInstance.h"

REG_OVERRIDE_SERIALIZABLE(SoundGesture, MacSoundGesture);
REG_SERIALIZABLE(MacSoundGesture);
RTTI_IMPL( MacSoundGesture, SoundGesture );

bool MacSoundGesture::Start()
{
    if (!SoundGesture::Start())
        return false;

    return true;
}

bool MacSoundGesture::Abort()
{
    return false;
}

bool MacSoundGesture::CanExecute(const ParamsMap & a_Params)
{
    if (!a_Params.ValidPath("sound"))
        return false;
    return true;
}

bool MacSoundGesture::Execute(GestureDelegate a_Callback, const ParamsMap & a_Params)
{
    if ( PushRequest( a_Callback, a_Params ) )
        Play();

    return true;
}

void MacSoundGesture::Play()
{
    Request * pReq = ActiveRequest();
    if (pReq != NULL)
    {
        if (pReq->m_Params.GetData().isMember("sound"))
            m_Sound = pReq->m_Params["sound"].asString();
        if (pReq->m_Params.GetData().isMember("volume"))
            m_fVolume = pReq->m_Params["volume"].asFloat();
        if (pReq->m_Params.GetData().isMember("pan"))
            m_fPan = pReq->m_Params["pan"].asFloat();

        ThreadPool::Instance()->InvokeOnThread(VOID_DELEGATE(MacSoundGesture, PlayThread, this));
    }
}

void MacSoundGesture::PlayThread()
{
    std::string file = Config::Instance()->GetStaticDataPath() + m_Sound;
    if ( system( StringUtil::Format( "afplay %s", file.c_str() ).c_str() ) != 0 )
        Log::Error( "MacSoundGesture", "Failed to play wav file." );

    ThreadPool::Instance()->InvokeOnMain( VOID_DELEGATE( MacSoundGesture, OnSoundDone, this ) );
}

void MacSoundGesture::OnSoundDone()
{
    // start the next sound if we have any..
    if ( PopRequest() )
        Play();
}


