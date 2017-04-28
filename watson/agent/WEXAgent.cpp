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


#include "WEXAgent.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Goal.h"
#include "services/WEX.h"
#include "skills/SkillManager.h"
#include "utils/JsonHelpers.h"
#include "tinyxml/tinyxml.h"

REG_SERIALIZABLE(WEXAgent);
RTTI_IMPL(WEXAgent, IAgent);

WEXAgent::WEXAgent()
{}

void WEXAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);
}

void WEXAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);
}

bool WEXAgent::OnStart()
{
	Log::Debug("WEXAgent", "WEX Agent has started!");
	SelfInstance::GetInstance()->GetBlackBoard()->SubscribeToType(WEXIntent::GetStaticRTTI(),
		DELEGATE(WEXAgent, OnWexIntent, const ThingEvent &, this), TE_ADDED);

	return true;
}

bool WEXAgent::OnStop()
{
	Log::Debug("WEXAgent", "WEX Agent has stopped!");
	SelfInstance::GetInstance()->GetBlackBoard()->UnsubscribeFromType(WEXIntent::GetStaticRTTI(), this);
	return true;
}

void WEXAgent::OnWexIntent(const ThingEvent & a_ThingEvent)
{
	WEXIntent::SP spWex = DynamicCast<WEXIntent>(a_ThingEvent.GetIThing());
	if (spWex)
	{
		if (!m_spActive)
		{
			ExecuteRequest(spWex);
		}
		else
		{
			// wex request already active, just push into the queue
			m_WexRequests.push_back(spWex);
		}
	}
}

void WEXAgent::ExecuteRequest(WEXIntent::SP a_pWEX)
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	Log::Debug("WEXAgent", "WEX Intent object added to blackboard detected");

	WEX * pWEX = pInstance->FindService<WEX>();
	if ( pWEX != NULL )
	{
		m_spActive = a_pWEX;
		m_spActive->SetState("PROCESSING");

		pWEX->Search(a_pWEX->GetText(), a_pWEX->GetCollection(), DELEGATE(WEXAgent, OnMessage, const std::string &, this));
	}
	else
	{
		Log::Error( "WEXAgent", "WEX service not available." );
		a_pWEX->SetState( "ERROR" );
	}
}

void WEXAgent::OnMessage(const std::string & a_Callback)
{
	bool bSuccess = false;
	if ( a_Callback.size() > 0 )
	{
		TiXmlDocument xml;
		xml.Parse(a_Callback.c_str());

		if (!xml.Error())
		{
			Json::Value json;
			JsonHelpers::MakeJSON(xml.FirstChildElement(), json);
			json["question"] = m_spActive->GetText();
			Goal::SP spGoal(new Goal("WEX"));
			spGoal->SetParams(json);

			m_spActive->AddChild(spGoal);
			m_spActive->SetState("COMPLETED");
			bSuccess = true;
		}
	}

	if (! bSuccess )
		m_spActive->SetState( "ERROR" );

	if (m_WexRequests.begin() != m_WexRequests.end())
	{
		WEXIntent::SP spWex = m_WexRequests.front();
		m_WexRequests.pop_front();

		ExecuteRequest(spWex);
	}
	else
	{
		m_spActive.reset();
	}

}