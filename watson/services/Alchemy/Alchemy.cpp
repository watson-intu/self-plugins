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


#include "Alchemy.h"
#include "AlchemyNews.h"
#include "utils/Config.h"

REG_SERIALIZABLE( Alchemy );
REG_OVERRIDE_SERIALIZABLE( ILanguageParser, Alchemy );
RTTI_IMPL( Alchemy, ILanguageParser );


Alchemy::Alchemy() : ILanguageParser("AlchemyV1", AUTH_USER )
{}

//! ISerializable
void Alchemy::Serialize(Json::Value & json)
{
	ILanguageParser::Serialize(json);
}

void Alchemy::Deserialize(const Json::Value & json)
{
	ILanguageParser::Deserialize(json);

}

//! IAlchemy interface
bool Alchemy::Start()
{
	if (!ILanguageParser::Start())
		return false;

	if (!StringUtil::EndsWith(m_pConfig->m_URL, "calls"))
	{
		Log::Error("Alchemy", "Configured URL not ended with calls");
		return false;
	}
	if (m_pConfig->m_User.size() == 0)
		Log::Warning("Alchemy", "API-Key expected in user field.");

	// add the AlchemyNews service if no INews service is available.
	if ( Config::Instance()->FindService<INews>() == NULL )
		Config::Instance()->GetService<AlchemyNews>();

	return true;
}

void Alchemy::GetServiceStatus( ServiceStatusCallback a_Callback )
{
	if (m_pConfig != NULL)
		new ServiceStatusChecker(this, a_Callback);
	else
		a_Callback(ServiceStatus(m_ServiceId, false));
}

//! Creates an object responsible for service status checking
Alchemy::ServiceStatusChecker::ServiceStatusChecker(Alchemy* a_pAlchemyService, ServiceStatusCallback a_Callback)
		: m_pAlchemyService(a_pAlchemyService), m_Callback(a_Callback)
{
	m_pAlchemyService->GetPosTags("This is a test",
	    DELEGATE(ServiceStatusChecker, OnCheckService, const Json::Value &, this));
}

//! Callback function invoked when service status is checked
void Alchemy::ServiceStatusChecker::OnCheckService(const Json::Value & parsedResults)
{
	if (m_Callback.IsValid())
	{
		bool success = !parsedResults.isNull();
		m_Callback(ServiceStatus(m_pAlchemyService->m_ServiceId, success));
	}
	delete this;
}

void Alchemy::GetPosTags(const std::string & a_Text,
	Delegate<const Json::Value &> a_Callback )
{
	std::string parameters = "/text/TextGetPOSTags";
	parameters += "?apikey=" + m_pConfig->m_User;
	parameters += "&outputMode=json";
	parameters += "&text=" + StringUtil::UrlEscape( a_Text );

	new RequestJson(this, parameters, "GET", NULL_HEADERS, EMPTY_STRING, a_Callback, 
		new CacheRequest( "GetPosTags", StringHash::DJB(a_Text.c_str()) ) );
}

void Alchemy::GetEntities(const std::string & a_Text, Delegate<const Json::Value &> a_Callback)
{
	std::string parameters = "/text/TextGetRankedNamedEntities";
	parameters += "?apikey=" + m_pConfig->m_User;
	parameters += "&outputMode=json";
	parameters += "&text=" + StringUtil::UrlEscape( a_Text );

	new RequestJson(this, parameters, "GET", NULL_HEADERS, EMPTY_STRING, a_Callback,
		new CacheRequest( "TextGetRankedNamedEntities", StringHash::DJB(a_Text.c_str()) ) );
}

