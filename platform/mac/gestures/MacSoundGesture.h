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

#ifndef SELF_MACSPEECHGESTURE_H
#define SELF_MACSPEECHGESTURE_H

#include "gestures/SoundGesture.h"
#include "utils/Sound.h"

//! This gesture wraps DirectSound so the local windows box can speak.
class MacSoundGesture : public SoundGesture
{
public:
    RTTI_DECL();

    //! Construction
    MacSoundGesture()
    {}

    //! IGesture interface
    virtual bool Start();
    virtual bool CanExecute( const ParamsMap & a_Params );
    virtual bool Execute( GestureDelegate a_Callback, const ParamsMap & a_Params );
    virtual bool Abort();

private:
    void Play();
    void PlayThread();
    void OnSoundDone();
};


#endif //SELF_MACSPEECHGESTURE_H
