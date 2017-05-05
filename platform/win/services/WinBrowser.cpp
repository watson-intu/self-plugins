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

#include "WinBrowser.h"
#include <windows.h>
#include <ShellAPI.h>

REG_SERIALIZABLE(WinBrowser);
RTTI_IMPL(WinBrowser, IBrowser);

REG_OVERRIDE_SERIALIZABLE(IBrowser, WinBrowser);

void WinBrowser::Serialize(Json::Value & json)
{
	IBrowser::Serialize(json);
}

void WinBrowser::Deserialize(const Json::Value & json)
{
	IBrowser::Deserialize(json);
}

bool WinBrowser::Start()
{
	Log::Status("WinBrowser", "Starting..");

	if (!IBrowser::Start())
		return false;

	return true;
}

bool WinBrowser::Stop()
{
	Log::Status("WinBrowser", "Stopping..");

	return IBrowser::Stop();
}

void WinBrowser::ShowURL(const Url::SP & a_spUrlAgent, UrlCallback a_Callback)
{
	Log::Debug("MacBrowser", "Opening the following URL: %s", a_spUrlAgent->GetURL().c_str());
	ShellExecute(NULL, "open", a_spUrlAgent->GetURL().c_str(),
		NULL, NULL, SW_SHOWNORMAL);
}
