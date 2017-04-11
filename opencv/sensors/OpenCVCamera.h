/**
* Copyright 2016 IBM Corp. All Rights Reserved.
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

#ifndef OPENCV_CAMERA_H
#define OPENCV_CAMERA_H

#include "SelfInstance.h"
#include "utils/ThreadPool.h"
#include "utils/Time.h"
#include "sensors/Camera.h"

#include "opencv2/opencv.hpp"

//! OpenCV implementation of the Camera class
class OpenCVCamera : public Camera
{
public:
	RTTI_DECL();

	OpenCVCamera() :
		m_CameraDevice(0),
		m_Width(320),
		m_Height(240),
		m_VideoCapture(NULL),
		m_bProcessing(false)
	{}

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! ISensor interface
	virtual bool OnStart();
	virtual bool OnStop();
	virtual void OnPause();
	virtual void OnResume();

private:
	//! Types
	typedef std::list<VideoData *> DataQueue;

	//! Data
	cv::VideoCapture *		m_VideoCapture;
	volatile bool			m_bProcessing;
	TimerPool::ITimer::SP	m_spWaitTimer;

	std::string				m_CameraStream;
	int						m_CameraDevice;
	int						m_Width;
	int						m_Height;

	void 					OnCaptureImage();
	void					OnSendData( IData * a_pData );
};

#endif // OPENCV_CAMERA_H
