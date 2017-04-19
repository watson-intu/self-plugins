/* ***************************************************************** */
/*                                                                   */
/* IBM Confidential                                                  */
/* OCO Source Materials                                              */
/*                                                                   */
/* (C) Copyright IBM Corp. 2001, 2014                                */
/*                                                                   */
/* The source code for this program is not published or otherwise    */
/* divested of its trade secrets, irrespective of what has been      */
/* deposited with the U.S. Copyright Office.                         */
/*                                                                   */
/* ***************************************************************** */

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
