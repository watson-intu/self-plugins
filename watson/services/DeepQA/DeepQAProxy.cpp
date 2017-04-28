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


#include "DeepQAProxy.h"
#include "SelfInstance.h"
#include "utils/IService.h"

REG_SERIALIZABLE( DeepQAProxy );
RTTI_IMPL( DeepQAProxy, IQuestionAnswerProxy );


DeepQAProxy::DeepQAProxy() : m_fMinConfidence(0.0f),
	m_ServiceId( "DeepQAV1" ),
	m_PipelineType( "wda" )
{}

//! ISerializable
void DeepQAProxy::Serialize(Json::Value & json)
{
    IQuestionAnswerProxy::Serialize(json);
	json["m_ServiceId"] = m_ServiceId;
	json["m_PipelineType"] = m_PipelineType;
	json["m_fMinConfidence"] = m_fMinConfidence;
	SerializeVector("m_LowConfidenceResponses", m_LowConfidenceResponses, json);
}

void DeepQAProxy::Deserialize(const Json::Value & json)
{
    IQuestionAnswerProxy::Deserialize(json);
	if ( json["m_ServiceId"].isString() )
		m_ServiceId = json["m_ServiceId"].asString();
	if ( json["m_PipelineType"].isString() )
		m_PipelineType = json["m_PipelineType"].asString();
	if ( json["m_fMinConfidence"].isDouble() )
		m_fMinConfidence = json["m_fMinConfidence"].asFloat();
	DeserializeVector("m_LowConfidenceResponses", json, m_LowConfidenceResponses);
    
    if ( m_LowConfidenceResponses.size() == 0 )
        m_LowConfidenceResponses.push_back("I don't know the answer to that one");
}

//! IQuestionAnswer
void DeepQAProxy::AskQuestion( QuestionIntent::SP a_spQuestion, Delegate<const Json::Value &> a_Callback)
{
    std::string temp = a_spQuestion->GetText();    
    new DeepQAProxy::DeepQARequest( this, temp, a_Callback );
}

void DeepQAProxy::Start()
{
    Log::Debug("DeepQAProxy", "Started...");
    m_pDeepQA = SelfInstance::GetInstance()->FindService<DeepQA>( m_ServiceId );
}

void DeepQAProxy::Stop()
{
	Log::Debug("DeepQAProxy", "Stopped..." );
}

//! DeepQARequest helper object
DeepQAProxy::DeepQARequest::DeepQARequest( DeepQAProxy * a_pDeepQAProxy, std::string a_Question, 
    Delegate<const Json::Value &> a_Callback ) :
    m_pDeepQAProxy( a_pDeepQAProxy ),
    m_Question( a_Question ),
    m_Callback( a_Callback )
{
    SendRequest();
}

void DeepQAProxy::DeepQARequest::SendRequest()
{
    if ( m_pDeepQAProxy->m_pDeepQA ) {
        m_pDeepQAProxy->m_pDeepQA->AskQuestion(m_Question, 
            DELEGATE( DeepQARequest, OnResponse, const Json::Value &, this ));
    }
    else
    {
        Log::Error("DeepQAProxy", "DeepQA service not configured");
        delete this;
    }
}

void DeepQAProxy::DeepQARequest::OnResponse( const Json::Value & json )
{    
	Json::Value answer;
	if (! json.isNull() )
    {
        if ( json.isMember("question") )
        {
            if ( json["question"].isMember("answers") )
            {
                Json::Value best = json["question"]["answers"][0];
                if ( best.isMember("text") && best.isMember("confidence") )
                {
					double confidence = best["confidence"].asDouble();
					answer["confidence"] = confidence;

					if ( m_pDeepQAProxy->m_LowConfidenceResponses.size() > 0 && confidence < m_pDeepQAProxy->m_fMinConfidence )
						answer["response"].append( m_pDeepQAProxy->m_LowConfidenceResponses[rand() % m_pDeepQAProxy->m_LowConfidenceResponses.size()] );
					else
						answer["response"].append( best["text"] );

                    answer["hasPriority"] = m_pDeepQAProxy->HasPriority();
					answer["pipelineType"] = m_pDeepQAProxy->m_PipelineType;
					answer["answers"] = json["question"]["answers"];

                }
            }
        } 
    }

	if ( m_Callback.IsValid() )
		m_Callback( answer );
	delete this;
}