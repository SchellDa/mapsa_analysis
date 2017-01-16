

#include "mpatransform.h"
#include "gtest/gtest.h"
#include <fstream>

using namespace core;

class Env : public ::testing::Environment
{
public:
	virtual void SetUp()
	{
		/*_mpa.setNumPixels({16, 3});
		_mpa.setPixelSize({0.1, 1.446});
		_mpa.setSensitiveSize({1.7, 4.338});*/
	}

	virtual void TearDown()
	{
	}
	MpaTransform _mpa;
};

Env* env;

TEST(mpatransform, translate_index_equality)
{
	for(size_t idx=0; idx < 48; ++idx) {
		auto pixCoord = env->_mpa.translatePixelIndex(idx);
		EXPECT_EQ(env->_mpa.pixelCoordToIndex(pixCoord), idx)
			<< " Pixel Coord: " << pixCoord(0) << " " << pixCoord(1) << std::endl;
	}
}

TEST(mpatransform, global_equality)
{
	for(size_t idx=0; idx < 48; ++idx) {
		auto global = env->_mpa.transform(idx);
		EXPECT_EQ(env->_mpa.getPixelIndex(global), idx)
			<< " Pixel Coord: " << global(0) << " " << global(1) << " " << global(2) << std::endl;
	}
}

TEST(mpatransform, random_track_hits)
{
	for(size_t idx=0; idx < 48; ++idx) {
		auto global = env->_mpa.transform(idx);
		Track track;
		track.points.push_back(global);
		track.points.push_back(global+Eigen::Vector3d(0.0, 0.0, -43.0));
		EXPECT_NO_THROW({
			EXPECT_EQ(env->_mpa.getPixelIndex(track, 0, 1), idx)
				<< " Pixel Coord: " << global(0) << " " << global(1) << " " << global(2) << std::endl;
		});
	}
}

double randomUnit()
{
	return static_cast<double>(rand())/RAND_MAX;
}

double randomInterval(float min, float max)
{
	assert(max > min);
	double x = randomUnit();
	return x*(max-min) + min;
}

TEST(mpatransform, mpaPlaneTrackIntersect_consistency)
{
	/* rotate and translate MPA randomly, then shoot a random track on it
	 * and check that */
	std::ofstream fout("rot_test_file.csv");
	for(size_t test_no=0; test_no < 100; ++test_no) {
		core::MpaTransform trans;
		trans.setOffset({
			randomInterval(-4, 4),
			randomInterval(-4, 4),
			randomInterval(10, 20)
		});
		trans.setRotation({
				randomInterval(-1, 1),
				randomInterval(-1, 1),
				randomInterval(-1, 1)
		});
		trans.setRotation({
				randomInterval(-1, 1),
				0.0,
				0.0
		});
		Track track;
		track.points.push_back({
				randomInterval(-1, 1),
				randomInterval(-1, 1),
				0.0
		});
		track.points.push_back({
				randomInterval(-5, 5),
				randomInterval(-5, 5),
				randomInterval(10, 20)
		});
		// random pixel
		auto a = trans.transform(randomInterval(0, 48));
		auto b = trans.mpaPlaneTrackIntersect(track, 0, 1);
		auto o = trans.getOffset();
		Eigen::Vector3d d = (b - a).normalized();
		Eigen::Vector3d n = trans.getNormal();
		fout << a(0) << " "
		     << a(1) << " "
		     << a(2) << "\n"
		     << b(0) << " "
		     << b(1) << " "
		     << b(2) << "\n"
		     << o(0) << " "
		     << o(1) << " "
		     << o(2) << "\n"
		     << n(0) << " "
		     << n(1) << " "
		     << n(2) << "\n"
		     << "0 0 0\n"
		     << d(0) << " "
		     << d(1) << " "
		     << d(2) << "\n\n\n";
		// now do consistency check
		ASSERT_NEAR((a-b).normalized().dot(n), 0.0, 0.01);
		ASSERT_NEAR((a-trans.getOffset()).normalized().dot(n), 0.0, 0.01);
		ASSERT_NEAR((b-trans.getOffset()).normalized().dot(n), 0.0, 0.01);
	}
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	::testing::AddGlobalTestEnvironment(env = new Env);
	return RUN_ALL_TESTS();
}

