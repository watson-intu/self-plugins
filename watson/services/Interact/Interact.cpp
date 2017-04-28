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


#include "Interact.h"
#include "SelfInstance.h"
#include "utils/JsonHelpers.h"

#define ENABLE_INTERACT_CACHE			0

REG_SERIALIZABLE( Interact );
RTTI_IMPL( Interact, IService );


Interact::Interact() : IService("InteractV1")
{}

//! ISerializable
void Interact::Serialize(Json::Value & json)
{
	IService::Serialize(json);
}

void Interact::Deserialize(const Json::Value & json)
{
	IService::Deserialize(json);
}

//! IService interface
bool Interact::Start()
{
	if (!IService::Start())
		return false;

	Log::Debug("Interact", "Url loaded as %s", m_pConfig->m_URL.c_str() );

	// Setup header
	m_Headers["Content-Type"] = "application/json";

	return true;
}

void Interact::Converse(const std::string & a_Question, Delegate<const Json::Value &> a_Callback)
{
	std::string endpoint = "/v1/api/converse";

	// Form question
	Json::Value question;
	question["text"] = a_Question;
	//question["userId"] = "";			//!> optional paramters
	//question["cityName"] = "";

#if ENABLE_INTERACT_CACHE
	new RequestJson(this, endpoint, "POST", NULL_HEADERS, question.toStyledString(), a_Callback,
		new CacheRequest( "Converse", JsonHelpers::Hash( question ) ) );
#else
	new RequestJson(this, endpoint, "POST", NULL_HEADERS, question.toStyledString(), a_Callback );
#endif
}
