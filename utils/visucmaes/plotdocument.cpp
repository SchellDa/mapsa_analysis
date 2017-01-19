
#include "plotdocument.h"

PlotDocument::PlotDocument(QObject* parent) :
	QObject(parent),
	_filename(""),
	_filenameSet(false)
{
}

PlotDocument::~PlotDocument()
{
}

void PlotDocument::newDocument()
{
	_filenameSet = false;
	_filename = "Unnamed";
	emit changedFilename(_filename);
}

void PlotDocument::open(QString filename)
{
	_filenameSet = true;
	_filename = filename;
	emit changedFilename(filename);
}

void PlotDocument::save(QString filename)
{
}

void PlotDocument::saveAs(QString filename)
{
	_filenameSet = true;
	_filename = filename;
	emit changedFilename(filename);
	save(filename);
}

void PlotDocument::addPlot()
{
	plot_config_t cfg;
	cfg.id = ++_config.max_plot_id;
	cfg.title = tr("New Plot", "Title of a new plot");
	cfg.xlabel = tr("X Axis", "Name of new axis.");
	cfg.ylabel = tr("Y Axis", "Name of new axis.");
	cfg.xmin = cfg.ymin = 0;
	cfg.xmax = cfg.ymax = 10;
	cfg.use_xmin = cfg.use_ymin = cfg.use_xmax = cfg.use_ymax = false;
	cfg.legend = true;
	cfg.job_query = "";
	_config.plots.push_back(cfg);
	emit changedConfig(_config);
}
