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


//
// Created by John Andersen on 6/23/16.
//

#include "RemoteCameraAgent.h"
#include "SelfInstance.h"

RTTI_IMPL(RemoteCameraAgent, IAgent);
REG_SERIALIZABLE(RemoteCameraAgent);

void RemoteCameraAgent::Serialize(Json::Value & json)
{
    IAgent::Serialize(json);
    json["m_fPersonInterval"] = m_fPersonInterval;
    json["m_bSaySomething"] = m_bSaySomething;
}

void RemoteCameraAgent::Deserialize(const Json::Value & json)
{
    IAgent::Deserialize(json);
    if (json.isMember("m_fPersonInterval"))
        m_fPersonInterval = json["m_fPersonInterval"].asFloat();
    if (json.isMember("m_bSaySomething"))
        m_bSaySomething = json["m_bSaySomething"].asBool();
}

bool RemoteCameraAgent::OnStart()
{
    SelfInstance::GetInstance()->GetBlackBoard()->SubscribeToType(Person::GetStaticRTTI(),
                                                                  DELEGATE( RemoteCameraAgent, OnPerson, const ThingEvent &, this ), TE_ADDED);
    SelfInstance::GetInstance()->GetBlackBoard()->SubscribeToType(CameraIntent::GetStaticRTTI(),
                                                                  DELEGATE(RemoteCameraAgent, OnCameraIntent, const ThingEvent &, this), TE_ADDED);

    m_spWaitTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(RemoteCameraAgent, OnTimer, this), m_fPersonInterval, true, true);

    return true;
}

bool RemoteCameraAgent::OnStop()
{

    return true;
}

void RemoteCameraAgent::OnCameraIntent(const ThingEvent & a_ThingEvent)
{
    CameraIntent::SP spCameraIntent = DynamicCast<CameraIntent>( a_ThingEvent.GetIThing() );
    PTZCamera * camera = SelfInstance::GetInstance()->FindService<PTZCamera>();
    camera->SetCameraCoordinates(spCameraIntent->GetTarget(), DELEGATE(RemoteCameraAgent, OnCameraMovement, const std::string & , this));
}

void RemoteCameraAgent::OnCameraMovement(const std::string & a_Response)
{
    Log::Debug("RemoteCameraAgent", "Camera has moved.");
}

void RemoteCameraAgent::OnPerson(const ThingEvent & a_ThingEvent)
{
    Person::SP spPerson = DynamicCast<Person>( a_ThingEvent.GetIThing() );
    if (m_bSaySomething) {
        Goal::SP spGoal(new Goal("RemoteCamera"));
        SelfInstance::GetInstance()->GetBlackBoard()->AddThing(spGoal);

        // reset the flag
        m_bSaySomething = false;
    }
}

void RemoteCameraAgent::OnTimer()
{
    m_bSaySomething = true;
}
