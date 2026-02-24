#include <Utils/DataAnalysis.h>
#include <algorithm>
#include <numeric>



void DataAnalysis::DataAnalyzer::Init(size_t numItems){
	numItems_ = numItems;
	currentSeries_.seriesData.resize(numItems);
	frame_.resize(numItems);
	std::iota(frame_.begin(), frame_.end(), 0);
}


void DataAnalysis::DataAnalyzer::AddDataSeries(const std::string &seriesName, std::vector<float> &&dataSeries){
	dataList_[seriesName] = std::move(dataSeries);
}


void DataAnalysis::DataAnalyzer::SetItemList(std::vector<std::string_view> &&itemList){
	itemList_ = std::move(itemList);
}


void DataAnalysis::DataAnalyzer::SortBySeries(const std::string &seriesName){
	std::vector<float> &dataSeries = dataList_[seriesName];
	currentSeries_.seriesName = seriesName;
	currentSeries_.seriesData.resize(frame_.size());
	for(size_t i = 0; i<frame_.size(); ++i){
		currentSeries_.seriesData[i] = std::pair<float, size_t>(dataSeries[frame_[i]], frame_[i]);
	}

	std::sort(currentSeries_.seriesData.begin(), currentSeries_.seriesData.end());
}
void DataAnalysis::DataAnalyzer::GenerateCumalativeHistogram(const std::string &seriesName, size_t numBins, float min, float max,
	std::vector<float> &cumalativeHistogramx,
	std::vector<float> &cumalativeHistogramy,
	std::vector<std::pair<float, float>> &binData)
{
	if(currentSeries_.seriesName==""){
		return;
	}
	if(numBins!=0){
		GenerateHistogram(seriesName, numBins, min, max, cumalativeHistogramx, cumalativeHistogramy, binData);
	}
	float sum = 0.0f;
	for(int i = 0; i<numBins; ++i){
		float temp = cumalativeHistogramy[i];
		cumalativeHistogramy[i]+= sum;
		sum += temp;
	}
}
void DataAnalysis::DataAnalyzer::GenerateHistogram(const std::string &seriesName, size_t numBins, float min, float max,
	std::vector<float> &histogramx,
	std::vector<float> &histogramy,
	std::vector<std::pair<float, float>> &binData)
{
	if(currentSeries_.seriesName==""){
		return;
	}
	histogramx.resize(numBins);
	histogramy.resize(numBins);
	binData.resize(numBins);
	size_t seriesSize = currentSeries_.seriesData.size();
	min = (currentSeries_.seriesData[0].first>min)? currentSeries_.seriesData[0].first: min;
	max = (currentSeries_.seriesData[seriesSize-1].first<max) ? currentSeries_.seriesData[seriesSize-1].first : max;
	//float max = 1.0f;
	float binSize = (max-min)/(float)numBins;
	//DebugPrint("Bin Size: %f\n", binSize);
	if(binSize==0.0f){
		binSize = 1.0f;
	}
	for(int i = 0; i<numBins; ++i){
		histogramx[i] = min+(i*binSize);
	}
	size_t lastBin = 0;
	float lastVal = min;
	binData[0].first = min;
	for(int i = 0; i<seriesSize; ++i){
		std::pair<float, size_t> temp = currentSeries_.seriesData[i];
		float seriesVal = temp.first;
		size_t bin = (size_t)((seriesVal-min)/binSize);
		if(bin>=numBins||bin<0) continue;
		if(bin!=lastBin){
			binData[lastBin].second = lastVal;
			binData[bin].first = seriesVal;
		}
		histogramy[bin] += ((seriesName!="") ? dataList_[seriesName][temp.second] : 1.0f);
		lastBin = bin;
		lastVal = seriesVal;
	}
	binData[numBins-1].second = max;
}
void DataAnalysis::DataAnalyzer::GetPercentileValue(const std::string &seriesName, const std::vector<float> &percentile, std::vector<float> &result){
	if(currentSeries_.seriesName==""){
		return;
	}
	result.resize(percentile.size());
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
	int low = 0;
	for(idx; idx<seriesSize+1; ++idx){
		for(int i = low; i<percentile.size(); ++i){
			if(prefixSum[idx]>=percentile[i]*sum){
				result[i] = currentSeries_.seriesData[idx-1].first;
				low = i+1;
				break;
			}
		}
	}
}


std::vector<std::pair<float, size_t>> *DataAnalysis::DataAnalyzer::ReturnCurrentSeriesP(){
	return &(currentSeries_.seriesData);
}

void DataAnalysis::DataAnalyzer::TruncateByValue(float min, float max){
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

void DataAnalysis::DataAnalyzer::TruncateByPercentile(const std::string &seriesName, float min, float max){
	if(min<0.0||max>100){
		return;
	}
	std::vector<float> percentileVals;
	GetPercentileValue(seriesName, {min/100.0f,max/100.0f},percentileVals);
	TruncateByValue(percentileVals[0], percentileVals[1]);
}

void DataAnalysis::DataAnalyzer::ResetFrame(){
	frame_.resize(numItems_);
	std::iota(frame_.begin(), frame_.end(), 0);
	
}
void DataAnalysis::DataAnalyzer::GetSeriesNames(std::vector<std::string_view> &names){
	for(auto iter = dataList_.begin(); iter!=dataList_.end(); ++iter){
		names.push_back(iter->first);
	}
}
