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


#include "NaoBrowser.h"

#include <qi/signal.hpp>
#include <qi/type/proxysignal.hpp>
#include "qi/anyobject.hpp"

#include "SelfInstance.h"
#include "NaoPlatform.h"
#include "utils/URL.h"

REG_SERIALIZABLE(NaoBrowser);
RTTI_IMPL( NaoBrowser, IBrowser);

#ifndef _WIN32
REG_OVERRIDE_SERIALIZABLE( IBrowser, NaoBrowser);
#endif

NaoBrowser::NaoBrowser() : IBrowser("URLServiceV1"),
	m_bServiceActive( false ),
	m_bThreadStopped( true ),
	m_bTabletConnected(false), 
	m_bLogoDisplayed( false ),
	m_LastUpdate( 0 ),
	m_LastCheck( 0 ),
	m_TabletCheckInterval(10.0), 
	m_TabletDisplayTime(0.0), 
	m_fBrightness(1.0)
{}

void NaoBrowser::Serialize(Json::Value & json)
{
	IBrowser::Serialize(json);

	json["m_TabletCheckInterval"] = m_TabletCheckInterval;
	json["m_TabletDisplayTime"] = m_TabletDisplayTime;
	json["m_fBrightness"] = m_fBrightness;
}

void NaoBrowser::Deserialize(const Json::Value & json)
{
	IBrowser::Deserialize(json);

	if ( json.isMember("m_TabletCheckInterval") )
		m_TabletCheckInterval = json["m_TabletCheckInterval"].asFloat();
	if ( json.isMember("m_TabletDisplayTime") )
		m_TabletDisplayTime = json["m_TabletDisplayTime"].asFloat();
	if ( json.isMember("m_fBrightness") )
		m_fBrightness = json["m_fBrightness"].asFloat();

}

bool NaoBrowser::Start() 
{
	Log::Status( "NaoBrowser", "Starting.." );

	if (! IBrowser::Start() )
		return false;

	m_bServiceActive = true;
	m_bThreadStopped = false;
	m_LogoUrl = SelfInstance::GetInstance()->GetLogoUrl();

	ThreadPool::Instance()->InvokeOnThread( VOID_DELEGATE(NaoBrowser, TabletThread, this) );

	return true;
}

bool NaoBrowser::Stop()
{
	Log::Status( "NaoBrowser", "Stopping.." );

	m_bServiceActive = false;
	while(! m_bThreadStopped )
		boost::this_thread::yield();

	return IBrowser::Stop();
}

void NaoBrowser::ShowURL( const Url::SP & a_spUrl, UrlCallback a_Callback )
{
	Log::Status( "NaoBrowser", "Sending URL: %s", a_spUrl->GetURL().c_str() );

	m_RequestListLock.lock();
	m_RequestList.push_back( UrlRequest( a_spUrl, a_Callback ) );
	m_RequestListLock.unlock();
}

//--------------------------


void NaoBrowser::TabletThread()
{
	m_LastCheck = Time().GetEpochTime();
	m_LastUpdate = Time().GetEpochTime();

	ConfigureTablet();

	while( m_bServiceActive )
	{
		double now = Time().GetEpochTime();
		if ( (now - m_LastCheck) > m_TabletCheckInterval )
			CheckConnection();

		if ( m_bTabletConnected )
		{
			m_RequestListLock.lock();
			if ( m_RequestList.begin() != m_RequestList.end() )
			{
				UrlRequest & req = m_RequestList.front();
				TabletShowURL( req.m_spUrl, req.m_Callback );
				m_RequestList.pop_front();
			}
			m_RequestListLock.unlock();

			if ( !m_bLogoDisplayed && ((now - m_LastUpdate) > m_TabletDisplayTime) && m_TabletDisplayTime > 0 )
				DisplayLogo();
		}

		boost::this_thread::sleep( boost::posix_time::milliseconds( 50 ) );
	}

	m_bThreadStopped = true;
}

void NaoBrowser::TabletShowURL( const Url::SP & a_spUrl, UrlCallback a_Callback )
{
	// Map "localhost" to the IP address of the robot relative to the tablet
	URL mappedURL = URL(a_spUrl->GetURL());
	if (_stricmp(mappedURL.GetHost().c_str(), "localhost") == 0)
		mappedURL.SetHost("198.18.0.1");
	std::string url( IBrowser::EscapeUrl( mappedURL.GetURL()).c_str() );
	try {
		Log::Status( "NaoBrowser", "Showing URL: %s", url.c_str() );
		m_LastUpdate = Time().GetEpochTime();
		m_bLogoDisplayed = false;

		// Check wifi status on tablet
		m_Tablet.call<void>("cleanWebview");

		IBrowser::URLServiceData * urlServiceData = new IBrowser::URLServiceData();
		urlServiceData->m_spUrl = a_spUrl;
		if (m_Tablet.call<bool>("showWebview", url))
			urlServiceData->m_JsonValue = Json::Value("COMPLETED");

		if (a_Callback.IsValid())
			ThreadPool::Instance()->InvokeOnMain<IBrowser::URLServiceData *>( a_Callback, urlServiceData );
	}
	catch (  const std::exception & ex )
	{
		Log::Debug("NaoBrowser", "Caught Exception: %s", ex.what());
	}
}

void NaoBrowser::CheckConnection()
{
	try {
		m_LastCheck = Time().GetEpochTime();

		if (m_Tablet)
		{
			std::string status = m_Tablet.call<std::string>("getWifiStatus");
			m_bTabletConnected = status.compare("CONNECTED") == 0;
			Log::Status("NaoBrowser", "Tablet Status: %s (%s)", status.c_str(), m_bTabletConnected ? "Connected" : "Failed" );			
		}
		else
		{
			m_bTabletConnected = false;
		}
	}
	catch (const std::exception & e)
	{
		Log::Error("NaoBrowser", "Threw exception during configure: %s", e.what() );
		m_bTabletConnected = false;		
	}

	if (! m_bTabletConnected)
		ConfigureTablet();
}

void NaoBrowser::ConfigureTablet()
{	
	try {
		m_bTabletConnected = false;
		m_bLogoDisplayed = false;

		if (NaoPlatform::Instance()->HasService("ALTabletService" ))
		{
			m_Tablet = NaoPlatform::Instance()->GetSession()->service("ALTabletService");
			if (m_Tablet)
			{
				// Configure wifi
				m_Tablet.call<void>("resetTablet");			
				m_Tablet.call<void>("enableWifi");
				// m_Tablet.call<bool>("configureWifi", m_Security, m_SSID, m_Password); <-- This will be uncommented when creds can be pulled from gateway

				std::string status = m_Tablet.call<std::string>( "getWifiStatus" );
				m_bTabletConnected = status.compare("CONNECTED") == 0;
				Log::Status("NaoBrowser", "Tablet Status: %s (%s)", status.c_str(), m_bTabletConnected ? "Connected" : "Failed" );			

				if ( m_bTabletConnected )
				{
					DisplayLogo();

					// Other config
					Log::Debug("NaoBrowser", "Setting brightness to %.2f", m_fBrightness);
					bool set = m_Tablet.call<bool>("setBrightness", m_fBrightness);
					if (! set )
						Log::Error("NaoBrowser", "Failed to set brightness");

					m_Tablet.connect("onTouchDown", qi::AnyFunction::fromDynamicFunction( boost::bind( &NaoBrowser::OnTouchData, this, _1 ) ) );
				}
			}
		}

		if ( m_bTabletConnected )
			Log::Status( "NaoBrowser", "Tablet configured." );
		else
			Log::Status( "NaoBrowser", "Tablet not configured." );
	}
	catch (const std::exception & e)
	{
		Log::Error("NaoBrowser", "Threw exception during configure: %s", e.what() );
	}
}

void NaoBrowser::DisplayLogo()
{
	try {
		if ( !m_Tablet.call<bool>("showImage", m_LogoUrl.c_str() ) )
			Log::Error( "NaoBrowser", "Failed to show logo" );
		else
			m_bLogoDisplayed = true;
	}
	catch (const std::exception & e)
	{
		Log::Error("NaoBrowser", "Threw exception during configure: %s", e.what() );
	}
}

qi::AnyReference NaoBrowser::OnTouchData(const std::vector <qi::AnyReference> & args)
{
	m_LastUpdate = Time().GetEpochTime();
	return qi::AnyReference();
}

