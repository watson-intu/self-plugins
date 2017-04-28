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


#ifndef WDC_RETRIEVEANDRANKPROXY_H
#define WDC_RETRIEVEANDRANKPROXY_H

#include "agent/QuestionAgent.h"
#include "services/RetrieveAndRank/RetrieveAndRank.h"

class RetrieveAndRankProxy : public IQuestionAnswerProxy
{
public:
    RTTI_DECL();

    //! Construction
    RetrieveAndRankProxy();

    //! ISerializable
    virtual void Serialize(Json::Value & json);
    virtual void Deserialize(const Json::Value & json);

    //! IQuestionAnswer
    virtual void Start();
    virtual void Stop();
    virtual void AskQuestion( QuestionIntent::SP a_spQuestion, Delegate<const Json::Value &> a_Callback);

private:
    //! Data
    RetrieveAndRank *			m_pRetrieveAndRank;
	std::string					m_ServiceId;
	std::vector<std::string>    m_LowConfidenceResponses;
	std::string					m_WorkspaceId;
	std::string					m_SolrId;
	std::string					m_Source;
	float                       m_fMinConfidence;

    //! Private request class
    class RetrieveAndRankRequest
    {
    public:
        RetrieveAndRankRequest(RetrieveAndRankProxy * m_pRetrieveAndRankProxy, const std::string & a_Question,
			const std::string & a_SolrId, const std::string & a_WorkspaceId, const std::string & a_Source, 
			Delegate<const Json::Value &>);
        void OnResponse(RetrieveAndRankResponse * a_pRetrieveAndRankResponse);

    private:
        RetrieveAndRankProxy *                      m_pRetrieveAndRankProxy;
        std::string                                 m_Question;
		std::string									m_SolrId;
		std::string									m_WorkspaceId;
		std::string									m_Source;
        Delegate<const Json::Value &>               m_Callback;
    };
};

#endif //WDC_RETRIEVEANDRANKPROXY_H
