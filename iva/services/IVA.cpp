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


#define _CRT_SECURE_NO_WARNINGS

#include "IVA.h"
#include "SelfInstance.h"

#include "utils/Form.h"

const float RETRY_LOGIN_INTERVAL = 30.0f;

REG_SERIALIZABLE(IVA);
REG_OVERRIDE_SERIALIZABLE(IFaceRecognition,IVA);
RTTI_IMPL( IVA, IFaceRecognition );

IVA::IVA() : IFaceRecognition("IVAV1")
{}

void IVA::Serialize(Json::Value & json)
{
	IFaceRecognition::Serialize(json);
}

void IVA::Deserialize(const Json::Value & json)
{
	IFaceRecognition::Deserialize(json);
}

bool IVA::Start()
{
	Log::Debug("IVA", "Starting IVA...");
	if (!IFaceRecognition::Start())
		return false;

	// we don't need to send basic auth
	m_Headers.erase("Authorization");
	Login();

	return true;
}

void IVA::ClassifyFace( const std::string & a_Face,
	Delegate<const TiXmlDocument &> a_Callback )
{
	Form form;
	form.AddFilePart( "image", "image", a_Face, "image/jpeg" );
	form.Finish();

	Headers headers;
	headers["X-IVA-Request"] = m_SessionId;
	headers["Content-Type"] = form.GetContentType();

	new RequestXml( this, "/dleProxyFr/:getFeature256SingleImage", "POST", headers, form.GetBody(), a_Callback );
}

bool IVA::SearchForFace( const TiXmlDocument & a_F256,
	float a_fThreshold, int a_MaxResults,
	Delegate<const Json::Value &> a_Callback )
{
	const TiXmlElement * pResult = a_F256.FirstChildElement( "Results" );
	if ( pResult == NULL )
		return false;
	const TiXmlElement * pStatus = pResult->FirstChildElement( "Status" );
	if ( pStatus == NULL )
		return false;
	const TiXmlElement * pFace = pResult->FirstChildElement( "Face" );
	if ( pFace == NULL )
		return false;
	const TiXmlElement * pF256 = pFace->FirstChildElement( "F256" );
	if ( pF256 == NULL )
		return false;

	Json::Value search;
	search["_type"] = "frSearch";
	search["_markup"] = "default";
	Json::Value item;
	item["threshold"] = a_fThreshold;
	item["maxResults"] = a_MaxResults;
	item["features"] = pF256->GetText();
	search["items"][0] = item;

	Headers headers;
	headers["Accept"] = "application/json";
	headers["Content-Type"] = "application/json";
	headers["X-IVA-Request"] = m_SessionId;

	new RequestJson( this, "/frSearch/:watchlist", "POST", headers, search.toStyledString(), a_Callback );
	return true;
}

bool IVA::AddFace( const TiXmlDocument & a_F256,
	const std::string & a_FaceImage,
	const std::string & a_PersonId,
	const std::string & a_Name,
	const std::string & a_Gender,
	const std::string & a_DOB,
	Delegate<const Json::Value &> a_Callback )
{
	AddFaceReq * pReq = new AddFaceReq( this );
	if (! pReq->Start( a_F256, a_FaceImage, a_PersonId, a_Name, a_Gender, a_DOB, a_Callback ) )
	{
		delete pReq;
		return false;
	}

	return true;
}

//------------------------------

void IVA::Login()
{
	// request session ID ..
	Json::Value login;
	login["_type"] = "session";
	login["_markup"] = "login";
	Json::Value items;
	items["userId"] = GetConfig()->m_User;
	items["password"] = GetConfig()->m_Password;
	items["locale"] = "en";
	items["mime"] = "json";
	items["timezone"] = "America/New_York";		// TODO 
	items["numberFormat"] = "######.##";
	items["timestampFormat"] = "yyyy-MM-dd'T'HH.mm.ss.SSS";
	items["dateFormat"] = "yyyy-MM-dd";
	login["items"][0] = items;

	Headers headers;
	headers["Accept"] = "application/json";
	headers["Content-Type"] = "application/json";
	headers["X-IVA-Request"] = m_SessionId;
	//headers["Cookie"] = "JSESSIONID=0000RxDAEQHhHgUeUJ9pPBRiI1t:-1; IVASESSIONID=-802999335";

	new Request( this, "/session", "POST", headers, login.toStyledString(), 
		DELEGATE( IVA, OnLoginSession, IService::Request *, this ) );
}

void IVA::OnLoginSession( IService::Request * a_pRequest )
{
	Log::Status( "IVA", "OnLoginSession: %s", a_pRequest->GetResponse().c_str() );

	const Cookies & cookies = a_pRequest->GetCookies();
	for( Cookies::const_iterator iSetCookie = cookies.begin(); iSetCookie != cookies.end(); ++iSetCookie )
	{
		std::vector<std::string> cookies;
		StringUtil::Split( iSetCookie->second, ";", cookies );

		for(size_t i=0;i<cookies.size();++i)
		{
			std::vector<std::string> kv;
			StringUtil::Split( cookies[i], "=", kv );

			if ( kv.size() != 2 )
				continue;
			if ( kv[0] == "IVASESSIONID" )
				m_SessionId = kv[1];
			else if ( kv[0] == "JSESSIONID" )
				m_JSessionId = kv[1];
		}
	}

	m_spRetryTimer.reset();
	if ( m_JSessionId.size() == 0 || m_SessionId.size() == 0 )
	{
		Log::Error( "IVA", "Failed to login to service, will retry in %f seconds.", RETRY_LOGIN_INTERVAL );
		if ( TimerPool::Instance() != NULL )
			m_spRetryTimer = TimerPool::Instance()->StartTimer( VOID_DELEGATE( IVA, Login, this ), RETRY_LOGIN_INTERVAL, true, false );
	}

	m_Headers["Cookie"] = "JSESSIONID=" + m_JSessionId + "; IVASESSIONID=" + m_SessionId;
}

//------------------------------

bool IVA::AddFaceReq::Start( const TiXmlDocument & a_F256,
	const std::string & a_FaceImage,
	const std::string & a_PersonId,
	const std::string & a_Name,
	const std::string & a_Gender,
	const std::string & a_DOB,
	Delegate<const Json::Value &> a_Callback ) 
{
	const TiXmlElement * pResult = a_F256.FirstChildElement( "Results" );
	if ( pResult == NULL )
		return false;

	//<Results><Timing>0.064</Timing><FileName>image</FileName><DLECode>DLE_W_0109</DLECode><Status>FR Feature Extraction &#x13; Face is not frontal. Facial match quality may degrade with non-frontal enrollment faces.</Status><Face><F256>0.6165 -6.3504 -5.3391 -13.0239 -0.0973 -9.9883 11.4153 -6.4570 15.8355 1.0788 20.1510 17.5951 -7.1724 0.4139 6.6838 -0.4952 -4.5128 12.6722 -5.1063 -2.3089 -10.6055 -6.3417 -14.4717 7.2971 -1.9838 -11.1928 -14.7495 16.4673 -9.1371 -12.1172 -5.5514 9.5825 -2.5176 -8.7385 21.3369 11.5865 12.9833 10.1630 -13.8693 -5.8165 6.6221 -2.2441 13.9780 -13.3803 16.0807 -9.6054 22.7298 5.8675 -18.3747 -16.1750 0.4032 -1.5285 -0.1982 -17.4374 2.5907 -5.2981 38.8106 3.5061 -5.2599 -8.1574 3.4742 -13.1186 -15.2119 -3.4147 -14.0982 2.7981 -13.9356 8.4495 -2.5669 2.0704 11.2463 -0.2928 5.8662 0.2160 -17.2828 4.9715 12.3843 7.0568 0.9047 -11.4179 8.6086 6.8528 10.7366 12.1707 12.2927 -6.5744 2.3041 -0.2815 18.1713 -1.1204 3.5948 13.7386 -2.9349 -17.8446 10.2510 6.2566 -5.1539 -1.1586 -11.4790 -1.4167 15.5319 -7.5912 -8.8199 -5.3994 -2.8592 19.7639 10.6854 -30.3857 -7.3318 6.7663 2.9734 1.9295 -10.7600 6.6638 1.9583 0.9783 -3.2504 2.1342 -11.0027 5.3454 -3.2135 3.3203 -1.8279 5.1674 2.6623 -9.4517 7.7426 1.2461 6.9951 -0.0318 14.8774 2.6740 -3.3000 -17.6370 2.8363 -0.6632 10.5034 6.4909 3.2685 -2.2509 2.7784 1.1734 -10.6742 17.2158 -19.4140 -3.0131 -2.3704 -15.5057 -7.1500 -13.2289 14.8897 -5.8013 15.5160 -13.0325 -0.0089 5.6545 8.8896 10.2119 22.1036 5.5096 1.6021 1.5328 -3.0141 -2.4776 -9.5128 4.7758 -6.6755 -12.2983 6.7337 6.2151 3.8802 -8.7262 12.0917 -4.2497 6.8140 -11.8950 0.5082 -9.5245 -2.3534 -7.9146 24.1788 -12.1433 1.3028 13.7380 3.6334 15.0911 -2.3768 -5.7348 4.7537 -7.5224 0.4359 -2.3956 1.0609 -11.5800 11.3684 7.1299 -3.0010 -1.3893 3.9988 -14.3261 -6.1891 3.2327 7.9145 7.1743 -8.0539 3.4162 9.3426 11.8292 13.8098 4.5767 5.9741 -5.2273 -9.3227 -9.7642 -0.9607 0.5212 -10.6932 -1.7009 -4.8142 -6.2554 -8.9581 2.1592 -5.5884 2.6274 5.7964 7.5079 -7.9041 1.4782 -27.9154 -8.1983 5.4678 -6.9570 -14.5523 1.7548 -6.0349 -14.4482 -12.1545 20.6386 10.8891 -5.8403 7.4397 3.6013 19.6444 8.9889 -10.2813 7.5666 -0.3139 10.8159 -15.1539 9.8051 6.6525 8.7129 -10.8988 2.7195 -6.0143 2.0331</F256><LandMarks><LeftEye>52 62</LeftEye><RightEye>79 59</RightEye><Nose>77 84</Nose><MouthLeft>63 96</MouthLeft><MouthRight>81 93</MouthRight></LandMarks><BBox>29.0 103.0 38.0 112.0</BBox></Face></Results>
	const TiXmlElement * pStatus = pResult->FirstChildElement( "Status" );
	if ( pStatus == NULL )
		return false;
	const TiXmlElement * pFace = pResult->FirstChildElement( "Face" );
	if ( pFace == NULL )
		return false;
	const TiXmlElement * pF256 = pFace->FirstChildElement( "F256" );
	if ( pF256 == NULL )
		return false;
	const TiXmlElement * pBBOX = pFace->FirstChildElement( "BBox" );
	if ( pBBOX == NULL )
		return false;
	const TiXmlElement * pLandMarks = pFace->FirstChildElement( "LandMarks" );
	if ( pLandMarks == NULL )
		return false;
	if ( a_PersonId.size() > 20 )
	{
		Log::Warning( "IVA", "PersonId can not be any larger than 20 characters." );
		return false;
	}

	m_FaceImage = a_FaceImage;
	m_PersonId = a_PersonId;

	m_Name = a_Name;
	m_Gender = a_Gender;
	if ( m_Gender.size() > 1 )
		m_Gender = m_Gender.substr( 0, 1 );
	m_DOB = a_DOB;
	if ( m_DOB[0] == 0 )
		m_DOB = m_Time.GetFormattedTime( "%Y-%m-%d" );
	m_Callback = a_Callback;

	m_Features = pF256->GetText();
	m_BBox = pBBOX->GetText();

	m_Landmarks.clear();
	const TiXmlElement * pMark = pLandMarks->FirstChildElement();
	while( pMark != NULL )
	{
		if ( m_Landmarks.size() > 0 )
			m_Landmarks += " ";
		m_Landmarks += pMark->GetText();
		pMark = pMark->NextSiblingElement();
	}

	EnrollUser();
	return true;
}

void IVA::AddFaceReq::EnrollUser()
{
	// frPerson
	// {"_type":"frPerson","_markup":"default","items":[{"personId":"1479277955771wdc1frd","firstName":"Richard","lastName":"Lyle","fullName":"Richard Lyle","dob":"1970-04-13","gender":"M"}]}

	Json::Value enroll;
	enroll["_type"] = "frPerson";
	enroll["_markup"] = "default";
	Json::Value item;
	item["personId"] = m_PersonId;

	std::vector<std::string> names;
	StringUtil::Split( m_Name, " ", names ); 
	if ( names.size() > 0 )
	{
		item["firstName"] = names[0];
		item["fullName"] = m_Name;
	}
	else
	{
		item["firstName"] = m_Gender != "F" ? "John" : "Jane";
		item["fullName"] = m_Gender != "F" ? "John Doe" : "Jane Doe";
	}
	item["lastName"] = names.size() > 1 ? names[names.size() - 1] : "";
	item["dob"] = m_DOB;
	item["gender"] = m_Gender;

	enroll["items"][0] = item;

	IService::Headers headers;
	headers["Accept"] = "application/json";
	headers["Content-Type"] = "application/json";
	headers["X-IVA-Request"] = m_pService->m_SessionId;

	new RequestJson( m_pService, "/frPerson", "POST", headers, enroll.toStyledString(), 
		DELEGATE( AddFaceReq, OnEnrollUser, const Json::Value &, this ) );
}

void IVA::AddFaceReq::OnEnrollUser( const Json::Value & a_Response )
{
	if (! a_Response.isNull() )
	{
		Log::Debug( "IVA::AddFaceReq", "Uploading files." );
		UploadFiles();
	}
	else
	{
		Log::Error( "IVA::AddFaceReq", "EnrollUser failed." );
		m_Callback( Json::Value() );
	}
}

void IVA::AddFaceReq::UploadFiles()
{
	m_CID = m_Time.GetFormattedTime( "%Y/%m/%d/wlf");

	Form form;
	form.AddFormField( "response_type", "json" );		// was "xml"
	form.AddFormField( "BBox", m_BBox ); 
	form.AddFormField( "FaceMarks", m_Landmarks );
	form.AddFormField( "f256", m_Features );
	// Wed Nov 16 2016 00:00:00 GMT-0600 (Central Standard Time)
	form.AddFormField( "dateTaken", m_Time.GetFormattedTime( "%a %b %d %H:%M:%S %Z" ) );
	form.AddFormField( "cid", m_CID );
	form.AddFormField( "imageFileName", m_PersonId + ".jpg" );
	form.AddFormField( "f256FileName", m_PersonId + ".F256" );
	form.AddFilePart( "image", "face.jpg", m_FaceImage, "image/jpeg" );
	form.Finish();

	IService::Headers headers;
	headers["Accept"] = "application/json";
	headers["Content-Type"] = form.GetContentType();
	headers["X-IVA-Request"] = m_pService->m_SessionId;

	new RequestJson( m_pService, "/frFile", "POST", headers, form.GetBody(),
		DELEGATE( AddFaceReq, OnUploadFiles, const Json::Value &, this ) );
}

void IVA::AddFaceReq::OnUploadFiles( const Json::Value & a_Response )
{
	if (! a_Response.isNull() )
	{
		Log::Debug( "IVA::AddFaceReq", "Uploading features." );
		UpdateFeatures();
	}
	else
	{
		Log::Error( "IVA::AddFaceReq", "UploadFiles failed." );
		m_Callback( Json::Value() );
	}
}

void IVA::AddFaceReq::UpdateFeatures()
{
	// watchlistFeature
	//{"_type":"watchlistFeature","_markup":"default","items":[{"personId":"1479277955771wdc1frd","imageTS":"2016-11-16T00.00.00.000",
	//"imagePath":"2016/11/16/wlf/img1479277955771wdc1frd_1479277956114.jpg",
	//"featurePath":"2016/11/16/wlf/img1479277955771wdc1frd_1479277956114.F256",

	Json::Value update;
	update["_type"] = "watchlistFeature";
	update["_markup"] = "default";
	Json::Value item;
	item["personId"] = m_PersonId;
	item["imageTS"] =  m_Time.GetFormattedTime( "%Y-%m-%dT00.00.00.000" );
	item["imagePath"] = m_CID + "/" + m_PersonId + ".jpg";
	item["featurePath"] = m_CID + "/" + m_PersonId + ".F256";

	//"bBox":{"lTX":10,"lTY":100,"rBX":100,"rBY":10},"eyes":{"lCX":20,"lCY":20,"rCX":80,"rCY":20},"nose":{"CX":50,"CY":50},"mouth":{"lCX":30,"lCY":80,"rCX":80,"rCY":80}}]}
	float v[ 4 ];
	sscanf( m_BBox.c_str(),"%f %f %f %f", &v[0], &v[1], &v[2], &v[3] );

	Json::Value bbox;
	bbox["lTX"] = (int)v[0];
	bbox["lTY"] = (int)v[1];
	bbox["rBX"] = (int)v[3];
	bbox["rBY"] = (int)v[2];
	item["bBox"] = bbox;

	int m[ 10 ];
	sscanf( m_Landmarks.c_str(), "%d %d %d %d %d %d %d %d %d %d", &m[0], &m[1], &m[2], &m[3], &m[4], &m[5], &m[6], &m[7], &m[8], &m[9] );

	Json::Value eyes;
	eyes["lCX"] = m[0];
	eyes["lCY"] = m[1];
	eyes["rCX"] = m[2];
	eyes["rCY"] = m[3];
	item["eyes"] = eyes;

	Json::Value nose;
	nose["CX"] = m[4];
	nose["CY"] = m[5];
	item["nose"] = nose;

	Json::Value mouth;
	mouth["lCX"] = m[6];
	mouth["lCY"] = m[7];
	mouth["rCX"] = m[8];
	mouth["rCY"] = m[9];
	item["mouth"] = mouth;

	update["items"][0] = item;

	IService::Headers headers;
	headers["Accept"] = "application/json";
	headers["Content-Type"] = "application/json";
	headers["X-IVA-Request"] = m_pService->m_SessionId;

	new RequestJson( m_pService, "/watchlistFeature", "POST", headers, update.toStyledString(), 
		DELEGATE( AddFaceReq, OnUpdateFeatures, const Json::Value &, this ) );
}

void IVA::AddFaceReq::OnUpdateFeatures( const Json::Value & a_Response )
{
	if (! a_Response.isNull() )
		Log::Status( "IVA::AddFaceReq", "Face added." );
	else
		Log::Error( "IVA::AddFaceReq", "UploadFiles failed." );

	m_Callback( a_Response );
}

