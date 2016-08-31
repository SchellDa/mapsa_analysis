

#include "mpatransform.h"
#include "gtest/gtest.h"

using namespace core;

class Env : public ::testing::Environment
{
public:
	virtual void SetUp()
	{
		_mpa.setNumPixels({16, 3});
		_mpa.setPixelSize({0.1, 1.446});
		_mpa.setSensitiveSize({1.7, 4.338});
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

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	::testing::AddGlobalTestEnvironment(env = new Env);
	return RUN_ALL_TESTS();
}

