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


#ifndef WDC_VISUALRECOGNITION_H
#define WDC_VISUALRECOGNITION_H

#include "utils/Delegate.h"
#include "utils/DataCache.h"
#include "services/IVisualRecognition.h"

class VisualRecognition : public IVisualRecognition
{
public:
	RTTI_DECL();

	//! Types
	typedef std::map< std::string, std::string >	ZipMap;

	//! Construction 
	VisualRecognition();

	//! ISerializable
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IService interface
	virtual bool Start();
	virtual void GetServiceStatus(ServiceStatusCallback a_Callback);

	//! IVisualRecognition interface
	virtual void GetClassifiers( OnGetClassifier a_Callback );
	virtual void GetClassifier( const std::string & a_ClassifierId,
		OnGetClassifier a_Callback );
	virtual void ClassifyImage(const std::string & a_ImageData,
		const std::vector<std::string> & a_Classifiers,
		OnClassifyImage a_Callback,
		bool a_bKnowledgeGraph = false );
	virtual void DetectFaces(const std::string & a_ImageData,
		OnDetectFaces a_Callback );
	virtual void CreateClassifier( const std::string & a_ClassiferName,
		const std::vector<std::string> & a_PositiveExamples,
		const std::string & a_NegativeExamples,
		OnCreateClassifier a_Callback );
	virtual void UpdateClassifier(
		const std::string & a_ClassifierId,
		const std::string & a_ClassifierName,
		const std::vector< std::string > & a_PositiveExamples,
		const std::string & a_NegativeExamples,
		OnClassifierTrained a_Callback);
	virtual void DeleteClassifier( const std::string & a_ClassifierId,
		OnDeleteClassifier a_Callback );

private:
	//! Types
	class ServiceStatusChecker
	{
	public:
		ServiceStatusChecker(VisualRecognition * a_pService,
			ServiceStatusCallback a_Callback);

	private:
		VisualRecognition *				m_pService;
		IService::ServiceStatusCallback m_Callback;

		void OnCheckService(const Json::Value &);
	};

	//! Data
	std::string				m_APIVersion;
	float					m_ClassifyThreshold;
};

#endif
