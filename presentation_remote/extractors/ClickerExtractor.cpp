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

#include "ClickerExtractor.h"

#include "SelfInstance.h"
#include "sensors/SensorManager.h"
#include "sensors/ISensor.h"
#include "sensors/ClickerData.h"
#include "utils/Delegate.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/IThing.h"
#include "utils/ThreadPool.h"

#include "jsoncpp/json/json.h"

REG_SERIALIZABLE(ClickerExtractor);
RTTI_IMPL( ClickerExtractor, IFeatureExtractor );

const char * ClickerExtractor::GetName() const
{
    return "ClickerExtractor";
}

bool ClickerExtractor::OnStart()
{
    SelfInstance * pInstance = SelfInstance::GetInstance();
    if ( pInstance != NULL )
    {
        pInstance->GetSensorManager()->RegisterForSensor( "ClickerData",
                                                          DELEGATE( ClickerExtractor, OnAddSensor, ISensor *, this ),
                                                          DELEGATE( ClickerExtractor, OnRemoveSensor, ISensor *, this ) );
    }

    Log::Status("ClickerExtractor", "ClickerExtractor started");
    return true;
}

bool ClickerExtractor::OnStop()
{
    SelfInstance * pInstance = SelfInstance::GetInstance();
    if ( pInstance != NULL )
        pInstance->GetSensorManager()->UnregisterForSensor( "ClickerData", this );

    Log::Status("ClickerExtractor", "ClickerExtractor stopped");
    return true;
}

void ClickerExtractor::OnAddSensor( ISensor * a_pSensor )
{
    Log::Status( "ClickerExtractor", "Adding new clicker sensor %s", a_pSensor->GetSensorId().c_str() );
    m_ClickerSensors.push_back( a_pSensor->shared_from_this() );
    a_pSensor->Subscribe( DELEGATE( ClickerExtractor, OnClickerData, IData *, this ) );
}

void ClickerExtractor::OnRemoveSensor( ISensor * a_pSensor )
{
    for(size_t i=0;i<m_ClickerSensors.size();++i)
    {
        if ( m_ClickerSensors[i].get() == a_pSensor )
        {
            m_ClickerSensors.erase( m_ClickerSensors.begin() + i );
            Log::Status( "ClickerExtractor", "Removing video sensor %s", a_pSensor->GetSensorId().c_str() );
            a_pSensor->Unsubscribe( this );
            break;
        }
    }
}

void ClickerExtractor::OnClickerData(IData * a_pData)
{
    ClickerData * pClicker = DynamicCast<ClickerData>(a_pData);
    if ( pClicker != NULL )
    {
        SelfInstance::GetInstance()->GetBlackBoard()->AddThing(IThing::SP(
                new IThing(TT_PERCEPTION, "Click", IThing::JsonObject("m_Input", pClicker->GetInput()), 3600.0f )));
        Log::Debug("ClickerExtractor", "Adding Click Object with input: %s", pClicker->GetInput().c_str());
    }
}