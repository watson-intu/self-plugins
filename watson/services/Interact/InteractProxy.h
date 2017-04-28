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


#ifndef SELF_INTERACT_PROXY_H
#define SELF_INTERACT_PROXY_H

#include "agent/QuestionAgent.h"
#include "blackboard/Text.h"

class Interact;

class InteractProxy : public IQuestionAnswerProxy
{
public:
	RTTI_DECL();

	//! Construction
	InteractProxy();
	virtual ~InteractProxy();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IQuestionAnswerProxy interface
	virtual void Start();
	virtual void Stop();
	virtual void AskQuestion( QuestionIntent::SP a_spQuestion, Delegate<const Json::Value &> a_Callback );

private:
	//! Data
	std::string						m_ServiceId;					// ID for the conversation service to use
	Interact *						m_pInteract;

	//! Helper request object
	class Request
	{
	public:
		Request( InteractProxy * a_pConversationProxy, QuestionIntent::SP a_spQuestion, Delegate<const Json::Value &> a_Callback );

		void OnConverse( const Json::Value & a_Response );

	private:
		InteractProxy *					    m_pProxy;
		QuestionIntent::SP                 	m_spQuestion;
		Delegate<const Json::Value &>  		m_Callback;
	};
};

#endif // SELF_INTERACT_PROXY_H