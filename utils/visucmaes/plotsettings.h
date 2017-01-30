#ifndef PLOT_SETTINGS_H
#define PLOT_SETTINGS_H

#include <QDialog>

namespace Ui {
	class PlotSettings;
};
class PlotDocument;

class PlotSettings : public QDialog
{
	Q_OBJECT
public:
	explicit PlotSettings(QWidget* parent, PlotDocument* doc, size_t plotId);
	virtual ~PlotSettings();

public slots:
	void accept();
	void apply();
	void test();

private:
	Ui::PlotSettings* ui;
	PlotDocument* _doc;
	size_t _plotId;
};

#endif//PLOT_SETTINGS_H
