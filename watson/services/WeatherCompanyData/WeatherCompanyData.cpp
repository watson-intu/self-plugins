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


#include "WeatherCompanyData.h"
#include "WeatherCompanyLocation.h"
#include "services/ILocation.h"
#include "utils/JsonHelpers.h"
#include "utils/Config.h"

REG_SERIALIZABLE( WeatherCompanyData );
REG_OVERRIDE_SERIALIZABLE( IWeather, WeatherCompanyData );
RTTI_IMPL(WeatherCompanyData, IWeather);

WeatherCompanyData::WeatherCompanyData() : IWeather("WeatherCompanyDataV1"), m_Language("en-US")
{}

void WeatherCompanyData::Serialize(Json::Value & json)
{
	IWeather::Serialize(json);

	json["m_Units"] = m_Units;
	json["m_Language"] = m_Language;
}

void WeatherCompanyData::Deserialize(const Json::Value & json)
{
	IWeather::Deserialize(json);

	if (json.isMember("m_Units"))
		m_Units = json["m_Units"].asString();
	if (json.isMember("m_Language"))
		m_Language = json["m_Language"].asString();
}

bool WeatherCompanyData::Start()
{
	Log::Debug("WeatherCompanyData", "Starting WeatherCompanyData!");
	if (!IWeather::Start())
		return false;

	if (!StringUtil::EndsWith(m_pConfig->m_URL, "api/weather"))
	{
		Log::Error("WeatherCompanyData", "Configured URL not ended with api/weather");
		return false;
	}

	if (Config::Instance()->FindService<ILocation>() == NULL)
		Config::Instance()->AddService( IService::SP( new WeatherCompanyLocation() ) );

	return true;
}

void WeatherCompanyData::GetCurrentConditions(Location * a_Location, SendCallback a_Callback)
{
	VerifyLocation(a_Location);
	std::string parameters = "/v1";
	parameters += "/geocode/" + StringUtil::Format("%f", a_Location->GetLatitude()) + "/"
				  + StringUtil::Format("%f", a_Location->GetLongitude());
	parameters += "/forecast/hourly/48hour.json";
	parameters += "?units=" + m_Units;
	parameters += "&language=" + m_Language;

	new RequestJson(this, parameters, "GET", m_Headers, EMPTY_STRING, a_Callback);
}

void WeatherCompanyData::GetHourlyForecast(Location * a_Location, SendCallback a_Callback)
{
	VerifyLocation(a_Location);
	std::string parameters = "/v1";
	parameters += "/geocode/" + StringUtil::Format("%f", a_Location->GetLatitude()) + "/"
				  + StringUtil::Format("%f", a_Location->GetLongitude());
	parameters += "/forecast/hourly/24hour.json";
	parameters += "?units=" + m_Units;
	parameters += "&language=" + m_Language;

	new RequestJson(this, parameters, "GET", m_Headers, EMPTY_STRING, a_Callback);
}

void WeatherCompanyData::GetTenDayForecast(Location * a_Location, SendCallback a_Callback)
{
	VerifyLocation(a_Location);
	std::string parameters = "/v1";
	parameters += "/geocode/" + StringUtil::Format("%f", a_Location->GetLatitude()) + "/"
				  + StringUtil::Format("%f", a_Location->GetLongitude());
	parameters += "/forecast/daily/10day.json";
	parameters += "?units=" + m_Units;
	parameters += "&language=" + m_Language;

	new RequestJson(this, parameters, "GET", m_Headers, EMPTY_STRING, a_Callback);
}

bool WeatherCompanyData::VerifyLocation(Location * a_Location)
{
	if ( a_Location == NULL )
		a_Location = new Location();
	return true;
}

