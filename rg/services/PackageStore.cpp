// Copyright (c) 2011 rubicon IT GmbH
#include "SelfInstance.h"
#include "PackageStore.h"
#include "utils/StringUtil.h"

const double UPDATE_CONFIG_INTERVAL = 300.0;

REG_SERIALIZABLE(PackageStore);
REG_OVERRIDE_SERIALIZABLE(IPackageStore,PackageStore);
RTTI_IMPL(PackageStore, IPackageStore);

PackageStore::PackageStore() : IPackageStore("PackageStoreV1")
{}

PackageStore::~PackageStore()
{}

void PackageStore::Serialize(Json::Value & json)
{
	IPackageStore::Serialize(json);
}

void PackageStore::Deserialize(const Json::Value & json)
{
	IPackageStore::Deserialize(json);
}

bool PackageStore::Start()
{
	if (!IPackageStore::Start())
		return false;

	return true;
}

bool PackageStore::Stop()
{
	return IPackageStore::Stop();
}

void PackageStore::GetServiceStatus(ServiceStatusCallback a_Callback)
{
	if (m_pConfig != NULL)
		new ServiceStatusChecker(this, a_Callback);
	else
		a_Callback(ServiceStatus(m_ServiceId, false));
}

void PackageStore::DownloadPackage(const std::string & a_PackageId, const std::string & a_Version, Delegate<const std::string &> a_Callback)
{
	if (a_PackageId.length() == 0)
	{
		Log::Error("PackageStore", "No package passed in, returning");
		return;
	}

	if (!a_Callback.IsValid())
	{
		Log::Error("PackageStore", "No callback passed in, returning");
		return;
	}

	Headers headers;
	headers["Content-Type"] = "application/json";

	new RequestData(this, "/download?packageId=" + a_PackageId + "&version=" + a_Version, "POST", headers, EMPTY_STRING, a_Callback, NULL, 300.0f);
}

void PackageStore::GetVersions(const std::string & a_PackageId, Delegate<const Json::Value &> a_Callback)
{
	if (a_PackageId.length() == 0)
	{
		Log::Error("PackageStore", "No package passed in, returning");
		return;
	}

	if (!a_Callback.IsValid())
	{
		Log::Error("PackageStore", "No callback passed in, returning");
		return;
	}

	Headers headers;
	headers["Content-Type"] = "application/json";

	new RequestJson(this, "/getVersions?packageId=" + a_PackageId, "POST", headers, EMPTY_STRING, a_Callback);
}

//! Creates an object responsible for service status checking
PackageStore::ServiceStatusChecker::ServiceStatusChecker(PackageStore* a_pPkgStoreService, ServiceStatusCallback a_Callback)
	: m_pPkgStoreService(a_pPkgStoreService), m_Callback(a_Callback)
{
	m_pPkgStoreService->GetVersions("Self-Nao.zip", DELEGATE(PackageStore::ServiceStatusChecker, OnCheckService, const Json::Value &, this));
}

//! Callback function invoked when service status is checked
void PackageStore::ServiceStatusChecker::OnCheckService(const Json::Value & versions)
{
	Log::Status("PackageStore", "Checking for service status");
	if (m_Callback.IsValid())
	{
		bool success = false;
		if (versions.isMember("devVersion") && versions.isMember("reqVersion") && versions.isMember("recVersion"))
			success = true;

		m_Callback(ServiceStatus(m_pPkgStoreService->m_ServiceId, success));
	}

	delete this;
}