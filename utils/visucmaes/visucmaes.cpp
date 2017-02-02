
#include "visucmaes.h"
#include "ui_visucmaes.h"
#include "plot.h"
#include "plotdocument.h"
#include "curvesettings.h"
#include "plotsettings.h"
#include "globalsettings.h"
#include "view3d.h"
#include <cassert>
#include <QFileDialog>
#include <QColorDialog>
#include <QProgressBar>
#include <QApplication>
#include "ui_view3d.h"
#include <QDoubleSpinBox>

Visucmaes::Visucmaes(QWidget* parent) :
 QMainWindow(parent),
 ui(new Ui::MainWindow()),
 _doc(new PlotDocument(this)),
 _progress(new QProgressBar(this)),
 _view3D(new View3D(this))
{
	ui = new Ui::MainWindow();
	ui->setupUi(this);
	_doc->createActions(ui->menuEdit);
	ui->statusbar->addPermanentWidget(_progress);
	_progress->setVisible(false);
	connect(_doc, &PlotDocument::changedFilename, this, &Visucmaes::generateWindowTitle);
	connect(_doc, &PlotDocument::forceRefresh, this, &Visucmaes::forceRefresh);
	connect(_doc->undoStack, &QUndoStack::cleanChanged, [=](bool clean) { setWindowModified(!clean); });
	connect(_doc->undoStack, &QUndoStack::cleanChanged, ui->actionSave, &QAction::setDisabled);
	ui->actionSave->setDisabled(_doc->undoStack->isClean());
	connect(_doc, &PlotDocument::changedConfig, this, &Visucmaes::createAndDeletePlots);
	connect(ui->actionView3D, &QAction::triggered, _view3D, &View3D::setVisible);
	connect(_view3D, &View3D::visibleChanged, ui->actionView3D, &QAction::setChecked);
	connect(_view3D->ui->x, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Visucmaes::setParameterSelectionX);
	connect(_view3D->ui->y, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Visucmaes::setParameterSelectionY);
	connect(_view3D->ui->z, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Visucmaes::setParameterSelectionZ);
	connect(_view3D->ui->phi, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Visucmaes::setParameterSelectionPhi);
	connect(_view3D->ui->theta, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Visucmaes::setParameterSelectionTheta);
	connect(_view3D->ui->omega, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Visucmaes::setParameterSelectionOmega);
	connect(&_doc->db, &Database::cacheChanged, _view3D, &View3D::setCache);
	connectPlotActions();
	changeSubwindowSelection(nullptr);
	_doc->newDocument();
}

Visucmaes::~Visucmaes()
{
	delete ui;
}

void Visucmaes::generateWindowTitle()
{
	QString title = QStringLiteral("VisuCMAES [*] - ")+_doc->filename();
	setWindowTitle(title);
}

void Visucmaes::newDocument()
{
	if(!_doc->undoStack->isClean()) {
		auto button = QMessageBox::warning(this,
			tr("Save Plot Config?"),
			tr("Do you want to save your changes before proceeding?\n\n"
			   "If you don't save, your changes will be lost."),
			QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		if(button == QMessageBox::Save) {
			save();
			_doc->newDocument();
		} else if(button == QMessageBox::Discard) {
			_doc->newDocument();
		}
	} else {
		_doc->newDocument();
	}
}

void Visucmaes::open()
{
	bool openDoc = true;
	if(!_doc->undoStack->isClean()) {
		auto button = QMessageBox::warning(this,
			tr("Save Plot Config?"),
			tr("Do you want to save your changes before proceeding?\n\n"
			   "If you don't save, your changes will be lost."),
			QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		if(button == QMessageBox::Save) {
			save();
		}
		if(button == QMessageBox::Cancel) {
			openDoc = false;
		}
	}
	if(openDoc) {
		auto fname = QFileDialog::getOpenFileName(this,
			tr("Open Plot Config"), QStringLiteral(""),
			QString("Plot Config (*.plc)"), nullptr);
		if(fname.size() > 0) {
			_doc->open(fname);
		}
	}
}

void Visucmaes::save()
{
	if(_doc->hasFilename()) {
		_doc->save();
	} else {
		auto fname = QFileDialog::getSaveFileName(this,
			tr("Save Plot Config"), QStringLiteral(""),
			QString("Plot Config (*.plc)"), nullptr);
		if(fname.size() > 0) {
			_doc->saveAs(fname);
		}
	}
}
void Visucmaes::saveAs()
{
	auto fname = QFileDialog::getSaveFileName(this,
		tr("Save Plot Config As..."), QStringLiteral(""),
		QString("Plot Config (*.plc)"), nullptr);
	if(fname.size() > 0) {
		_doc->saveAs(fname);
	}
}

void Visucmaes::openDatabase()
{
	auto fname = QFileDialog::getOpenFileName(this,
		tr("Open Database"), QStringLiteral(""),
		QString("Database (*.sqldat *.sqlite *.database);;All (*.*)"), nullptr);
	if(fname.size() > 0) {
		try {
			_doc->db.open(fname);
		} catch(Database::database_error& e) {
			qDebug() << "Database Error" << e.what();
			QMessageBox::critical(this,
			tr("Database Error"),
			e.what(),
			QMessageBox::Ok);
		}
	}
}

void Visucmaes::changeSubwindowSelection(QMdiSubWindow* win)
{
	bool enable = win != nullptr;
	ui->actionPlotEdit->setEnabled(enable);
	ui->actionPlotClone->setEnabled(enable);
	ui->actionPlotDelete->setEnabled(enable);
	ui->actionCurveAdd->setEnabled(enable);
	ui->actionCurveEdit->setEnabled(false);
	ui->actionCurveClone->setEnabled(false);
	ui->actionCurveDelete->setEnabled(false);
	ui->actionExportPlot->setEnabled(enable);
	if(win != nullptr) {
		currentPlot()->deselectAll();
	}
}

void Visucmaes::untabMdiArea()
{
	ui->actionWindowsTabbed->setChecked(false);
}

void Visucmaes::setTabbed(bool tabbed)
{
	if(tabbed) {
		ui->mdiArea->setViewMode(QMdiArea::TabbedView);
	} else {
		ui->mdiArea->setViewMode(QMdiArea::SubWindowView);
	}
}

void Visucmaes::setParameterSelectionX(double par)
{
	_view3D->ui->x->setValue(par);
	auto windows = ui->mdiArea->subWindowList();
	for(auto win: windows) {
		auto plot = dynamic_cast<Plot*>(win->widget());
		plot->setParameterSelectionX(par);
	}
}

void Visucmaes::setParameterSelectionY(double par)
{
	_view3D->ui->y->setValue(par);
	auto windows = ui->mdiArea->subWindowList();
	for(auto win: windows) {
		auto plot = dynamic_cast<Plot*>(win->widget());
		plot->setParameterSelectionY(par);
	}
}

void Visucmaes::setParameterSelectionZ(double par)
{
	_view3D->ui->z->setValue(par);
	auto windows = ui->mdiArea->subWindowList();
	for(auto win: windows) {
		auto plot = dynamic_cast<Plot*>(win->widget());
		plot->setParameterSelectionZ(par);
	}
}

void Visucmaes::setParameterSelectionPhi(double par)
{
	_view3D->ui->phi->setValue(par);
	auto windows = ui->mdiArea->subWindowList();
	for(auto win: windows) {
		auto plot = dynamic_cast<Plot*>(win->widget());
		plot->setParameterSelectionPhi(par);
	}
}

void Visucmaes::setParameterSelectionTheta(double par)
{
	_view3D->ui->theta->setValue(par);
	auto windows = ui->mdiArea->subWindowList();
	for(auto win: windows) {
		auto plot = dynamic_cast<Plot*>(win->widget());
		plot->setParameterSelectionTheta(par);
	}
}

void Visucmaes::setParameterSelectionOmega(double par)
{
	_view3D->ui->omega->setValue(par);
	auto windows = ui->mdiArea->subWindowList();
	for(auto win: windows) {
		auto plot = dynamic_cast<Plot*>(win->widget());
		plot->setParameterSelectionOmega(par);
	}
}

void Visucmaes::connectPlotActions()
{
	connect(ui->actionPlotAdd, &QAction::triggered, _doc, &PlotDocument::addPlot);
	connect(ui->actionPlotEdit, &QAction::triggered, [this](){
		PlotSettings dialog(this, _doc, currentPlot()->id());
		dialog.exec();
	});
	connect(ui->actionPlotClone, &QAction::triggered, [this](){
		assert(currentPlot() != nullptr);
		assert(_doc != nullptr);
		_doc->clonePlot(currentPlot()->id());
	});
	connect(ui->actionPlotDelete, &QAction::triggered, [this](){
		assert(currentPlot() != nullptr);
		assert(_doc != nullptr);
		_doc->deletePlot(currentPlot()->id());
	});
	connect(ui->actionCurveAdd, &QAction::triggered, [this]() {
		assert(currentPlot() != nullptr);
		assert(_doc != nullptr);
		_doc->addCurve(currentPlot()->id());
	});
	connect(ui->actionCurveEdit, &QAction::triggered, [this](){
		const auto config = currentPlot()->selectedCurve()->config;
		assert(config);
		CurveSettings dialog(this, _doc, config->plot_id, config->id);
		dialog.exec();
	});
	connect(ui->actionCurveClone, &QAction::triggered, [this](){
		const auto config = currentPlot()->selectedCurve()->config;
		_doc->cloneCurve(config->plot_id, config->id);
	});
	connect(ui->actionCurveDelete, &QAction::triggered, [this](){
		const auto config = currentPlot()->selectedCurve()->config;
		assert(config);
		_doc->deleteCurve(config->plot_id, config->id);
	});
	connect(ui->actionGlobalJobQueryEdit, &QAction::triggered, [this](){
		GlobalSettings dialog(this, _doc);
		dialog.exec();
	});
}

Plot* Visucmaes::currentPlot() const
{
	return dynamic_cast<Plot*>(ui->mdiArea->currentSubWindow()->widget());
}

void Visucmaes::createAndDeletePlots()
{
	_progress->setVisible(true);
	size_t numCurves = 0;
	for(const auto& plot: _doc->config().plots) {
		for(const auto& curve: plot.curves) {
			++numCurves;
		}
	}
	_progress->setRange(0, numCurves);
	_progress->setValue(0);
	setEnabled(false);
	repaint();
	QApplication::processEvents();
	auto config = _doc->config();
	auto windows = ui->mdiArea->subWindowList();
	QVector<size_t> availablePlotIds;
	for(auto win: windows) {
		auto plot = dynamic_cast<Plot*>(win->widget());
		availablePlotIds.push_back(plot->id());
		if(!plot->setConfig(config)) {
			qDebug() << "Close mdi subwindow.";
			win->close();
		}
	}
	for(const auto& plot: config.plots) {
		if(availablePlotIds.indexOf(plot.id) == -1) {
			auto win = new Plot(this, plot.id, _doc);
			connect(win, &Plot::selectedParameterX, this, &Visucmaes::setParameterSelectionX);
			connect(win, &Plot::selectedParameterY, this, &Visucmaes::setParameterSelectionY);
			connect(win, &Plot::selectedParameterZ, this, &Visucmaes::setParameterSelectionZ);
			connect(win, &Plot::selectedParameterPhi, this, &Visucmaes::setParameterSelectionPhi);
			connect(win, &Plot::selectedParameterTheta, this, &Visucmaes::setParameterSelectionTheta);
			connect(win, &Plot::selectedParameterOmega, this, &Visucmaes::setParameterSelectionOmega);
			connect(win, &Plot::selectionChanged, ui->actionCurveEdit, &QAction::setEnabled);
			connect(win, &Plot::selectionChanged, ui->actionCurveDelete, &QAction::setEnabled);
			connect(win, &Plot::selectionChanged, ui->actionCurveClone, &QAction::setEnabled);
			connect(win, &Plot::curveProcessed, [this]() {
				_progress->setValue(_progress->value()+1);
				repaint();
				_progress->repaint();
			});
			ui->mdiArea->addSubWindow(win);
			win->show();
			bool result = win->setConfig(config);
			assert(result = true);
		}
	}
	_progress->setVisible(false);
	setEnabled(true);
	repaint();
}

void Visucmaes::forceRefresh()
{
	_progress->setVisible(true);
	setEnabled(false);
	repaint();
	QApplication::processEvents();
	size_t numCurves = 0;
	for(const auto& plot: _doc->config().plots) {
		for(const auto& curve: plot.curves) {
			++numCurves;
		}
	}
	_progress->setRange(0, numCurves);
	_progress->setValue(0);
	repaint();
	auto windows = ui->mdiArea->subWindowList();
	for(auto win: windows) {
		auto plot = dynamic_cast<Plot*>(win->widget());
		plot->forcedRefresh();
	}
	_progress->setVisible(false);
	setEnabled(true);
	repaint();
}

