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


#ifndef WDC_ALCHEMY_H
#define WDC_ALCHEMY_H

#include "services/ILanguageParser.h"

class Alchemy : public ILanguageParser
{
public:
	RTTI_DECL();

	//! Construction 
	Alchemy();

	//! ISerializable
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IService interface
	virtual bool Start();
	virtual void GetServiceStatus(ServiceStatusCallback a_Callback);

	//! IAlchemy interface
	virtual void GetPosTags(const std::string & a_Text,
		Delegate<const Json::Value &> a_Callback );
	virtual void GetEntities(const std::string & a_Text,
		Delegate<const Json::Value &> a_Callback);

private:

	//! This class is responsible for checking whether the service is available or not
	class ServiceStatusChecker
	{
	public:
		ServiceStatusChecker(Alchemy* a_pDlgService, ServiceStatusCallback a_Callback);

	private:
		Alchemy* m_pAlchemyService;
		IService::ServiceStatusCallback m_Callback;

		void OnCheckService(const Json::Value & parsedResults);
	};
};

#endif
