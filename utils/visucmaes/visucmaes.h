#ifndef VISUCMAES_H
#define VISUCMAES_H

#include <QMainWindow>
namespace Ui {
	class MainWindow;
}

class PlotDocument;

class Visucmaes : public QMainWindow
{
	Q_OBJECT
public:
	explicit Visucmaes(QWidget* parent = nullptr);
	~Visucmaes();

public slots:
	void createPlot();

private slots:
	void createAndDeletePlots();

private:
	Ui::MainWindow* ui;
	PlotDocument* _doc;
	size_t _maxCreatedPlotId;
};

#endif//VISUCMAES_H
