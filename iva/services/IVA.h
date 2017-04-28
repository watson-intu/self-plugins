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


#ifndef SELF_IVA_H
#define SELF_IVA_H

#include "services/IFaceRecognition.h"

//! Intelligent Video Analytics service
class IVA : public IFaceRecognition
{
public:
	RTTI_DECL();

	//! Construction
	IVA();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IService interface
	virtual bool Start();

	//! Classify the provided face image data, should return a response containing the F256 data for the provided image
	void ClassifyFace( const std::string & a_FaceImageData,
		Delegate<const TiXmlDocument &> a_Callback );
	//! Search for a face in the DB with the given F256 profile..
	bool SearchForFace( const TiXmlDocument & a_F256,
		float a_fThreasHold, int a_MaxResults,
		Delegate<const Json::Value &> a_Callback );
	//! Upload a new face along with a name
	bool AddFace( const TiXmlDocument & a_F256,
		const std::string & a_FaceImage,
		const std::string & a_PersonId,
		const std::string & a_Name,
		const std::string & a_Gender,
		const std::string & a_DOB,
		Delegate<const Json::Value &> a_Callback );

private:
	//! Data
	std::string			m_SessionId;
	std::string			m_JSessionId;
	TimerPool::ITimer::SP
						m_spRetryTimer;

	void Login();
	void OnLoginSession( IService::Request * a_pRequest );

	class AddFaceReq
	{
	public:
		AddFaceReq(IVA * a_pService) : m_pService( a_pService )
		{}

		bool Start( const TiXmlDocument & a_F256,
			const std::string & a_FaceImage,
			const std::string & a_PersonId,
			const std::string & a_Name,
			const std::string & a_Gender,
			const std::string & a_DOB,
			Delegate<const Json::Value &> a_Callback );

	private:
		void EnrollUser();
		void OnEnrollUser( const Json::Value & a_Response );
		void UploadFiles();
		void OnUploadFiles( const Json::Value & a_Response );
		void UpdateFeatures();
		void OnUpdateFeatures( const Json::Value & a_Response );

		//! Data
		IVA *				m_pService;

		std::string			m_Features;
		std::string			m_BBox;
		std::string			m_Landmarks;

		std::string			m_FaceImage;
		std::string			m_PersonId;
		std::string			m_Name;
		std::string			m_Gender;
		std::string			m_DOB;
		Delegate<const Json::Value &>
			m_Callback;

		Time				m_Time;
		std::string			m_CID;
	};

};

#endif //SELF_IVA_H
