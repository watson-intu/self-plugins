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

#include "Interact.h"
#include "SelfInstance.h"
#include "utils/JsonHelpers.h"

#define ENABLE_INTERACT_CACHE			0

REG_SERIALIZABLE( Interact );
RTTI_IMPL( Interact, IService );


Interact::Interact() : IService("InteractV1")
{}

//! ISerializable
void Interact::Serialize(Json::Value & json)
{
	IService::Serialize(json);
}

void Interact::Deserialize(const Json::Value & json)
{
	IService::Deserialize(json);
}

//! IService interface
bool Interact::Start()
{
	if (!IService::Start())
		return false;

	Log::Debug("Interact", "Url loaded as %s", m_pConfig->m_URL.c_str() );

	// Setup header
	m_Headers["Content-Type"] = "application/json";

	return true;
}

void Interact::Converse(const std::string & a_Question, Delegate<const Json::Value &> a_Callback)
{
	std::string endpoint = "/v1/api/converse";

	// Form question
	Json::Value question;
	question["text"] = a_Question;
	//question["userId"] = "";			//!> optional paramters
	//question["cityName"] = "";

#if ENABLE_INTERACT_CACHE
	new RequestJson(this, endpoint, "POST", NULL_HEADERS, question.toStyledString(), a_Callback,
		new CacheRequest( "Converse", JsonHelpers::Hash( question ) ) );
#else
	new RequestJson(this, endpoint, "POST", NULL_HEADERS, question.toStyledString(), a_Callback );
#endif
}
