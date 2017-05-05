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

#ifndef SELF_WINPLATFORM_H
#define SELF_WINPLATFORM_H

#include "SelfInstance.h"
#include "services/IBrowser.h"

class WinPlatform
{
public:
	static WinPlatform * Instance();

	//! Construction
	WinPlatform();
	~WinPlatform();

private:
	static      WinPlatform * sm_pInstance;
	void        OnResponse(IBrowser::URLServiceData * a_UrlAgentData);
};

#endif //SELF_WINPLATFORM_H
