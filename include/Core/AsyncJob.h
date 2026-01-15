#pragma once
#include <thread>
#include <atomic>

struct JobState{
	std::atomic<float_t> percent{0.0f};
	std::atomic<uint32_t> stage{0};
	std::atomic<bool> completed{false};
	std::atomic<bool> error{false};
};

class AsyncJob{
public:
	AsyncJob(){
	}

	template<typename Func, typename... Args>
	void Start(Func &&fn, Args&&... args){
		assert(!worker_.joinable()&&"Thread in use");
		worker_ = std::thread([this, fn = std::forward<Func>(fn)](auto&&... args){ 
			std::invoke(fn,std::forward<decltype(args)>(args)...,state_);
			state_.completed = true;
		},
		std::forward<Args>(args)...
		);
	}

	void Join(){
		if(worker_.joinable()){
			worker_.join();
		}
	}

	void Reset(){
		assert(!worker_.joinable()&&"Thread in use");
		state_.completed = false;
		state_.error = false;
		state_.percent = 0.0f;
		state_.stage = 0;
	}

	float_t GetPercent() const{ return state_.percent.load(); }
	uint32_t GetStage() const{ return state_.stage.load(); }
	bool GetCompleted() const{ return state_.completed.load(); }
	bool GetError() const{ return state_.error.load(); }

protected:

private:
	JobState state_;
	std::thread worker_;



};