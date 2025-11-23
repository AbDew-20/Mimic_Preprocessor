#pragma once

#include<Core/PCH.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <string_view>

class DataAnalyzer{
public:
	DataAnalyzer(size_t numItems);
		

	void AddDataSeries(std::string &seriesName, std::vector<float> &&dataSeries);
	void SortBySeries(std::string &seriesName);
	void SetItemList(std::vector<std::string_view> &&itemList);
	void GenerateCumalativeHistogram(std::string &seriesName, size_t numBins, float min, float max, std::vector<std::pair<float, float>> *pCumalativeHistogram, std::vector<std::pair<float, float>> *pBinData);
	void GenerateHistogram(std::string &seriesName, size_t numBins, float min, float max, std::vector<std::pair<float, float>> *pHistogram,	std::vector<std::pair<float, float>> *pBinData);
	float GetPercentileValue(std::string &seriesName, float percentile);
	void Truncate(float min, float max);
	void ReturnCurrentSeries(std::vector<std::pair<float, size_t>> *pSeries);
	void ResetFrame();

protected:

private:
	size_t numItems_;
	struct SeriesView{
		std::string_view seriesName;
		std::vector<std::pair<float, size_t>> seriesData;
	};
	std::unordered_map<std::string, std::vector<float>> dataList_;
	std::vector<std::string_view> itemList_;
	std::vector<size_t> frame_;

	SeriesView currentSeries_;



};
