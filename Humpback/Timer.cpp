// (c) Li Hongcheng
// 2021/12/25


#include "Timer.h"


namespace Humpback {

	Timer::Timer()
		: m_secondsPerCount(0.0f), m_baseTime(0), m_pausedTime(0), m_currentTime(0), 
		m_prevTime(0), m_deltaTime(0.0f), m_stopped(false)
	{

		// Quirey the frequency.
		__int64 countsPerSeconds;
		QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSeconds);

		m_secondsPerCount = 1.0f / (double)countsPerSeconds;
	}

	float Timer::DeltaTime() const
	{
		return (float)m_deltaTime;
	}

	float Timer::TotalTime() const
	{
		if (m_stopped)
		{
			return (float)(((m_stopTime - m_pausedTime) - m_baseTime) * m_secondsPerCount);
		}
		else
		{
			return (float)(((m_currentTime - m_pausedTime) - m_baseTime)* m_secondsPerCount);
		}
	}

	void Timer::Reset()
	{
		__int64 currentTime = 0;
		QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);

		m_currentTime = currentTime;
		m_baseTime = currentTime;
		m_prevTime = currentTime;
		m_stopTime = 0;
		m_pausedTime = 0;

		m_stopped = false;
	}

	void Timer::Start()
	{
		if (m_stopped)
		{
			__int64 startTime;
			QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

			m_pausedTime += (startTime - m_stopTime);
			m_prevTime = startTime;
			m_stopTime = 0;
			m_stopped = false;
		}
	}

	void Timer::Stop()
	{
		if (m_stopped == false)
		{
			__int64 currTime;
			QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
			
			m_stopTime = currTime;
			m_stopped = true;
		}
	}

	void Timer::Tick()
	{
		if (m_stopped)
		{
			m_deltaTime = 0.0f;
			return;
		}

		// Get the time of this frame.
		__int64 curTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&curTime);

		m_currentTime = curTime;

		// Time difference between this frame and the previous.
		m_deltaTime = (m_currentTime - m_prevTime) * m_secondsPerCount;
		
		m_prevTime = m_currentTime;

		if (m_deltaTime < 0.0f)
		{
			m_deltaTime = 0.0f;
		}
	}
}