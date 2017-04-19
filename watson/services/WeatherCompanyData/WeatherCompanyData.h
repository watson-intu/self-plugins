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

#ifndef WDC_WEATHERCOMPANYDATA_H
#define WDC_WEATHERCOMPANYDATA_H

#include "services/IWeather.h"

class WeatherCompanyData : public IWeather
{
public:
    RTTI_DECL();

    //! Types
    typedef Delegate<const Json::Value &>			SendCallback;

    //! Construction
    WeatherCompanyData();

    //! ISerializable interface
    virtual void Serialize(Json::Value & json);
    virtual void Deserialize(const Json::Value & json);

    //! IService interface
    virtual bool Start();

	//! IWeather interface
    void GetCurrentConditions( SendCallback a_Callback );
    void GetCurrentConditions(const std::string & a_Lat, const std::string & a_Long, 
		SendCallback a_Callback);
    void GetHourlyForecast( SendCallback a_Callback );
    void GetTenDayForecast( SendCallback a_Callback );
    void GetTenDayForecast(const std::string & a_Lat, const std::string & a_Long,
		SendCallback a_Callback);

private:
	//! Data
    float m_Latitude;
    float m_Longitude;
    std::string m_Units;
    std::string m_Language;
};

#endif
