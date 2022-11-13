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
		float TotalTime() const;
		
		void Reset();
		void Start();
		void Stop();
		void Tick();
	
	private:

		double m_secondsPerCount;
		double m_deltaTime;

		__int64 m_baseTime;
		__int64 m_pausedTime;
		__int64 m_stopTime;
		__int64 m_currentTime;
		__int64 m_prevTime;


		bool m_stopped = false;

	};
}
