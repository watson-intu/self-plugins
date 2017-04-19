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