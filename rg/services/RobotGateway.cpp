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

#include "SelfInstance.h"
#include "RobotGateway.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Health.h"
#include "utils/StringUtil.h"
#include "utils/URL.h"
#include "topics/TopicManager.h"

#include "RobotAuthenticate.h"
#include "RobotMail.h"

const double UPDATE_CONFIG_INTERVAL	= 300.0;

REG_SERIALIZABLE(RobotGateway);
REG_OVERRIDE_SERIALIZABLE(IGateway,RobotGateway);
RTTI_IMPL(RobotGateway, IGateway);

RobotGateway::RobotGateway() : IGateway( "RobotGatewayV1" ), 
	m_bApplyRemoteConfigs( true ),
	m_bApplyParentHost( true ),
	m_PersistLogInterval( 60.0 ), 
	m_PersistLogLevel( LL_DEBUG_LOW ), 
	m_ConfigsFetched( false ),
	m_ParentFetched( false ),
	m_HeartBeatInterval( 15.0f )
{}

RobotGateway::~RobotGateway()
{}

void RobotGateway::Serialize(Json::Value & json)
{
	IGateway::Serialize( json );

	json["m_bApplyRemoteConfigs"] = m_bApplyRemoteConfigs;
	json["m_bApplyParentHost"] = m_bApplyParentHost;
	json["m_PersistLogInterval"] = m_PersistLogInterval;
	json["m_PersistLogLevel"] = (int)m_PersistLogLevel;
	json["m_HeartBeatInterval"] = m_HeartBeatInterval;
	SerializeVector( "m_PersistLogFilter", m_PersistLogFilter, json );
}

void RobotGateway::Deserialize(const Json::Value & json)
{
	IGateway::Deserialize( json );

	if (json["m_bApplyRemoteConfigs"].isBool() )
		m_bApplyRemoteConfigs = json["m_bApplyRemoteConfigs"].asBool();
	if (json["m_bApplyParentHost"].isBool() )
		m_bApplyParentHost = json["m_bApplyParentHost"].asBool();
	DeserializeVector( "m_PersistLogFilter", json, m_PersistLogFilter );
	if (json["m_PersistLogInterval"].isDouble() )
		m_PersistLogInterval = json["m_PersistLogInterval"].asDouble();
	if (json["m_PersistLogInterval"].isInt() )
		m_PersistLogLevel = (LogLevel)json["m_PersistLogLevel"].asInt();
	if (json["m_PersistLogInterval"].isDouble() )
		m_HeartBeatInterval = json["m_HeartBeatInterval"].asFloat();
}

bool RobotGateway::Start()
{
	if (! IGateway::Start() )
		return false;

	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance != NULL )
	{
		m_EmbodimentToken = pInstance->GetLocalConfig().m_BearerToken;
		m_GroupId = pInstance->GetLocalConfig().m_GroupId;
		m_RobotName = pInstance->GetLocalConfig().m_Name;
		m_MacId = pInstance->GetLocalConfig().m_MacId;
		m_SelfVersion = pInstance->GetSelfVersion();
		m_RobotType = pInstance->GetRobotType();
		m_OrganizationId = pInstance->GetLocalConfig().m_OrgId;
		m_EmbodimentId = pInstance->GetLocalConfig().m_EmbodimentId;
	}

	if ( m_EmbodimentToken.size() == 0 )
	{
		Log::Warning( "RobotGateway", "No bearer token provided, service will not start." );
		return false;
	}

	// add sub-services if no existing service is found.
	if ( pInstance->FindService<IAuthenticate>() == NULL )
		pInstance->AddService( new RobotAuthenticate() );
	if ( pInstance->FindService<IMail>() == NULL )
		pInstance->AddService( new RobotMail() );

	m_Headers["Content-Type"] = "application/json";
	m_Headers["macId"] = m_MacId;
	m_Headers["groupId"] = m_GroupId;
	m_Headers["orgId"] = m_OrganizationId;
	m_Headers["Authorization"] = "Bearer " + m_EmbodimentToken;
	m_Headers["_id"] = m_EmbodimentId;

	Log::RegisterReactor( this );

	Log::Status("RobotGateway", "Registering embodiment");
	RegisterEmbodiment(DELEGATE(RobotGateway, OnRegisteredEmbodiment, const Json::Value &, this));

	while (!m_ConfigsFetched || !m_ParentFetched)
	{
		ThreadPool::Instance()->ProcessMainThread();
		boost::this_thread::sleep(boost::posix_time::milliseconds(5));
	}

	//Start heartbeat timer pool
	TimerPool * pPool = TimerPool::Instance();
	if ( pPool != NULL )
	{
		m_spHeartbeatTimer = pPool->StartTimer(
				VOID_DELEGATE( RobotGateway, Heartbeat, this ), m_HeartBeatInterval, true, true );
	}
	return true;
}

bool RobotGateway::Stop()
{
	m_spConfigTimer.reset();
	m_spPersistLogTimer.reset();

	Log::RemoveReactor( this, false );

	return IGateway::Stop();
}

void RobotGateway::GetServiceStatus(ServiceStatusCallback a_Callback)
{
	if (m_pConfig != NULL && m_EmbodimentToken.size() > 0 )
		new ServiceStatusChecker(this, a_Callback);
	else
		a_Callback(ServiceStatus(m_ServiceId, false));
}

void RobotGateway::Process(const LogRecord & a_Record)
{
	if ( a_Record.m_Level >= m_PersistLogLevel )
	{
		bool bFiltered = m_PersistLogFilter.size() > 0 ? true : false;
		for(size_t i=0;i<m_PersistLogFilter.size() && bFiltered;++i)
			if ( m_PersistLogFilter[i] == a_Record.m_SubSystem )
				bFiltered = false;

		if (! bFiltered )
		{
			Json::Value json;
			json["time"] = a_Record.m_Time;

			double epochTime = (double)a_Record.m_TimeEpoch;
			json["timeEpoch"] = epochTime;
			if (epochTime > m_NewestLogTime || m_NewestLogTime == 0)
				m_NewestLogTime = epochTime;
			if (epochTime < m_OldestLogTime || m_OldestLogTime == 0)
				m_OldestLogTime = epochTime;

			json["level"] = Log::LevelText( a_Record.m_Level );
			json["subsystem"] = a_Record.m_SubSystem;
			json["message"] = a_Record.m_Message;

			m_PersistLogs.push_back( json.toStyledString() );

			if (! m_spPersistLogTimer )
			{
				m_spPersistLogTimer = TimerPool::Instance()->StartTimer( VOID_DELEGATE( RobotGateway, OnPersistLogs, this ), 
					m_PersistLogInterval, true, false );
			}
		}
	}
}

void RobotGateway::SetLogLevel( LogLevel a_Level )
{
	m_PersistLogLevel = a_Level;
}

void RobotGateway::UpdateEmbodimentName(Delegate<const Json::Value &> a_Callback)
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	assert( pInstance != NULL );

	m_RobotName = pInstance->GetLocalConfig().m_Name;
	m_Headers["embodimentName"] = m_RobotName;
	

	new RequestJson(this, "/v1/embodiments/updateEmbodimentName", "GET", NULL_HEADERS, EMPTY_STRING, a_Callback);
}

void RobotGateway::RegisterEmbodiment(Delegate<const Json::Value &> a_Callback)
{
	Headers headers;
	headers["Content-Type"] = "application/json";

	Json::Value json;
	json["_id"] = m_EmbodimentId; // id is empty on the first registration and non-empty afterwards
	json["embodimentName"] = m_RobotName;
	json["groupId"] = m_GroupId;
	json["orgId"] = m_OrganizationId;
	json["macId"] = m_MacId;
	StringUtil::ToUpper(m_RobotType);
	json["type"] = m_RobotType;
	json["embodimentToken"] = "token";

	new RequestJson( this, "/v1/auth/registerEmbodiment", "POST", headers, Json::FastWriter().write(json), a_Callback);
}

void RobotGateway::SendBacktrace( const std::string & a_BT )
{
	Json::Value upload;
	upload["backtraceData"] = a_BT;

	new RequestJson( this, "/v1/persistence/persistBacktrace", "POST", NULL_HEADERS, Json::FastWriter().write( upload ),
		DELEGATE( RobotGateway, OnBacktracePersisted, const Json::Value &, this ) );
}

void RobotGateway::Heartbeat()
{
	new RequestJson(this, "/v1/embodiments/heartbeat", "GET", NULL_HEADERS, EMPTY_STRING,
	                DELEGATE( RobotGateway, OnHeartbeat, const Json::Value &, this ) );
}

void RobotGateway::GetOrganization( Delegate<const Json::Value &> a_Callback )
{
	new RequestJson( this, "/v1/membership/getOrganizationbyId", "GET", NULL_HEADERS, EMPTY_STRING, a_Callback );
}

void RobotGateway::GetOrgAdminList( Delegate<const Json::Value &> a_Callback )
{
	new RequestJson( this, "/v1/membership/getOrgAdminList", "GET", NULL_HEADERS, EMPTY_STRING, a_Callback );
}

void RobotGateway::GetParent( const std::string & a_ParentId, Delegate<const Json::Value &> a_Callback )
{
	Headers headers;
	headers["parentId"] = a_ParentId;

	new RequestJson( this, "/v1/membership/getParent", "GET", headers, EMPTY_STRING, a_Callback );
}

void RobotGateway::GetServices(Delegate<ServiceList *> a_Callback)
{
	new RequestObj<ServiceList>( this, "/v1/services/getServiceListing", "GET", NULL_HEADERS, EMPTY_STRING, a_Callback );
}

//--------------------------------------------------------

//! Creates an object responsible for service status checking
RobotGateway::ServiceStatusChecker::ServiceStatusChecker(RobotGateway* a_pRbgService, ServiceStatusCallback a_Callback)
	: m_pRbgService(a_pRbgService), m_Callback(a_Callback)
{
	m_pRbgService->GetServices(DELEGATE(RobotGateway::ServiceStatusChecker, OnCheckService, ServiceList*, this));
}

//! Callback function invoked when service status is checked
void RobotGateway::ServiceStatusChecker::OnCheckService(ServiceList* a_pServiceList)
{
	if (m_Callback.IsValid())
		m_Callback(ServiceStatus(m_pRbgService->m_ServiceId, a_pServiceList != NULL));

	delete a_pServiceList;
	delete this;
}

void RobotGateway::OnRegisteredEmbodiment(const Json::Value & a_Response)
{
	SelfInstance * pInstance = SelfInstance::GetInstance();

	Log::Debug( "RobotGateway", "OnRegisteredEmbodiment: %s", a_Response.toStyledString().c_str() );
	if( !a_Response.isNull() )
	{
		if ( a_Response["embodimentName"].isString() )
			m_EmbodimentName = a_Response["embodimentName"].asString();

		if ( a_Response["embodimentToken"].isString() )
		{
			m_EmbodimentToken = a_Response["embodimentToken"].asString();
			m_Headers["Authorization"] = "Bearer " + m_EmbodimentToken;

			if ( pInstance != NULL && pInstance->GetLocalConfig().m_BearerToken != m_EmbodimentToken )
			{
				Log::Warning( "RobotGateway", "Updating bearer token." );
				pInstance->SetBearerToken( m_EmbodimentToken );			// update our token
			}
		}
		else
			Log::Error( "RobotGateway", "embodimentToken is missing." );

		if ( a_Response["_id"].isString() )
		{
			m_EmbodimentId = a_Response["_id"].asString();
			m_Headers["_id"] = m_EmbodimentId;
			if ( pInstance != NULL )
				pInstance->SetEmbodimentId( m_EmbodimentId );

			Log::Status( "RobotGateway", "Received embodiment ID %s", m_EmbodimentId.c_str() );
		}
		else
			Log::Error( "RobotGateway", "_id field is missing." );

		GetOrganization( DELEGATE(RobotGateway, OnOrganization, const Json::Value &,this) );
		GetOrgAdminList( DELEGATE(RobotGateway, OnOrgAdminList, const Json::Value &, this) );
		UpdateConfig();
	}
	else
	{
		Log::Error("RobotGateway", "Error in gateway response: %s.", a_Response.toStyledString().c_str());
		m_ConfigsFetched = true;
		m_ParentFetched = true;
	}
}

void RobotGateway::UpdateConfig()
{
	if (! m_spConfigTimer && TimerPool::Instance() != NULL )
	{
		m_spConfigTimer = TimerPool::Instance()->StartTimer( VOID_DELEGATE( RobotGateway, UpdateConfig, this ), 
			UPDATE_CONFIG_INTERVAL, true, true );
	}

	GetServices(DELEGATE(RobotGateway, OnConfigured, ServiceList*, this));
}

void RobotGateway::OnOrganization(const Json::Value & a_Response)
{
	if (! a_Response.isNull() )
	{
		Log::Status( "RobotGateway", "OnOrganization: %s", a_Response.toStyledString().c_str() );

		if ( m_bApplyParentHost )
		{
			if ( a_Response["parent"].isString() )
			{
				const std::string & parentId = a_Response["parent"].asString();
				if ( parentId.size() > 0 )
					GetParent( parentId, DELEGATE(RobotGateway, OnParent, const Json::Value &,this) );
			}
			else
			{
				Log::Error( "RobotGateway", "No parent found in response: %s", a_Response.toStyledString().c_str() );
				m_ParentFetched = true;
			}

		}
		else
		{
			m_ParentFetched = true;
		}
	}
	else
	{
		m_ParentFetched = true;
		Log::Error( "RobotGateway", "Failed to get organization" );
	}
}

void RobotGateway::OnParent(const Json::Value & a_Response)
{
	if( !a_Response.isNull() )
	{
		Log::Status( "RobotGateway", "OnParent: %s", a_Response.toStyledString().c_str() );
		if ( a_Response["parentIp"].isString() && a_Response["parentName"].isString() )
		{
			std::string parentName( a_Response["parentName"].asString() );
			if ( parentName != m_EmbodimentName )
			{
				URL url( a_Response["parentIp"].asString() );
				std::string parentHost( url.GetURL() );

				SelfInstance * pInstance = SelfInstance::GetInstance();
				if ( pInstance != NULL )
					pInstance->GetTopicManager()->SetParentHost( parentHost );
			}
			else
			{
				SelfInstance * pInstance = SelfInstance::GetInstance();
				if ( pInstance != NULL )
					pInstance->GetTopicManager()->SetParentHost( EMPTY_STRING );
			}
		}
	}
	else
	{
		Log::Error( "RobotGateway", "Failed to get parent" );
	}
	m_ParentFetched = true;
}

void RobotGateway::OnOrgAdminList(const Json::Value & a_Response)
{
	if (! a_Response.isNull() )
	{
//		Log::Status( "RobotGateway", "OnOrgAdminList: %s", a_Response.toStyledString().c_str() );
		if ( a_Response.isMember( "adminEmails" ) )
			m_OrgAdminList = a_Response["adminEmails"].asString();
	}
	else
	{
		Log::Error( "RobotGateway", "getOrgAdminList failed." );
	}
}

void RobotGateway::OnConfigured(ServiceList* a_pServiceList)
{
	if (a_pServiceList != NULL)
	{
		Log::Status( "RobotGateway", "Received %u services from gateway.", a_pServiceList->m_Services.size() );
		if (m_bApplyRemoteConfigs)
		{
			for (size_t i = 0; i < a_pServiceList->m_Services.size(); ++i)
			{
				Service & service = a_pServiceList->m_Services[i];

				ServiceConfig creds;
				creds.m_ServiceId = service.m_ServiceName;
				creds.m_URL = service.m_Endpoint;
				creds.m_User = service.m_Username;
				creds.m_Password = service.m_Password;

				for (std::vector<ServiceAttributes>::const_iterator it = service.m_ServiceAttributes.begin();
					it != service.m_ServiceAttributes.end(); ++it)
				{
					creds.m_CustomMap[(*it).m_Key] = (*it).m_Value;
				}

				if (Config::Instance()->AddServiceConfig(creds, true))
					Log::Status("RobotGateway", "Applied credentials for %s.", creds.m_ServiceId.c_str());
			}
		}
		m_ConfigsFetched = true;
	}
	else
	{
		Log::Error( "RobotGateway", "Failed to grab services from gateway, proceeding with previous configuration." );
		m_ConfigsFetched = true;

		// post health object to the blackboard..
		SelfInstance * pInstance = SelfInstance::GetInstance();
		if ( pInstance != NULL )
			pInstance->GetBlackBoard()->AddThing( Health::SP( new Health( "ConfigurationFailure", true, true ) ) );
	}
}

void RobotGateway::OnPersistLogs()
{
	m_spPersistLogTimer.reset();

	Json::Value json;
	json["macId"] = m_MacId;
	json["orgId"] = m_OrganizationId;
	json["groupId"] = m_GroupId;
	json["startTime"] = m_OldestLogTime;
	json["endTime"] = m_NewestLogTime;

	size_t i = 0;
	for( std::list<std::string>::iterator iLog = m_PersistLogs.begin(); iLog != m_PersistLogs.end(); ++iLog )
		json["logs"][i++] = *iLog;
	new RequestJson( this, "/v1/persistence/persistLog", "POST", NULL_HEADERS, json.toStyledString(),
	                 DELEGATE( RobotGateway, OnLogsPersisted, const Json::Value &, this ) );

	m_PersistLogs.clear();
	m_OldestLogTime = 0;
	m_NewestLogTime = 0;
}

void RobotGateway::OnLogsPersisted(const Json::Value & a_Response)
{
//	Log::Debug( "RobotGateway", "OnLogsPersisted: %s", a_Response.toStyledString().c_str() );
}

void RobotGateway::OnBacktracePersisted(const Json::Value & a_Response)
{
	Log::Debug( "RobotGateway", "OnBacktracePersisted: %s", a_Response.toStyledString().c_str() );
}

void RobotGateway::OnHeartbeat(const Json::Value & response)
{
	//Log::Debug( "RobotGateway", "OnHeartbeat: %s", response.toStyledString().c_str() );
}

