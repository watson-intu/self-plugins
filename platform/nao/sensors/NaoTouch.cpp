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

#include "NaoTouch.h"
#include "SelfInstance.h"
#include "utils/ThreadPool.h"
#include "NaoPlatform.h"
#include "utils/QiHelpers.h"

#ifndef _WIN32
REG_SERIALIZABLE(NaoTouch);
REG_OVERRIDE_SERIALIZABLE( TouchSensor, NaoTouch);
#endif

RTTI_IMPL(NaoTouch, TouchSensor);

bool NaoTouch::OnStart()
{
	Log::Debug("NaoTouch", "Starting up touch device");

	try {
		StartTouch();
		return true;
	}
	catch( const std::exception & ex )
	{
		Log::Error("NaoTouch", "Caught exception OnStart(): %s", ex.what() );
		return false;
	}
}

bool NaoTouch::OnStop()
{
	Log::Debug("NaoTouch", "Stopping touch device");
	if(StopTouch())
		return true;
	else
		return false;
}

void NaoTouch::StartTouch()
{
	try
	{
		DoStartTouch();
	}
	catch( const std::exception & ex )
	{
		Log::Error( "NaoTouch", "Caught Exception: %s", ex.what() );
	}
}

void NaoTouch::DoStartTouch()
{
	m_Memory = NaoPlatform::Instance()->GetSession()->service("ALMemory");

	m_TouchSub = m_Memory.call<qi::AnyObject>("subscriber", "TouchChanged" );
	m_TouchSub.connect("signal", qi::AnyFunction::fromDynamicFunction( boost::bind( &NaoTouch::OnTouch, this, _1 ) ) );
}

bool NaoTouch::StopTouch()
{
	try {
		DoStopTouch();
		return true;
	}
	catch( const std::exception & ex )
	{
		Log::Error( "NaoTouch", "Caught Exception: %s", ex.what() );
		return false;
	}
}

void NaoTouch::DoStopTouch()
{
	m_Memory.reset();
}

qi::AnyReference NaoTouch::OnTouch( const std::vector<qi::AnyReference> & args )
{
	try
	{
		if ( !NaoPlatform::Instance()->IsRestPosture() )
		{
			return DoOnTouch(args);
		}

	}
	catch( const std::exception & ex )
	{
		Log::Error( "NaoTouch", "Caught Exception: %s", ex.what() );
	}

	return qi::AnyReference();
}

qi::AnyReference NaoTouch::DoOnTouch(const std::vector <qi::AnyReference> & args)
{
	for (size_t i = 0; i < args.size(); ++i)
	{
		qi::AnyReference arg = args[i].content();
		qi::AnyReference hit = arg[0].content();
		std::string eventName = hit[0].content().asString();
		bool touched = hit[1].content().as<bool>();
		Json::Value touch;
		touch["m_SensorName"] = eventName;
		
		for (size_t i = 0; i < m_TouchTranslations.size(); ++i)
		{
			bool bTouched = true;
			TouchTranslation &response = m_TouchTranslations[i];

			for (size_t k = 0; k < response.m_Conditions.size(); ++k)
			{
				if (response.m_Conditions[k]->Test(touch))
				{
					bTouched = true;
					break;
				}
				else
					bTouched = false;
			}

			if (bTouched)
			{
				Log::Debug("NaoTouch", "Sending TouchType: %s", response.m_TouchType.c_str());
				ThreadPool::Instance()->InvokeOnMain(DELEGATE(NaoTouch, SendData, TouchData * , this),
													 new TouchData(response.m_TouchType, touched ? 1.0f : 0.0f));
				return qi::AnyReference();
			}
		}
	}
	return qi::AnyReference();
}

void NaoTouch::OnPause()
{
	++m_Paused;
}

void NaoTouch::OnResume()
{
	--m_Paused;
}

void NaoTouch::SendData( TouchData * a_pData )
{
	if (m_Paused <= 0 )
		ISensor::SendData( a_pData );
}



