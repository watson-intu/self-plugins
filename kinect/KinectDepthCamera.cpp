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

#define WRITE_DEPTH_IMAGE			1

#include <windows.h>
#include <NuiApi.h>
#pragma comment( lib, "Kinect10.lib" )

#include "KinectDepthCamera.h"
#include "SelfInstance.h"
#include "utils/JpegHelpers.h"

#include "opencv2/opencv.hpp"

REG_SERIALIZABLE(KinectDepthCamera);
REG_OVERRIDE_SERIALIZABLE(DepthCamera, KinectDepthCamera);
RTTI_IMPL(KinectDepthCamera, DepthCamera);

KinectDepthCamera::KinectDepthCamera() :
	m_Width(640),
	m_Height(480),
	m_bNearMode(false),
	m_pSensor(NULL),
	m_bProcessing(false),
	m_hDepthStream(NULL),
	m_hDepthStreamEvent(NULL)
{}

void KinectDepthCamera::Serialize(Json::Value & json)
{
	DepthCamera::Serialize(json);

	json["m_Width"] = m_Width;
	json["m_Height"] = m_Height;
	json["m_bNearMode"] = m_bNearMode;
}

void KinectDepthCamera::Deserialize(const Json::Value & json)
{
	DepthCamera::Deserialize(json);

	if (json["m_Width"].isInt())
		m_Width = json["m_Width"].asInt();
	if (json["m_Height"].isInt())
		m_Height = json["m_Height"].asInt();
	if (json["m_bNearMode"].isBool())
		m_bNearMode = json["m_bNearMode"].asBool();
}

bool KinectDepthCamera::OnStart()
{
	if (m_Paused == 0)
	{
		int iSensorCount = 0;

		HRESULT hr = NuiGetSensorCount(&iSensorCount);
		if (!FAILED(hr))
		{
			// Look at each Kinect sensor
			for (int i = 0; i < iSensorCount; ++i)
			{
				// Create the sensor so we can check status, if we can't create it, move on to the next
				INuiSensor * pNuiSensor = NULL;
				hr = NuiCreateSensorByIndex(i, &pNuiSensor);
				if (FAILED(hr))
					continue;

				// Get the status of the sensor, and if connected, then we can initialize it
				if (S_OK == pNuiSensor->NuiStatus())
				{
					hr = pNuiSensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH);
					if (!FAILED(hr))
					{
						m_hDepthStreamEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
						hr = pNuiSensor->NuiImageStreamOpen(
							NUI_IMAGE_TYPE_DEPTH,
							NUI_IMAGE_RESOLUTION_640x480,
							0,
							2,
							m_hDepthStreamEvent,
							&m_hDepthStream);

						if (!FAILED(hr))
						{
							pNuiSensor->NuiImageStreamSetImageFrameFlags(m_hDepthStream, m_bNearMode ? NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE : 0);
							m_pSensor = pNuiSensor;
							break;
						}
					}
				}

				// This sensor wasn't OK, so release it since we're not using it
				pNuiSensor->Release();
			}
		}

		if (m_pSensor != NULL)
		{
			m_spWaitTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(KinectDepthCamera, OnCaptureData, this), (1.0f / m_fFramesPerSec), false, true);
			Log::Status("KinectCamera", "Camera has started");
		}
		else
			Log::Warning("kinectCamera", "Failed to open camera");
	}

	return true;
}

bool KinectDepthCamera::OnStop()
{
	m_spWaitTimer.reset();

	if (m_pSensor != NULL)
	{
		m_pSensor->NuiShutdown();
		m_pSensor->Release();
		m_pSensor = NULL;
	}

	if (m_hDepthStreamEvent != NULL)
	{
		CloseHandle(m_hDepthStreamEvent);
		m_hDepthStreamEvent = NULL;
	}

	Log::Debug("KinectCamera", "Camera has stopped...");
	return true;
}


void KinectDepthCamera::OnCaptureData()
{
	if (WAIT_OBJECT_0 == WaitForSingleObject(m_hDepthStreamEvent, 0))
	{
		// Attempt to get the depth frame
		NUI_IMAGE_FRAME imageFrame;

		HRESULT hr = m_pSensor->NuiImageStreamGetNextFrame(m_hDepthStream, 0, &imageFrame);
		if (!FAILED(hr))
		{
			BOOL nearMode;
			INuiFrameTexture * pTexture = NULL;

			hr = m_pSensor->NuiImageFrameGetDepthImagePixelFrameTexture(m_hDepthStream, &imageFrame, &nearMode, &pTexture);
			if (!FAILED(hr))
			{
				// Lock the frame data so the Kinect knows not to modify it while we're reading it
				NUI_LOCKED_RECT LockedRect;
				pTexture->LockRect(0, &LockedRect, NULL, 0);

				// Make sure we've received valid data
				if (LockedRect.Pitch != 0)
				{
					// Get the min and max reliable depth for the current frame
					int minDepth = (nearMode ? NUI_IMAGE_DEPTH_MINIMUM_NEAR_MODE : NUI_IMAGE_DEPTH_MINIMUM) >> NUI_IMAGE_PLAYER_INDEX_SHIFT;
					int maxDepth = (nearMode ? NUI_IMAGE_DEPTH_MAXIMUM_NEAR_MODE : NUI_IMAGE_DEPTH_MAXIMUM) >> NUI_IMAGE_PLAYER_INDEX_SHIFT;

					const NUI_DEPTH_IMAGE_PIXEL * pBufferRun = reinterpret_cast<const NUI_DEPTH_IMAGE_PIXEL *>(LockedRect.pBits);
					const NUI_DEPTH_IMAGE_PIXEL * pBufferEnd = reinterpret_cast<const NUI_DEPTH_IMAGE_PIXEL *>(LockedRect.pBits + LockedRect.size);

					byte * pRGB = new byte[m_Width * m_Height * 3];
					for (int x = 0; x < m_Width; ++x)
					{
						for (int y = 0; y < m_Height; ++y)
						{
							// sample the depth from the locked rect..
							unsigned int src = ((x * 640) / m_Width) + (((y * 480) / m_Height) * 640);
							unsigned short depth = pBufferRun[src].depth;

							unsigned int dst = (x + (y * m_Width)) * 3;
							pRGB[dst + 0] = (depth / 256) / 256;	// red represents 256 millimeters
							pRGB[dst + 1] = (depth / 256) % 256;	// green
							pRGB[dst + 2] = depth % 256;			// blue
						}
					}

					std::string jpeg;
					if (JpegHelpers::EncodeImage(pRGB, m_Width, m_Height, 3, jpeg))
					{
						ThreadPool::Instance()->InvokeOnMain<IData *>(
							DELEGATE(KinectDepthCamera, OnSendData, IData *, this), new DepthVideoData(jpeg));

#if WRITE_DEPTH_IMAGE
						FILE * fp = fopen("depth.jpg", "wb");
						if (fp != NULL)
						{
							fwrite(jpeg.data(), 1, jpeg.size(), fp);
							fclose(fp);
						}
#endif
					}

					delete[] pRGB;
				}

				// We're done with the texture so unlock it
				pTexture->UnlockRect(0);
				pTexture->Release();
			}

			m_pSensor->NuiImageStreamReleaseFrame(m_hDepthStream, &imageFrame);
		}
	}

	//cv::Mat frame;
	//if (m_VideoCapture != NULL && !m_bProcessing)
	//{
	//	m_bProcessing = true;
	//	if (m_VideoCapture->read(frame))
	//	{
	//		cv::Mat resized;
	//		if (m_Width > 0 && m_Height > 0)
	//			cv::resize(frame, resized, cv::Size(m_Width, m_Height));

	//		std::vector<unsigned char> jpeg;
	//		if (cv::imencode(".jpg", resized.empty() ? frame : resized, jpeg))
	//		{
	//			ThreadPool::Instance()->InvokeOnMain<IData *>( 
	//				DELEGATE( KinectCamera, OnSendData, IData *, this), new VideoData(jpeg) );
	//		}
	//		resized.release();
	//		frame.release();
	//	}
	//	m_bProcessing = false;
	//}
}

void KinectDepthCamera::OnSendData(IData * a_pData)
{
	SendData(a_pData);
}

void KinectDepthCamera::OnPause()
{
	if (m_Paused == 0)
	{
		OnStop();
	}
	++m_Paused;
}

void KinectDepthCamera::OnResume()
{
	--m_Paused;
	if (m_Paused == 0)
	{
		OnStart();
	}
}


