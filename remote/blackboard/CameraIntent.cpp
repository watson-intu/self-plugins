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

#include "CameraIntent.h"
#include "utils/StringUtil.h"

RTTI_IMPL(CameraIntent, IIntent);
REG_SERIALIZABLE(CameraIntent);

CameraIntent::CameraIntent()
{}

void CameraIntent::Serialize(Json::Value &json)
{
    IIntent::Serialize(json);

}

void CameraIntent::Deserialize(const Json::Value &json)
{
    IIntent::Deserialize(json);

}

void CameraIntent::Create(const Json::Value & a_Intent, const Json::Value & a_Parse)
{
    std::vector<std::string> m_Words;
    StringUtil::Split(a_Intent["top_class"].asString(), "_",m_Words);
    m_Target = m_Words.at(1);
}

