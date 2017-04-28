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


#ifndef WDC_DEEPQA_PROXY_H
#define WDC_DEEPQA_PROXY_H

#include "agent/QuestionAgent.h"
#include "services/DeepQA/DeepQA.h"

class DeepQAProxy : public IQuestionAnswerProxy 
{
public:
    RTTI_DECL();

    //! Construction
    DeepQAProxy();

    //! ISerializable
    virtual void Serialize(Json::Value & json);
    virtual void Deserialize(const Json::Value & json);

    //! IQuestionAnswer
	virtual void Start();
	virtual void Stop();
	virtual void AskQuestion( QuestionIntent::SP a_spQuestion, Delegate<const Json::Value &> a_Callback);
    
private:
    //! Data
    DeepQA *                    m_pDeepQA;
	std::string					m_ServiceId;
	std::string					m_PipelineType;
    float                       m_fMinConfidence;
    std::vector<std::string>    m_LowConfidenceResponses;

    //! Private request class
    class DeepQARequest 
    {
    public:
        DeepQARequest( DeepQAProxy * a_pDeepQAProxy, std::string a_Question, Delegate<const Json::Value &> );
        void SendRequest();
        void OnResponse(const Json::Value & json);

    private:
        DeepQAProxy *                       m_pDeepQAProxy;
        std::string                         m_Question;
        Delegate<const Json::Value &>       m_Callback;
    };

};

#endif
