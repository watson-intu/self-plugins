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


#include "RaspiAnimateGesture.h"
#include "SelfInstance.h"
#include "utils/ThreadPool.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Status.h"

#ifndef _WIN32
#include "wiringPi.h"
#endif

REG_OVERRIDE_SERIALIZABLE( AnimateGesture, RaspiAnimateGesture );
REG_SERIALIZABLE(RaspiAnimateGesture);
RTTI_IMPL( RaspiAnimateGesture, AnimateGesture );

bool RaspiAnimateGesture::Execute( GestureDelegate a_Callback, const ParamsMap & a_Params )
{
	if ( PushRequest( a_Callback, a_Params ) )
		ThreadPool::Instance()->InvokeOnThread(  DELEGATE( RaspiAnimateGesture, AnimateThread, Request *, this ), ActiveRequest() );
	return true;
}

bool RaspiAnimateGesture::Abort()
{
	return false;
}

void RaspiAnimateGesture::AnimateThread( Request * a_pReq )
{
	try {
		DoAnimateThread(a_pReq);
	}
	catch( const std::exception & ex )
	{
		Log::Error( "RaspiAnimateGesture", "Caught Exception: %s", ex.what() );
	}

	// now invoke the main thread to notify them the move is completed..
	ThreadPool::Instance()->InvokeOnMain( DELEGATE(RaspiAnimateGesture, AnimateDone, Request *, this ), a_pReq );
}

void RaspiAnimateGesture::DoAnimateThread(RaspiAnimateGesture::Request * a_pReq)
{
#ifndef _WIN32
	Log::Debug( "RaspiAnimateGesture", "Gesture %s is running.", m_GestureId.c_str() );
	if ( !m_bWiredPi )
	{
		wiringPiSetup();
		pinMode(m_PinNumber, OUTPUT);
		m_bWiredPi = true;
	}
	for(size_t i=0;i<5;++i)
	{
		digitalWrite(m_PinNumber, HIGH);
		delay(200);
		digitalWrite(m_PinNumber, LOW);
		delay(200);
	}
#endif
}

void RaspiAnimateGesture::AnimateDone( Request * a_pReq )
{
	Log::Debug("RaspiAnimateGesture", "In AnimateDone");

	// pop the request, if true is returned then start the next request..
	if ( PopRequest() )
		ThreadPool::Instance()->InvokeOnThread(DELEGATE(RaspiAnimateGesture, AnimateThread, Request * , this), ActiveRequest());
}
