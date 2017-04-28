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


#ifndef KINECT_CAMERA_H
#define KINECT_CAMERA_H

#include "utils/TimerPool.h"
#include "sensors/Camera.h"

struct INuiSensor;

//! OpenCV implementation of the Camera class
class KinectCamera : public Camera
{
public:
	RTTI_DECL();

	//! Construction
	KinectCamera();

	//! ISerializable interface
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! ISensor interface
	virtual bool OnStart();
	virtual bool OnStop();
	virtual void OnPause();
	virtual void OnResume();

	static INuiSensor *		GrabKinect();
	static void				FreeKinect(INuiSensor * a_pSensor);

private:
	//! Data
	INuiSensor *			m_pSensor;
	volatile bool			m_bProcessing;
	TimerPool::ITimer::SP	m_spWaitTimer;
	HANDLE					m_hImageStream;
	HANDLE					m_hImageStreamEvent;

	int						m_Width;
	int						m_Height;

	void 					OnCaptureData();
	void					OnSendData( IData * a_pData );

	static INuiSensor *		sm_pSharedSensor;
	static unsigned int		sm_nSharedSensorCount;
};

#endif // KINECT_CAMERA_H
