#include <Core/Clock.h>
#include <Core/PCH.h>


Clock::Clock() :
	deltaTime_(0),
	totalTime_(0){
	t0_ = std::chrono::high_resolution_clock::now();
}


void Clock::Tick(){
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	deltaTime_ = t1-t0_;
	totalTime_ += deltaTime_;
	t0_ = t1;
}

void Clock::Reset(){
	t0_ = std::chrono::high_resolution_clock::now();
	deltaTime_ = std::chrono::high_resolution_clock::duration();
	totalTime_ = std::chrono::high_resolution_clock::duration();
}
double Clock::GetDeltaNanoSeconds()const{
	return deltaTime_.count()*1.0;
}

double Clock::GetDeltaMicroSeconds()const{
	return deltaTime_.count()*1e-3;
}
double Clock::GetDeltaMilliSeconds()const{
	return deltaTime_.count()*1e-6;
}
double Clock::GetDeltaSeconds()const{
	return deltaTime_.count()*1e-9;
}
double Clock::GetTotalNanoSeconds()const{
	return totalTime_.count()*1.0;
}
double Clock::GetTotalMicroSeconds()const{
	return totalTime_.count()*1e-3;
}
double Clock::GetTotalMilliSeconds()const{
	return totalTime_.count()*1e-6;
}
double Clock::GetTotalSeconds()const{
	return totalTime_.count()*1e-9;
}

