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


#include "ConversationProxy.h"
#include "Conversation.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Image.h"
#include "blackboard/Entity.h"
#include "SelfInstance.h"

#include <boost/filesystem.hpp>
#include <fstream>

RTTI_IMPL( ConversationProxy, ITextClassifierProxy );
REG_SERIALIZABLE( ConversationProxy );

ConversationProxy::ConversationProxy() :
	m_ServiceId( "ConversationV1" ),
	m_WorkspaceKey( "workspace_id" ),
	m_WorkspaceId( "Self" ),
	m_IntentOverride( "m_IntentOverride" ),
	m_PreviousIntent(""),
	m_EmotionalState ( 0.5f ),
	m_bUseCache( true ),
	m_PreCacheFile( "shared/conversation-precache.txt" )
{}

ConversationProxy::~ConversationProxy()
{}


//! ISerializable interface
void ConversationProxy::Serialize(Json::Value & json)
{
	ITextClassifierProxy::Serialize( json );
	json["m_ServiceId"] = m_ServiceId;
	json["m_WorkspaceKey"] = m_WorkspaceKey;
	json["m_WorkspaceId"] = m_WorkspaceId;
	json["m_IntentOverride"] = m_IntentOverride;
	json["m_Context"] = m_Context;
	json["m_PreCacheFile"] = m_PreCacheFile;
}

void ConversationProxy::Deserialize(const Json::Value & json)
{
	ITextClassifierProxy::Deserialize( json );
	if ( json.isMember( "m_ServiceId" ) )
		m_ServiceId = json["m_ServiceId"].asString();
	if ( json.isMember( "m_WorkspaceKey" ) )
		m_WorkspaceKey = json["m_WorkspaceKey"].asString();
	if ( json.isMember( "m_WorkspaceId" ) )
		m_WorkspaceId = json["m_WorkspaceId"].asString();
	if ( json.isMember( "m_IntentOverride" ) )
		m_IntentOverride = json["m_IntentOverride"].asString();
	if ( json.isMember( "m_Context" ) )
		m_Context = json["m_Context"];
	if ( json.isMember( "m_PreCacheFile" ) )
		m_PreCacheFile = json["m_PreCacheFile"].asString();
	if ( json.isMember( "m_bUseCache" ) )
		m_bUseCache = json["m_bUseCache"].asBool();
}

//! ITextClassifierProxy interface
void ConversationProxy::Start()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert( pInstance != NULL );

	pInstance->GetBlackBoard()->SubscribeToType("EmotionalState",
		DELEGATE(ConversationProxy, OnEmotionalState, const ThingEvent &, this), TE_ADDED);
	pInstance->GetBlackBoard()->SubscribeToType("Entity",
		DELEGATE(ConversationProxy, OnEntity, const ThingEvent &, this), TE_ADDED);

	Conversation * pConversation = pInstance->FindService<Conversation>( m_ServiceId );
	if (pConversation != NULL )
	{
		std::string precacheFile = pInstance->GetStaticDataPath() + m_PreCacheFile;
		if ( boost::filesystem::exists( precacheFile ) )
		{
			std::ifstream input( precacheFile.c_str() );

			std::string line;
			while( std::getline( input, line ) )
			{
				if ( line.size() == 0 || line[0] == '#' )
					continue;		// skip newlines or lines starting with #

				line = StringUtil::Trim( line, " \r\n\t");
				pConversation->Message(
					m_WorkspaceId,
					m_MergedContext,
					line, 
					m_IntentOverride,
					Delegate<ConversationResponse *>() );
			}
			input.close();
		}
	}

	Log::Status("ConversationProxy", "ConversationProxy started for workspace %s", m_WorkspaceId.c_str() );
}

void ConversationProxy::Stop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert( pInstance != NULL );

	pInstance->GetBlackBoard()->UnsubscribeFromType("EmotionalState", this);
	pInstance->GetBlackBoard()->UnsubscribeFromType("Entity", this);
}

void ConversationProxy::OnEmotionalState(const ThingEvent & a_ThingEvent)
{
	IThing::SP spThing = a_ThingEvent.GetIThing();
	if (spThing->GetDataType() == "EmotionalState")
	{
		m_EmotionalState = spThing->GetData()["m_EmotionalState"].asFloat();
	}
}

void ConversationProxy::OnEntity(const ThingEvent & a_ThingEvent)
{
	Entity * pEntity = DynamicCast<Entity>( a_ThingEvent.GetIThing().get() );
	if (pEntity != NULL)
	{
		Image::SP spImage = pEntity->FindParentType<Image>( true );
		if ( spImage )
		{
			if( spImage->GetGUID() != m_EntityParentGUID )
			{
				m_EntityParentGUID = spImage->GetGUID();
				m_RecognizedObjects.clear();
			}

			m_RecognizedObjects.append( pEntity->GetTopClass() );
		}
	}
}

void ConversationProxy::ClassifyText( Text::SP a_spText, Delegate<ClassifyResult *> a_Callback )
{
	SP spThis( boost::static_pointer_cast<ConversationProxy>( shared_from_this() ) );
	new Request( spThis, a_spText, a_Callback );
}

//! Helper object for handling specific request
ConversationProxy::Request::Request( const ConversationProxy::SP & a_pProxy, Text::SP a_spText, Delegate<ClassifyResult *> a_Callback) : 
	m_pProxy( a_pProxy ),
	m_spText( a_spText ),
	m_Callback( a_Callback ),
	m_bTextClassified( false )
{
	Conversation * pConversation = Config::Instance()->FindService<Conversation>( m_pProxy->m_ServiceId );

	bool bRequestSent = false;
	if ( pConversation != NULL )
	{
		JsonHelpers::Merge( m_pProxy->m_MergedContext, m_pProxy->m_Context );
		JsonHelpers::Merge( m_pProxy->m_MergedContext, m_spText->GetData() );
		m_pProxy->m_MergedContext["m_EmotionalState"] = m_pProxy->m_EmotionalState;
		m_pProxy->m_MergedContext["m_PreviousIntent"] = m_pProxy->m_PreviousIntent;
		m_pProxy->m_MergedContext["m_Objects"] = m_pProxy->m_RecognizedObjects;

		m_pProxy->m_WorkspaceId = pConversation->
			GetConfig()->GetKeyValue( m_pProxy->m_WorkspaceKey, m_pProxy->m_WorkspaceId );

		if ( m_pProxy->m_WorkspaceId.size() > 0 )
		{
			// check the configuration for a "workspace_id", use it if found..
			bRequestSent = true;
			pConversation->Message(
				m_pProxy->m_WorkspaceId,
				m_pProxy->m_MergedContext,
				m_spText->GetText(),
				m_pProxy->m_IntentOverride,
				DELEGATE(Request, OnTextClassified, ConversationResponse *, this),
				m_pProxy->m_bUseCache);

			// cache miss, post this to the blackboard so we can make a sound that we are getting a result.
			if (!m_bTextClassified)
			{
				Log::Debug("ConversationProxy", "Missed cache: %s", m_spText->GetText().c_str());
				a_spText->AddChild(IThing::SP(new IThing(TT_PERCEPTION, "CacheMiss")));
				// let the callback delete this object
				m_bTextClassified = true;		
			}
			else
			{
				// callback was invoked because it was cached
				delete this;
			}
		}
	}

	if (! bRequestSent )
	{
		ClassifyResult * pResult = new ClassifyResult();
		pResult->m_TopClass = "failure";
		if ( m_Callback.IsValid() )
			m_Callback( pResult );
		delete this;
	}
}

void ConversationProxy::Request::OnTextClassified( ConversationResponse * a_pResponse )
{
#if ENABLE_DEBUGGING
	Log::Debug( "NLCProxy", "m_Intent = %s", a_Result.toStyledString().c_str() );
#endif

	ClassifyResult * pResult = new ClassifyResult();
	if ( a_pResponse != NULL && a_pResponse->m_Intents.size() > 0 )
	{
		// save the context off for the next call..
		m_pProxy->m_MergedContext = a_pResponse->m_Context;

		pResult->m_Result["text"] = m_spText->GetText();
		pResult->m_Result["conversation"] = a_pResponse->ToJson();
		pResult->m_Result["top_class"] = a_pResponse->m_Intents[0].m_Intent;
		pResult->m_Result["confidence"] = a_pResponse->m_Intents[0].m_fConfidence;

		pResult->m_pParentProxy = m_pProxy.get();

		bool bAnswer = false;
		std::vector<std::string> & output = a_pResponse->m_Output;
		for( size_t i=0;i<output.size();++i)
		{
			if ( output[i].size() == 0 )
				continue;
			pResult->m_Result["goal_params"]["answer"]["response"][i] = output[i];
			bAnswer = true;
		}

		if ( bAnswer )
			pResult->m_Result["goal_params"]["question"] = m_spText->ToJson();

		bool bIgnore = false;
		for (size_t i = 0; i < m_pProxy->m_Filters.size() && !bIgnore; ++i)
		{
			bIgnore |= m_pProxy->m_Filters[i]->ApplyFilter(pResult->m_Result);
			if ( bIgnore )
			{
				Log::Debug("ConversationProxy", "Filter %s triggered, ignoring filtered intent: %s, TextId: %p", 
					m_pProxy->m_Filters[i]->GetRTTI().GetName().c_str(), 
					pResult->m_Result["top_class"].asCString(), 
					m_spText.get() );
			}
		}

		if (! bIgnore )
		{
			pResult->m_TopClass = pResult->m_Result["top_class"].asString();
			pResult->m_fConfidence = pResult->m_Result["confidence"].asDouble();
			pResult->m_bPriority = m_pProxy->m_bPriority;
			m_pProxy->m_PreviousIntent = pResult->m_Result["top_class"].asString();
		}
		else
		{
			pResult->m_TopClass = "ignored";
			m_pProxy->m_bFocus = false;
		}

//		Log::Status( "ConversationProxy", "Response: %s", pResult->m_Result.toStyledString().c_str() );
	}
	else
	{
		m_pProxy->m_Context.clear();

		Log::Error( "ConversationProxy", "Request Failed - Text: %s, TextId: %p",
			m_spText->GetText().c_str(), m_spText.get() );
		pResult->m_TopClass = "failure";
	}
	
	// If valid, hit callback to text classifier
	if ( m_Callback.IsValid() )
		m_Callback( pResult );

	delete a_pResponse;
	if ( m_bTextClassified )
		delete this;
	else
		m_bTextClassified = true;
}

