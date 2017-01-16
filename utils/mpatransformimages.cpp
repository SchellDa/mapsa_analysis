
#include "mpatransform.h"
#include <iostream>
#include <fstream>
#include <stdio.h>

void angularPoints()
{
	core::MpaTransform transform;
	transform.setOffset({0, 0, 0});
	std::ofstream of("angular.csv");
	of << "# Index\tWorld X\tWorld Y\tWorld Z\tTheta\tPhi\tOmega\n";
	size_t num_frames = 0;
	double roty = 0.0;
	double rotz = 0.0;
	for(double rotx = -1.570796; rotx < 1.570796; rotx += 0.005) {
		roty += 0.02;
		rotz += 0.1;
		transform.setRotation({rotx, roty, rotz});
		for(double x=0.0; x <= transform.num_pixels_x+0.04; x += 0.25) {
			for(double y=0.0; y <= transform.num_pixels_y+0.04; y += 0.25) {
				auto pc = transform.pixelCoordToGlobal(Eigen::Vector2d{x, y});
				of << pc(0) << "\t" << pc(1) << "\t" << pc(2) << "\t"
				   << rotx << "\t" << roty << "\t" << rotz << "\n";
			}
		}
		of << "\n\n";
		++num_frames;
	}
	of << "# num_frames = " << num_frames << "\n";
	of.close();
}

void planeOffsetTest()
{
	core::MpaTransform transform;
	transform.setOffset({-4, -6, 900});
	transform.setRotation({0.4, -0.2, 0.1});
	std::ofstream of("planeoffset.csv");
	of << "# Index\tWorld X\tWorld Y\tWorld Z\n";
	auto o = transform.getOffset();
	of << o(0) << " " << o(1) << " " << o(2) << "\n";
	for(size_t i = 0; i < 48; ++i) {
		auto a = transform.transform(i);
		of << a(0) << " " << a(1) << " " << a(2) << "\n";
	}
	of << "\n\n";
	of.close();
}

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
	std::ofstream random_hit_rotated("random_hit_rotated.csv");
	random_hit_rotated << "# In X\tIn Y\tOut X\tOut Y\n";
	transform.setRotation({3.141*0.2, 3.141*0.01, 3.141*0.002});
	for(size_t i = 0; i < 100; ++i) {
		double x = static_cast<double>(rand())/RAND_MAX * 16;
		double y = static_cast<double>(rand())/RAND_MAX * 3;
		auto global = transform.pixelCoordToGlobal(Eigen::Vector2d{x, y});
		auto back = transform.globalToPixelCoord(global);
		random_hit_rotated << x << "\t"
		                << y << "\t"
				<< back(0) << "\t"
				<< back(1) << "\n";
	}
	random_hit_rotated.close();
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
	angularPoints();
	planeOffsetTest();
	return 0;
}
