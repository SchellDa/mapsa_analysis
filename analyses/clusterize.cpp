
#include "clusterize.h"
#include <list>
#include <algorithm>
#include <TCanvas.h>
#include <TImage.h>

REGISTER_ANALYSIS_TYPE(Clusterize, "Textual analysis description here.")

Clusterize::Clusterize() :
 Analysis(), _file(nullptr), _aligner(), _clusterSizeHist(nullptr)
{
	addProcess("clusterize", CS_ALWAYS /* CS_TRACK */,
	           std::bind(&Clusterize::clusterize, this, std::placeholders::_1, std::placeholders::_2),
	           std::bind(&Clusterize::finishClusterize, this)
	           );
	addProcess("align", CS_TRACK,
	           std::bind(&Clusterize::align, this, std::placeholders::_1, std::placeholders::_2),
	           std::bind(&Clusterize::finishAlign, this)
	           );
	addProcess("cutClusterSize", CS_TRACK,
	           std::bind(&Clusterize::cutClusterSize, this, std::placeholders::_1, std::placeholders::_2),
	           std::bind(&Clusterize::finishCutClusterSize, this)
	           );
	getOptionsDescription().add_options()
		("align,a", "Force calculation of alignemnt data")
	;
}

Clusterize::~Clusterize()
{
	_clusterFile.close();
	if(_file) {
		_file->Flush();
		_file->Close();
		delete _file;
	}
}

void Clusterize::init(const po::variables_map& vm)
{
	_clusterFile.open(getFilename(".csv"));
	_aligner.setNSigma(_config.get<double>("n_sigma_cut"));
	_file = new TFile(getRootFilename().c_str(), "RECREATE");
	_clusterSizeHist = new TH1F("clusterSizeHist", "Cluster Sizes", 3*16, 0, 3*16);
	_cutClusterSizeHist = new TH1F("cutClusterSizeHist", "Cut Cluster Sizes", 3*16, 0, 3*16);
	if(vm.count("align") < 1) {
		_aligner.loadAlignmentData(getFilename(".align"));
	}
	if(!_aligner.gotAlignmentData()) {
		_aligner.initHistograms();
	}
}

std::string Clusterize::getUsage(const std::string& argv0) const
{
	return Analysis::getUsage(argv0);
}

std::string Clusterize::getHelp(const std::string& argv0) const
{
        return Analysis::getHelp(argv0);
}

bool Clusterize::clusterize(const core::TrackStreamReader::event_t& track_event,
                      const core::MPAStreamReader::event_t& mpa_event)
{
	event_t evt;
	evt.eventNumber = mpa_event.eventNumber;
	std::unordered_map<Eigen::Vector2i, int> pixels;
	for(size_t idx = 0; idx < mpa_event.data.size(); ++idx) {
		auto coord = _mpaTransform.translatePixelIndex(idx);
		if(mpa_event.data[idx] > 0)
			pixels[coord] = mpa_event.data[idx];
	}
	while(pixels.size()) {
		auto cluster = getCluster(pixels);
		double mass = 0.0;
		cluster_t clst;
		for(const auto& hit: cluster) {
			clst.center = clst.center + hit.first.cast<double>();
			clst.points[hit.first] = hit.second;
			mass += 1.0;
			// mass += hit.second;
			auto num_removed = pixels.erase(hit.first);
			assert(num_removed == 1);
		}
		_clusterSizeHist->Fill(cluster.size());
		clst.center /= mass;
		_clusterFile << clst.center(0) << "\t" << clst.center(1) << "\n";
		for(const auto& hit: cluster)
			_clusterFile << hit.first(0) << "\t" << hit.first(1) << "\n";
		_clusterFile << "\n\n";
		evt.clusters.push_back(clst);
	}
	_clusterFile.flush();
	_eventData[evt.eventNumber] = evt;
	return true;
}

void Clusterize::finishClusterize()
{
	auto canvas = new TCanvas("canvas", "", 800, 600);
	canvas->SetLogy();
	_clusterSizeHist->GetXaxis()->SetTitle("size");
	_clusterSizeHist->GetYaxis()->SetTitle("count");
	_clusterSizeHist->Draw();
	_clusterSizeHist->Write();
	
	auto img = TImage::Create();
	img->FromPad(canvas);
	img->WriteImage(getFilename("_size.png").c_str());
	delete img;
}

bool Clusterize::align(const core::TrackStreamReader::event_t& track_event,
                       const core::MPAStreamReader::event_t& mpa_event)
{
	if(_aligner.gotAlignmentData()) {
		return false;
	}
	const auto& clusters = _eventData[track_event.eventNumber].clusters;
	for(const auto& track: track_event.tracks) {
		auto b = track.extrapolateOnPlane(4, 5, 840, 2);
		for(const auto& cluster: clusters) {
			auto a = _mpaTransform.pixelCoordToGlobal(cluster.center);
			_aligner.Fill(b(0) - a(0), b(1) - a(1));
		}
	}
	return true;
}

void Clusterize::finishAlign()
{
	_aligner.calculateAlignment();
	_aligner.writeHistogramImage(getFilename("_align.png"));
	_aligner.writeHistograms();
	_aligner.saveAlignmentData(getFilename(".align"));
	auto offset = _mpaTransform.getOffset();
	offset += _aligner.getOffset();
	auto cuts = _aligner.getCuts();
	std::cout << "x_off = " << offset(0)
	          << "\ny_off = " << offset(1)
		  << "\nz_off = " << offset(2)
	          << "\nx_sigma = " << cuts(0)
	          << "\ny_width = " << cuts(1) << std::endl;
	_mpaTransform.setOffset(offset);
}

bool Clusterize::cutClusterSize(const core::TrackStreamReader::event_t& track_event,
                       const core::MPAStreamReader::event_t& mpa_event)
{
	const auto& clusters = _eventData[track_event.eventNumber].clusters;
	for(const auto& track: track_event.tracks) {
		auto b = track.extrapolateOnPlane(4, 5, 840, 2);
		for(const auto& cluster: clusters) {
			auto a = _mpaTransform.pixelCoordToGlobal(cluster.center);
			if(_aligner.pointsCorrelated(a, b)) {
				_cutClusterSizeHist->Fill(cluster.points.size());
			}
		}
	}
	return true;
}

void Clusterize::finishCutClusterSize()
{
	auto canvas = new TCanvas("canvas2", "", 800, 600);
	canvas->SetLogy();
	_cutClusterSizeHist->GetXaxis()->SetTitle("size");
	_cutClusterSizeHist->GetYaxis()->SetTitle("count");
	_cutClusterSizeHist->Draw();
	_cutClusterSizeHist->Write();
	
	auto img = TImage::Create();
	img->FromPad(canvas);
	img->WriteImage(getFilename("_cutsize.png").c_str());
	delete img;
	delete canvas;
}

std::unordered_map<Eigen::Vector2i, int> Clusterize::getCluster(const std::unordered_map<Eigen::Vector2i, int>& pixels) const
{
	assert(pixels.size() > 0);
	std::list<Eigen::Vector2i> in_cluster, queue;
	queue.push_back(pixels.begin()->first);
	while(queue.size()) {
		auto next = queue.front();
		queue.pop_front();
		in_cluster.push_back(next);
		for(int i=0; i<4; i++) {
			Eigen::Vector2i neigh(next);
			if(i == 0) neigh += Eigen::Vector2i(-1, 0);
			else if(i == 1) neigh += Eigen::Vector2i(1, 0);
			else if(i == 2) neigh += Eigen::Vector2i(0, -1);
			else if(i == 3) neigh += Eigen::Vector2i(0, 1);
			try {
				pixels.at(neigh);
			} catch(std::out_of_range& e) {
				continue;
			}
			// if pixel is not yet in cluster and is not yet in the queue
			if(std::find(in_cluster.begin(), in_cluster.end(), neigh) == in_cluster.end()) {
				if(std::find(queue.begin(), queue.end(), neigh) == queue.end())
					queue.push_back(neigh);
			}
		}
	}
	std::unordered_map<Eigen::Vector2i, int> cluster;
	for(const auto& coord: in_cluster) {
		cluster[coord] = pixels.at(coord);
	}
	return cluster;
}
