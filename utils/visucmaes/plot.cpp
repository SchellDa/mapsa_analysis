
#include "plot.h"

Plot::Plot(QWidget* parent, size_t id)
 : QCustomPlot(parent), _id(id)
{
	
}

Plot::~Plot()
{
}

bool Plot::setConfig(const PlotDocument::global_config_t global_config)
{
	_curves.clear();
	clearPlottables();
	_global = global_config;
	bool found = false;
	for(const auto& config: _global.plots) {
		qDebug() << config.id << _id;
		if(config.id == _id) {
			_config = &config;
			found = true;
			break;
		}
	}
	if(!found) {
		return false;
	}
	setWindowTitle(_config->title);
	xAxis->setLabel(_config->xlabel);
	yAxis->setLabel(_config->ylabel);
	for(const auto& curve_cfg: _config->curves) {
		curve_t curve { nullptr, nullptr, &curve_cfg };
		QPen pen;
		pen.setColor(curve.config->color);
		QCPScatterStyle sstyle;
		sstyle.setPen(pen);
		sstyle.setShape(curve.config->shape);
		if(curve.config->mode == PlotDocument::cmStatisticalBox) {
			curve.statbox = new QCPStatisticalBox(xAxis, yAxis);
			curve.statbox->setOutlierStyle(sstyle);
			curve.statbox->setPen(pen);
			curve.statbox->setName(curve.config->title);
		} else {
			curve.graph = new QCPGraph(xAxis, yAxis);
			if(curve.config->mode == PlotDocument::cmStatisticalBox) {
				curve.graph->setAntialiased(false);
				curve.graph->setLineStyle(QCPGraph::lsStepLeft);
			} else {
				if(curve.config->draw_lines) {
					curve.graph->setLineStyle(QCPGraph::lsNone);
				} else {
					curve.graph->setLineStyle(QCPGraph::lsLine);
				}
			}
			curve.graph->setScatterStyle(sstyle);
			curve.graph->setName(curve.config->title);
		}
		_curves.push_back(curve);
	}
	refresh();
	return true;
}

void Plot::refresh()
{
	// Execute query and get write data to plots
	for(const auto& curve: _curves) {
	}
	// apply auto-ranges and (semi-)fixed ranges
	if(!(_config->use_xmin && _config->use_xmax)) {
		xAxis->rescale();
	}
	if(_config->use_xmin) {
		xAxis->setRangeLower(_config->xmin);
	}
	if(_config->use_xmax) {
		xAxis->setRangeUpper(_config->xmax);
	}
	if(!(_config->use_ymin && _config->use_ymax)) {
		yAxis->rescale();
	}
	if(_config->use_ymin) {
		yAxis->setRangeLower(_config->ymin);
	}
	if(_config->use_ymax) {
		yAxis->setRangeUpper(_config->ymax);
	}
	replot();
}

