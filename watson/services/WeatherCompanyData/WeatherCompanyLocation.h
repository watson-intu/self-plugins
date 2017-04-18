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

#ifndef WDC_WEATHER_LOCATION_H
#define WDC_WEATHER_LOCATION_H

#include "services/ILocation.h"

class WeatherCompanyLocation : public ILocation
{
public:
	RTTI_DECL();

	//! Types
	typedef Delegate<const Json::Value &>			SendCallback;

	//! Construction
	WeatherCompanyLocation();

	//! IService interface
	virtual bool Start();

	//! ILocation interface
	void GetLocation( const std::string & a_Location, Delegate<const Json::Value &> a_Callback);
	void GetTimeZone(const double & a_Latitude, const double & a_Longitude, Delegate<const Json::Value &> a_Callback);

private:
	//! Data
	std::string m_Language;
};

#endif
