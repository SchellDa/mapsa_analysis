
#include "trackstreamreader.h"
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
		std::ofstream fout;
		float_wo_decimal = std::tmpnam(s);
		fout.open(s);
		fout << "# X     Y       Z       SensorID        Evt     Run\n"
		     << "1.0000\t0.0000\t0\t0\t11\t4\n"
		     << "0.0000\t1.0000\t0\t1\t11\t4\n"
		     << "0.0000\t0.0000\t3\t2\t11\t4\n"
		     << "6.0000\t5.0000\t4\t3\t11\t4\n\n\n"
		     << "1.0000\t0.0000\t0\t0\t15\t4\n"
		     << "0.0000\t2.0000\t0\t1\t15\t4\n"
		     << "0.0000\t0.0000\t3\t2\t15\t4\n"
		     << "6.0000\t5.0000\t4\t3\t15\t4\n\n\n"
		     << "1.0000\t0.0000\t0\t0\t15\t4\n"
		     << "0.0000\t2.0000\t0\t1\t15\t4\n"
		     << "0.0000\t0.0000\t3\t2\t15\t4\n"
		     << "6.0000\t5.0000\t4\t3\t15\t4\n\n\n"
		     << "1.0000\t0.0000\t0\t0\t54\t4\n"
		     << "0.0000\t2.0000\t0\t1\t54\t4\n"
		     << "0.0000\t0.0000\t3\t2\t54\t4\n"
		     << "6.0000\t5.0000\t4\t3\t54\t4\n\n\n"
		     << "1.0000\t0.0000\t0\t0\t75\t4\n"
		     << "0.0000\t2.0000\t0\t1\t75\t4\n"
		     << "0.0000\t0.0000\t3\t2\t75\t4\n"
		     << "6.0000\t5.0000\t4\t3\t75\t4\n\n\n"
		     << "1.0000\t0.0000\t0\t0\t99\t4\n"
		     << "0.0000\t2.0000\t0\t1\t99\t4\n"
		     << "0.0000\t0.0000\t3\t2\t99\t4\n"
		     << "6.0000\t5.0000\t4\t3\t99\t4";
		fout.flush();
		fout.close();


		bad_evt_order = std::tmpnam(s);
		fout.open(s);
		fout << "# X     Y       Z       SensorID        Evt     Run\n"
		     << "1.0000\t0.0000\t0.0000\t0\t11\t4\n"
		     << "0.0000\t1.0000\t0.0000\t1\t11\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t14\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t14\t4\n\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t15\t4\n"
		     << "0.0000\t2.0000\t0.0000\t1\t15\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t15\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t15\t4\n\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t15\t4\n"
		     << "0.0000\t2.0000\t0.0000\t1\t15\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t15\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t15\t4\n\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t75\t4\n"
		     << "0.0000\t2.0000\t0.0000\t1\t75\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t75\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t75\t4\n\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t99\t4\n"
		     << "0.0000\t2.0000\t0.0000\t1\t99\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t99\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t99\t4\n\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t111\t4\n"
		     << "0.0000\t2.0000\t0.0000\t1\t111\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t111\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t111\t4\n\n\n";
		fout.flush();
		fout.close();


		valid1 = std::tmpnam(s);
		fout.open(s);
		fout << "# X     Y       Z       SensorID        Evt     Run\n"
		     << "1.0000\t0.0000\t0.0000\t0\t11\t4\n"
		     << "0.0000\t1.0000\t0.0000\t1\t11\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t11\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t11\t4\n\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t15\t4\n"
		     << "0.0000\t2.0000\t0.0000\t1\t15\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t15\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t15\t4\n\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t15\t4\n"
		     << "0.0000\t2.0000\t0.0000\t1\t15\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t15\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t15\t4\n\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t54\t4\n"
		     << "0.0000\t2.0000\t0.0000\t1\t54\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t54\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t54\t4\n\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t75\t4\n"
		     << "0.0000\t2.0000\t0.0000\t1\t75\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t75\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t75\t4\n\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t99\t4\n"
		     << "0.0000\t2.0000\t0.0000\t1\t99\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t99\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t99\t4";
		fout.flush();
		fout.close();

		valid2 = std::tmpnam(s);
		fout.open(s);
		fout << "# X     Y       Z       SensorID        Evt     Run\n"
		     << "\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t11\t4\n"
		     << "0.0000\t1.0000\t0.0000\t1\t11\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t11\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t11\t4\n\n\n\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t15\t4\n"
		     << "0.0000\t2.0000\t0.0000\t1\t15\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t15\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t15\t4\n\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t15\t4\n"
		     << "0.0000\t2.0000\t0.0000\t1\t15\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t15\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t15\t4\n\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t54\t4\n"
		     << "0.0000\t2.0000\t0.0000\t1\t54\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t54\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t54\t4\n\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t75\t4\n"
		     << "0.0000\t2.0000\t0.0000\t1\t75\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t75\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t75\t4\n\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t99\t4\n"
		     << "0.0000\t2.0000\t0.0000\t1\t99\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t99\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t99\t4\n";
		fout.flush();
		fout.close();

		valid3 = std::tmpnam(s);
		fout.open(s);
		fout << "# X     Y       Z       SensorID        Evt     Run\n"
		     << "1.0000\t0.0000\t0.0000\t0\t11\t4\n"
		     << "0.0000\t1.0000\t0.0000\t1\t11\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t11\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t11\t4\n\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t15\t4\n"
		     << "0.0000\t2.0000\t0.0000\t1\t15\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t15\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t15\t4\n\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t15\t4\n"
		     << "0.0000\t2.0000\t0.0000\t1\t15\t4 # Comment!!\n"
		     << "0.0000\t0.0000\t3.0000\t2\t15\t4\n"
		     << "# Yes, quite commentry\n"
		     << "6.0000\t5.0000\t4.0000\t3\t15\t4\n\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t54\t4\n"
		     << "0.0000\t2.0000\t0.0000\t1\t54\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t54\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t54\t4\n\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t75\t4\n"
		     << "0.0000\t2.0000\t0.0000\t1\t75\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t75\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t75\t4\n\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t99\t4\n"
		     << "0.0000\t2.0000\t0.0000\t1\t99\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t99\t4\t35 # Extras should be supported\n"
		     << "6.0000\t5.0000\t4.0000\t3\t99\t4\n\n";
		fout.flush();
		fout.close();


		negatives = std::tmpnam(s);
		fout.open(s);
		fout << "# X     Y       Z       SensorID        Evt     Run\n"
		     << "-1.0000\t0.0000\t0.0000\t0\t11\t4\n"
		     << "0.0000\t1.0000\t0.0000\t1\t11\t4\n"
		     << "0.0000\t-0.0000\t3.0000\t2\t11\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t11\t4\n\n\n"
		     << "1.0000\t0.0000\t0.0000\t0\t15\t4\n"
		     << "0.0000\t-2.0000\t0.0000\t1\t15\t4\n"
		     << "0.0000\t0.0000\t3.0000\t2\t15\t4\n"
		     << "6.0000\t5.0000\t4.0000\t3\t15\t4\n\n\n"
		     << "-1.0000\t0.0000\t0.0000\t0\t15\t4\n"
		     << "0.0000\t2.0000\t0.0000\t1\t15\t4 # Comment!!\n"
		     << "0.0000\t0.0000\t3.0000\t2\t15\t4\n"
		     << "# Yes, quite commentry\n"
		     << "6.0000\t5.0000\t4.0000\t3\t15\t4\n\n\n";
		fout.flush();
		fout.close();
	}

	virtual void TearDown()
	{
		std::remove(valid1.c_str());
		std::remove(valid2.c_str());
		std::remove(valid3.c_str());
		std::remove(bad_evt_order.c_str());
		std::remove(float_wo_decimal.c_str());
		std::remove(negatives.c_str());
	}

	std::string negatives;
	std::string float_wo_decimal;
	std::string bad_evt_order;
	std::string parse_error;
	std::string valid1;
	std::string valid2;
	std::string valid3;
};

DataFileEnv* env;

TEST(trackstreamreader, read)
{
	TrackStreamReader reader(env->valid1);
	int totalEvts = 0;
	int eventNumbers[] = {11, 15, 54, 75, 99};
	size_t numTracks[] = {1, 2, 1, 1, 1};
	for(const auto& evt: reader) {
		EXPECT_EQ(evt.eventNumber, eventNumbers[totalEvts]) << "Wrong event number";
		EXPECT_EQ(evt.tracks.size(), numTracks[totalEvts]) << "Wrong number of tracks in event "
			<< evt.eventNumber;
		EXPECT_EQ(evt.runID, 4);
		for(const auto& tr: evt.tracks) {
			EXPECT_EQ(tr.sensorIDs.size(), 4);
			EXPECT_EQ(tr.points.size(), 4);
			EXPECT_NO_THROW({
				EXPECT_EQ(tr.sensorIDs.at(0), 0);
				EXPECT_EQ(tr.sensorIDs.at(1), 1);
				EXPECT_EQ(tr.sensorIDs.at(2), 2);
				EXPECT_EQ(tr.sensorIDs.at(3), 3);
			});
		}
		totalEvts += 1;
	}
	EXPECT_EQ(totalEvts, 5);
}

TEST(trackstreamreader, read2)
{
	TrackStreamReader reader(env->valid2);
	int totalEvts = 0;
	int eventNumbers[] = {11, 15, 54, 75, 99};
	size_t numTracks[] = {1, 2, 1, 1, 1};
	for(const auto& evt: reader) {
		EXPECT_EQ(evt.eventNumber, eventNumbers[totalEvts]) << "Wrong event number";
		EXPECT_EQ(evt.tracks.size(), numTracks[totalEvts]) << "Wrong number of tracks in event "
			<< evt.eventNumber;
		EXPECT_EQ(evt.runID, 4);
		for(const auto& tr: evt.tracks) {
			EXPECT_EQ(tr.sensorIDs.size(), 4);
			EXPECT_EQ(tr.points.size(), 4);
			EXPECT_NO_THROW({
				EXPECT_EQ(tr.sensorIDs.at(0), 0);
				EXPECT_EQ(tr.sensorIDs.at(1), 1);
				EXPECT_EQ(tr.sensorIDs.at(2), 2);
				EXPECT_EQ(tr.sensorIDs.at(3), 3);
			});
		}
		totalEvts += 1;
	}
	EXPECT_EQ(totalEvts, 5);
}

TEST(trackstreamreader, read3)
{
	TrackStreamReader reader(env->valid3);
	int totalEvts = 0;
	int eventNumbers[] = {11, 15, 54, 75, 99};
	size_t numTracks[] = {1, 2, 1, 1, 1};
	for(const auto& evt: reader) {
		EXPECT_EQ(evt.eventNumber, eventNumbers[totalEvts]) << "Wrong event number";
		EXPECT_EQ(evt.tracks.size(), numTracks[totalEvts]) << "Wrong number of tracks in event "
			<< evt.eventNumber;
		EXPECT_EQ(evt.runID, 4);
		for(const auto& tr: evt.tracks) {
			EXPECT_EQ(tr.sensorIDs.size(), 4);
			EXPECT_EQ(tr.points.size(), 4);
			EXPECT_NO_THROW({
				EXPECT_EQ(tr.sensorIDs.at(0), 0);
				EXPECT_EQ(tr.sensorIDs.at(1), 1);
				EXPECT_EQ(tr.sensorIDs.at(2), 2);
				EXPECT_EQ(tr.sensorIDs.at(3), 3);
			});
		}
		totalEvts += 1;
	}
	EXPECT_EQ(totalEvts, 5);
}

TEST(trackstreamreader, negative_numbers)
{
	EXPECT_NO_THROW({
		TrackStreamReader reader(env->negatives);
		for(const auto& evt: reader) {
		}
	});
}

TEST(trackstreamreader, float_wo_decimal)
{
	TrackStreamReader reader(env->float_wo_decimal);
	int totalEvts = 0;
	int eventNumbers[] = {11, 15, 54, 75, 99};
	size_t numTracks[] = {1, 2, 1, 1, 1};
	for(const auto& evt: reader) {
		EXPECT_EQ(evt.eventNumber, eventNumbers[totalEvts]) << "Wrong event number";
		EXPECT_EQ(evt.tracks.size(), numTracks[totalEvts]) << "Wrong number of tracks in event "
			<< evt.eventNumber;
		EXPECT_EQ(evt.runID, 4);
		for(const auto& tr: evt.tracks) {
			EXPECT_EQ(tr.sensorIDs.size(), 4);
			EXPECT_EQ(tr.points.size(), 4);
			EXPECT_NO_THROW({
				EXPECT_EQ(tr.sensorIDs.at(0), 0);
				EXPECT_EQ(tr.sensorIDs.at(1), 1);
				EXPECT_EQ(tr.sensorIDs.at(2), 2);
				EXPECT_EQ(tr.sensorIDs.at(3), 3);
			});
		}
		totalEvts += 1;
	}
	EXPECT_EQ(totalEvts, 5);
}

TEST(trackstreamreader, bad_evt_order)
{
	EXPECT_THROW({
		TrackStreamReader reader(env->bad_evt_order);
		for(const auto& evt: reader) {
		}
	}, TrackStreamReader::consistency_error);
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	::testing::AddGlobalTestEnvironment(env = new DataFileEnv);
	return RUN_ALL_TESTS();
}

