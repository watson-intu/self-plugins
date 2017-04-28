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


#define NO_OPENSSL_INCLUDES

#include "WatsonAvatar.h"
#include "utils/StringUtil.h"
#include "utils/WebClientService.h"

REG_SERIALIZABLE( WatsonAvatar );
REG_OVERRIDE_SERIALIZABLE(IAvatar, WatsonAvatar);
RTTI_IMPL( WatsonAvatar, IAvatar );

WatsonAvatar::WatsonAvatar() : IAvatar( "WatsonAvatarV1", AUTH_NONE ),
	m_pSocket( NULL )
{}

WatsonAvatar::~WatsonAvatar()
{
	if (m_pSocket != NULL)
	{
		m_pSocket->close();
		delete m_pSocket;
	}
}

bool WatsonAvatar::Stop()
{
	return true;
}

bool WatsonAvatar::ChangeState(const std::string & a_NewState)
{
	Log::Debug( "WatsonAvatar", "Setting state: %s", a_NewState.c_str() );
	WebClientService::Instance()->GetService().post( boost::bind( &WatsonAvatar::SendState, this, a_NewState ));
	return true;
}

void WatsonAvatar::SendState(std::string a_NewState)
{
	try {
		// open the socket connection if needed..
		if ( m_pSocket == NULL )
			m_pSocket = new boost::asio::ip::tcp::socket( WebClientService::Instance()->GetService() );

		if ( !m_pSocket->is_open() )
		{
			URL url(GetConfig()->m_URL);
			boost::asio::ip::tcp::resolver resolver(WebClientService::Instance()->GetService());
			boost::asio::ip::tcp::resolver::query q(url.GetHost(), StringUtil::Format("%u", url.GetPort() ));
			boost::asio::ip::tcp::resolver::iterator i = resolver.resolve(q);

			Log::Status( "WatsonAvatar", "Connecting to %", GetConfig()->m_URL.c_str() );
			boost::asio::connect( *m_pSocket, i );
		}

		std::string json = StringUtil::Format( "<StateChange state=\"%s\" />\0", a_NewState.c_str() );
		boost::asio::write( *m_pSocket, boost::asio::buffer( json ) );
	}
	catch( const std::exception & e )
	{
		Log::Error( "WatsonAvatar", "Caught Exception: %s", e.what() );
		if ( m_pSocket != NULL )
			m_pSocket->close();
	}
}

