//
// Created by John Andersen on 6/23/16.
//

#ifndef SELF_REMOTECAMERA_H
#define SELF_REMOTECAMERA_H

#include "sensors/Camera.h"
#include "utils/TimerPool.h"
#include "services/PTZCamera.h"
#include "sensors/VideoData.h"

class RemoteCamera : public Camera
{
public:
    RTTI_DECL();

    //! Construction
    RemoteCamera() : m_pCameraService( NULL ), m_Width( 320 ), m_Height( 240 ), m_StopThread(false), m_ThreadStopped(true)
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
    //!Data
	std::string				m_ServiceId;
	int						m_Width;
	int						m_Height;
	bool					m_StopThread;
	bool					m_ThreadStopped;
	PTZCamera *				m_pCameraService;

	void					StreamingThread(void * args);

	//! Callbacks
	void                    OnGetImage(const std::string & a_Image);
	void					OnRemoteVideo( const ITopics::Payload & a_Payload );
	void					OnSendData( VideoData * a_pData );
};

#endif //SELF_REMOTECAMERA_H
