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


#ifndef SELF_WEXAGENT_H
#define SELF_WEXAGENT_H


#include "agent/IAgent.h"
#include "blackboard/WEXIntent.h"

class SkillInstance;

class WEXAgent : public IAgent
{
public:
    RTTI_DECL();

    WEXAgent();

    //! ISerializable interface
    void Serialize( Json::Value & json );
    void Deserialize( const Json::Value & json );

    //! IAgent interface
    virtual bool OnStart();
    virtual bool OnStop();

private:
    //! Types
    typedef std::list<WEXIntent::SP>		WEXList;
    //! Event Handlers
    void                    OnWexIntent(const ThingEvent & a_ThingEvent);
    void                    ExecuteRequest(WEXIntent::SP a_pWEX);
    void                    OnMessage( const std::string & a_Callback );

    WEXIntent::SP	        m_spActive;
    WEXList                 m_WexRequests;

};

#endif //SELF_WEXAGENT_H
