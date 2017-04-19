/**
* Copyright 2015 IBM Corp. All Rights Reserved.
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

#include "Wayblazer.h"

REG_SERIALIZABLE( Wayblazer );
REG_OVERRIDE_SERIALIZABLE( IBrowser, Wayblazer );
RTTI_IMPL( Wayblazer, IBrowser );

class RequestURL : public IService::RequestJson
{
public:
	RequestURL( IService * a_pService,
		const std::string & a_Parameters,		// additional data to append onto the endpoint
		const std::string & a_RequestType,		// type of request GET, POST, DELETE
		const Headers & a_Headers,				// additional headers to add to the request
		const std::string & a_Body,				// the body to send if any
		Delegate<IBrowser::URLServiceData *> a_Callback,
		const Url::SP & a_spUrl,
		float a_fTimeOut = 30.0f ) :
		m_Callback(a_Callback), m_spUrl(a_spUrl),
		RequestJson(a_pService, a_Parameters, a_RequestType, a_Headers, a_Body,
			DELEGATE(RequestURL, OnResponse, const Json::Value &, this), NULL, a_fTimeOut )
	{}

private:
	void OnResponse( const Json::Value & a_JsonResponseData )
	{
		IBrowser::URLServiceData * urlServiceData = new IBrowser::URLServiceData();
		urlServiceData->m_JsonValue = a_JsonResponseData;
		urlServiceData->m_spUrl = m_spUrl;

		if (m_Callback.IsValid())
			m_Callback( urlServiceData );
	}

	Url::SP m_spUrl;
	IBrowser::UrlCallback	m_Callback;
};

Wayblazer::Wayblazer() : IBrowser( "URLServiceV1" ),
	m_AvailabilitySuffix( "/heartbeat" ),
	m_FunctionalSuffix( "/displayurl" )
{}

void Wayblazer::Serialize(Json::Value & json)
{
	IBrowser::Serialize( json );

	json["m_HeartBeatInterface"] = m_HeartBeatInterval;
	json["m_AvailabilitySuffix"] = m_AvailabilitySuffix;
	json["m_FunctionalSuffix"] = m_FunctionalSuffix;
}

void Wayblazer::Deserialize(const Json::Value & json)
{
	IBrowser::Deserialize( json );

	if( json["m_HeartBeatInterval"].isNumeric() )
		m_HeartBeatInterval = json["m_HeartBeatInterval"].asInt();
	if( json["m_AvailabilitySuffix"].isString() )
		m_AvailabilitySuffix = json["m_AvailabilitySuffix"].asString();
	if( json["m_FunctionalSuffix"].isString() )
		m_FunctionalSuffix = json["m_FunctionalSuffix"].asString();
}

bool Wayblazer::Start()
{
	if (! IBrowser::Start() )
		return false;

	if (! StringUtil::EndsWith( m_pConfig->m_URL, "/api" ) ) // need to update this
	{
		Log::Error( "URLService", "Configured URL not ended with /api" ); // need to update this
		return false;
	}

	TimerPool * pTimers = TimerPool::Instance();
	if ( pTimers != NULL )
	{
		m_spHeartBeatTimer = pTimers->StartTimer( 
			VOID_DELEGATE( Wayblazer, MakeHeartBeat, this ), m_HeartBeatInterval, true, true );
	}

	return true;
}

bool Wayblazer::Stop()
{
	m_spHeartBeatTimer.reset();
	return IBrowser::Stop();
}

//TODO: Check status of WB server here (and only server)
void Wayblazer::GetServiceStatus( ServiceStatusCallback a_Callback )
{
	if (m_pConfig != NULL)
		new ServiceStatusChecker(this, a_Callback);
	else
		a_Callback(ServiceStatus(m_ServiceId, false));
}

//! Creates an object responsible for service status checking
Wayblazer::ServiceStatusChecker::ServiceStatusChecker(Wayblazer* a_pService, ServiceStatusCallback a_Callback)
	: m_pService(a_pService), m_Callback(a_Callback)
{
	new RequestJson( a_pService, m_pService->m_AvailabilitySuffix, "GET", NULL_HEADERS, EMPTY_STRING,
		DELEGATE(ServiceStatusChecker, OnCheckService, const Json::Value &, this) );
}

//! Callback function invoked when service status is checked
void Wayblazer::ServiceStatusChecker::OnCheckService(const Json::Value & a_Json)
{
	if (m_Callback.IsValid())
		m_Callback(ServiceStatus(m_pService->m_ServiceId, a_Json.isNull() != false ));

	delete this;
}

void Wayblazer::ShowURL( const Url::SP & a_spUrl, UrlCallback a_Callback )
{
	// Example request URL
	// {"url":"http://WatsonLabs:IBM4you.@hilton.wayblazer.com/locations/washington-dc/search?q=Can%20you%20recommend%20a%20shell%20fish%20restaurant"}
	Headers headers;
	headers["Content-Type"] = "application/json";

	Json::Value json;
	json["url"] = EscapeUrl( a_spUrl->GetURL() );

	new RequestURL( this, m_FunctionalSuffix, "POST", headers, json.toStyledString(), a_Callback, a_spUrl);
}

void Wayblazer::MakeHeartBeat()
{
	new RequestJson( this, m_AvailabilitySuffix, "GET", NULL_HEADERS, EMPTY_STRING, 
		DELEGATE(Wayblazer, OnHeartBeatResponse, const Json::Value &, this)  );
}

void Wayblazer::OnHeartBeatResponse(const Json::Value & a_Response )
{
	if ( a_Response.isNull() )
		Log::DebugLow("URLService", "Connection to Wayblazer Failed");
}

