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

#define WRITE_SCENE_PCD			0

#include "PCLObjectRecognition.h"
#include "SelfInstance.h"

#include "opencv2/opencv.hpp"
#include "pcl/point_cloud.h"
#include "pcl/point_types.h"
#include "pcl/io/pcd_io.h"
#include "pcl/features/normal_3d_omp.h"
#include "pcl/features/shot_omp.h"
#include "pcl/filters/voxel_grid.h"

REG_SERIALIZABLE(PCLObjectRecognition);
REG_OVERRIDE_SERIALIZABLE(IObjectRecognition,PCLObjectRecognition);
RTTI_IMPL(PCLObjectRecognition,IObjectRecognition);

PCLObjectRecognition::PCLObjectRecognition() : 
	IObjectRecognition( "PCL", AUTH_NONE ), 
	m_ModelSS( 0.01f ),
	m_DescRad( 0.02f )
{}

void PCLObjectRecognition::Serialize(Json::Value & json)
{
	IObjectRecognition::Serialize(json);
	json["m_ModelSS"] = m_ModelSS;
	json["m_DescRad"] = m_DescRad;
	SerializeVector( "m_Objects", m_Objects, json );
}

void PCLObjectRecognition::Deserialize(const Json::Value & json)
{
	IObjectRecognition::Deserialize(json);
	if ( json["m_ModelSS"].isNumeric() )
		m_ModelSS = json["m_ModelSS"].asFloat();
	if ( json["m_DescRad"].isNumeric() )
		m_DescRad = json["m_DescRad"].asFloat();
	DeserializeVector( "m_Objects", json, m_Objects );

	// if no data is provided, initialize with some default data..
	if ( m_Objects.size() == 0 )
	{
		std::vector<std::string> models;
		models.push_back( "shared/pcd/drill/d000.pcd" );
		models.push_back( "shared/pcd/drill/d045.pcd" );
		models.push_back( "shared/pcd/drill/d090.pcd" );
		models.push_back( "shared/pcd/drill/d135.pcd" );
		models.push_back( "shared/pcd/drill/d180.pcd" );
		models.push_back( "shared/pcd/drill/d225.pcd" );
		models.push_back( "shared/pcd/drill/d270.pcd" );
		models.push_back( "shared/pcd/drill/d315.pcd" );
		m_Objects.push_back( ObjectModel( "drill", models ) );
	}
}

bool PCLObjectRecognition::Start()
{
	if (! IObjectRecognition::Start() )
		return false;

	int modelsLoaded = 0;
	for(size_t i=0;i<m_Objects.size();++i)
	{
		if ( m_Objects[i].LoadPCD( m_ModelSS, m_DescRad ) )
		{
			Log::Status( "PCLObjectRecognition", "Loaded models for %s", m_Objects[i].m_ObjectId.c_str() );
			modelsLoaded += 1;
		}
		else
			Log::Error( "PCLObjectRecognition", "Failed to load models for object %s", m_Objects[i].m_ObjectId.c_str() );
	}

	Log::Status( "PCLObjectRecognition", "Loaded %d models", modelsLoaded );
	return true;
}

void PCLObjectRecognition::ClassifyObjects(const std::string & a_DepthImageData,
	OnClassifyObjects a_Callback )
{
	ThreadPool::Instance()->InvokeOnThread<ProcessDepthData *>( DELEGATE( PCLObjectRecognition, ProcessThread, ProcessDepthData *, this ), 
		new ProcessDepthData( a_DepthImageData, a_Callback ) );
}

void PCLObjectRecognition::ProcessThread( ProcessDepthData * a_pData )
{
	//Log::Status( "PCLObjectRecogition", "Processing %u bytes of depth data.", a_pData->m_DepthData.size() );

	// convert depth data into PCD
	const std::string & data = a_pData->m_DepthData;
	std::vector<unsigned char> encoded( (unsigned char *)data.data(), (unsigned char *)data.data() + data.size() );
	cv::Mat decoded = cv::imdecode( encoded, CV_LOAD_IMAGE_ANYDEPTH );
	
	pcl::PointCloud<pcl::PointXYZ>::Ptr scene( new pcl::PointCloud<pcl::PointXYZ> );
	scene->height = decoded.rows;
	scene->width = decoded.cols;
	scene->is_dense = false;
	scene->points.resize( scene->width * scene->height );

	const float constant = 1.0f / 525;
	const int centerX = scene->width >> 1;
	const int centerY = scene->height >> 1;
	register int depth_idx = 0;
	for(int v=-centerY;v<centerY;++v)
	{
		for(register int u=-centerX;u<centerX;++u,++depth_idx)
		{
			pcl::PointXYZ & pt = scene->points[depth_idx];
			pt.z = decoded.at<unsigned short>( depth_idx ) * 0.001f;
			pt.x = static_cast<float>(u) * pt.z * constant;
			pt.y = static_cast<float>(v) * pt.z * constant;
		}
	}
	scene->sensor_origin_.setZero();
	scene->sensor_orientation_.w() = 0.0f;
	scene->sensor_orientation_.x() = 1.0f;
	scene->sensor_orientation_.y() = 0.0f;
	scene->sensor_orientation_.z() = 0.0f;

#if WRITE_SCENE_PCD
	// save to a local file 
	pcl::io::savePCDFile( "scene.pcd", *scene );
#endif

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


bool PCLObjectRecognition::ObjectModel::LoadPCD( float a_ModelSS, float a_DescRad )
{
	const std::string & staticData = SelfInstance::GetInstance()->GetStaticDataPath();
	for(size_t i=0;i<m_Models.size();++i)
	{
		ModelPCD pcd;
		pcd.m_Model = (new pcl::PointCloud<PointType>())->makeShared();
		if ( pcl::io::loadPCDFile( staticData + m_Models[i], *pcd.m_Model ) < 0 )
		{
			Log::Error( "ObjectModel", "Failed to load %s", m_Models[i].c_str() );
			return false;
		}

		// compute normals
		pcd.m_Normals = (new pcl::PointCloud<NormalType>())->makeShared();
		pcl::NormalEstimationOMP<PointType, NormalType> norm_est;
		norm_est.setKSearch(10);
		norm_est.setInputCloud(pcd.m_Model);
		norm_est.compute(*pcd.m_Normals);

		// down sample clouds to extract keypoints
		pcd.m_Keypoints = (new pcl::PointCloud<PointType>())->makeShared();
		pcl::VoxelGrid<PointType> uniform_sampling;
		uniform_sampling.setInputCloud(pcd.m_Model);
		uniform_sampling.setLeafSize(a_ModelSS, a_ModelSS, a_ModelSS);
		uniform_sampling.filter(*pcd.m_Keypoints);

		// compute descriptors for keypoints
		pcd.m_Descriptors = (new pcl::PointCloud<DescriptorType>())->makeShared();
		pcl::SHOTEstimationOMP<PointType, NormalType, DescriptorType> descr_est;
		descr_est.setRadiusSearch(a_DescRad);
		descr_est.setInputCloud(pcd.m_Keypoints);
		descr_est.setInputNormals(pcd.m_Normals);
		descr_est.setSearchSurface(pcd.m_Model);
		descr_est.compute(*pcd.m_Descriptors);

		m_PCD.push_back( pcd );
	}

	return true;
}