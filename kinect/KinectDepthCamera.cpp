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
#define WRITE_DEPTH_IMAGE			0

#include <windows.h>
#include <NuiApi.h>
#pragma comment( lib, "Kinect10.lib" )

#include "KinectDepthCamera.h"
#include "KinectCamera.h"
#include "SelfInstance.h"

#include "opencv2/opencv.hpp"

REG_SERIALIZABLE(KinectDepthCamera);
REG_OVERRIDE_SERIALIZABLE(DepthCamera, KinectDepthCamera);
RTTI_IMPL(KinectDepthCamera, DepthCamera);

KinectDepthCamera::KinectDepthCamera() :
	m_Width(640),
	m_Height(480),
	m_bNearMode(true),
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
		m_pSensor = KinectCamera::GrabKinect();
		if ( m_pSensor != NULL )
		{
			m_hDepthStreamEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			HRESULT hr = m_pSensor->NuiImageStreamOpen(
				NUI_IMAGE_TYPE_DEPTH,
				NUI_IMAGE_RESOLUTION_640x480,
				0,
				2,
				m_hDepthStreamEvent,
				&m_hDepthStream);

			if (!FAILED(hr))
			{
				m_pSensor->NuiImageStreamSetImageFrameFlags(m_hDepthStream, m_bNearMode ? NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE : 0);
				m_spWaitTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(KinectDepthCamera, OnCaptureData, this), (1.0f / m_fFramesPerSec), false, true);
				Log::Status("KinectCamera", "Camera has started");
			}
			else
			{
				KinectCamera::FreeKinect( m_pSensor );
				m_pSensor = NULL;
			}
		}

		if (m_pSensor == NULL)
			Log::Warning("kinectCamera", "Failed to open camera");
	}

	return true;
}

bool KinectDepthCamera::OnStop()
{
	m_spWaitTimer.reset();
	while( m_bProcessing )
		boost::this_thread::yield();

	if (m_hDepthStreamEvent != NULL)
	{
		CloseHandle(m_hDepthStreamEvent);
		m_hDepthStreamEvent = NULL;
		m_hDepthStream = NULL;
	}

	if (m_pSensor != NULL)
	{
		KinectCamera::FreeKinect(m_pSensor);
		m_pSensor = NULL;
	}

	Log::Debug("KinectCamera", "Camera has stopped...");
	return true;
}


void KinectDepthCamera::OnCaptureData()
{
	if (! m_bProcessing )
	{
		m_bProcessing = true;
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

						// extract the depth from the data, build a buffer of 16-bit depth values in MM, which we will encode into png below..
						cv::Mat encode( cv::Size(m_Width, m_Height), CV_16UC1);
						for (int x = 0; x < m_Width; ++x)
						{
							for (int y = 0; y < m_Height; ++y)
							{
								// sample the depth from the locked rect..
								unsigned int src = ((x * 640) / m_Width) + (((y * 480) / m_Height) * 640);
								unsigned short depth = pBufferRun[src].depth;
								encode.at<unsigned short>(cv::Point(x,y)) = pBufferRun[src].depth;
							}
						}

						std::vector<unsigned char> encoded;
						if ( cv::imencode(".png", encode, encoded) )
						{
							ThreadPool::Instance()->InvokeOnMain<IData *>(
								DELEGATE(KinectDepthCamera, OnSendData, IData *, this), new DepthVideoData(encoded));

	#if WRITE_DEPTH_IMAGE
							FILE * fp = fopen("depth.png", "wb");
							if (fp != NULL)
							{
								fwrite(encoded.data(), 1, encoded.size(), fp);
								fclose(fp);
							}
	#endif
						}
					}

					// We're done with the texture so unlock it
					pTexture->UnlockRect(0);
					pTexture->Release();
				}

				m_pSensor->NuiImageStreamReleaseFrame(m_hDepthStream, &imageFrame);
			}
		}
		m_bProcessing = false;
	}
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


