// (c) Li Hongcheng
// 2021/12/25


#pragma once

#include<Windows.h>

namespace Humpback {
	class Timer 
	{
	public :
		Timer();

		float DeltaTime() const;	// in seconds
		
		void Reset();
		void Start();
		void Stop();
		void Tick();
	
	private:

		double m_secondsPerCount;

		__int64 m_baseTime;
		__int64 m_pausedTime;
		__int64 m_currentTime;
		__int64 m_prevTime;

		double m_deltaTime;

		bool m_stopped;

	};
}
