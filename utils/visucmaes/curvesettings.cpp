
#include "curvesettings.h"
#include "plotdocument.h"
#include "ui_curvesettings.h"

CurveSettings::CurveSettings(QWidget* parent, PlotDocument* doc, size_t plotId, size_t curveId) :
 QDialog(parent), _doc(doc), _plotId(plotId), _curveId(curveId), ui(new Ui::CurveSettings), _color()
{
	ui->setupUi(this);
	auto config = _doc->config();
	auto cfg = PlotDocument::curveById(config, _plotId, _curveId);
	ui->title->setText(cfg.title);
	ui->query->setPlainText(cfg.query);
	ui->mode->setCurrentIndex(static_cast<int>(cfg.mode));
	ui->draw_lines->setCheckState(cfg.draw_lines? Qt::Checked : Qt::Unchecked);
	ui->shape->setCurrentIndex(static_cast<int>(cfg.shape));
	_color = cfg.color;
	ui->gradient->setCurrentIndex(static_cast<int>(cfg.gradient));
	ui->hist_bins_x->setValue(static_cast<int>(cfg.hist_nbins_x));
	ui->hist_low_x->setValue(cfg.hist_low_x);
	ui->hist_high_x->setValue(cfg.hist_high_x);
	ui->hist_bins_y->setValue(static_cast<int>(cfg.hist_nbins_y));
	ui->hist_low_y->setValue(cfg.hist_low_y);
	ui->hist_high_y->setValue(cfg.hist_high_y);
	connect(ui->color, &QPushButton::clicked, this, &CurveSettings::chooseColor);
	connect(ui->test, &QPushButton::clicked, this, &CurveSettings::test);
	updateColor();
}

CurveSettings::~CurveSettings()
{
	delete ui;
}

void CurveSettings::accept()
{
	apply();
	QDialog::accept();
}

void CurveSettings::apply()
{
	PlotDocument::curve_config_t cfg;
	cfg.id = _curveId;
	cfg.plot_id = _plotId;
	cfg.title = ui->title->text();
	cfg.query = ui->query->toPlainText();
	cfg.mode = static_cast<PlotDocument::curve_mode_t>(ui->mode->currentIndex());
	cfg.draw_lines = ui->draw_lines->checkState() == Qt::Checked;
	cfg.shape = static_cast<QCPScatterStyle::ScatterShape>(ui->shape->currentIndex()); 
	cfg.color = _color;
	cfg.gradient = static_cast<QCPColorGradient::GradientPreset>(ui->gradient->currentIndex()); 
	cfg.hist_nbins_x = static_cast<size_t>(ui->hist_bins_x->value());
	cfg.hist_low_x = ui->hist_low_x->value();
	cfg.hist_high_x	= ui->hist_high_x->value();
	cfg.hist_nbins_y = static_cast<size_t>(ui->hist_bins_y->value());
	cfg.hist_low_y = ui->hist_low_y->value();
	cfg.hist_high_y	= ui->hist_high_y->value();
	_doc->editCurve(cfg);
}

void CurveSettings::test()
{
	bool statbox = static_cast<PlotDocument::curve_mode_t>(ui->mode->currentIndex()) == PlotDocument::cmStatisticalBox;
	try {
		_doc->db.testQuery(getFullQuery(), false, statbox);
		ui->query_results->setPlainText(tr("Query Okay.", "Query test result"));
	} catch(Database::unknown_column& e) {
		ui->query_results->setPlainText(QString(tr(
		"Query Error!\n"
		"------------\n\n"
		"Invalid result column name: '%1'\n\n"
		"It is expected that only columns with the following names are returned:\n"
		" * x - X value of datapoint\n"
		" * xerrs - Error on X value\n"
		" * y - Y value of datapoint\n"
		" * yerrs - Error on Y value\n"
		" * median - Median for statistical box plots\n"
		" * first_quartile - Upper box edge for statistical box plots\n"
		" * third_quartile - Lower box edge for statistical box plots\n"
		" * upper_whisker - Height of upper whisker for statistical box plots.\n"
		" * lower_whisker - Height of lower whisker for statistical box plots.\n\n"
		"The interpretation of statistical box plots is up to the user. It could be\n"
		"e.g. the first and last percentile or the first standard deviation (sigma).\n",
		"Query test result")).arg(e.column()));
	} catch(Database::database_error& e) {
		ui->query_results->setPlainText(QString(tr(
		"SQL Error!\n"
		"----------\n\n"
		"(Err %2) while %1\n"
		"%3\n\n"
		"Please see the SQLite3 Handbook for more information. \n",
		"Query test result")).arg(e.context(), QString::number(e.errorCode()), e.details()));
	} catch(Database::no_database& e) {
		ui->query_results->setPlainText(QString(tr(
		"No Database Opened!\n", "Query test result")));
	}
}

void CurveSettings::chooseColor()
{
	auto newColor = QColorDialog::getColor(_color, this, "Choose Curve Color");
	if(newColor.isValid()) {
		_color = newColor;
		updateColor();
	}
}

void CurveSettings::updateColor()
{
	QString style("border: 1px solid black;\nborder-radius: 5px; background-color: ");
	style += _color.name();
	style += ";";
	ui->color->setStyleSheet(style);
}

QString CurveSettings::getFullQuery() const
{
	auto config = _doc->config();
	auto plot_cfg = PlotDocument::plotById(config, _plotId);
	auto curve_cfg = PlotDocument::curveById(plot_cfg, _curveId);
	QString jobQuery = config.global_query;
	if(plot_cfg.job_query.size() > 0) {
		jobQuery = plot_cfg.job_query;
	}
	QString query(ui->query->toPlainText());
	return query.replace("%jobs", jobQuery);
}
