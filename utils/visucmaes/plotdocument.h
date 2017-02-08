#ifndef PLOT_DOCUMENT_H
#define PLOT_DOCUMENT_H

#include <QObject>
#include <QColor>
#include <QPen>
#include <QUndoStack>
#include "qcustomplot.h"
#include "database.h"

class Database;

class PlotDocument : public QObject
{
	Q_OBJECT
public:
	enum curve_mode_t {
		cmPoints,
		cmErrors,
		cmStatisticalBox,
		cmHistogram,
		cmHistogram2D
	};
	enum axis_parameter_t {
		apNone,
		apX,
		apY,
		apZ,
		apPhi,
		apTheta,
		apOmega
	};
	struct curve_config_t
	{
		size_t id;
		size_t plot_id;
		QString title;
		QString query;
		curve_mode_t mode;
		bool draw_lines;
		QCPScatterStyle::ScatterShape shape;
		QColor color;
		QCPColorGradient::GradientPreset gradient;
		size_t hist_nbins_x;
		double hist_low_x;
		double hist_high_x;
		size_t hist_nbins_y;
		double hist_low_y;
		double hist_high_y;
	};
	struct plot_config_t
	{
		size_t id;
		size_t max_curve_id;
		QString title;
		QString xlabel;
		QString ylabel;
		QString zlabel;
		double xmin;
		double xmax;
		double ymin;
		double ymax;
		double zmin;
		double zmax;
		bool use_xmin;
		bool use_xmax;
		bool use_ymin;
		bool use_ymax;
		bool use_zmin;
		bool use_zmax;
		bool legend;
		QString job_query;
		axis_parameter_t selection_x;
		axis_parameter_t selection_y;
		axis_parameter_t selection_z;
		double xlog;
		double ylog;
		double zlog;
		QVector<curve_config_t> curves;
	};
	struct global_config_t
	{
		size_t max_plot_id;
		QString global_query;
		QVector<plot_config_t> plots;
	};

	class UndoCommand : public QUndoCommand
	{
	public:
		UndoCommand(PlotDocument* doc, global_config_t old_cfg, global_config_t new_cfg, QString description) :
		 _doc(doc), _oldCfg(old_cfg), _newCfg(new_cfg)
		{
			setText(description);
		}

		virtual void redo()
		{
			_doc->setConfig(_newCfg);
		}

		virtual void undo()
		{
			_doc->setConfig(_oldCfg);
		}
	private:
		PlotDocument* _doc;
		global_config_t _oldCfg;
		global_config_t _newCfg;
	};

	PlotDocument(QObject* parent=nullptr);
	~PlotDocument();
	
	QString filename() const { return _filename; }
	bool hasFilename() const { return _filenameSet; }
	global_config_t config() const { return _config; }
	
	QUndoStack* undoStack;

	void createActions(QMenu* parent);

	void applyConfig(global_config_t cfg, QString changeDescription);
	void setConfig(global_config_t cfg);

	static plot_config_t& plotById(global_config_t& cfg, size_t plotId);
	static curve_config_t& curveById(global_config_t& cfg, size_t plotId, size_t curveId);
	static curve_config_t& curveById(plot_config_t& cfg, size_t curveId);

	Database db;

public slots:
	void newDocument();
	void open(QString filename);
	void save();
	void saveAs(QString filename);

	void addPlot();
	void editPlot(plot_config_t cfg);
	void clonePlot(size_t plotId);
	void deletePlot(size_t plotId);
	void togglePlotLegend(size_t plotId);

	void addCurve(size_t plotId);
	void editCurve(curve_config_t cfg);
	void cloneCurve(size_t plotId, size_t curveId);
	void deleteCurve(size_t plotId, size_t curveId);

	void editGlobalQuery(QString query);
	
signals:

	void changedFilename(QString fname);
	void changedConfig(global_config_t config);
	void forceRefresh();
	
private:
	global_config_t _config;
	QString _filename;
	bool _filenameSet;
};

#endif//PLOT_DOCUMENT_H
