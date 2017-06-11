
#ifndef REF_PRE_ALIGN_H
#define REF_PRE_ALIGN_H

#include "mergedanalysis.h"
#include <TH1F.h>

class RefPreAlign : public core::MergedAnalysis
{
public:
	RefPreAlign();
	virtual ~RefPreAlign();

	virtual void init();
	virtual void run(const core::run_data_t& run);
	virtual void finalize();

private:
	TFile* _file;
	TH1F* _refResX;
	TH1F* _refResY;
	TH1F* _dutResX;
	TH1F* _dutResY;
};

#endif//REF_PRE_ALIGN_H
