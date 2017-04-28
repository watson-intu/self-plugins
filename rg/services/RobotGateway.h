// Copyright (c) 2011 rubicon IT GmbH
#ifndef SELF_ROBOT_GATEWAY_H
#define SELF_ROBOT_GATEWAY_H

#include "blackboard/ThingEvent.h"
#include "services/IGateway.h"
#include "utils/TimerPool.h"

//! This service wraps the RobotGateway BlueMix application. Depending on the configuration,
//! some of these calls may go directly to BlueMix.
class RobotGateway : public IGateway, public ILogReactor
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<RobotGateway>			SP;
	typedef boost::weak_ptr<RobotGateway>			WP;

	//! Construction
	RobotGateway();
	~RobotGateway();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IService interface
	virtual bool Start();
	virtual bool Stop();
	virtual void GetServiceStatus(ServiceStatusCallback a_Callback);

	//! ILogReactor interface
	virtual void Process(const LogRecord & a_Record);
	virtual void SetLogLevel( LogLevel a_Level );

	//! IGateway interface
	virtual void RegisterEmbodiment( Delegate<const Json::Value &> a_Callback );
	virtual void UpdateEmbodimentName(Delegate<const Json::Value &> a_Callback);
	virtual void SendBacktrace( const std::string & a_BT );
	virtual void Heartbeat();
	virtual void GetOrganization( Delegate<const Json::Value &> a_Callback );
	virtual void GetOrgAdminList( Delegate<const Json::Value &> a_Callback );
	virtual void GetParent( const std::string & a_ParentId, Delegate<const Json::Value &> a_Callback );
	virtual void GetServices( Delegate<ServiceList *> a_Callback );

private:
	//! This class is responsible for checking whether the service is available or not
	class ServiceStatusChecker
	{
	public:
		ServiceStatusChecker(RobotGateway* a_pRbgService, ServiceStatusCallback a_Callback);

	private:
		RobotGateway* m_pRbgService;
		IService::ServiceStatusCallback m_Callback;

		void OnCheckService(ServiceList* a_ServiceList);
	};

	//! Data
	TimerPool::ITimer::SP 
						m_spConfigTimer;
	std::string			m_GroupId;
	std::string			m_RobotName;
	std::string         m_RobotType;
	std::string			m_OrganizationId;
	std::string			m_MacId;
	std::string			m_SelfVersion;
	std::string         m_EmbodimentToken;
	std::string			m_EmbodimentId;
	std::string			m_EmbodimentName;
	std::string         m_OrgAdminList;

	bool				m_bApplyRemoteConfigs;
	bool				m_bApplyParentHost;
	bool 				m_ConfigsFetched;
	bool 				m_ParentFetched;

	double				m_PersistLogInterval;			// how often to upload persisted logs
	float				m_HeartBeatInterval;
	LogLevel			m_PersistLogLevel;				// what level to persist
	std::vector<std::string>
						m_PersistLogFilter;				// array of sub-system to persist to the gateway
	std::list<std::string>
						m_PersistLogs;					// list of items to persist to the gatewaY
	double              m_OldestLogTime;
	double              m_NewestLogTime;
	TimerPool::ITimer::SP
						m_spPersistLogTimer;			// timer for persisting logs to the back-end
	TimerPool::ITimer::SP
						m_spHeartbeatTimer;				// Timer to hit robot gateway's heartbeat endpoint

	//! Callbacks
	void UpdateConfig();
	void OnConfigured(ServiceList* a_pServiceList);
	void OnOrganization(const Json::Value & a_Response);
	void OnOrgAdminList(const Json::Value & a_Response);
	void OnParent(const Json::Value & a_Response);
	void OnRegisteredEmbodiment(const Json::Value & a_Response);
	void OnPersistLogs();
	void OnLogsPersisted(const Json::Value & a_Response);
	void OnBacktracePersisted(const Json::Value & a_Response);
	void OnHeartbeat(const Json::Value & response);

	friend class RobotMail;
};

#endif //ROBOT_GATEWAY_H
