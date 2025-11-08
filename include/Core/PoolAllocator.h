#pragma once
#include <vector>
#include <numeric>
#include<utility>
template <class C> class PoolAllocator{
public:
	PoolAllocator(size_t poolSize);
	C *Insert(C &element);
	template<class ... Args>
	C *Emplace(Args&&... args);
	void Remove(C *pElem);
	C *GetPoolPointer(uint32_t pool);
	inline bool iIsEmpty(){
		return top_==0;
	}
	inline uint32_t GetPoolSize(uint32_t pool){
		return data_.at(pool).size();
	}
	inline uint32_t GetNumPools(){
		return data_.size();
	}
private:
	void AddPool();
	uint32_t GetIndex(C *pElem);
	std::vector<std::vector<C>> data_;
	std::vector<uint32_t> freeList_;
	std::vector<typename std::vector<C>::iterator> iteratorList_;
	size_t top_;
	size_t poolSize_;
};
template<class C>
PoolAllocator<C>::PoolAllocator(size_t poolSize) :
	poolSize_(poolSize),
	top_(0){
	data_.push_back(std::vector<C>());
	data_.at(0).reserve(poolSize);
	freeList_.reserve(poolSize);
	freeList_.resize(poolSize);
	std::iota(freeList_.begin(), freeList_.end(), 0);
	iteratorList_.push_back(data_.back().begin());
}
template<class C>
C *PoolAllocator<C>::Insert(C &elem){
	if(top_==freeList_.size()){
		AddPool();
	}
	uint32_t idx = freeList_.at(top_);
	top_++;
	uint32_t pool = idx/poolSize_;
	uint32_t index = idx%poolSize_;
	auto iter = iteratorList_.at(pool)+index;
	if(idx<data_.at(pool).size()){
		data_.at(pool).at(index) = std::move(elem);
	}
	else{
		iter = data_.at(pool).insert(iter, std::move(elem));
	}
	iteratorList_.at(pool) = data_.at(pool).begin();
	return &(*iter);
}
template<class C>
template<class... Args>
C *PoolAllocator<C>::Emplace(Args&&... args){
	if(top_==freeList_.size()){
		AddPool();
	}
	uint32_t idx = freeList_.at(top_);
	top_++;
	uint32_t pool = idx/poolSize_;
	uint32_t index = idx%poolSize_;
	auto iter = iteratorList_.at(pool)+index;
	if(idx<data_.at(pool).size()){
		data_.at(pool).at(index) = std::move(C(std::forward<Args>(args)...));
	}
	else{
		iter = data_.at(pool).emplace(iter, std::forward<Args>(args)...);
	}
	iteratorList_.at(pool) = data_.at(pool).begin();
	return &(*(iter));
}
template<class C>
uint32_t PoolAllocator<C>::GetIndex(C *pElem){
	for(int i = 0; i<data_.size(); i++){
		if(data_.at(i).empty()){
			continue;
		}
		int idx = pElem-&data_.at(i)[0];
		if(idx>=0&&idx<poolSize_){
			return (uint32_t)(idx+(i*poolSize_));
		}
	}
	return 0;
}
template<class C>
void PoolAllocator<C>::Remove(C *pElem){
	uint32_t idx = GetIndex(pElem);
	if(std::find(freeList_.begin()+top_, freeList_.end(), idx)!=freeList_.end()){
		return;
	}
	top_--;
	auto iter = freeList_.begin()+top_; //TODO: useless?
	freeList_.at(top_) = idx;
}
template<class C>
void PoolAllocator<C>::AddPool(){
	data_.push_back(std::vector<C>());
	data_.back().reserve(poolSize_);
	iteratorList_.push_back(data_.back().begin());
	uint32_t end = freeList_.size();
	freeList_.reserve(end+poolSize_); //TODO: potentially redundant
	freeList_.resize(end+poolSize_);
	std::iota(freeList_.begin()+end, freeList_.end(), end);
}
template<class C>
C *PoolAllocator<C>::GetPoolPointer(uint32_t pool){
	if(data_.at(pool).empty()){
		return nullptr;
	}
	return &(*iteratorList_.at(pool));
}