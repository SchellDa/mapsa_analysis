
#include "mpatransform.h"
#include <iostream>
#include <fstream>
#include <stdio.h>

int main(int argc, char* argv[]) {
	core::MpaTransform transform;
	std::ofstream pixel_grid_file("pixel_grid.csv");
	pixel_grid_file << "# PixX\tPixel Y\tGlobal X\tGlobal Y\n";
	for(double x=0.0; x <= transform.num_pixels_x+0.04; x += 0.25) {
		for(double y=0.0; y <= transform.num_pixels_y+0.04; y += 0.25) {
			auto global = transform.pixelCoordToGlobal(Eigen::Vector2d{x, y});
			pixel_grid_file << x << "\t"
			                << y << "\t"
					<< global(0) << "\t"
					<< global(1) << "\n";
		}
	}
	pixel_grid_file.close();
	std::ofstream random_hit_file("random_hit_file.csv");
	random_hit_file << "# In X\tIn Y\tOut X\tOut Y\n";
	for(size_t i = 0; i < 100; ++i) {
		double x = static_cast<double>(rand())/RAND_MAX * 16;
		double y = static_cast<double>(rand())/RAND_MAX * 3;
		auto global = transform.pixelCoordToGlobal(Eigen::Vector2d{x, y});
		auto back = transform.globalToPixelCoord(global);
		random_hit_file << x << "\t"
		                << y << "\t"
				<< back(0) << "\t"
				<< back(1) << "\n";
		
	}
	random_hit_file.close();
	std::ofstream pixel_index_file("pixel_index_file.csv");
	pixel_index_file << "# Index\tPixel X\tPixel Y\tWorld X\tWorld Y\n";
	for(size_t i = 0; i < transform.num_pixels; ++i) {
		auto pc = transform.translatePixelIndex(i);
		auto wc = transform.pixelCoordToGlobal(pc, true);
		pixel_index_file << i << "\t"
		                 << pc(0) << "\t" << pc(1) << "\t"
		                 << wc(0) << "\t" << wc(1) << "\n";
	}
	pixel_index_file.close();
	return 0;
}
