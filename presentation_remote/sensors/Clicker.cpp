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

#include "Clicker.h"

#if defined(_WIN32)
#include <conio.h>
#define WINVER 0x0500
#include <windows.h>
#include <iostream>
#endif
#include <stdio.h>

RTTI_IMPL(ClickerData, IData);
REG_SERIALIZABLE( ClickerData );

REG_SERIALIZABLE(Clicker);
RTTI_IMPL(Clicker, ISensor);

bool Clicker::OnStart()
{
#if defined(__APPLE__)
    system("stty raw");
#endif
    ThreadPool::Instance()->InvokeOnThread<void *>(DELEGATE(Clicker, StreamingThread, void *, this), NULL);

    return true;
}

bool Clicker::OnStop()
{
    m_StopThread = true;
#if defined(__APPLE__)
    system("stty cooked");
#elif defined(_WIN32)
    INPUT input[2] = {0};
    input[0].type = INPUT_KEYBOARD;
    // Press the 'a' key
    input[0].ki.wVk = 0x41;
    input[0].ki.dwFlags = 0;
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = VK_UP;
    input[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(_countof(input), input, sizeof(INPUT));
#endif
    while (!m_ThreadStopped)
        tthread::this_thread::yield();
    Log::Debug("Clicker", "Clicker has stopped!");
    return true;
}

void Clicker::StreamingThread(void * args)
{
    m_ThreadStopped = false;
    while (!m_StopThread)
    {
        char input;
#if defined(_WIN32)
        input = _getch();
#elif defined(__APPLE__)
        input = getchar();
#endif
        std::string str(1, input);
        ThreadPool::Instance()->InvokeOnMain<ClickerData *>(
                DELEGATE(Clicker, SendingData, ClickerData *, this), new ClickerData(str) );
    }
    m_ThreadStopped = true;
}

void Clicker::SendingData(ClickerData * a_pData)
{
    SendData(a_pData);
}

void Clicker::OnPause()
{
    ++m_Paused;
}

void Clicker::OnResume()
{
    --m_Paused;
}

