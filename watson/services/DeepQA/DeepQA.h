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


#ifndef WDC_DEEPQA_H
#define WDC_DEEPQA_H

#include "utils/IService.h"

class DeepQA : public IService
{
public:
    RTTI_DECL();

    //! Construction
    DeepQA();

    //! ISerializable
    virtual void Serialize(Json::Value & json);
    virtual void Deserialize(const Json::Value & json);

    //! IService interface
    virtual bool Start();

    void AskQuestion(const std::string & a_Input, Delegate<const Json::Value &> a_Callback,
        int a_NumAnswers = 1, bool a_bInferQuestion = false, Json::Value a_Evidence = Json::Value() );

};

#endif
