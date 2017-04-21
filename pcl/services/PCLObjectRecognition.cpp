/**
* Copyright 2015 IBM Corp. All Rights Reserved.
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

#include "PCLObjectRecognition.h"

REG_SERIALIZABLE(PCLObjectRecognition);
REG_OVERRIDE_SERIALIZABLE(IObjectRecognition,PCLObjectRecognition);
RTTI_IMPL(PCLObjectRecognition,IObjectRecognition);

void PCLObjectRecognition::ClassifyObjects(const std::string & a_DepthImageData,
	OnClassifyObjects a_Callback )
{
	ThreadPool::Instance()->InvokeOnThread<ProcessDepthData *>( DELEGATE( PCLObjectRecognition, ProcessThread, ProcessDepthData *, this ), 
		new ProcessDepthData( a_DepthImageData, a_Callback ) );
}

void PCLObjectRecognition::ProcessThread( ProcessDepthData * a_pData )
{
	Log::Status( "PCLObjectRecogition", "Processing %u bytes of depth data.", a_pData->m_DepthData.size() );

	// convert depth data into PCD

	// find objects in PCD
	a_pData->m_Results["objects"] = Json::Value( Json::arrayValue );

	// return results..
	ThreadPool::Instance()->InvokeOnMain<ProcessDepthData *>( DELEGATE( PCLObjectRecognition, SendResults, ProcessDepthData *, this ), a_pData );
}

void PCLObjectRecognition::SendResults( ProcessDepthData * a_pData )
{
	a_pData->m_Callback( a_pData->m_Results );
	delete a_pData;
}
