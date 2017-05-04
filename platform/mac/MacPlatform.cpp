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

#include "MacPlatform.h"
#include "blackboard/URL.h"

MacPlatform * MacPlatform::sm_pInstance = NULL;

MacPlatform * MacPlatform::Instance()
{
    static tthread::mutex lock;
    tthread::lock_guard<tthread::mutex> guard( lock );
    if ( sm_pInstance == NULL )
        new MacPlatform();

    return sm_pInstance;
}

MacPlatform::MacPlatform()
{
    IBrowser * pService = SelfInstance::GetInstance()->FindService<IBrowser>();
    if ( pService != NULL )
    {
        Log::Debug("MacPlatform", "Starting browser! %s", pService->GetRTTI().GetName().c_str());
        Url::SP spUrl(new Url("http://127.0.0.1:9443/www/chat"));
        pService->ShowURL( spUrl,
                           DELEGATE( MacPlatform, OnResponse, IBrowser::URLServiceData *, this ) );
    }
    sm_pInstance = this;
}

void MacPlatform::OnResponse( IBrowser::URLServiceData * a_UrlAgentData )
{
    // Received callback that URL has loaded
}
