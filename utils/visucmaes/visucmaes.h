#ifndef VISUCMAES_H
#define VISUCMAES_H

#include <QMainWindow>
namespace Ui {
	class MainWindow;
}

class QMdiSubWindow;
class PlotDocument;
class Plot;
class View3D;
class QProgressBar;

class Visucmaes : public QMainWindow
{
	Q_OBJECT
public:
	explicit Visucmaes(QWidget* parent = nullptr);
	~Visucmaes();

	void generateWindowTitle();
	void connectPlotActions();

	Plot* currentPlot() const;

public slots:
	void newDocument();
	void open();
	void save();
	void saveAs();
	void openDatabase();
	void changeSubwindowSelection(QMdiSubWindow* win);
	void untabMdiArea();
	void setTabbed(bool tabbed);

	void setParameterSelectionX(double par);
	void setParameterSelectionY(double par);
	void setParameterSelectionZ(double par);
	void setParameterSelectionPhi(double par);
	void setParameterSelectionTheta(double par);
	void setParameterSelectionOmega(double par);
	
private slots:
	void createAndDeletePlots();
	void forceRefresh();

private:
	Ui::MainWindow* ui;
	PlotDocument* _doc;
	QProgressBar* _progress;
	View3D* _view3D;
};

#endif//VISUCMAES_H
