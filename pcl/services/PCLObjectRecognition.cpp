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

#define WRITE_OBJECTS_PCD			0

#include "PCLObjectRecognition.h"

#include "opencv2/opencv.hpp"
#include "pcl/point_cloud.h"
#include "pcl/point_types.h"
#include "pcl/io/pcd_io.h"

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
	const std::string & data = a_pData->m_DepthData;
	std::vector<unsigned char> encoded( (unsigned char *)data.data(), (unsigned char *)data.data() + data.size() );
	cv::Mat decoded = cv::imdecode( encoded, CV_LOAD_IMAGE_ANYDEPTH );
	
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud( new pcl::PointCloud<pcl::PointXYZ> );
	cloud->height = decoded.rows;
	cloud->width = decoded.cols;
	cloud->is_dense = false;
	cloud->points.resize( cloud->width * cloud->height );

	const float constant = 1.0f / 525;
	const int centerX = cloud->width >> 1;
	const int centerY = cloud->height >> 1;
	register int depth_idx = 0;
	for(int v=-centerY;v<centerY;++v)
	{
		for(register int u=-centerX;u<centerX;++u,++depth_idx)
		{
			pcl::PointXYZ & pt = cloud->points[depth_idx];
			pt.z = decoded.at<unsigned short>( depth_idx ) * 0.001f;
			pt.x = static_cast<float>(u) * pt.z * constant;
			pt.y = static_cast<float>(v) * pt.z * constant;
		}
	}
	cloud->sensor_origin_.setZero();
	cloud->sensor_orientation_.w() = 0.0f;
	cloud->sensor_orientation_.x() = 1.0f;
	cloud->sensor_orientation_.y() = 0.0f;
	cloud->sensor_orientation_.z() = 0.0f;

#if WRITE_OBJECTS_PCD
	// save to a local file 
	pcl::io::savePCDFile( "objects.pcd", *cloud );
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
