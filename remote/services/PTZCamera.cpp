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

#include "PTZCamera.h"
#include "SelfInstance.h"

REG_SERIALIZABLE(PTZCamera);
RTTI_IMPL(PTZCamera, IService);

PTZCamera::PTZCamera() : IService("PTZCamera")
{}

void PTZCamera::Serialize(Json::Value & json)
{
    IService::Serialize(json);
}


void PTZCamera::Deserialize(const Json::Value & json)
{
    IService::Deserialize(json);

}

bool PTZCamera::Start()
{
    Log::Debug("PTZCamera", "Starting PTZCamera!");
    if (!IService::Start())
        return false;

    return true;
}

IService::Request * PTZCamera::GetImage(GetImageObject a_Callback)
{
    Log::Debug("PTZCamera", "Invoking Get Image Request");

    return new RequestData(this, EMPTY_STRING, "GET", NULL_HEADERS, EMPTY_STRING, a_Callback);
}

IService::Request * PTZCamera::SetCameraCoordinates(const std::string & a_Direction, GetImageObject a_Callback)
{
    Log::Debug("PTZCamera", "Invoking Set Camera Request");
    std::string parameters = StringUtil::Format("/axis-cgi/com/ptz.cgi?move=%s&camera=1", a_Direction.c_str());

   return new RequestData(this, parameters, "GET", NULL_HEADERS, EMPTY_STRING, a_Callback);
}


