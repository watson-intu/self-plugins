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

#include "OpenCVCamera.h"
#include "SelfInstance.h"

REG_OVERRIDE_SERIALIZABLE(Camera, OpenCVCamera);

REG_SERIALIZABLE(OpenCVCamera);
RTTI_IMPL(OpenCVCamera, Camera);


void OpenCVCamera::Serialize(Json::Value & json)
{
	Camera::Serialize(json);

	json["m_CameraStream"] = m_CameraStream;
	json["m_CameraDevice"] = m_CameraDevice;
	json["m_Width"] = m_Width;
	json["m_Height"] = m_Height;
}

void OpenCVCamera::Deserialize(const Json::Value & json)
{
	Camera::Deserialize(json);

	if (json["m_CameraStream"].isString())
		m_CameraStream = json["m_CameraStream"].asString();
	if (json["m_CameraDevice"].isInt())
		m_CameraDevice = json["m_CameraDevice"].asInt();
	if (json["m_Width"].isInt())
		m_Width = json["m_Width"].asInt();
	if (json["m_Height"].isInt())
		m_Height = json["m_Height"].asInt();
}

bool OpenCVCamera::OnStart()
{
	if (m_Paused == 0)
	{
		if (m_CameraStream.size() > 0)
			m_VideoCapture = new cv::VideoCapture(m_CameraStream);
		else
			m_VideoCapture = new cv::VideoCapture(m_CameraDevice);

		m_spWaitTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(OpenCVCamera, OnCaptureImage, this), (1.0f / m_fFramesPerSec), false, true);
		Log::Debug("OpenCVCamera", "Open CV Camera has started");
	}

	return true;
}

bool OpenCVCamera::OnStop()
{
	m_spWaitTimer.reset();
	delete m_VideoCapture;
	m_VideoCapture = NULL;
	Log::Debug("OpenCVCamera", "Open CV Camera has stopped...");
	return true;
}


void OpenCVCamera::OnCaptureImage()
{
	cv::Mat frame;
	if (m_VideoCapture != NULL && !m_bProcessing)
	{
		m_bProcessing = true;
		if (m_VideoCapture->read(frame))
		{
			cv::Mat resized;
			if (m_Width > 0 && m_Height > 0)
				cv::resize(frame, resized, cv::Size(m_Width, m_Height));

			std::vector<unsigned char> jpeg;
			if (cv::imencode(".jpg", resized.empty() ? frame : resized, jpeg))
			{
				ThreadPool::Instance()->InvokeOnMain<IData *>( 
					DELEGATE( OpenCVCamera, OnSendData, IData *, this), new VideoData(jpeg) );
			}
			resized.release();
			frame.release();
		}
		m_bProcessing = false;
	}
}

void OpenCVCamera::OnSendData( IData * a_pData )
{
	SendData( a_pData );
}

void OpenCVCamera::OnPause()
{
	if(m_Paused == 0)
	{
		OnStop();
	}
	++m_Paused;
}

void OpenCVCamera::OnResume()
{
	--m_Paused;
	if(m_Paused == 0)
	{
		OnStart();
	}
}


