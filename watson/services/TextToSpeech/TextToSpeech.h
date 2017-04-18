/**
* Copyright 2015 IBM Corp. All Rights Reserved.
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

#ifndef WDC_TEXT_TO_SPEECH_H
#define WDC_TEXT_TO_SPEECH_H

#include "services/ITextToSpeech.h"
#include "utils/Sound.h"

class TextToSpeech : public ITextToSpeech
{
public:
	RTTI_DECL();

	//! Construction
	TextToSpeech();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IService interface
	virtual bool Start();
	virtual void GetServiceStatus( IService::ServiceStatusCallback a_Callback );

	//! ITextToSpeech interface
	virtual void GetVoices( GetVoicesCallback a_Callback );
	virtual void SetVoice( const std::string & a_Voice );
	virtual void Synthesis( const std::string & a_Text, AudioFormatType a_eFormat, 
		Delegate<const std::string &> a_Callback, bool a_IsStreaming = false );
	virtual void ToSound( const std::string & a_Text, ToSoundCallback a_Callback );
	virtual void ToSound( const std::string & a_Text, StreamCallback a_Callback, 
		WordsCallback a_WordsCallback = WordsCallback() );

	//! Static
	static std::string & GetFormatName( AudioFormatType a_eFormat );
	static std::string & GetFormatId( AudioFormatType a_eFormat );

private:
	//! This class is responsible for checking whether the service is available or not
	class ServiceStatusChecker
	{
	public:
		ServiceStatusChecker(TextToSpeech* a_pTtsService, ServiceStatusCallback a_Callback);

	private:
		TextToSpeech* m_pTtsService;
		IService::ServiceStatusCallback m_Callback;

		void OnCheckService(Voices* a_pVoices);
	};

	struct Connection : public boost::enable_shared_from_this<Connection>
	{
		//! Types
		typedef boost::shared_ptr<Connection>		SP;
		typedef boost::weak_ptr<Connection>			WP;
		typedef std::list<IWebSocket::FrameSP>	FramesList;

		Connection(TextToSpeech * a_pTTS, const std::string & a_Text, StreamCallback a_Callback, WordsCallback a_WordsCallback );

		TextToSpeech *	m_pTTS;
		std::string		m_Text;
		IWebClient::SP	m_spSocket;          // use to communicate with the server
		StreamCallback	m_Callback;
		WordsCallback	m_WordsCallback;

		bool Start();

		void OnListenMessage(IWebSocket::FrameSP a_spFrame);
		void OnListenState(IWebClient *);
	};

	//! Types
	typedef std::list<Connection::SP>		Connectionlist;

	//! Data
	std::string		m_Voice;
	Connectionlist	m_Connections;
};

#endif
