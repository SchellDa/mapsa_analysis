#ifndef ALIBAVA_HIT_GENERATOR_H
#define ALIBAVA_HIT_GENERATOR_H

#include <Eigen/Dense>
#include "datastructures.h"

#define ALIBAVA_N 256
#define ALIBAVA_PITCH 90
#define ALIBAVA_STRIP_L 15000

namespace core {

class AlibavaHitGenerator 
{
public:
	//AlibavaHitGenerator::AlibavaHitGenerator(){}
	//virtual AlibavaHitGenerator::~AlibavaHitGenerator(){};
	
	static std::vector<Eigen::Vector2d> getLocalHits(run_data_t run);
	static std::vector<Eigen::Vector3d> getHits(run_data_t run, double z=0.);
	
};
}//core
#endif//ALIBAVA_HIT_GENERATOR_H
