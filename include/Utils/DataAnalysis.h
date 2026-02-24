#pragma once

#include<Core/PCH.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <string_view>

namespace DataAnalysis{
	class DataAnalyzer{
	public:
		DataAnalyzer() = default;


		void Init(size_t numItems);
		void AddDataSeries(const std::string &seriesName, std::vector<float> &&dataSeries);
		void SortBySeries(const std::string &seriesName);
		void SetItemList(std::vector<std::string_view> &&itemList);
		void GenerateCumalativeHistogram(const std::string &seriesName, size_t numBins, float min, float max,
			std::vector<float> &cumalativeHistogramx,
			std::vector<float> &cumalativeHistogramy,
			std::vector<std::pair<float, float>> &binData);
		void GenerateHistogram(const std::string &seriesName, size_t numBins, float min, float max,
			std::vector<float> &histogramx,
			std::vector<float> &histogramy,
			std::vector<std::pair<float, float>> &binData);
		void GetPercentileValue(const std::string &seriesName, const std::vector<float> &percentile, std::vector<float> &result);
		void TruncateByValue(float min, float max);
		void TruncateByPercentile(const std::string &seriesName, float min, float max);
		std::vector<std::pair<float, size_t>> *ReturnCurrentSeriesP();
		std::pair<float, float> GetCurrentSeriesExtents(){ return std::pair<float, float>(currentSeries_.seriesData[0].first, (*(currentSeries_.seriesData.end()-1)).first); }
		void GetSeriesNames(std::vector<std::string_view> &names);
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
}
