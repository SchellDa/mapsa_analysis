#include "plotsettings.h"
#include "plotdocument.h"
#include "ui_plotsettings.h"

PlotSettings::PlotSettings(QWidget* parent, PlotDocument* doc, size_t plotId) :
 QDialog(parent), ui(new Ui::PlotSettings), _doc(doc), _plotId(plotId)
{
	ui->setupUi(this);
	auto global_cfg = _doc->config();
	PlotDocument::plot_config_t cfg = PlotDocument::plotById(global_cfg, _plotId);
	ui->title->setText(cfg.title);
	ui->xlabel->setText(cfg.xlabel);
	ui->ylabel->setText(cfg.ylabel);
	ui->xmin->setValue(cfg.xmin);
	ui->xmax->setValue(cfg.xmax);
	ui->ymin->setValue(cfg.ymin);
	ui->ymax->setValue(cfg.ymax);
	ui->use_xmin->setCheckState(cfg.use_xmin? Qt::Checked : Qt::Unchecked);
	ui->use_xmax->setCheckState(cfg.use_xmax? Qt::Checked : Qt::Unchecked);
	ui->use_ymin->setCheckState(cfg.use_ymin? Qt::Checked : Qt::Unchecked);
	ui->use_ymax->setCheckState(cfg.use_ymax? Qt::Checked : Qt::Unchecked);
	ui->show_legend->setCheckState(cfg.legend? Qt::Checked : Qt::Unchecked);
	ui->query->setPlainText(cfg.job_query);
	ui->selection_x->setCurrentIndex(static_cast<int>(cfg.selection_x));
	ui->selection_y->setCurrentIndex(static_cast<int>(cfg.selection_y));
	connect(ui->test, &QPushButton::clicked, this, &PlotSettings::test);
}

PlotSettings::~PlotSettings()
{
	delete ui;
}

void PlotSettings::accept()
{
	apply();
	QDialog::accept();
}

void PlotSettings::apply()
{
	auto global_cfg = _doc->config();
	PlotDocument::plot_config_t cfg = PlotDocument::plotById(global_cfg, _plotId);
	cfg.title = ui->title->text();
	cfg.xlabel = ui->xlabel->text();
	cfg.ylabel = ui->ylabel->text();
	cfg.xmin = ui->xmin->value();
	cfg.xmax = ui->xmax->value();
	cfg.ymin = ui->ymin->value();
	cfg.ymax = ui->ymax->value();
	cfg.use_xmin = ui->use_xmin->checkState() == Qt::Checked;
	cfg.use_xmax = ui->use_xmax->checkState() == Qt::Checked;
	cfg.use_ymin = ui->use_ymin->checkState() == Qt::Checked;
	cfg.use_ymax = ui->use_ymax->checkState() == Qt::Checked;
	cfg.legend = ui->show_legend->checkState() == Qt::Checked;
	cfg.job_query = ui->query->toPlainText();
	cfg.selection_x = static_cast<PlotDocument::axis_parameter_t>(ui->selection_x->currentIndex()); 
	cfg.selection_y = static_cast<PlotDocument::axis_parameter_t>(ui->selection_y->currentIndex()); 
	_doc->editPlot(cfg);
}

void PlotSettings::test()
{
	try {
		_doc->db.testQuery(ui->query->toPlainText(), true, false);
		ui->query_results->setPlainText(tr("Query Okay.", "Query test result"));
	} catch(Database::unknown_column& e) {
		ui->query_results->setPlainText(QString(tr(
		"Query Error!\n"
		"------------\n\n"
		"Invalid result column name: '%1'\n\n"
		"For job queries, the only allowed result column name is job_id "
		"(because that is its intended purpose...).\n"
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
