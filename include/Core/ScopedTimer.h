#pragma once
#include <Core/Helper.h>
#include <Core/Clock.h>
#include <string>


class ScopedTimer{
public:
	ScopedTimer(const char* name):
	name_("[Timer "){
		name_.append(name);
	}
	ScopedTimer():
	name_("[Timer"){
	
	}
	~ScopedTimer(){
		clock_.Tick();
		name_.append("] Elapsed time: %f ms\n");
		DebugPrint(name_.c_str(), clock_.GetDeltaMilliSeconds());
	}
protected:
private:
	Clock clock_;
	std::string name_;
};