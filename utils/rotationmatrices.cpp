
#include "mpatransform.h"
#include <iostream>

void test(Eigen::Vector3d angles);

int main(int argc, char* argv[])
{
	test({0, 0, 0});
	test({0.1, 0, 0});
	test({0, 0.1, 0});
	test({0, 0, 0.1});
	test({0.1, 0.2, 0.3});
	test({0.3, 0.2, 0.1});
	return 0;
}

void test(Eigen::Vector3d angles)
{
	core::MpaTransform t;
	t.setOffset({0, 0, 0});
	t.setRotation(angles);
	std::cout << "Testing angles: " << angles(0) << " " << angles(1) << " " << angles(2)
	          << "\n------\n"
		  << t.getRotationMatrix()
		  << "\n"
		  << std::endl;
}
