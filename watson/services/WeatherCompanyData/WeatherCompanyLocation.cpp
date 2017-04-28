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


#include "WeatherCompanyLocation.h"
#include "utils/JsonHelpers.h"

REG_SERIALIZABLE( WeatherCompanyLocation );
RTTI_IMPL( WeatherCompanyLocation, ILocation );

WeatherCompanyLocation::WeatherCompanyLocation() : ILocation("WeatherCompanyDataV1"), m_Language( "en-US" )
{}

bool WeatherCompanyLocation::Start()
{
	Log::Debug("WeatherCompanyData", "Starting WeatherLocation!");
	if ( !ILocation::Start() )
		return false;

	if (! StringUtil::EndsWith( m_pConfig->m_URL, "api/weather" ) )
	{
		Log::Error( "WeatherCompanyData", "Configured URL not ended with api/weather" );
		return false;
	}

	return true;
}

void WeatherCompanyLocation::GetLocation(const std::string & a_Location, Delegate<const Json::Value &> a_Callback)
{
	std::string parameters = "/v3";
	parameters += "/location/search";
	parameters += "?query=" + StringUtil::UrlEscape(a_Location);
	parameters += "&language=" + m_Language;

	new RequestJson(this, parameters, "GET", m_Headers, EMPTY_STRING, a_Callback);
}

void WeatherCompanyLocation::GetTimeZone(const double & a_Latitude, const double & a_Longitude, Delegate<const Json::Value &> a_Callback)
{
	std::string parameters = "/v3";
	parameters+= "/location/point";
	parameters += "?geocode=" + StringUtil::Format("%f,%f", a_Latitude, a_Longitude);
	parameters += "&language=" + m_Language;

	new RequestJson(this, parameters, "GET", m_Headers, EMPTY_STRING, a_Callback);
}

