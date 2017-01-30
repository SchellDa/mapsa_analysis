#include "globalsettings.h"
#include "plotdocument.h"
#include "ui_globalsettings.h"

GlobalSettings::GlobalSettings(QWidget* parent, PlotDocument* doc) :
 QDialog(parent), ui(new Ui::GlobalSettings), _doc(doc)
{
	ui->setupUi(this);
	auto cfg = _doc->config();
	ui->query->setPlainText(cfg.global_query);
	connect(ui->test, &QPushButton::clicked, this, &GlobalSettings::test);
}

GlobalSettings::~GlobalSettings()
{
	delete ui;
}

void GlobalSettings::accept()
{
	apply();
	QDialog::accept();
}

void GlobalSettings::apply()
{
	_doc->editGlobalQuery(ui->query->toPlainText());
}

void GlobalSettings::test()
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
