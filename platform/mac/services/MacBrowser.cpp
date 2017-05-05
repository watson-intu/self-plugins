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

#include "MacBrowser.h"

REG_SERIALIZABLE( MacBrowser );
RTTI_IMPL( MacBrowser, IBrowser );

REG_OVERRIDE_SERIALIZABLE( IBrowser, MacBrowser );

void MacBrowser::Serialize(Json::Value & json)
{
    IBrowser::Serialize(json);
}

void MacBrowser::Deserialize(const Json::Value & json)
{
    IBrowser::Deserialize(json);
}

bool MacBrowser::Start()
{
    Log::Status( "MacBrowser", "Starting.." );

    if (! IBrowser::Start() )
        return false;

    return true;
}

bool MacBrowser::Stop()
{
    Log::Status( "MacBrowser", "Stopping.." );

    return IBrowser::Stop();
}

void MacBrowser::ShowURL(const Url::SP & a_spUrlAgent, UrlCallback a_Callback)
{
    Log::Debug("MacBrowser", "Opening the following URL: %s", a_spUrlAgent->GetURL().c_str());
    std::string command = "open " + a_spUrlAgent->GetURL();
    system(command.c_str());
}
