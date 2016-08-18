
#include "mpastreamreader.h"
#include "gtest/gtest.h"
#include <cstdio>
#include <fstream>

using namespace core;

class DataFileEnv : public ::testing::Environment
{
public:
	virtual void SetUp()
	{
		char s[4096];
		filename = std::tmpnam(s);
		std::ofstream fout(filename);
		fout << "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]\n"
		     << "[11, 12, 13, 14, 15, 16, 17, 18, 19, 20]\n"
		     << "[21, 22, 23, 24, 25, 26, 27, 28, 29, 30]\n";
		fout.flush();
		fout.close();
	}

	virtual void TearDown()
	{
		std::remove(filename.c_str());
	}

	std::string getFilename() const { return filename; }
private:
	std::string filename;
};

DataFileEnv* env;

TEST(mpastreamreader, read)
{
	MPAStreamReader reader(env->getFilename());
	int totalEvts = 0;
	for(const auto& evt: reader) {
		int pixelNo = 0;
		EXPECT_EQ(evt.eventNumber, totalEvts) << "Wrong event number";
		EXPECT_EQ(evt.data.size(), 10) << "Invalid container size in event" << evt.eventNumber;
		for(const auto& counter: evt.data) {
			EXPECT_EQ(counter, 1 + pixelNo + evt.eventNumber*10) << "Pixel counter read error in event "
				<< evt.eventNumber << " at pixel " << pixelNo;
			++pixelNo;
		}
		totalEvts += 1;
	}
	EXPECT_EQ(totalEvts, 3);
}

TEST(mpastreamreader, filenotfound)
{
	EXPECT_THROW({	
		MPAStreamReader reader(env->getFilename()+"abc");
		reader.begin();
	}, std::ios_base::failure);
}
int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	::testing::AddGlobalTestEnvironment(env = new DataFileEnv);
	return RUN_ALL_TESTS();
}

