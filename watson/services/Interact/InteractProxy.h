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

#ifndef SELF_INTERACT_PROXY_H
#define SELF_INTERACT_PROXY_H

#include "agent/QuestionAgent.h"
#include "blackboard/Text.h"

class Interact;

class SELF_API InteractProxy : public IQuestionAnswerProxy
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