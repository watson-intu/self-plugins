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


#ifndef POINT_GREY_CAMERA_H
#define POINT_GREY_CAMERA_H

#include "SelfInstance.h"
#include "utils/ThreadPool.h"
#include "utils/Time.h"
#undef GetBinaryType
#include "sensors/Camera.h"

namespace FlyCapture2 {
	class PGRGuid;
}

//! PointGrey implementation of the Camera class
class PointGreyCamera : public Camera
{
public:
	RTTI_DECL();

	PointGreyCamera() :
		m_Width(320),
		m_Height(240),
		m_StopThread(false),
		m_ThreadStopped(true),
		m_GUID(NULL)
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
	TimerPool::ITimer::SP	m_spWaitTimer;

	int						m_Width;
	int						m_Height;
	bool					m_StopThread;
	bool					m_ThreadStopped;
	FlyCapture2::PGRGuid *	m_GUID;

	void 					StreamingThread(void * args);
	void					SendingData(VideoData * a_pData);
};

#endif // OPENCV_CAMERA_H
