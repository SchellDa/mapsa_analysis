

#include "mpa_cluster_test.h"
#include "mpahitgenerator.h"
#include <iostream>
#include <TImage.h>
#include <TCanvas.h>
#include <TFitResult.h>
#include <fstream>
#include "mpatransform.h"
#include <TF1.h>
#include <algorithm>
#include <TGraph.h>
#include <cmath>

REGISTER_ANALYSIS_TYPE(MpaClusterTest, "Calculate clustering properties of specified runs")

MpaClusterTest::MpaClusterTest()
{
}

MpaClusterTest::~MpaClusterTest()
{
}

void MpaClusterTest::init()
{
	_file = new TFile(getRootFilename().c_str(), "recreate");
	_clusterMap = new TH2F("cluster_map", "Cluster Positions", 160, 0, 16, 60, -3, 3);
	_clusterAreas = new TH1F("cluster_area", "Area of Cluster Pixels", 1000, 0, 0.14*5);
	_clusterSizes = new TH1F("cluster_size", "Number of Pixels in Cluster", 20, 0, 20);
	_clusterNormalizedArea = new TH1F("cluster_normalized_area", "Area / Num Pixels", 10000, 0.14, 0.35);
	_dutTelCorrelationX = new TH2F("dut_tel_correlation_x", "Plane 3 <-> DUT Correlation", 50, -5, 5, 50, -5, 5);
	_dutTelCorrelationY = new TH2F("dut_tel_correlation_y", "Plane 3 <-> DUT Correlation", 50, -5, 5, 50, -5, 5);
}

void MpaClusterTest::run(const core::run_data_t& run)
{
	std::vector<int> sizes;
	std::vector<double> areas;
	core::MpaTransform transform;
	transform.setOffset(Eigen::Vector3d{0, 0, 385});
	transform.setRotation({0, 0, 3.1415 / 180 * 90});
	for(size_t evt = 0; evt < run.tree->GetEntries(); ++evt) {
		run.tree->GetEntry(evt);
		auto clusters = core::MpaHitGenerator::getCounterClustersLocal(run, transform, &sizes, &areas);
		std::vector<Eigen::Vector3d> hits;
		for(auto& cluster: clusters) {
			hits.push_back(transform.pixelCoordToGlobal(cluster));
		}
		for(size_t i = 0; i < clusters.size(); ++i) {
			_clusterMap->Fill(clusters[i](0), clusters[i](1));
			_clusterAreas->Fill(areas[i]);
			_clusterSizes->Fill(sizes[i]);
			_clusterNormalizedArea->Fill(areas[i] / sizes[i]);
		}
		for(auto hit: hits) {
			auto& telData = (*run.telescopeHits)->p3;
			for(size_t j = 0; j < telData.x.GetNoElements(); ++j) {
				_dutTelCorrelationX->Fill(hit(0), telData.x[j]);
				_dutTelCorrelationY->Fill(hit(1), telData.y[j]);
			}
		}
	}
}

void MpaClusterTest::finalize()
{
	if(_file) {
		_file->Write();
		_file->Close();
	}
}

