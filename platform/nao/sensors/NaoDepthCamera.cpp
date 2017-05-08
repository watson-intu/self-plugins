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


#include "NaoDepthCamera.h"
#include "SelfInstance.h"

#ifndef _WIN32
#include <qi/os.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <alvision/alvisiondefinitions.h>
#include <alvision/alimage.h>
#include <alproxies/alvideodeviceproxy.h>
#endif

#include "tinythread++/tinythread.h"

REG_OVERRIDE_SERIALIZABLE(DepthCamera, NaoDepthCamera);

#ifndef _WIN32
REG_SERIALIZABLE(NaoDepthCamera);
#endif
RTTI_IMPL(NaoDepthCamera, DepthCamera);

void NaoDepthCamera::Serialize(Json::Value & json)
{
	DepthCamera::Serialize(json);

	json["m_Width"] = m_Width;
	json["m_Height"] = m_Height;
}

void NaoDepthCamera::Deserialize(const Json::Value & json)
{
	DepthCamera::Deserialize(json);

	if (json["m_Width"].isInt())
		m_Width = json["m_Width"].asInt();
	if (json["m_Height"].isInt())
		m_Height = json["m_Height"].asInt();
}

bool NaoDepthCamera::OnStart()
{
    Log::Debug("NaoDepthCamera", "Starting up video device");

    m_StopThread = false;
    ThreadPool::Instance()->InvokeOnThread<void *>( DELEGATE(NaoDepthCamera, StreamingThread, void *, this ), NULL );
    return true;
}

bool NaoDepthCamera::OnStop()
{
    m_StopThread = true;
    while(! m_ThreadStopped )
        tthread::this_thread::yield();
    return true;
}

void NaoDepthCamera::StreamingThread(void * arg)
{
    try
    {
        DoStreamingThread(arg);
    }
    catch( const std::exception & ex )
    {
        Log::Error( "NaoDepthCamera", "Caught Exception: %s", ex.what() );
    }
    m_ThreadStopped = true;
}

void NaoDepthCamera::DoStreamingThread(void *arg)
{
#ifndef _WIN32
	std::string robotIp("127.0.0.1");
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance != NULL )
		robotIp = URL(pInstance->GetLocalConfig().m_RobotUrl).GetHost();

    AL::ALVideoDeviceProxy  camProxy(robotIp, 9559);
    m_ClientName = camProxy.subscribeCamera(m_ClientName, 2, 1/*AL::kQVGA*/, 17, 15);

    AL::ALValue lImage;
    lImage.arraySetSize(7);

    while(!m_StopThread)
    {
        AL::ALValue img = camProxy.getImageRemote(m_ClientName);
        if(img.getSize() != 12) 
		{
            Log::Error("NaoDepthCamera", "Image Size: %d", img.getSize());
			boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
			continue;
        }

		int width = (int)img[0];
		int height = (int)img[1];
		int depth = (int)img[2];

		if ( depth != 2 )
		{
			Log::Error( "NaoDepthCamera", "depth != 2");
			continue;
		}

		cv::Mat imgHeader = cv::Mat(cv::Size(width, height), CV_16UC1);
		imgHeader.data = (uchar*)img[6].GetBinary();

		if ( imgHeader.data == NULL )
		{
			Log::Error("NaoDepthCamera", "Failed to grab remote image.");
			boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
			continue;
		}

		Log::Debug( "NaoDepthCamera", "Grabbed image %d x %d x %d", width, height, depth );

        std::vector<unsigned char> outputVector;
        if ( cv::imencode(".png", imgHeader, outputVector) && m_Paused <= 0 )
            ThreadPool::Instance()->InvokeOnMain<DepthVideoData *>( DELEGATE( NaoDepthCamera, SendingData, DepthVideoData *, this ), new DepthVideoData(outputVector));
        else
            Log::Error( "NaoDepthCamera", "Failed to imencode()" );

        camProxy.releaseImage(m_ClientName);

        boost::this_thread:sleep(boost::posix_time::milliseconds(1000 / m_fFramesPerSec));
    }

    Log::Debug("NaoDepthCamera", "Closing Video feed with m_ClientName: %s", m_ClientName.c_str());
    camProxy.unsubscribe(m_ClientName);
    Log::Status("NaoDepthCamera", "Stopped video device");
#endif
}

void NaoDepthCamera::SendingData( DepthVideoData * a_pData )
{
    SendData( a_pData );
}

void NaoDepthCamera::OnPause()
{
    m_Paused++;
}

void NaoDepthCamera::OnResume()
{
    m_Paused--;
}


