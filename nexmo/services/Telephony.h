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


#ifndef SELF_TELEPHONY_H
#define SELF_TELEPHONY_H

#include "utils/IWebClient.h"
#include "services/ITelephony.h"
#include "SelfLib.h"			// include last always

//! This service interfaces with the telephony gateway to allow for sending & accepting phone calls and SMS messages.
class Telephony : public ITelephony
{
public:
	RTTI_DECL();

	//! Types
	typedef Delegate<const Json::Value &>		OnCommand;
	typedef Delegate<const std::string &>		OnAudioOut;
	
	//! Construction
	Telephony();
	~Telephony();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IService interface
	virtual bool Start();

	//! ITelephony interface
	virtual const std::string &	GetAudioInFormat() const;
	virtual const std::string &	GetAudioOutFormat() const;
	virtual const std::string & GetMyNumber() const;

	virtual bool Connect(
				const std::string & a_SelfId,
				OnCommand a_OnCommand, 
				OnAudioOut a_OnAudioOut );
	virtual bool Dial( const std::string & a_Number );
	virtual bool Answer(
				const std::string & fromNumber,
				const std::string & toNumber );
	virtual bool HangUp();
	virtual bool Text( const std::string & a_Number, 
				const std::string & a_Message );
	virtual bool Disconnect();
	virtual void SendAudioIn( const std::string & a_Audio );

private:
	//! Data
	OnCommand			m_OnCommand;
	OnAudioOut			m_OnAudioOut;
	IWebClient::SP		m_spConnection;
	bool				m_bConnected;
	bool				m_bInCall;
	std::string			m_AudioInFormat;
	std::string			m_AudioOutFormat;
	std::string			m_MyNumber;
	std::string         m_TelephonySelfId;

	TimerPool::ITimer::SP
						m_spReconnectTimer;
	std::list<std::string>
						m_Outgoing;
	TimerPool::ITimer::SP 
						m_spSendTimer;
	size_t				m_nSendBytes;

	//! IWebClient callbacks
	void				OnListenMessage( IWebSocket::FrameSP a_spFrame );
	void				OnListenState( IWebClient * a_pClient );
	void				OnListenData( IWebClient::RequestData * a_pData );

	void				StartSendTimer();
	void				OnSendAudioData();

	void				OnReconnect();
};

//----------------------------

inline const std::string & Telephony::GetAudioInFormat() const
{
	return m_AudioInFormat;
}

inline const std::string & Telephony::GetAudioOutFormat() const
{
	return m_AudioOutFormat;
}

inline const std::string & Telephony::GetMyNumber() const
{
	return m_MyNumber;
}

#endif //SELF_TELEPHONY_H