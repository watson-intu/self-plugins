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


#include "RetrieveAndRankProxy.h"
#include "SelfInstance.h"
#include "utils/IService.h"

REG_SERIALIZABLE(RetrieveAndRankProxy);
RTTI_IMPL(RetrieveAndRankProxy, IQuestionAnswerProxy);


RetrieveAndRankProxy::RetrieveAndRankProxy() : m_ServiceId ( "RetrieveAndRankV1" )
{}

//! ISerializable
void RetrieveAndRankProxy::Serialize(Json::Value & json)
{
	IQuestionAnswerProxy::Serialize(json);
	json["m_ServiceId"] = m_ServiceId;
	json["m_WorkspaceId"] = m_WorkspaceId;
	json["m_fMinConfidence"] = m_fMinConfidence;
	json["m_SolrId"] = m_SolrId;
	json["m_Source"] = m_Source;
	SerializeVector("m_LowConfidenceResponses", m_LowConfidenceResponses, json);
}

void RetrieveAndRankProxy::Deserialize(const Json::Value & json)
{
	IQuestionAnswerProxy::Deserialize(json);
	if (json["m_ServiceId"].isString())
		m_ServiceId = json["m_ServiceId"].asString();
	if (json["m_WorkspaceId"].isString())
		m_WorkspaceId = json["m_WorkspaceId"].asString();
	if (json["m_SolrId"].isString())
		m_SolrId = json["m_SolrId"].asString();
	if (json["m_Source"].isString())
		m_Source = json["m_Source"].asString();
	if (json["m_fMinConfidence"].isDouble())
		m_fMinConfidence = json["m_fMinConfidence"].asFloat();
	DeserializeVector("m_LowConfidenceResponses", json, m_LowConfidenceResponses);

	if (m_LowConfidenceResponses.size() == 0)
		m_LowConfidenceResponses.push_back("I don't know the answer to that one");
}

//! IQuestionAnswer
void RetrieveAndRankProxy::AskQuestion(QuestionIntent::SP a_spQuestion, Delegate<const Json::Value &> a_Callback)
{
	new RetrieveAndRankRequest(this, a_spQuestion->GetText(), m_SolrId, m_WorkspaceId, m_Source, a_Callback);
}

void RetrieveAndRankProxy::Start()
{
	Log::Debug("RetrieveAndRankProxy", "Started...");
	m_pRetrieveAndRank = SelfInstance::GetInstance()->FindService<RetrieveAndRank>();
}

void RetrieveAndRankProxy::Stop()
{
	Log::Debug("RetrieveAndRankProxy", "Stopped...");
}

RetrieveAndRankProxy::RetrieveAndRankRequest::RetrieveAndRankRequest(
	RetrieveAndRankProxy * a_pRetrieveAndRank,
	const std::string & a_Question,
	const std::string & a_SolrId,
	const std::string & a_WorkspaceId,
	const std::string & a_Source,
	Delegate<const Json::Value &> a_Callback) :
	m_pRetrieveAndRankProxy(a_pRetrieveAndRank),
	m_Question(a_Question),
	m_SolrId(a_SolrId),
	m_WorkspaceId(a_WorkspaceId),
	m_Source(a_Source),
	m_Callback(a_Callback)
{
	if (m_pRetrieveAndRankProxy->m_pRetrieveAndRank != NULL)
	{
		m_pRetrieveAndRankProxy->m_pRetrieveAndRank->Ask(m_SolrId, m_WorkspaceId, m_Question, m_Source,
			DELEGATE(RetrieveAndRankRequest, OnResponse, RetrieveAndRankResponse *, this));
	}
	else
	{
		Log::Error("RetrieveAndRankProxy", "RetrieveAndRank Service not configured");
		if (m_Callback.IsValid())
			m_Callback(Json::Value());
		delete this;
	}
}

void RetrieveAndRankProxy::RetrieveAndRankRequest::OnResponse(RetrieveAndRankResponse * a_pRetrieveAndRankResponse)
{
	Json::Value answer;
	if (a_pRetrieveAndRankResponse != NULL)
	{
		Log::Debug("RetrieveAndRankProxy", "Received confidence as %f", a_pRetrieveAndRankResponse->m_MaxScore);
		double confidence = a_pRetrieveAndRankResponse->m_MaxScore;		
		answer["confidence"] = confidence;

		if (m_pRetrieveAndRankProxy->m_LowConfidenceResponses.size() > 0 && confidence < m_pRetrieveAndRankProxy->m_fMinConfidence)
			answer["response"].append(m_pRetrieveAndRankProxy->m_LowConfidenceResponses[rand() % m_pRetrieveAndRankProxy->m_LowConfidenceResponses.size()]);
		else
		{
			std::vector<Documents> m_Documents = a_pRetrieveAndRankResponse->m_Docs;
			std::string m_Body = m_Documents[0].m_Body;
			answer["response"].append(m_Body);
		}
	}
	else
		Log::Error("RetreiveAndRankProxy", "Response from R&R Service was null!");

	if (m_Callback.IsValid())
		m_Callback(answer);
	delete this;
}