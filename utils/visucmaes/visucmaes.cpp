
#include "visucmaes.h"
#include "ui_visucmaes.h"
#include "plot.h"
#include "plotdocument.h"
#include <cassert>

Visucmaes::Visucmaes(QWidget* parent) :
 QMainWindow(parent),
 ui(new Ui::MainWindow()),
 _doc(new PlotDocument(this)),
 _maxCreatedPlotId(0)
{
	ui = new Ui::MainWindow();
	ui->setupUi(this);
	connect(_doc, &PlotDocument::changedFilename, [this](QString fname) {
		setWindowTitle(QStringLiteral("VisuCMAES - ")+fname);
	});
	connect(_doc, &PlotDocument::changedConfig, this, &Visucmaes::createAndDeletePlots);
	_doc->newDocument();
}

Visucmaes::~Visucmaes()
{
}

void Visucmaes::createPlot()
{
	_doc->addPlot();
}

void Visucmaes::createAndDeletePlots()
{
	auto config = _doc->config();
	auto windows = ui->mdiArea->subWindowList();
	for(auto win: windows) {
		if(!dynamic_cast<Plot*>(win->widget())->setConfig(config)) {
			qDebug() << "Close mdi subwindow.";
			win->close();
		}
	}
	for(const auto& plot: config.plots) {
		qDebug() << plot.id << _maxCreatedPlotId;
		if(plot.id > _maxCreatedPlotId) {
			qDebug() << "Add new plot window.";
			_maxCreatedPlotId = plot.id;
			auto win = new Plot(this, plot.id);
			ui->mdiArea->addSubWindow(win);
			win->show();
			bool result = win->setConfig(config);
			assert(result = true);
		}
	}
}
