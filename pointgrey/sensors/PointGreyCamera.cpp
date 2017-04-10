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

#include "PointGreyCamera.h"
#include "utils/JpegHelpers.h"
#include "SelfInstance.h"

#include "FlyCapture2.h"

#undef GetBinaryType

REG_SERIALIZABLE(PointGreyCamera);
REG_OVERRIDE_SERIALIZABLE(Camera, PointGreyCamera);

RTTI_IMPL(PointGreyCamera, Camera);


void PointGreyCamera::Serialize(Json::Value & json)
{
	Camera::Serialize(json);

	json["m_Width"] = m_Width;
	json["m_Height"] = m_Height;
}

void PointGreyCamera::Deserialize(const Json::Value & json)
{
	Camera::Deserialize(json);

	if (json["m_Width"].isInt())
		m_Width = json["m_Width"].asInt();
	if (json["m_Height"].isInt())
		m_Height = json["m_Height"].asInt();
}

bool PointGreyCamera::OnStart()
{
	FlyCapture2::BusManager busMgr;
	unsigned int numCamera;
	FlyCapture2::Error bError = busMgr.GetNumOfCameras(&numCamera);
	if (bError != FlyCapture2::ErrorType::PGRERROR_OK)
	{
		Log::Error("PointGreyCamera", "Failed to find cameras!");
		return false;
	}
	
	for (unsigned int i = 0; i < numCamera; ++i)
	{		
		FlyCapture2::PGRGuid * guid = new FlyCapture2::PGRGuid();
		bError = busMgr.GetCameraFromIndex(i, guid);
		if (bError != FlyCapture2::ErrorType::PGRERROR_OK)
		{
			Log::Error("PointGreyCamera", "Failed to open camera!");
			return false;
		}
		m_GUID = guid;
		ThreadPool::Instance()->InvokeOnThread<void *>(DELEGATE(PointGreyCamera, StreamingThread, void *, this), NULL);
		break;
	}

	Log::Debug("PointGreyCamera", "Point Grey Camera has started");
	return true;
}

bool PointGreyCamera::OnStop()
{
	m_StopThread = true;
	while (!m_ThreadStopped)
		tthread::this_thread::yield();
	Log::Debug("PointGreyCamera", "Point Grey Camera has stopped!");
	delete m_GUID;
	m_GUID = NULL;
	return true;
}

void PointGreyCamera::StreamingThread(void * args)
{
	m_ThreadStopped = false;
	FlyCapture2::Camera cam;
	FlyCapture2::Error bError;
	bError = cam.Connect(m_GUID);
	if (bError != FlyCapture2::ErrorType::PGRERROR_OK)
	{
		Log::Error("PointGreyCamera", "Failed to connect to Camera!");
		m_ThreadStopped = true;
		return;
	}

	FlyCapture2::CameraInfo camInfo;
	bError = cam.GetCameraInfo(&camInfo);
	if (bError != FlyCapture2::ErrorType::PGRERROR_OK)
	{
		Log::Error("PointGreyCamera", "Failed to grab camera info");
		m_ThreadStopped = true;
		return;
	}

	FlyCapture2::FC2Config config;
	bError = cam.GetConfiguration(&config);
	if (bError != FlyCapture2::ErrorType::PGRERROR_OK)
	{
		Log::Error("PointGreyCamera", "Failed to grab camera configuration");
		m_ThreadStopped = true;
		return;
	}

	bError = cam.SetConfiguration(&config);
	if (bError != FlyCapture2::ErrorType::PGRERROR_OK)
	{
		Log::Error("PointGreyCamera", "Failed to set the camera configuration");
		m_ThreadStopped = true;
		return;
	}

	bError = cam.StartCapture();
	if (bError != FlyCapture2::ErrorType::PGRERROR_OK)
	{
		Log::Error("PointGreyCamera", "Failed to start the camera");
		m_ThreadStopped = true;
		return;
	}

	while (!m_StopThread)
	{
		if (m_Paused <= 0)
		{
			FlyCapture2::Image rawImage;
			bError = cam.RetrieveBuffer(&rawImage);
			if (bError != FlyCapture2::ErrorType::PGRERROR_OK)
			{
				Log::Error("PointGreyCamera", "Failed to retrieve buffer");
				break;
			}

			FlyCapture2::Image convertedImage;
			bError = rawImage.Convert(FlyCapture2::PixelFormat::PIXEL_FORMAT_RGB, &convertedImage);
			if (bError != FlyCapture2::ErrorType::PGRERROR_OK)
			{
				Log::Error("PointGreyCamera", "Failed to convert image");
				m_ThreadStopped = true;
				return;
			}
			std::string encodedImage;
			JpegHelpers::EncodeImage(convertedImage.GetData(), convertedImage.GetCols(), convertedImage.GetRows(), 3, encodedImage);
			ThreadPool::Instance()->InvokeOnMain<VideoData *>(
				DELEGATE(PointGreyCamera, SendingData, VideoData *, this), new VideoData((const unsigned char *)encodedImage.data(), (int)encodedImage.size()));
			tthread::this_thread::sleep_for(tthread::chrono::milliseconds((unsigned int)(1000 / m_fFramesPerSec)));
		}
	}
	bError = cam.StopCapture();
	if (bError != FlyCapture2::ErrorType::PGRERROR_OK)
	{
		Log::Error("PointGreyCamera", "Failed to stop capturing from camera");
		m_ThreadStopped = true;
		return;
	}

	bError = cam.Disconnect();
	if (bError != FlyCapture2::ErrorType::PGRERROR_OK)
	{
		Log::Error("PointGreyCamera", "Failed to disconnect from camera!");
		m_ThreadStopped = true;
		return;
	}

	m_ThreadStopped = true;
}

void PointGreyCamera::SendingData(VideoData * a_pData)
{
	SendData(a_pData);
}

void PointGreyCamera::OnPause()
{
	++m_Paused;
}

void PointGreyCamera::OnResume()
{
	--m_Paused;
}

