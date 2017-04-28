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


#include "WEX.h"
#include "SelfInstance.h"
#include "utils/JsonHelpers.h"
#include "utils/MD5.h"

REG_SERIALIZABLE( WEX );
RTTI_IMPL( WEX, IService );

WEX::WEX() : IService("WEXV1")
{}

//! ISerializable
void WEX::Serialize(Json::Value & json)
{
    IService::Serialize(json);
    json["m_Collection"] = m_Collection;
}

void WEX::Deserialize(const Json::Value & json)
{
    IService::Deserialize(json);
    if( json.isMember("m_Collection") )
        m_Collection = json["m_Collection"].asString();
}

//! IService interface
bool WEX::Start()
{
    if (!IService::Start())
        return false;

    Log::Debug("WEX", "Url loaded as %s", m_pConfig->m_URL.c_str() );

    return true;
}

void WEX::Search(const std::string & a_Query, const std::string & a_Collection, Delegate<const std::string &> a_Callback )
{
    std::string params = "/v10/search";
    params += "?collection=" + a_Collection;
    params += "&query=" + StringUtil::UrlEscape(a_Query, false);

    std::string url = m_pConfig->m_URL + params;
    Log::Debug("WEX", "Requesting data from the following URL: %s", url.c_str());

    new RequestData(this, params, "GET", NULL_HEADERS, EMPTY_STRING, a_Callback,
		new CacheRequest( "Search", MakeMD5( url ) ) );
}

void WEX::Search(const std::string & a_Query, Delegate<const std::string &> a_Callback )
{
    std::string params = "/v10/search";
    params += "?collection=" + m_Collection;
    params += "&query=" + StringUtil::UrlEscape(a_Query, false);

    std::string url = m_pConfig->m_URL + params;
    Log::Debug("WEX", "Requesting data from the following URL: %s", url.c_str());

    new RequestData(this, params, "GET", NULL_HEADERS, EMPTY_STRING, a_Callback,
		new CacheRequest( "Search", MakeMD5( url.c_str() ) ) );
}
