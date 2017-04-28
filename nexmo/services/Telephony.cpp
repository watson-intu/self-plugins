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


#include "Telephony.h"
#include "sensors/AudioData.h"
#include "SelfInstance.h"

const float RECONNECT_TIME = 5.0f;
const unsigned int FRAME_SIZE = 320;			// how many frames to send per web socket frame

REG_SERIALIZABLE( Telephony );
REG_OVERRIDE_SERIALIZABLE( ITelephony, Telephony );
RTTI_IMPL( Telephony, ITelephony );

//-----------------------------

Telephony::Telephony() : ITelephony( "TelephonyV1" ),
	m_bConnected( false ),
	m_AudioInFormat( "audio/L16;rate=16000" ), 
	m_AudioOutFormat( "audio/L16;rate=16000" ),
	m_bInCall( false ),
	m_nSendBytes( FRAME_SIZE * 2 )
{}

void Telephony::Serialize(Json::Value & json)
{
	ITelephony::Serialize(json);
}

void Telephony::Deserialize(const Json::Value & json)
{
	ITelephony::Deserialize(json);
}

Telephony::~Telephony()
{
	Disconnect();
}

bool Telephony::Start()
{
	if (! ITelephony::Start() )
		return false;

	return true;
}


//! Connect to the back-end, this will register and we will become available to receive phone calls. The provided
//! Callback will be invoke when a call is incoming, the user must call Answer() to answer the incoming call.
bool Telephony::Connect( const std::string & a_SelfId,
					OnCommand a_OnCommand,
					OnAudioOut a_OnAudioOut )
{
	if ( m_spConnection )
		return false;
	if ( GetConfig()->m_User.size() == 0 || GetConfig()->m_Password.size() == 0 )
	{
		Log::Warning( "Telephony", "Credentials not setup, can not connect." );
		return false;
	}

	m_TelephonySelfId = a_SelfId;
	m_OnCommand = a_OnCommand;
	m_OnAudioOut = a_OnAudioOut;

	std::string url = GetConfig()->m_URL;
	StringUtil::Replace(url, "https://", "wss://", true );
	StringUtil::Replace(url, "http://", "ws://", true );

	m_spConnection = IWebClient::Create( url );
	m_spConnection->SetHeaders(m_Headers);
	m_spConnection->SetFrameReceiver( DELEGATE( Telephony, OnListenMessage, IWebSocket::FrameSP, this ) );
	m_spConnection->SetStateReceiver( DELEGATE( Telephony, OnListenState, IWebClient *, this ) );
	m_spConnection->SetDataReceiver( DELEGATE( Telephony, OnListenData, IWebClient::RequestData *, this ) );

	if (! m_spConnection->Send() )
	{
		OnReconnect();
	}
	else
	{
		Json::Value json;
		json["command"] = "handshake";
		json["self_id"] = m_TelephonySelfId;
		json["api_key"] = GetConfig()->m_User;
		json["api_secret"] = GetConfig()->m_Password;
		Log::Debug("Telephony", "Sending handshake %s", json.toStyledString().c_str());

		m_spConnection->SendText( json.toStyledString() );
	}

	return true;
}

//! Make outgoing call.
bool Telephony::Dial( const std::string & a_Number )
{
	if (!m_spConnection )
		return false;

	Json::Value json;
	json["command"] = "dial";
	json["to_number"] = a_Number;
	json["from_number"] = m_MyNumber;

	m_spConnection->SendText( json.toStyledString() );
	return true;
}

//! Answer an incoming call, returns true on success.
bool Telephony::Answer(const std::string & fromNumber, const std::string & toNumber)
{
	if (!m_spConnection )
		return false;

	Json::Value json;
	json["command"] = "answer";
	json["to_number"] = toNumber;
	json["from_number"] = fromNumber;

	m_spConnection->SendText( json.toStyledString() );

	m_bInCall = true;
	StartSendTimer();

	return true;
}

//! Hang up current call.
bool Telephony::HangUp()
{
	if (!m_spConnection )
		return false;

	Json::Value json;
	json["command"] = "hang_up";

	m_spConnection->SendText( json.toStyledString() );

	m_bInCall = false;
	return true;
}

//! Send a SMS message
bool Telephony::Text( const std::string & a_Number, 
	const std::string & a_Message )
{
	if (!m_spConnection )
		return false;

	Json::Value json;
	json["command"] = "sms_outgoing";
	json["to_number"] = a_Number;
	json["from_number"] = m_MyNumber;
	json["message"] = a_Message;

	m_spConnection->SendText( json.toStyledString() );
	return true;
}

//! Disconnect from the back-end..
bool Telephony::Disconnect()
{
	if (!m_spConnection )
		return false;

	m_spConnection->Close();
	m_spConnection.reset();
	m_bConnected = false;
	m_bInCall = false;

	return true;
}

//! Send binary audio data up to the gateway, the format of the audio must match the format 
//! specified by GetAudioFormat(), usually audio/L16;rate=16000
void Telephony::SendAudioIn( const std::string & a_Audio )
{
	if ( m_spConnection && m_bInCall )
		m_Outgoing.push_back( a_Audio );
}

//---------------------------------------------

void Telephony::OnListenMessage( IWebSocket::FrameSP a_spFrame )
{
	if ( a_spFrame->m_Op == IWebSocket::TEXT_FRAME )
	{
		Json::Value json;

		Json::Reader reader( Json::Features::strictMode() );
		if (reader.parse(a_spFrame->m_Data, json)) 
		{
			Log::Status( "Telephony", "Command received: %s", json.toStyledString().c_str() );
			if ( m_OnCommand.IsValid() )
				m_OnCommand( json );

			std::string command = json["command"].asString();
			if ( command == "ack" )
			{
				if ( json.isMember( "my_number") )
					m_MyNumber = json["my_number"].asString();
			}
			else if ( command == "hang_up" )
			{
				m_bInCall = false;
			}
		}
		else
		{
			Log::Error("SpeechToText", "Failed to parse JSON from server: %s", a_spFrame->m_Data.c_str() );
		}
	}
	else if ( a_spFrame->m_Op == IWebSocket::BINARY_FRAME )
	{
		// TODO: echo cancellation..
//		Log::DebugLow( "Telephony", "Received %u bytes of audio", a_spFrame->m_Data.size() );
		if ( m_OnAudioOut.IsValid() && a_spFrame->m_Data.size() > 0 )
			m_OnAudioOut( a_spFrame->m_Data );
	}
}

void Telephony::OnListenState( IWebClient * a_pClient )
{
	if ( a_pClient == m_spConnection.get() )
	{
		Log::Debug("Telephony", "m_ListenSocket.GetState() = %u", a_pClient->GetState() );
		if ( a_pClient->GetState() == IWebClient::DISCONNECTED || a_pClient->GetState() == IWebClient::CLOSED )
		{
			m_bConnected = false;
			OnReconnect();
		}
		else if ( a_pClient->GetState() == IWebClient::CONNECTED )
		{
			m_bConnected = true;
		}
	}
}

void Telephony::OnListenData( IWebClient::RequestData * a_pData )
{
	const std::string & transId = a_pData->m_Headers["X-Global-Transaction-ID"];
	const std::string & dpTransId = a_pData->m_Headers["X-DP-Watson-Tran-ID"];

	Log::Status( "Telephony", "Connected, Global Transaction ID: %s, DP Transaction ID: %s",
		transId.c_str(), dpTransId.c_str() );
}

void Telephony::StartSendTimer()
{
	unsigned int bps, channels, freq;
	if ( AudioData::ParseAudioFormat( m_AudioOutFormat, freq, bps, channels ) )
	{
		unsigned int nBytesPerSecond = freq * (bps / 8) * channels;
		m_nSendBytes = FRAME_SIZE * (bps / 8) * channels;

		double sendInterval =  (double)m_nSendBytes / (double)nBytesPerSecond;
		m_spSendTimer = TimerPool::Instance()->StartTimer( VOID_DELEGATE( Telephony, OnSendAudioData, this ), 
			sendInterval, true, true );

		Log::Status( "Telephony", "Started send timer, Interval: %f, Send Bytes: %u, Bytes Sec: %u", 
			sendInterval, m_nSendBytes, nBytesPerSecond );
	}
	else
		Log::Error( "Telephony", "Unsupported audio format: %s", m_AudioOutFormat.c_str() );
}

void Telephony::OnSendAudioData()
{
	if ( m_spConnection != NULL && m_bInCall )
	{
		std::string frame;
		while( m_Outgoing.size() > 0 && frame.size() < m_nSendBytes )
		{
			std::string & data = m_Outgoing.front();

			size_t bytes = data.size();
			size_t remaining = m_nSendBytes - frame.size();
			if ( bytes > remaining )
				bytes = remaining;

			frame += std::string( data.c_str(), bytes );
			if ( bytes == data.size() )
				m_Outgoing.pop_front();
			else
				data.erase( 0, bytes );
		}

		if ( frame.size() < m_nSendBytes )
			frame += std::string( m_nSendBytes - frame.size(), '\0' );			// fill frame with zeros

//		Log::DebugLow( "Telephony", "Sending %u bytes of audio", frame.size() );
		m_spConnection->SendBinary( frame );
	}
	else
	{
		Log::Status( "Telephony", "Ending send timer." );
		m_spSendTimer.reset();
	}
}

void Telephony::OnReconnect()
{
	if ( m_spConnection != NULL )
	{
		if (! m_spReconnectTimer )
		{
			// if not in the correct state and not already trying to reconnect, go ahead and try to connect in a few seconds.
			Log::Status( "Telephony", "Disconnected, will try to reconnect in %f seconds.", RECONNECT_TIME );
			m_spReconnectTimer = TimerPool::Instance()->StartTimer( 
				VOID_DELEGATE( Telephony, OnReconnect, this ), RECONNECT_TIME, true, false );
			m_bConnected = false;
		}
		else if ( m_spConnection->GetState() == IWebClient::CLOSED 
			|| m_spConnection->GetState() == IWebClient::DISCONNECTED )
		{
			Log::Status( "Telephony", "Trying to reconnect." );

			// this may get recursive because it can call OnListenState()
			m_spReconnectTimer.reset();
			if (!m_spConnection->Send())
			{
				Log::Error("Telephony", "Failed to connect web socket to %s", m_spConnection->GetURL().GetURL().c_str());
				OnReconnect();
			}
			else
			{
				Json::Value json;
				json["command"] = "handshake";
				json["self_id"] = m_TelephonySelfId;
				json["api_key"] = GetConfig()->m_User;
				json["api_secret"] = GetConfig()->m_Password;
				Log::Status("Telephony", "Sending handshake %s", json.toStyledString().c_str());

				m_spConnection->SendText(json.toStyledString());
			}
		}
		else
		{
			Log::Error( "Telephony", "m_pConnection not getting into the correct state.");
			m_spConnection->Close();
		}
	}

}
