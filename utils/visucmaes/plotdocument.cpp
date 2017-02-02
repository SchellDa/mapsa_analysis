
#include "plotdocument.h"
#include <cassert>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QtGlobal>

PlotDocument::PlotDocument(QObject* parent) :
	QObject(parent),
	db(nullptr),
	undoStack(new QUndoStack(this)),
	_filename(""),
	_filenameSet(false)
{
	connect(&db, QOverload<>::of(&Database::opened), [this]() {
		emit forceRefresh();
	});
}

PlotDocument::~PlotDocument()
{
}

void PlotDocument::createActions(QMenu* parent)
{
	QIcon iconUndo;
        iconUndo.addFile(QStringLiteral(":/icons/icons/arrow_undo.png"), QSize(), QIcon::Normal, QIcon::Off);
	QIcon iconRedo;
        iconRedo.addFile(QStringLiteral(":/icons/icons/arrow_redo.png"), QSize(), QIcon::Normal, QIcon::Off);
	auto actionRedo = undoStack->createRedoAction(parent, tr("Redo", "The Redo action"));
        actionRedo->setIcon(iconRedo);
	actionRedo->setShortcut(QKeySequence(tr("Ctrl+Shift+Z", "Edit|Redo")));
	auto actionUndo = undoStack->createUndoAction(parent, tr("Undo", "The Undo action"));
        actionUndo->setIcon(iconUndo);
	actionUndo->setShortcut(QKeySequence(tr("Ctrl+Z", "Edit|Undo")));
	parent->addAction(actionUndo);
	parent->addAction(actionRedo);
}

void PlotDocument::applyConfig(global_config_t cfg, QString changeDescription)
{
	auto cmd = new UndoCommand(this, _config, cfg, changeDescription);
	undoStack->push(cmd);
}

void PlotDocument::setConfig(global_config_t cfg)
{
	_config = cfg;
	emit changedConfig(_config);
}

void PlotDocument::newDocument()
{
	_filenameSet = false;
	_filename = tr("Unnamed Document");
	_config.max_plot_id = 0;
	_config.global_query = "SELECT job_id FROM jobs";
	_config.plots.clear();
	undoStack->clear();
	undoStack->setClean();
	emit changedFilename(_filename);
	emit changedConfig(_config);
}

void PlotDocument::open(QString filename)
{
	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qDebug() << "Cannot open file" << filename << " for reading config!";
		return;
	}
	_filenameSet = true;
	_filename = filename;
	_config.plots.clear();
	QJsonParseError error;
	auto doc = QJsonDocument::fromJson(file.readAll(), &error);
	if(doc.isNull()) {
		qDebug() << "Error while parsing JSON config " << _filename << error.errorString();
		return;
	}
	if(!doc.isObject()) {
		qDebug() << "Invalid JSON layout: Root is not object";
		return;
	}
	auto root = doc.object();
	_config.global_query = root.value("query").toString();
	_config.max_plot_id = root.value("max_plot_id").toInt();
	for(const auto& plotref: root.value("plots").toArray()) {
		QJsonObject plot(plotref.toObject());
		plot_config_t pcfg;
		pcfg.id = static_cast<size_t>(plot.value("id").toInt());
		pcfg.max_curve_id = static_cast<size_t>(plot.value("max_curve_id").toInt());
		pcfg.title = plot.value("title").toString();
		pcfg.xlabel = plot.value("xlabel").toString();
		pcfg.ylabel = plot.value("ylabel").toString();
		pcfg.xmin = plot.value("xmin").toDouble();
		pcfg.xmax = plot.value("xmax").toDouble();
		pcfg.ymin = plot.value("ymin").toDouble();
		pcfg.ymax = plot.value("ymax").toDouble();
		pcfg.use_xmin = plot.value("use_xmin").toBool();
		pcfg.use_xmax = plot.value("use_xmax").toBool();
		pcfg.use_ymin = plot.value("use_ymin").toBool();
		pcfg.use_ymax = plot.value("use_ymax").toBool();
		pcfg.legend = plot.value("legend").toBool();
		pcfg.job_query = plot.value("job_query").toString();
		pcfg.xlog = plot.value("xlog").toBool();
		pcfg.ylog = plot.value("xlog").toBool();
		pcfg.selection_x = static_cast<axis_parameter_t>(plot.value("selection_x").toInt());
		pcfg.selection_y = static_cast<axis_parameter_t>(plot.value("selection_y").toInt());
		for(const auto& curveref: plot.value("curves").toArray()) {
			QJsonObject curve(curveref.toObject());
			curve_config_t ccfg;
			ccfg.id = static_cast<size_t>(curve.value("id").toInt());
			ccfg.plot_id = pcfg.id;
			ccfg.title = curve.value("title").toString();
			ccfg.query = curve.value("query").toString();
			ccfg.mode = static_cast<curve_mode_t>(curve.value("mode").toInt());
			ccfg.draw_lines = curve.value("draw_lines").toBool();
			ccfg.shape = static_cast<QCPScatterStyle::ScatterShape>(curve.value("shape").toInt());
			ccfg.color = QColor(curve.value("color").toString());
			ccfg.hist_nbins = static_cast<size_t>(curve.value("hist_bins").toInt());
			ccfg.hist_low = curve.value("hist_low").toDouble();
			ccfg.hist_high = curve.value("hist_high").toDouble();
			pcfg.curves.push_back(ccfg);
		}
		_config.plots.push_back(pcfg);
	}

	undoStack->clear();
	undoStack->setClean();
	emit changedFilename(filename);
	emit changedConfig(_config);
}

void PlotDocument::save()
{
	assert(_filenameSet == true);
	undoStack->setClean();
	QJsonObject global_config;
	global_config.insert("query", QJsonValue(_config.global_query));
	global_config.insert("max_plot_id", static_cast<qint64>(_config.max_plot_id));
	QJsonArray plots;
	for(const auto& plot: _config.plots) {
		QJsonObject plot_config;
		plot_config.insert("id", static_cast<qint64>(plot.id));
		plot_config.insert("max_curve_id", static_cast<qint64>(plot.max_curve_id));
		plot_config.insert("title", QJsonValue(plot.title));
		plot_config.insert("xlabel", QJsonValue(plot.xlabel));
		plot_config.insert("ylabel", QJsonValue(plot.ylabel));
		plot_config.insert("xmin", QJsonValue(plot.xmin));
		plot_config.insert("xmax", QJsonValue(plot.xmax));
		plot_config.insert("ymin", QJsonValue(plot.ymin));
		plot_config.insert("ymax", QJsonValue(plot.ymax));
		plot_config.insert("use_xmin", QJsonValue(plot.use_xmin));
		plot_config.insert("use_xmax", QJsonValue(plot.use_xmax));
		plot_config.insert("use_ymin", QJsonValue(plot.use_ymin));
		plot_config.insert("use_ymax", QJsonValue(plot.use_ymax));
		plot_config.insert("legend", QJsonValue(plot.legend));
		plot_config.insert("job_query", QJsonValue(plot.job_query));
		plot_config.insert("xlog", plot.xlog);
		plot_config.insert("ylog", plot.ylog);
		plot_config.insert("selection_x", static_cast<int>(plot.selection_x));
		plot_config.insert("selection_y", static_cast<int>(plot.selection_y));
		QJsonArray curves;
		for(const auto& curve: plot.curves) {
			QJsonObject curve_config;
			curve_config.insert("id", static_cast<qint64>(curve.id));
			curve_config.insert("plot_id", static_cast<qint64>(curve.plot_id));
			curve_config.insert("title", curve.title);
			curve_config.insert("query", curve.query);
			curve_config.insert("mode", static_cast<int>(curve.mode));
			curve_config.insert("shape", static_cast<int>(curve.shape));
			curve_config.insert("draw_lines", curve.draw_lines);
			curve_config.insert("color", curve.color.name());
			curve_config.insert("hist_bins", static_cast<int>(curve.hist_nbins));
			curve_config.insert("hist_low", curve.hist_low);
			curve_config.insert("hist_high", curve.hist_high);
			curves.push_back(curve_config);
		}
		plot_config.insert("curves", curves);
		plots.push_back(plot_config);
	}
	global_config.insert("plots", plots);
	QJsonDocument doc(global_config);
	QFile file(_filename);
	if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		qDebug() << "Cannot open file" << _filename << " for saving config!";
		return;
	}
	file.write(doc.toJson());
}

void PlotDocument::saveAs(QString filename)
{
	_filenameSet = true;
	_filename = filename;
	emit changedFilename(filename);
	save();
}

void PlotDocument::addPlot()
{
	plot_config_t cfg;
	global_config_t config(_config);
	cfg.id = ++config.max_plot_id;
	cfg.max_curve_id = 0;
	cfg.title = tr("New Plot", "Title of a new plot");
	cfg.xlabel = tr("X Axis", "Name of new axis.");
	cfg.ylabel = tr("Y Axis", "Name of new axis.");
	cfg.xmin = cfg.ymin = 0;
	cfg.xmax = cfg.ymax = 10;
	cfg.use_xmin = cfg.use_ymin = cfg.use_xmax = cfg.use_ymax = false;
	cfg.legend = true;
	cfg.job_query = "";
	cfg.xlog = false;
	cfg.ylog = false;
	cfg.selection_x = apNone;
	cfg.selection_y = apNone;
	config.plots.push_back(cfg);
	applyConfig(config, tr("add plot", "undo/redo X"));
}

void PlotDocument::editPlot(plot_config_t cfg)
{
	global_config_t config(_config);
	for(size_t i = 0; i < config.plots.size(); ++i) {
		if(config.plots[i].id == cfg.id) {
			config.plots[i] = cfg;
			applyConfig(config, tr("edit plot settings", "undo/redo X"));
			return;
		}
	}
	assert(false);
}

void PlotDocument::clonePlot(size_t plotId)
{
	global_config_t config(_config);
	for(size_t i = 0; i < config.plots.size(); ++i) {
		if(config.plots[i].id == plotId) {
			plot_config_t plot = config.plots[i];
			plot.id = ++config.max_plot_id;
			for(auto& curve: plot.curves) {
				curve.plot_id = plot.id;
			}
			config.plots.push_back(plot);
			applyConfig(config, tr("clone plot", "undo/redo X"));
			return;
		}
	}
	assert(false);
}

void PlotDocument::deletePlot(size_t plotId)
{
	global_config_t config(_config);
	for(size_t i = 0; i < config.plots.size(); ++i) {
		if(config.plots[i].id == plotId) {
			config.plots.remove(i);
			applyConfig(config, tr("delete plot", "undo/redo X"));
			return;
		}
	}
	assert(false);
}

void PlotDocument::addCurve(size_t plotId)
{
	global_config_t config(_config);
	auto& plot = plotById(config, plotId);
	curve_config_t newCurve { 
		++plot.max_curve_id,
		plot.id,
		"New Curve",
		"",
		cmPoints,
		true,
		QCPScatterStyle::ssCross,
		QColor::fromHsv((plot.max_curve_id*25)%255, 255, 255),
		10,
		0.0,
		10.0
	};
	plot.curves.push_back(newCurve);
	applyConfig(config, tr("add curve", "undo/redo X"));
}

void PlotDocument::editCurve(curve_config_t cfg)
{
	global_config_t config(_config);
	curveById(config, cfg.plot_id, cfg.id) = cfg;
	applyConfig(config, tr("edit curve settings", "undo/redo X"));
}

void PlotDocument::cloneCurve(size_t plotId, size_t curveId)
{
	global_config_t config(_config);
	auto& plot = plotById(config, plotId);
	curve_config_t newCurve;
	for(size_t i=0; i < plot.curves.size(); ++i) {
		if(plot.curves[i].id == curveId) {
			newCurve = plot.curves[i];
			newCurve.id = ++plot.max_curve_id;
			newCurve.color = QColor::fromHsv((newCurve.id*25)%255, 255, 255),
			plot.curves.push_back(newCurve);
			break;
		}
	}
	applyConfig(config, tr("clone curve", "undo/redo X"));
}

void PlotDocument::deleteCurve(size_t plotId, size_t curveId)
{
	global_config_t config(_config);
	auto& plot = plotById(config, plotId);
	for(size_t i=0; i < plot.curves.size(); ++i) {
		if(plot.curves[i].id == curveId) {
			plot.curves.remove(i);
		}
	}
	applyConfig(config, tr("delete curve", "undo/redo X"));
}

void PlotDocument::editGlobalQuery(QString query)
{
	global_config_t config(_config);
	config.global_query = query;
	applyConfig(config, tr("edit global query", "undo/redo X"));
}

PlotDocument::plot_config_t& PlotDocument::plotById(global_config_t& cfg, size_t id)
{
	for(size_t i = 0; i < cfg.plots.size(); ++i) {
		if(cfg.plots[i].id == id) {
			return cfg.plots[i];
		}
	}
	throw std::out_of_range("No plot found");
}

PlotDocument::curve_config_t& PlotDocument::curveById(global_config_t& cfg, size_t plotId, size_t curveId)
{
	return curveById(plotById(cfg, plotId), curveId);
}

PlotDocument::curve_config_t& PlotDocument::curveById(plot_config_t& cfg, size_t curveId)
{
	for(size_t i = 0; i < cfg.curves.size(); ++i) {
		if(cfg.curves[i].id == curveId) {
			return cfg.curves[i];
		}
	}
	throw std::out_of_range("No curve found");
}
