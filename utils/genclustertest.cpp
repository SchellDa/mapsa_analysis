
#include "mpahitgenerator.h"
#include <fstream>
#include <random>
#include <iostream>
#include <algorithm>

double randomUniform(double low, double high)
{
	return static_cast<double>(rand())/RAND_MAX * (high - low) + low;
}

void fillRandomCluster(std::vector<Eigen::Vector2i>& pixel_list)
{
	Eigen::Vector2i pixel(randomUniform(-20, 20), randomUniform(-20, 20));
	pixel_list.push_back(pixel);
	bool newPixel;
	do {
		newPixel = randomUniform(0, 1) < 0.7;
		std::cout << "New pixel? " << newPixel << std::endl;
		if(newPixel) {
			double dist = randomUniform(0, 1);
			if(dist > 0.75) {
				pixel(0) += 1;
			} else if (dist > 0.5) {
				pixel(0) -= 1;
			} else if (dist > 0.25) {
				pixel(1) += 1;
			} else {
				pixel(1) -= 1;
			}
			if(std::find(pixel_list.begin(), pixel_list.end(), pixel) == pixel_list.end()) {
				pixel_list.push_back(pixel);
			}
		}
	} while(newPixel);
}

int main(int argc, char* argv[])
{
	std::ofstream fhits("cluster_hits.csv");
	std::ofstream fclusters("cluster_clusters.csv");
	for(size_t eventNo = 0; eventNo < 1000; ++eventNo) {
		std::vector<Eigen::Vector2i> pixels;
		const size_t numGenerations = randomUniform(1, 6);
		for(size_t i = 0; i < numGenerations; ++i) {
			fillRandomCluster(pixels);
		}
		std::vector<int> size;
		std::vector<double> area;
		auto clusters = core::MpaHitGenerator::clusterize(pixels, &size, &area);
		for(const auto& p: pixels) {
			fhits << p(0) << " " << p(1) << "\n";
		}
		for(size_t i = 0; i < clusters.size(); ++i) {
			fclusters << clusters[i](0) << " " << clusters[i](1) << " " << size[i] << " " << area[i] << "\n";
		}
		fhits << "\n\n";
		fclusters << "\n\n";
	}
	return 0;
}
