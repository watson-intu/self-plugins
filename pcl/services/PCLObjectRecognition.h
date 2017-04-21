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

#ifndef PLC_OBJECT_RECOGNITION_H
#define PLC_OBJECT_RECOGNITION_H

#include "services/IObjectRecognition.h"

class PCLObjectRecognition : public IObjectRecognition
{
public:
	RTTI_DECL();

	//! Types
	typedef Delegate<const Json::Value &>	OnClassifyObjects;

	//! Construction 
	PCLObjectRecognition() : IObjectRecognition( "PCL", AUTH_NONE )
	{}

	//! IObjectRecognition interface
	virtual void ClassifyObjects(const std::string & a_DepthImageData,
		OnClassifyObjects a_Callback );

private:
	//! Types
	struct ProcessDepthData
	{
		ProcessDepthData( const std::string & a_DepthData, OnClassifyObjects a_Callback ) :
			m_DepthData( a_DepthData ), m_Callback( a_Callback )
		{}

		std::string			m_DepthData;
		Json::Value			m_Results;
		OnClassifyObjects	m_Callback;
	};

	void ProcessThread( ProcessDepthData * a_pData );
	void SendResults( ProcessDepthData * a_pData );
};

#endif
