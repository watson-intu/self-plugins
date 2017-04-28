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

