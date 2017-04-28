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


#ifndef WDC_ALCHEMYNEWS_H
#define WDC_ALCHEMYNEWS_H

#include "services/INews.h"

class AlchemyNews : public INews
{
public:
	RTTI_DECL();

	AlchemyNews();

	//! ISerializable
	void Serialize(Json::Value & json);
	void Deserialize(const Json::Value & json);

	//! IService interface
	bool Start();

	//! INews interface
	virtual void GetNews(const std::string & a_Subject, time_t a_StartDate, time_t a_EndDate, int a_NumberOfArticles,
		Delegate<const Json::Value &> a_Callback);

private:
	//!Data
	std::vector<std::string>	m_ReturnParameters;
};

#endif // WDC_ALCHEMYNEWS_H


