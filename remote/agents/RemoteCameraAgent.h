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

#ifndef SELF_REMOTECAMERAAGENT_H
#define SELF_REMOTECAMERAAGENT_H

#include "agent/IAgent.h"
#include "blackboard/Person.h"
#include "blackboard/CameraIntent.h"
#include "blackboard/Goal.h"
#include "services/PTZCamera.h"
#include "utils/TimerPool.h"

class RemoteCameraAgent : public IAgent
{
public:

    RTTI_DECL();

    //! Constructor
    RemoteCameraAgent(): m_fPersonInterval(30.0f), m_bSaySomething(false) {}

    //! ISerializable interface
    virtual void Serialize(Json::Value &json);
    virtual void Deserialize(const Json::Value &json);

    //! IAgent interface
    virtual bool OnStart();
    virtual bool OnStop();


private:

    void    OnCameraIntent(const ThingEvent & a_ThingEvent);
    void    OnPerson(const ThingEvent & a_ThingEvent);
    void    OnCameraMovement(const std::string & a_Response);
    void    OnTimer();

    float   m_fPersonInterval;
    bool    m_bSaySomething;
    TimerPool::ITimer::SP   m_spWaitTimer;
};

#endif //SELF_REMOTECAMERAAGENT_H
