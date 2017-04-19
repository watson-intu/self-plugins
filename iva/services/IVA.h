/* ***************************************************************** */
/*                                                                   */
/* IBM Confidential                                                  */
/* OCO Source Materials                                              */
/*                                                                   */
/* (C) Copyright IBM Corp. 2001, 2014                                */
/*                                                                   */
/* The source code for this program is not published or otherwise    */
/* divested of its trade secrets, irrespective of what has been      */
/* deposited with the U.S. Copyright Office.                         */
/*                                                                   */
/* ***************************************************************** */


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
