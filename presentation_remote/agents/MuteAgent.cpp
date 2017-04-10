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

#include "MuteAgent.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"

REG_SERIALIZABLE( MuteAgent );
RTTI_IMPL(MuteAgent, IAgent);

void MuteAgent::Serialize(Json::Value & json)
{
    IAgent::Serialize(json);
}

void MuteAgent::Deserialize(const Json::Value & json)
{
    IAgent::Deserialize(json);
}

bool MuteAgent::OnStart()
{
    Log::Debug("MuteAgent", "Starting MuteAgent");
    SelfInstance::GetInstance()->GetBlackBoard()->SubscribeToType("Click",
                                                                  DELEGATE( MuteAgent, OnClick, const ThingEvent &, this ), TE_ADDED);
    return true;
}

bool MuteAgent::OnStop()
{
    Log::Debug("MuteAgent", "Stopping MuteAgent");
    SelfInstance::GetInstance()->GetBlackBoard()->UnsubscribeFromType("Click", this);
    return true;
}

void MuteAgent::OnClick(const ThingEvent & a_ThingEvent)
{
    IThing::SP spThing = a_ThingEvent.GetIThing();
    if (spThing)
    {
        if(spThing->GetData()["m_Input"].compare("b") == 0) {
            if(m_IsMuted)
            {
                Log::Debug("MuteAgent", "Resuming microphone");
                SelfInstance::GetInstance()->GetSensorManager()->ResumeSensorType("AudioData");

            }
            else
            {
                Log::Debug("MuteAgent", "Muting microphone");
                SelfInstance::GetInstance()->GetSensorManager()->PauseSensorType("AudioData" );
            }
            m_IsMuted = !m_IsMuted;
        }
        else
        {
            Log::Debug("MuteAgent", "Not sure what to do with this input: %s", spThing->GetData()["m_Input"].asString().c_str());
        }

    }
}