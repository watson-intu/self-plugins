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

#define WRITE_SCENE_PCD			1

#include "PCLObjectRecognition.h"
#include "SelfInstance.h"

#include "opencv2/opencv.hpp"
#include "pcl/point_cloud.h"
#include "pcl/point_types.h"
#include "pcl/common/transforms.h"
#include "pcl/io/pcd_io.h"
#include "pcl/features/normal_3d_omp.h"
#include "pcl/features/shot_omp.h"
#include "pcl/features/board.h"
#include "pcl/filters/voxel_grid.h"
#include "pcl/segmentation/sac_segmentation.h"
#include "pcl/recognition/cg/hough_3d.h"
#include "pcl/registration/icp.h"

REG_SERIALIZABLE(PCLObjectRecognition);
REG_OVERRIDE_SERIALIZABLE(IObjectRecognition,PCLObjectRecognition);
RTTI_IMPL(PCLObjectRecognition,IObjectRecognition);

PCLObjectRecognition::PCLObjectRecognition() : 
	IObjectRecognition( "PCL", AUTH_NONE ), 
	m_ModelSS( 0.01f ),
	m_DescRad( 0.02f ),
	m_SceneSS( 0.01f ),
	m_ShotDist( 0.25f ),
	m_RFRad( 0.015f ),
	m_CGSize( 0.02f ),
	m_CGThresh( 1.0f ),
	m_LowHeight( -2.5f ),
	m_DistThreshold(0.01f)
{}

void PCLObjectRecognition::Serialize(Json::Value & json)
{
	IObjectRecognition::Serialize(json);
	json["m_ModelSS"] = m_ModelSS;
	json["m_DescRad"] = m_DescRad;
	json["m_SceneSS"] = m_SceneSS;
	json["m_ShotDist"] = m_ShotDist;
	json["m_RFRad"] = m_RFRad;
	json["m_CGSize"] = m_CGSize;
	json["m_CGThresh"] = m_CGThresh;
	json["m_LowHeight"] = m_LowHeight;
	json["m_DistThreshold"] = m_DistThreshold;

	SerializeVector( "m_Objects", m_Objects, json );
}

void PCLObjectRecognition::Deserialize(const Json::Value & json)
{
	IObjectRecognition::Deserialize(json);
	if ( json["m_ModelSS"].isNumeric() )
		m_ModelSS = json["m_ModelSS"].asFloat();
	if ( json["m_DescRad"].isNumeric() )
		m_DescRad = json["m_DescRad"].asFloat();
	if ( json["m_SceneSS"].isNumeric() )
		m_SceneSS = json["m_SceneSS"].asFloat();
	if ( json["m_ShotDist"].isNumeric() )
		m_ShotDist = json["m_ShotDist"].asFloat();
	if ( json["m_RFRad"].isNumeric() )
		m_RFRad = json["m_RFRad"].asFloat();
	if ( json["m_CGSize"].isNumeric() )
		m_CGSize = json["m_CGSize"].asFloat();
	if ( json["m_CGThresh"].isNumeric() )
		m_CGThresh = json["m_CGThresh"].asFloat();
	if ( json["m_LowHeight"].isNumeric() )
		m_LowHeight = json["m_LowHeight"].asFloat();
	if ( json["m_DistThreshold"].isNumeric() )
		m_DistThreshold = json["m_DistThreshold"].asFloat();

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
	double startTime = Time().GetEpochTime();
	Log::Status( "PCLObjectRecogition", "Processing %u bytes of depth data.", a_pData->m_DepthData.size() );

	// convert depth data into PCD
	const std::string & data = a_pData->m_DepthData;
	std::vector<unsigned char> encoded( (unsigned char *)data.data(), (unsigned char *)data.data() + data.size() );
	cv::Mat decoded = cv::imdecode( encoded, CV_LOAD_IMAGE_ANYDEPTH );
	
	pcl::PointCloud<pcl::PointXYZ>::Ptr spScene( new pcl::PointCloud<PointType>() );
	spScene->height = decoded.rows;
	spScene->width = decoded.cols;
	spScene->is_dense = false;
	spScene->points.resize( spScene->width * spScene->height );

	const float constant = 1.0f / 525;
	const int centerX = spScene->width >> 1;
	const int centerY = spScene->height >> 1;
	register int depth_idx = 0;
	for(int v=-centerY;v<centerY;++v)
	{
		for(register int u=-centerX;u<centerX;++u,++depth_idx)
		{
			pcl::PointXYZ & pt = spScene->points[depth_idx];
			pt.z = decoded.at<unsigned short>( depth_idx ) * 0.001f;
			pt.x = static_cast<float>(u) * pt.z * constant;
			pt.y = static_cast<float>(v) * pt.z * -constant;
		}
	}
	spScene->sensor_origin_.setZero();
	spScene->sensor_orientation_.w() = 0.0f;
	spScene->sensor_orientation_.x() = 1.0f;
	spScene->sensor_orientation_.y() = 0.0f;
	spScene->sensor_orientation_.z() = 0.0f;

#if WRITE_SCENE_PCD
	// save to a local file 
	pcl::io::savePCDFile( "scene.pcd", *spScene );
#endif

#if 1
	// segmentation for removing table and floor
	pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients);
	pcl::PointIndices::Ptr inliers(new pcl::PointIndices);
	pcl::SACSegmentation<PointType> seg;
	seg.setOptimizeCoefficients(false);
	seg.setModelType(pcl::SACMODEL_PLANE);
	seg.setMethodType(pcl::SAC_RANSAC);
	seg.setInputCloud( spScene );
	seg.setDistanceThreshold(m_DistThreshold);
	seg.segment(*inliers, *coefficients);

	if (inliers->indices.size() > 0) 
	{
		size_t iindex = 0;
		size_t p;

		std::vector<PointType, Eigen::aligned_allocator<PointType> > points;
		points.swap( spScene->points );
		for (p = 0; p < points.size(); p++) {
			if (p != inliers->indices[iindex]) {
				spScene->points.push_back(points[p]);
			}
			else {
				if (++iindex >= inliers->indices.size())
					break;
			}
		}
		for (; p < points.size(); p++) {
			spScene->points.push_back(points[p]);
		}

		spScene->width = spScene->points.size();
		spScene->height = 1;
	}
#if WRITE_SCENE_PCD
	// save to a local file 
	pcl::io::savePCDFile( "scene_segmented.pcd", *spScene );
#endif
#endif

#if 1
	//
	// Temporary remove lower part of points to reduce calculation
	//
	std::vector<PointType, Eigen::aligned_allocator<PointType> > points;
	points.swap( spScene->points );

	for (size_t p = 0; p < points.size(); p++) 
	{
		if ( points[p].z < -m_LowHeight)
			spScene->points.push_back(points[p]);
	}
	spScene->width = spScene->points.size();
	spScene->height = 1;

#if WRITE_SCENE_PCD
	// save to a local file 
	pcl::io::savePCDFile( "scene_clipped.pcd", *spScene );
#endif
#endif

	//
	// Compute Normals
	//
	pcl::PointCloud<NormalType>::Ptr scene_normals(new pcl::PointCloud<NormalType>());
	pcl::NormalEstimationOMP<PointType, NormalType> norm_est;
	norm_est.setKSearch(10);
	norm_est.setInputCloud(spScene);
	norm_est.compute(*scene_normals);

#if WRITE_SCENE_PCD
	// save to a local file 
	pcl::io::savePCDFile( "scene_normals.pcd", *scene_normals );
#endif

	//
	// Downsample Clouds to Extract keypoints
	//
	pcl::PointCloud<PointType>::Ptr scene_keypoints(new pcl::PointCloud<PointType>());
	pcl::VoxelGrid<PointType> uniform_sampling;
	uniform_sampling.setInputCloud(spScene);
	uniform_sampling.setLeafSize(m_SceneSS, m_SceneSS, m_SceneSS);
	uniform_sampling.filter(*scene_keypoints);

#if WRITE_SCENE_PCD
	// save to a local file 
	pcl::io::savePCDFile( "scene_keypoints.pcd", *scene_keypoints );
#endif

	//
	// Compute Descriptor for keypoints
	//
	pcl::PointCloud<DescriptorType>::Ptr scene_descriptors(new pcl::PointCloud<DescriptorType>());
	pcl::SHOTEstimationOMP<PointType, NormalType, DescriptorType> descr_est;
	descr_est.setRadiusSearch(m_DescRad);
	descr_est.setInputCloud(scene_keypoints);
	descr_est.setInputNormals(scene_normals);
	descr_est.setSearchSurface(spScene);
	descr_est.compute(*scene_descriptors);

#if WRITE_SCENE_PCD
	// save to a local file 
	pcl::io::savePCDFile( "scene_descriptors.pcd", *scene_descriptors );
#endif
	// find objects in PCD
	a_pData->m_Results["objects"] = Json::Value( Json::arrayValue );

	//
	// Loop for each reference model
	//
	std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f> > rototranslations_closest;
	std::vector<pcl::Correspondences> clustered_corrs_closest;
	int i_closest_object = -1;

	for(size_t i=0;i<m_Objects.size();++i)
	{
		ObjectModel & object = m_Objects[i];

		int corrs_closest = 0;
		int i_closest = -1;

		std::vector<int> i_max_corrs;
		i_max_corrs.resize( object.m_PCD.size() );
		for(size_t k=0;k<i_max_corrs.size();++k)
			i_max_corrs[k] = -1;

		for (size_t mi = 0; mi < object.m_PCD.size(); mi++) 
		{
			double elapsed = Time().GetEpochTime() - startTime;
			Log::Status( "PCLObjectRecognition", "Checking model %d of object %s, %.2f seconds elapsed.", 
				mi, object.m_ObjectId.c_str(), elapsed );

			//
			// Find Model-Scene Correspondences with KdTree
			//
			pcl::CorrespondencesPtr model_scene_corrs(new pcl::Correspondences());
			pcl::KdTreeFLANN<DescriptorType> match_search;
			match_search.setInputCloud( object.m_PCD[mi].m_Descriptors);

			//
			// For each scene keypoint descriptor, find nearest neighbor
			// into the model keypoints descriptor cloud and add it to the correspondences vector.
			//
			for (size_t i = 0; i < scene_descriptors->size(); ++i) 
			{
				std::vector<int> neigh_indices(1);
				std::vector<float> neigh_sqr_dists(1);
				if (!pcl_isfinite(scene_descriptors->at(i).descriptor[0])) {
					continue;
				}
				int found_neighs = match_search.nearestKSearch(
					scene_descriptors->at(i), 1, neigh_indices, neigh_sqr_dists);
				if(found_neighs == 1 && neigh_sqr_dists[0] < m_ShotDist) {
					pcl::Correspondence corr(neigh_indices[0], static_cast<int>(i), neigh_sqr_dists[0]);
					model_scene_corrs->push_back(corr);
				}
			}

			//
			// Actual Clustering
			//
			std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f> > rototranslations;
			std::vector<pcl::Correspondences> clustered_corrs;

			//
			// Compute (Keypoints) Reference Frames only for Hough
			//
			pcl::PointCloud<RFType>::Ptr model_rf(new pcl::PointCloud<RFType>());
			pcl::PointCloud<RFType>::Ptr scene_rf(new pcl::PointCloud<RFType>());
			pcl::BOARDLocalReferenceFrameEstimation<PointType, NormalType, RFType> rf_est;
			rf_est.setFindHoles(true);
			rf_est.setRadiusSearch(m_RFRad);
			rf_est.setInputCloud(object.m_PCD[mi].m_Keypoints);
			rf_est.setInputNormals(object.m_PCD[mi].m_Normals);
			rf_est.setSearchSurface(object.m_PCD[mi].m_Model);
			rf_est.compute(*model_rf);
			rf_est.setInputCloud(scene_keypoints);
			rf_est.setInputNormals(scene_normals);
			rf_est.setSearchSurface(spScene);
			rf_est.compute(*scene_rf);

			//
			// Clustering
			//
			pcl::Hough3DGrouping<PointType, PointType, RFType, RFType> clusterer;
			clusterer.setHoughBinSize(m_CGSize);
			clusterer.setHoughThreshold(m_CGThresh);
			clusterer.setUseInterpolation(true);
			clusterer.setUseDistanceWeight(false);
			clusterer.setInputCloud(object.m_PCD[mi].m_Keypoints);
			clusterer.setInputRf(model_rf);
			clusterer.setSceneCloud(scene_keypoints);
			clusterer.setSceneRf(scene_rf);
			clusterer.setModelSceneCorrespondences(model_scene_corrs);
			//    clusterer.cluster (clustered_corrs);
			clusterer.recognize(rototranslations, clustered_corrs);

			//
			//  Output results
			//
			size_t max_corrs = 0;
			for (size_t i = 0; i < rototranslations.size(); i++) {
				Eigen::Matrix3f rotation = rototranslations[i].block<3,3>(0, 0);
				Eigen::Vector3f translation = rototranslations[i].block<3,1>(0, 3);
				if (clustered_corrs[i].size() > max_corrs && translation(0) * translation(1) * translation(2) != 0.0) {
					max_corrs = clustered_corrs[i].size();
					i_max_corrs[mi] = i;
				}
			}
			if (i_max_corrs[mi] != -1) 
			{
				if (max_corrs > corrs_closest) 
				{
					corrs_closest = max_corrs;
					i_closest = mi;
					rototranslations_closest = rototranslations;
					clustered_corrs_closest = clustered_corrs;
				}

				Log::Status( "PCLObjectRecognition", "Object %s, Model[%d] instance %d/%u, has max # of correspondences: %u in %u", 
					object.m_ObjectId.c_str(), i_max_corrs[mi], rototranslations.size(), clustered_corrs[i_max_corrs[mi]].size(), model_scene_corrs->size() );
			}
		} // loop for each reference model

		if ( i_closest >= 0 )
		{
			if (i_max_corrs[i_closest] != -1) 
			{
				pcl::PointCloud<PointType>::Ptr rotated_model(new pcl::PointCloud<PointType>());
				pcl::transformPointCloud(*object.m_PCD[i_closest].m_Model, *rotated_model,
					rototranslations_closest[i_max_corrs[i_closest]]);

				//
				// Apply Iterative Closest Point Algorithm
				//
				pcl::IterativeClosestPoint<PointType, PointType> icp;
				icp.setInputSource(rotated_model);
				icp.setInputTarget(spScene);
				Eigen::Matrix4f pose = rototranslations_closest[i_max_corrs[i_closest]];
				Eigen::Matrix3f rotation = pose.block<3,3>(0, 0);
				Eigen::Vector3f translation = pose.block<3,1>(0, 3);
				Eigen::Matrix4f adj = icp.getFinalTransformation();
				Eigen::Matrix3f rotation2 = adj.block<3,3>(0, 0);
				Eigen::Vector3f translation2 = adj.block<3,1>(0, 3);
				rotation = rotation2 * rotation;
				translation = translation2 + translation;

				//
				// Rotation for relative angle from 0 deg reference model
				//
				Eigen::Matrix3f rotate_45deg;
				rotate_45deg <<
					0.996948, 0.019142, 0.075708,
					-0.0221982, 0.998964, 0.0397446,
					-0.0748677, -0.0413032, 0.996339;
				for (size_t j = 0; j < i_closest; j++) {
					rotation = rotate_45deg * rotation;
				}

				//
				// Print the rotation matrix and translation vector
				//
				Json::Value result;
				result["objectId"] = object.m_ObjectId;
				result["confidence"] = 0.9f;		// TODO
				for(int i=0;i<3;++i)
					result["transform"].append( translation(i) );

				for(int i=0;i<3;++i)
					for(int k=0;k<3;++k)
						result["rotation"].append( rotation(i,k) );
				a_pData->m_Results["objects"].append( result );
			}
		}
	}

	double elapsed = Time().GetEpochTime() - startTime;
	Log::Status( "PCLObjectRecognition", "Recognition completed in %.2f seconds, found %u objects", elapsed, a_pData->m_Results["objects"].size() );

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
		if ( pcl::io::loadPCDFile( staticData + m_Models[i], *pcd.m_Model ) < 0 )
		{
			Log::Error( "ObjectModel", "Failed to load %s", m_Models[i].c_str() );
			return false;
		}

		// compute normals
		pcl::NormalEstimationOMP<PointType, NormalType> norm_est;
		norm_est.setKSearch(10);
		norm_est.setInputCloud(pcd.m_Model);
		norm_est.compute(*pcd.m_Normals);

		// down sample clouds to extract keypoints
		pcl::VoxelGrid<PointType> uniform_sampling;
		uniform_sampling.setInputCloud(pcd.m_Model);
		uniform_sampling.setLeafSize(a_ModelSS, a_ModelSS, a_ModelSS);
		uniform_sampling.filter(*pcd.m_Keypoints);

		// compute descriptors for keypoints
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