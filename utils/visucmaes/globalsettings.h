#ifndef GLOBAL_SETTINGS_H
#define GLOBAL_SETTINGS_H

#include <QDialog>

namespace Ui {
	class GlobalSettings;
};
class PlotDocument;

class GlobalSettings : public QDialog
{
	Q_OBJECT
public:
	explicit GlobalSettings(QWidget* parent, PlotDocument* doc);
	virtual ~GlobalSettings();

public slots:
	void accept();
	void apply();
	void test();

private:
	Ui::GlobalSettings* ui;
	PlotDocument* _doc;
};

#endif//GLOBAL_SETTINGS_H
