/* ***************************************************************** */
/*                                                                   */
/* IBM Confidential                                                  */
/* OCO Source Materials                                              */
/*                                                                   */
/* (C) Copyright IBM Corp. 2001, 2014                                */
/*                                                                   */
/* The source code for this program is not published or otherwise    */
/* divested of its trade secrets, irrespective of what has been      */
/* deposited with the U.S. Copyright Office.                         */
/*                                                                   */
/* ***************************************************************** */

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