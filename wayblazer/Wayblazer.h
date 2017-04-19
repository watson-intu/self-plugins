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

#ifndef SELF_URLSERVICE_H
#define SELF_URLSERVICE_H

#include "services/IBrowser.h"

class Wayblazer : public IBrowser
{
public:
	RTTI_DECL();

	//! Construction
	Wayblazer();

	//! IService interface
	virtual bool Start();
	virtual bool Stop();
	virtual void GetServiceStatus(ServiceStatusCallback a_Callback);

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IURL interface
	virtual void ShowURL(const Url::SP & a_spUrlAgent, UrlCallback a_Callback);

	//! Callbacks
	void OnResult(const Json::Value & json);

protected:

	//! Data
	int m_HeartBeatInterval;
	TimerPool::ITimer::SP m_spHeartBeatTimer;
	std::string	m_AvailabilitySuffix;
	std::string	m_FunctionalSuffix;

	//! This class is responsible for checking whether the service is available or not
	class ServiceStatusChecker
	{
	public:
		ServiceStatusChecker(Wayblazer *a_pURLService, ServiceStatusCallback a_Callback);

	private:
		Wayblazer * m_pService;
		IService::ServiceStatusCallback m_Callback;

		void OnCheckService(const Json::Value & a_Json );
	};

	void MakeHeartBeat();
	void OnHeartBeatResponse(const Json::Value & a_Response);
};

#endif
