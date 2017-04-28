// Copyright (c) 2011 rubicon IT GmbH
#ifndef RG_PACKAGESTORE_H
#define RG_PACKAGESTORE_H

#include "services/IPackageStore.h"

//! This service wraps the PackageStore application.
class PackageStore : public IPackageStore
{
public:
	RTTI_DECL();

	//! Construction
	PackageStore();
	~PackageStore();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IService interface
	virtual bool Start();
	virtual bool Stop();
	virtual void GetServiceStatus(ServiceStatusCallback a_Callback);

	//! IPackageStore interface
	void DownloadPackage(const std::string & a_PackageId, const std::string & a_Version,
		Delegate<const std::string &> a_Callback);
	void GetVersions(const std::string & a_PackageId, 
		Delegate<const Json::Value &> a_Callback);

private:
	//! This class is responsible for checking whether the service is available or not
	class ServiceStatusChecker
	{
	public:
		ServiceStatusChecker(PackageStore* a_pPkgStoreService, ServiceStatusCallback a_Callback);

	private:
		PackageStore* m_pPkgStoreService;
		IService::ServiceStatusCallback m_Callback;

		void OnCheckService(const Json::Value & response);
	};
};

#endif