#include <Utils/DataAnalyzer.h>
#include <algorithm>
#include <numeric>


DataAnalyzer::DataAnalyzer(size_t numItems):
	numItems_(numItems){
	currentSeries_.seriesData.resize(numItems);
	frame_.resize(numItems);
	std::iota(frame_.begin(), frame_.end(), 0);
}


void DataAnalyzer::AddDataSeries(std::string &seriesName, std::vector<float> &&dataSeries){
	dataList_[seriesName] = std::move(dataSeries);
}


void DataAnalyzer::SetItemList(std::vector<std::string_view> &&itemList){
	itemList_ = std::move(itemList);
}


void DataAnalyzer::SortBySeries(std::string &seriesName){
	std::vector<float> &dataSeries = dataList_[seriesName];
	currentSeries_.seriesName = seriesName;
	currentSeries_.seriesData.resize(frame_.size());
	for(size_t i = 0; i<frame_.size(); ++i){
		currentSeries_.seriesData[i] = std::pair<float, size_t>(dataSeries[frame_[i]], frame_[i]);
	}

	std::sort(currentSeries_.seriesData.begin(), currentSeries_.seriesData.end());
}
void DataAnalyzer::GenerateCumalativeHistogram(std::string &seriesName, size_t numBins, float min, float max, std::vector<std::pair<float, float>> *pCumalativeHistogram, std::vector<std::pair<float, float>> *pBinData){
	if(currentSeries_.seriesName==""){
		return;
	}
	if(numBins!=0){
		GenerateHistogram(seriesName, numBins, min, max, pCumalativeHistogram, pBinData);
	}
	float sum = 0.0f;
	for(int i = 0; i<numBins; ++i){
		float temp = (*pCumalativeHistogram)[i].second;
		(*pCumalativeHistogram)[i].second += sum;
		sum += temp;
	}
}
void DataAnalyzer::GenerateHistogram(std::string &seriesName, size_t numBins, float min, float max, std::vector<std::pair<float, float>> *pHistogram, std::vector<std::pair<float, float>> *pBinData){
	if(currentSeries_.seriesName==""){
		return;
	}
	pHistogram->resize(numBins);
	pBinData->resize(numBins);
	size_t seriesSize = currentSeries_.seriesData.size();
	min = (currentSeries_.seriesData[0].first>min)? currentSeries_.seriesData[0].first: min;
	max = (currentSeries_.seriesData[seriesSize-1].first<max) ? currentSeries_.seriesData[seriesSize-1].first : max;
	//float max = 1.0f;
	float binSize = (max-min)/(float)numBins;
	DebugPrint("Bin Size: %f\n", binSize);
	if(binSize==0.0f){
		binSize = 1.0f;
	}
	for(int i = 0; i<numBins; ++i){
		(*pHistogram)[i].first = min+(i*binSize);
	}
	size_t lastBin = 0;
	float lastVal = min;
	(*pBinData)[0].first = min;
	for(int i = 0; i<seriesSize; ++i){
		std::pair<float, size_t> temp = currentSeries_.seriesData[i];
		float seriesVal = temp.first;
		size_t bin = (size_t)((seriesVal-min)/binSize);
		if(bin!=lastBin){
			(*pBinData)[lastBin].second = lastVal;
			(*pBinData)[bin].first = seriesVal;
		}
		if(bin>=numBins||bin<0) continue;
		(*pHistogram)[bin].second += ((seriesName!="") ? dataList_[seriesName][temp.second] : 1.0f);
		lastBin = bin;
		lastVal = seriesVal;
	}
	(*pBinData)[numBins-1].second = max;
}
float DataAnalyzer::GetPercentileValue(std::string &seriesName, float percentile){
	if(currentSeries_.seriesName==""){
		return 0.0f;
	}
	size_t seriesSize = currentSeries_.seriesData.size();
	std::vector<float> prefixSum(seriesSize+1);
	for(int i = 0; i<seriesSize; ++i){
		prefixSum[i+1] = prefixSum[i];
		std::pair<float, size_t> temp = currentSeries_.seriesData[i];
		float seriesVal = temp.first;
		prefixSum[i+1] += ((seriesName!="") ? dataList_[seriesName][temp.second] : 1.0f);
	}
	float sum = prefixSum[seriesSize];
	int idx = 1;
	for(idx; idx<seriesSize+1; ++idx){
		if(prefixSum[idx]>percentile*sum) break;
	}
	return currentSeries_.seriesData[idx-1].first;

}


void DataAnalyzer::ReturnCurrentSeries(std::vector<std::pair<float, size_t>> *pSeries){
	(*pSeries) = currentSeries_.seriesData;
}

void DataAnalyzer::Truncate(float min, float max){
	if(currentSeries_.seriesName==""){
		return;
	}
	frame_.resize(currentSeries_.seriesData.size());
	size_t idx = 0;
	for(int i = 0; i<currentSeries_.seriesData.size(); ++i){
		std::pair<float,size_t> val = currentSeries_.seriesData[i];
		if(val.first>=min&&val.first<=max){
			frame_[idx] = val.second;
			++idx;
		}
	}
	frame_.resize(idx);

}

void DataAnalyzer::ResetFrame(){
	frame_.resize(numItems_);
	std::iota(frame_.begin(), frame_.end(), 0);
	
}
