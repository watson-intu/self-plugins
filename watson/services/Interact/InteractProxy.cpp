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

#include "InteractProxy.h"
#include "services/Interact/Interact.h"
#include "blackboard/BlackBoard.h"
#include "SelfInstance.h"

RTTI_IMPL( InteractProxy, IQuestionAnswerProxy );
REG_SERIALIZABLE( InteractProxy );

InteractProxy::InteractProxy() :
	m_ServiceId( "InteractV1" ),
	m_pInteract( NULL )
{}

InteractProxy::~InteractProxy()
{}


//! ISerializable interface
void InteractProxy::Serialize(Json::Value & json)
{
	IQuestionAnswerProxy::Serialize( json );
	json["m_ServiceId"] = m_ServiceId;
}

void InteractProxy::Deserialize(const Json::Value & json)
{
	IQuestionAnswerProxy::Deserialize( json );
	if ( json.isMember( "m_ServiceId" ) )
		m_ServiceId = json["m_ServiceId"].asString();
}

//! ITextClassifierProxy interface
void InteractProxy::Start()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert( pInstance != NULL );

	m_pInteract = pInstance->FindService<Interact>( m_ServiceId );
	if (m_pInteract == NULL )
		Log::Error("InteractProxy", "Interact service not available.");
}

void InteractProxy::Stop()
{}

void InteractProxy::AskQuestion( QuestionIntent::SP a_spQuestion, Delegate<const Json::Value &> a_Callback )
{
	new Request( this, a_spQuestion, a_Callback );
}

//! Helper object for handling specific request
InteractProxy::Request::Request(InteractProxy * a_pProxy, QuestionIntent::SP a_spQuestion, Delegate<const Json::Value &> a_Callback) : 
	m_pProxy( a_pProxy ),
	m_spQuestion( a_spQuestion ),
	m_Callback( a_Callback )
{
	if ( m_pProxy->m_pInteract != NULL )
	{
		// check the configuration for a "workspace_id", use it if found..
		m_pProxy->m_pInteract->Converse( a_spQuestion->GetText(),
			DELEGATE(Request, OnConverse, const Json::Value &, this) );
	}
	else
		OnConverse( Json::Value::nullRef );
}

void InteractProxy::Request::OnConverse( const Json::Value & a_Response )
{
	Json::Value answer;
	if (!a_Response.isNull())
	{
		Log::Status("InteractProxy", "OnConverse: %s", a_Response.toStyledString().c_str() );

		std::string expertise = a_Response["expertise"].asString();
		double confidence = expertise.size() > 0 ? 0.5f : 0.0f;
		answer["confidence"] = confidence;
		answer["response"][0] = a_Response["response"].asString();
	}
	else
		Log::Error("InteractProxy", "Response from Interact was null!");

	// If valid, hit callback to text classifier
	if ( m_Callback.IsValid() )
		m_Callback( answer );
	delete this;
}

