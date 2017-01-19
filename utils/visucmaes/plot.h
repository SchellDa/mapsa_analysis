#ifndef _PLOT_H_
#define _PLOT_H_

#include "qcustomplot.h"
#include "plotdocument.h"

class Plot : public QCustomPlot
{
public:
	explicit Plot(QWidget* parent, size_t id);
	~Plot();
	
	size_t id() const { return _id; }
	void setId(size_t id) { _id = id; }
	bool setConfig(const PlotDocument::global_config_t config);

public slots:
	void refresh();

private:
	struct curve_t
	{
		QCPGraph* graph;
		QCPStatisticalBox* statbox;
		const PlotDocument::curve_config_t* config;
	};
	size_t _id;
	const PlotDocument::plot_config_t* _config;
	PlotDocument::global_config_t _global;
	QVector<curve_t> _curves;
};

#endif//_PLOT_H_
