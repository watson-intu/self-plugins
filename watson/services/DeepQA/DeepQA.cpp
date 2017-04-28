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


#include "DeepQA.h"
#include "utils/JsonHelpers.h"

REG_SERIALIZABLE( DeepQA );
RTTI_IMPL( DeepQA, IService );

DeepQA::DeepQA() : IService("DeepQAV1")
{}

//! ISerializable
void DeepQA::Serialize(Json::Value & json)
{
    IService::Serialize(json);
}

void DeepQA::Deserialize(const Json::Value & json)
{
    IService::Deserialize(json);
}

//! IService interface
bool DeepQA::Start()
{
    if (!IService::Start())
        return false;

    Log::Debug("DeepQA", "Url loaded as %s", m_pConfig->m_URL.c_str() );
    
    // Setup header
    m_Headers["Content-Type"] = "application/json";
    m_Headers["X-SyncTimeout"] = "-1";

    return true;
}

void DeepQA::AskQuestion(const std::string & a_Question, Delegate<const Json::Value &> a_Callback, 
    int a_NumAnswers, bool a_bInferQuestion, Json::Value a_Evidence)
{
    std::string endpoint = "/v1/question/";

    // Form question
    Json::Value question;
    question["formattedAnswer"] = true;
    question["questionText"] = a_Question;
    question["status"] = "Accepted";
    question["evidenceRequest"] = a_Evidence;
    question["inferQuestion"] = a_bInferQuestion;
    Json::Value body;
    body["question"] = question;

    Log::Debug("DeepQAService", "Body by string: %s", body.toStyledString().c_str() );    
    new RequestJson(this, endpoint, "POST", NULL_HEADERS, body.toStyledString(), a_Callback,
		new CacheRequest( "AskQuestion", JsonHelpers::Hash( body ) ) );
}
