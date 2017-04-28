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


#include "NaoPlatform.h"
#include "NaoAnimateGesture.h"
#include "SelfInstance.h"
#include "utils/ThreadPool.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Status.h"

REG_OVERRIDE_SERIALIZABLE( AnimateGesture, NaoAnimateGesture );
REG_SERIALIZABLE(NaoAnimateGesture);
RTTI_IMPL( NaoAnimateGesture, AnimateGesture );

bool NaoAnimateGesture::Execute( GestureDelegate a_Callback, const ParamsMap & a_Params )
{
	if ( PushRequest( a_Callback, a_Params ) )
		ExecuteAnimation();
	return true;
}

bool NaoAnimateGesture::Abort()
{
	PopAllRequests();
	return true;
}

void NaoAnimateGesture::ExecuteAnimation()
{
	NaoPlatform * pPlatform = NaoPlatform::Instance();
	assert( pPlatform != NULL );
	Request * pReq = ActiveRequest();
	assert( pReq != NULL );

	std::vector<AnimationEntry> anims;
	for(size_t i=0;i<m_Animations.size();++i)
	{
		if ( m_Animations[i].m_RequiredPosture.size() == 0 || m_Animations[i].m_RequiredPosture == pPlatform->GetPosture() )
			anims.push_back( m_Animations[i] );
	}

	if ( anims.size() > 0 )
	{
		const AnimationEntry & entry = anims[ rand() % anims.size() ];

		Log::Debug( "NaoAnimateGesture", "Gesture %s is running behavior %s.", m_GestureId.c_str(), entry.m_AnimId.c_str() );
		ThreadPool::Instance()->InvokeOnThread(  DELEGATE( NaoAnimateGesture, AnimateThread, std::string, this ), entry.m_AnimId );
	}
	else
		AnimateDone();
}

void NaoAnimateGesture::AnimateThread( std::string a_Anim )
{
	NaoPlatform * pPlatform = NaoPlatform::Instance();
	assert( pPlatform != NULL );

	try {
		qi::AnyObject behavior = pPlatform->GetSession()->service("ALBehaviorManager");
		behavior.async<void>( "runBehavior", a_Anim ).wait();
	}
	catch( const std::exception & ex )
	{
		Log::Error( "NaoAnimateGesture", "Caught Exception: %s", ex.what() );
	}

	//// now invoke the main thread to notify them the move is completed..
	ThreadPool::Instance()->InvokeOnMain( VOID_DELEGATE(NaoAnimateGesture, AnimateDone, this ) );
}

void NaoAnimateGesture::AnimateDone()
{
	NaoPlatform * pPlatform = NaoPlatform::Instance();
	assert( pPlatform != NULL );

	try {
		qi::AnyObject leds = pPlatform->GetSession()->service("ALLeds");
		leds.async<void>("fadeRGB", "FaceLeds",  255, 255, 255, 0.0f );
	}
	catch( const std::exception & ex )
	{
		Log::Error( "NaoAnimateGesture", "Caught Exception: %s", ex.what() );
	}

	// pop the request, if true is returned then start the next request..
	if ( PopRequest() )
		ExecuteAnimation();
}
