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


#ifndef PLC_OBJECT_RECOGNITION_H
#define PLC_OBJECT_RECOGNITION_H

#include "services/IObjectRecognition.h"

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

class PCLObjectRecognition : public IObjectRecognition
{
public:
	RTTI_DECL();

	//! Types
	typedef Delegate<const Json::Value &>	OnClassifyObjects;

	typedef pcl::PointXYZ		PointType;
	typedef pcl::Normal			NormalType;
	typedef pcl::SHOT352		DescriptorType;
	typedef pcl::ReferenceFrame RFType;

	//! This structure is used to hold loaded point cloud data for a particular angle of a model
	struct ModelPCD
	{
		ModelPCD() : 
			m_Model( new pcl::PointCloud<PointType>() ),
			m_Keypoints( new pcl::PointCloud<PointType>() ),
			m_Normals( new pcl::PointCloud<NormalType>() ),
			m_Descriptors( new pcl::PointCloud<DescriptorType>() )
		{}

		pcl::PointCloud<PointType>::Ptr	m_Model;
		pcl::PointCloud<PointType>::Ptr m_Keypoints;
		pcl::PointCloud<NormalType>::Ptr m_Normals;
		pcl::PointCloud<DescriptorType>::Ptr m_Descriptors;
	};

	struct ObjectModel : public ISerializable
	{
		ObjectModel()
		{}
		ObjectModel( const std::string & a_ObjectId, const std::vector<std::string> & a_Models ) :
			m_ObjectId( a_ObjectId ), m_Models( a_Models )
		{}

		std::string						m_ObjectId;		// the ID of this object
		std::vector<std::string>		m_Models;		// list of files containing the PCD
		std::vector<ModelPCD>			m_PCD;			// loaded point-cloud data

		//! ISerializable interface
		virtual void Serialize(Json::Value & json)
		{
			json["m_ObjectId"] = m_ObjectId;
			SerializeVector( "m_Models", m_Models, json );
		}
		virtual void Deserialize(const Json::Value & json)
		{
			if ( json["m_ObjectId"].isString() )
				m_ObjectId = json["m_ObjectId"].asString();
			DeserializeVector( "m_Models", json, m_Models );
		}

		bool LoadPCD( float a_ModelSS, float a_DescRad );
	};

	//! Construction 
	PCLObjectRecognition();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IService interface
	virtual bool Start();

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

	//! Data
	std::vector<ObjectModel>		m_Objects;
	float							m_ModelSS;
	float							m_DescRad;
	float							m_SceneSS;
	float							m_ShotDist;
	float							m_RFRad;
	float							m_CGSize;
	float							m_CGThresh;
	float							m_LowHeight;
	float							m_DistThreshold;
};

#endif
