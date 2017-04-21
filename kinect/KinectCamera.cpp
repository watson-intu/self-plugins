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

#define _CRT_SECURE_NO_WARNINGS
#define WRITE_DEPTH_IMAGE			1

#include <windows.h>
#include <NuiApi.h>
#pragma comment( lib, "Kinect10.lib" )

#include "KinectCamera.h"
#include "SelfInstance.h"
#include "utils/JpegHelpers.h"

REG_SERIALIZABLE(KinectCamera);
REG_OVERRIDE_SERIALIZABLE(Camera, KinectCamera);
RTTI_IMPL(KinectCamera, Camera);

KinectCamera::KinectCamera() :
	m_Width(640),
	m_Height(480),
	m_pSensor(NULL),
	m_bProcessing(false),
	m_hImageStream(NULL),
	m_hImageStreamEvent(NULL)
{}

void KinectCamera::Serialize(Json::Value & json)
{
	Camera::Serialize(json);

	json["m_Width"] = m_Width;
	json["m_Height"] = m_Height;
}

void KinectCamera::Deserialize(const Json::Value & json)
{
	Camera::Deserialize(json);

	if (json["m_Width"].isInt())
		m_Width = json["m_Width"].asInt();
	if (json["m_Height"].isInt())
		m_Height = json["m_Height"].asInt();
}

bool KinectCamera::OnStart()
{
	if (m_Paused == 0)
	{
		m_pSensor = GrabKinect();
		if (m_pSensor != NULL)
		{
			m_hImageStreamEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			HRESULT hr = m_pSensor->NuiImageStreamOpen(
				NUI_IMAGE_TYPE_COLOR,
				NUI_IMAGE_RESOLUTION_640x480,
				0,
				2,
				m_hImageStreamEvent,
				&m_hImageStream);
			if (! FAILED(hr) )
			{
				m_spWaitTimer = TimerPool::Instance()->StartTimer(VOID_DELEGATE(KinectCamera, OnCaptureData, this), (1.0f / m_fFramesPerSec), false, true);
				Log::Status("KinectCamera", "Camera has started");
			}
			else
			{
				FreeKinect( m_pSensor );
				m_pSensor = NULL;
			}
		}

		if ( m_pSensor == NULL )
			Log::Warning("kinectCamera", "Failed to open camera");
	}

	return true;
}

bool KinectCamera::OnStop()
{
	m_spWaitTimer.reset();

	if (m_hImageStreamEvent != NULL)
	{
		CloseHandle(m_hImageStreamEvent);
		m_hImageStreamEvent = NULL;
	}

	if (m_pSensor != NULL)
	{
		FreeKinect( m_pSensor );
		m_pSensor = NULL;
	}

	Log::Debug("KinectCamera", "Camera has stopped...");
	return true;
}


void KinectCamera::OnCaptureData()
{
	if (WAIT_OBJECT_0 == WaitForSingleObject(m_hImageStreamEvent, 0))
	{
		// Attempt to get the depth frame
		NUI_IMAGE_FRAME imageFrame;

		HRESULT hr = m_pSensor->NuiImageStreamGetNextFrame(m_hImageStream, 0, &imageFrame);
		if (!FAILED(hr))
		{
			// Lock the frame data so the Kinect knows not to modify it while we're reading it
			NUI_LOCKED_RECT LockedRect;
			imageFrame.pFrameTexture->LockRect(0, &LockedRect, NULL, 0);

			// Make sure we've received valid data
			if (LockedRect.Pitch != 0)
			{
				unsigned int * pRGB = new unsigned int[ m_Width * m_Height ];
				for (int x = 0; x < m_Width; ++x)
				{
					for (int y = 0; y < m_Height; ++y)
					{
						// sample the depth from the locked rect..
						unsigned int src = ((x * 640) / m_Width) + (((y * 480) / m_Height) * 640);
						unsigned int dst = x + (y * m_Width);
						pRGB[dst] = ((unsigned int *)LockedRect.pBits)[ src ];
					}
				}

				std::string jpeg;
				if ( JpegHelpers::EncodeImage( pRGB, m_Width, m_Height, 4, jpeg ) )
				{
					ThreadPool::Instance()->InvokeOnMain<IData *>(
						DELEGATE(KinectCamera, OnSendData, IData *, this), new VideoData(jpeg));
				}

				delete [] pRGB;
			}

			// We're done with the texture so unlock it
			imageFrame.pFrameTexture->UnlockRect(0);

			m_pSensor->NuiImageStreamReleaseFrame(m_hImageStream, &imageFrame);
		}
	}
}

void KinectCamera::OnSendData(IData * a_pData)
{
	SendData(a_pData);
}

void KinectCamera::OnPause()
{
	if (m_Paused == 0)
	{
		OnStop();
	}
	++m_Paused;
}

void KinectCamera::OnResume()
{
	--m_Paused;
	if (m_Paused == 0)
	{
		OnStart();
	}
}


INuiSensor * KinectCamera::sm_pSharedSensor = NULL;
unsigned int KinectCamera::sm_nSharedSensorCount = 0;

INuiSensor * KinectCamera::GrabKinect()
{
	if ( sm_pSharedSensor == NULL )
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
					hr = pNuiSensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH );
					if (!FAILED(hr))
					{
						sm_pSharedSensor = pNuiSensor;
						break;
					}
				}
				// This sensor wasn't OK, so release it since we're not using it
				pNuiSensor->Release();
			}
		}

	}

	sm_nSharedSensorCount += 1;
	return sm_pSharedSensor;
}

void KinectCamera::FreeKinect(INuiSensor * a_pSensor)
{
	if ( sm_pSharedSensor == a_pSensor )
	{
		sm_nSharedSensorCount -= 1;
		if ( sm_nSharedSensorCount == 0 )
		{
			sm_pSharedSensor->NuiShutdown();
			sm_pSharedSensor->Release();
			sm_pSharedSensor = NULL;
		}
	}
}