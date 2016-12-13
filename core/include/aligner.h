#ifndef ALIGNER_H
#define ALIGNER_H

#include "mpatransform.h"
#include <Eigen/Dense>

class TH1D;

namespace core
{

class Aligner
{
public:
	struct histogram_cfg_t
	{
		double min;
		double max;
		int nbins;
	};

	Aligner();
	TH1D* getHistX() const;
	TH1D* getHistY() const;

	void setNSigma(const double& nsigma) { _nsigma = nsigma; }
	double getNSigma() const { return _nsigma; }

	void initHistograms(const std::string& xname="alignHistX", const std::string& yname="alignHistY");
	void writeHistograms();
	void writeHistogramImage(const std::string& filename);
	void calculateAlignment(const bool& quiet=false);

	void Fill(const double& xdiff, const double& ydiff);
	void clear();

	Eigen::Vector3d getOffset() const;
	Eigen::Vector2d getCuts() const;

	bool pointsCorrelated(const Eigen::Vector3d& a, const Eigen::Vector3d& b) const
	{
		return pointsCorrelated(Eigen::Vector2d{a(0), a(1)}, Eigen::Vector2d(b(0), b(1)));
	}
	bool pointsCorrelatedX(const Eigen::Vector3d& a, const Eigen::Vector3d& b) const
	{
		return pointsCorrelatedX(a(0), b(0));
	}
	bool pointsCorrelatedY(const Eigen::Vector3d& a, const Eigen::Vector3d& b) const
	{
		return pointsCorrelatedY(a(1), b(1));
	}
	bool pointsCorrelatedX(const Eigen::Vector2d& a, const Eigen::Vector2d& b) const
	{
		return pointsCorrelatedX(a(0), b(0));
	}
	bool pointsCorrelatedY(const Eigen::Vector2d& a, const Eigen::Vector2d& b) const
	{
		return pointsCorrelatedY(a(1), b(1));
	}
	
	bool pointsCorrelated(const Eigen::Vector2d& a, const Eigen::Vector2d& b) const;
	bool pointsCorrelatedX(const double& a, const double& b) const;
	bool pointsCorrelatedY(const double& a, const double& b) const;

	bool loadAlignmentData(const std::string& filename);
	void saveAlignmentData(const std::string& filename, const std::string& extra="") const;
	void appendAlignmentData(const std::string& filename, const std::string& extra="") const;

	bool gotAlignmentData() const { return _calculated; }

	histogram_cfg_t xHistogramConfig;
	histogram_cfg_t yHistogramConfig;

	void setCuts(Eigen::Vector2d cuts)
	{
		_cuts = cuts;
	}

	void setFixedMean(const bool& fixed) { _fixedMean = fixed; }

private:
	bool rebinIfNeccessary(TH1D* cor, const double& nrms, const double& binratio);
	Eigen::Vector2d alignPlateau(TH1D* cor, const double& nrms, const double& binratio, const bool& quiet, const bool& fixedMean=false);
	Eigen::Vector2d alignGaussian(TH1D* cor, const double& nrms, const double& binratio, const bool& quiet, const bool& fixedMean=false);
	double _nsigma;
	TH1D* _alignX;
	TH1D* _alignY;
	bool _calculated;
	Eigen::Vector3d _offset;
	Eigen::Vector2d _cuts;
	bool _fixedMean;
};

}//ns core

#endif//ALIGNER_H
