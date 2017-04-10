//
// Created by John Andersen on 6/23/16.
//

#include "RemoteCamera.h"
#include "SelfInstance.h"

REG_SERIALIZABLE(RemoteCamera);
RTTI_IMPL(RemoteCamera, ISensor);

void RemoteCamera::Serialize(Json::Value & json)
{
	Camera::Serialize(json);

	json["m_ServiceId"] = m_ServiceId;
	json["m_Width"] = m_Width;
	json["m_Height"] = m_Height;
}

void RemoteCamera::Deserialize(const Json::Value & json)
{
	Camera::Deserialize(json);

	if (json["m_ServiceId"].isInt())
		m_ServiceId = json["m_ServiceId"].asString();
	if (json["m_Width"].isInt())
		m_Width = json["m_Width"].asInt();
	if (json["m_Height"].isInt())
		m_Height = json["m_Height"].asInt();
}

bool RemoteCamera::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance != NULL )
	{
		ITopics * pTopics = pInstance->GetTopics();
		pTopics->RegisterTopic( "video-input", "image/jpeg" );
		pTopics->Subscribe( "video-input", DELEGATE( RemoteCamera, OnRemoteVideo, const ITopics::Payload &, this ) );

		m_pCameraService = pInstance->FindService<PTZCamera>( m_ServiceId );
		if (m_pCameraService != NULL)
		{
			ThreadPool::Instance()->InvokeOnThread<void *>(DELEGATE(RemoteCamera, StreamingThread, void *, this), NULL);
		}
	}

    return true;
}

void RemoteCamera::StreamingThread(void * args)
{
	m_ThreadStopped = false;

	while (!m_StopThread)
	{
		if (m_Paused <= 0)
		{
			m_pCameraService->GetImage(DELEGATE(RemoteCamera, OnGetImage, const std::string &, this));
			tthread::this_thread::sleep_for(tthread::chrono::milliseconds((int)(1000 / m_fFramesPerSec)));
		}
	}

	m_ThreadStopped = true;
}

bool RemoteCamera::OnStop()
{
	m_StopThread = true;
	while (!m_ThreadStopped)
		tthread::this_thread::yield();
	return true;
}

void RemoteCamera::OnPause()
{
    m_Paused++;
}

void RemoteCamera::OnResume()
{
    m_Paused--;
}

void RemoteCamera::OnGetImage( const std::string & a_Image )
{
	if (a_Image.size() > 0)
	{
		ThreadPool::Instance()->InvokeOnMain<VideoData *>(DELEGATE(RemoteCamera, OnSendData, VideoData *, this),
			new VideoData((const unsigned char *)a_Image.c_str(), a_Image.size()));
	}
	else
	{
		Log::Error("RemoteCamera", "Could not connect to Remote Camera - exiting camera...");
		m_StopThread = true;
	}
}

void RemoteCamera::OnRemoteVideo( const ITopics::Payload & a_Payload )
{
	OnSendData( new VideoData( (unsigned char *)a_Payload.m_Data.c_str(), a_Payload.m_Data.size() ) );
}

void RemoteCamera::OnSendData( VideoData * a_pData )
{
	SendData( a_pData );
}

