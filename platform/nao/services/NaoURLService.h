/**
* Copyright 2016 IBM Corp. All Rights Reserved.
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

#ifndef SELF_NAOURLSERVICE_H
#define SELF_NAOURLSERVICE_H

#include "utils/IService.h"
#include "blackboard/URL.h"
#include "SelfLib.h"
#include "services/URLService.h"
#include "SelfInstance.h"
#include "qi/session.hpp"
#include "NaoPlatform.h"

class NaoURLService : public URLService
{
public:
    RTTI_DECL();

    //! Construction
    NaoURLService();

    //! IService
    virtual bool Start();
	virtual bool Stop();

    //! ISerializable
    virtual void Serialize(Json::Value & json);
    virtual void Deserialize(const Json::Value & json);

    //! Upload the specified dialog
    virtual void SendURL( const Url::SP & a_spUrl, UrlCallback a_Callback );
   
private:
	void TabletThread();
	void ShowURL( const Url::SP & a_spUrl, UrlCallback a_Callback );
	void ConfigureTablet();
    void CheckConnection();
    void DisplayLogo();

	qi::AnyReference OnTouchData(const std::vector <qi::AnyReference> & args);    

	struct UrlRequest 
	{
		UrlRequest( const Url::SP & a_spReq, UrlCallback a_Callback ) :
			m_spUrl( a_spReq ), m_Callback( a_Callback )
		{}
		UrlRequest()
		{}

		Url::SP			m_spUrl;
		UrlCallback		m_Callback;
	};
	typedef std::list< UrlRequest >	RequestList;

    //! Members
	float                   m_TabletCheckInterval;
	float                   m_TabletDisplayTime;
	float                   m_fBrightness;

	volatile bool			m_bServiceActive;
	volatile bool			m_bThreadStopped;
	volatile bool           m_bTabletConnected;
	RequestList				m_RequestList;
	boost::mutex			m_RequestListLock;
	bool					m_bLogoDisplayed;
	std::string				m_LogoUrl;
	double					m_LastUpdate;
	double					m_LastCheck;

	qi::AnyObject           m_Tablet;
};

#endif
