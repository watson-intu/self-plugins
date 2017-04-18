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
#include "utils/JsonHelpers.h"
#include "utils/Config.h"

REG_SERIALIZABLE(WeatherCompanyData);
RTTI_IMPL(WeatherCompanyData, IWeather);

WeatherCompanyData::WeatherCompanyData() : IWeather("WeatherCompanyDataV1"), m_Latitude(30.16f), m_Longitude(97.44f), m_Language("en-US")
{}

void WeatherCompanyData::Serialize(Json::Value & json)
{
	IWeather::Serialize(json);

	json["m_Latitude"] = m_Latitude;
	json["m_Longitude"] = m_Longitude;
	json["m_Units"] = m_Units;
	json["m_Language"] = m_Language;
}

void WeatherCompanyData::Deserialize(const Json::Value & json)
{
	IWeather::Deserialize(json);

	if (json.isMember("m_Latitude"))
		m_Latitude = json["m_Latitude"].asFloat();
	if (json.isMember("m_Longitude"))
		m_Longitude = json["m_Longitude"].asFloat();
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
		Config::Instance()->AddService(new WeatherCompanyLocation());

	return true;
}

void WeatherCompanyData::GetCurrentConditions(SendCallback a_Callback)
{
	std::string parameters = "/v1";
	parameters += "/geocode/" + StringUtil::Format("%f", m_Latitude) + "/" + StringUtil::Format("%f", m_Longitude);
	parameters += "/forecast/hourly/48hour.json";
	parameters += "?units=" + m_Units;
	parameters += "&language=" + m_Language;

	new RequestJson(this, parameters, "GET", m_Headers, EMPTY_STRING, a_Callback);
}

void WeatherCompanyData::GetCurrentConditions(const std::string & a_Lat, const std::string & a_Long, SendCallback a_Callback)
{
	std::string parameters = "/v1";
	parameters += "/geocode/" + a_Lat + "/" + a_Long;
	parameters += "/forecast/hourly/48hour.json";
	parameters += "?units=" + m_Units;
	parameters += "&language=" + m_Language;

	new RequestJson(this, parameters, "GET", m_Headers, EMPTY_STRING, a_Callback);
}

void WeatherCompanyData::GetHourlyForecast(SendCallback a_Callback)
{
	std::string parameters = "/v1";
	parameters += "/geocode/" + StringUtil::Format("%f", m_Latitude) + "/" + StringUtil::Format("%f", m_Longitude);
	parameters += "/forecast/hourly/24hour.json";
	parameters += "?units=" + m_Units;
	parameters += "&language=" + m_Language;

	new RequestJson(this, parameters, "GET", m_Headers, EMPTY_STRING, a_Callback);
}

void WeatherCompanyData::GetTenDayForecast(SendCallback a_Callback)
{
	std::string parameters = "/v1";
	parameters += "/geocode/" + StringUtil::Format("%f", m_Latitude) + "/" + StringUtil::Format("%f", m_Longitude);
	parameters += "/forecast/daily/10day.json";
	parameters += "?units=" + m_Units;
	parameters += "&language=" + m_Language;

	new RequestJson(this, parameters, "GET", m_Headers, EMPTY_STRING, a_Callback);
}

void WeatherCompanyData::GetTenDayForecast(const std::string & a_Lat, const std::string & a_Long, SendCallback a_Callback)
{
	std::string parameters = "/v1";
	parameters += "/geocode/" + a_Lat + "/" + a_Long;
	parameters += "/forecast/daily/10day.json";
	parameters += "?units=" + m_Units;
	parameters += "&language=" + m_Language;

	new RequestJson(this, parameters, "GET", m_Headers, EMPTY_STRING, a_Callback);
}

