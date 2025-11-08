#pragma once

#include <chrono>


class Clock{
public:
	Clock();
	void Tick();
	void Reset();
	double GetDeltaNanoSeconds()const;
	double GetDeltaMicroSeconds()const;
	double GetDeltaMilliSeconds()const;
	double GetDeltaSeconds()const;

	double GetTotalNanoSeconds()const;
	double GetTotalMicroSeconds()const;
	double GetTotalMilliSeconds()const;
	double GetTotalSeconds()const;
private:
	std::chrono::high_resolution_clock::time_point t0_;
	std::chrono::high_resolution_clock::duration deltaTime_;
	std::chrono::high_resolution_clock::duration totalTime_;

};