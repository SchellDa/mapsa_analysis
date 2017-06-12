

#ifndef MPA_CLUSTER_TEST_H
#define MPA_CLUSTER_TEST_H

#include "mergedanalysis.h"
#include <TH1F.h>
#include <TH2F.h>
#include "triplettrack.h"

class MpaClusterTest : public core::MergedAnalysis
{
public:
	MpaClusterTest();
	virtual ~MpaClusterTest();

	virtual void init();
	virtual void run(const core::run_data_t& run);
	virtual void finalize();

private:
	TFile* _file;
	TH2F* _clusterMap;
	TH1F* _clusterAreas;
	TH1F* _clusterNormalizedArea;
	TH1F* _clusterSizes;
	TH2F* _dutTelCorrelationX;
	TH2F* _dutTelCorrelationY;
};

#endif//MPA_TRIPLET_EFFICIENCY_H
