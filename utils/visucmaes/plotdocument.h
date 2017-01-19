#ifndef PLOT_DOCUMENT_H
#define PLOT_DOCUMENT_H

#include <QObject>
#include <QColor>
#include <QPen>
#include "qcustomplot.h"

class PlotDocument : public QObject
{
	Q_OBJECT
public:
	enum curve_mode_t {
		cmPoints,
		cmErrors,
		cmStatisticalBox,
		cmHistogram
	};
	struct curve_config_t
	{
		size_t id;
		QString title;
		QString query;
		curve_mode_t mode;
		bool draw_lines;
		QCPScatterStyle::ScatterShape shape;
		QColor color;
		size_t hist_nbins;
		double hist_low;
		double hist_high;
	};
	struct plot_config_t
	{
		size_t id;
		QString title;
		QString xlabel;
		QString ylabel;
		double xmin;
		double xmax;
		double ymin;
		double ymax;
		bool use_xmin;
		bool use_xmax;
		bool use_ymin;
		bool use_ymax;
		bool legend;
		QString job_query;
		QVector<curve_config_t> curves;
	};
	struct global_config_t
	{
		size_t max_plot_id;
		QString global_query;
		QVector<plot_config_t> plots;
	};
	PlotDocument(QObject* parent=nullptr);
	~PlotDocument();

	bool hasFilename() const { return _filenameSet; }
	global_config_t config() const { return _config; }

public slots:
	void newDocument();
	void open(QString filename);
	void save(QString filename);
	void saveAs(QString filename);

	void addPlot();

signals:

	void changedFilename(QString fname);
	void changedConfig(global_config_t config);
	
private:
	global_config_t _config;
	QString _filename;
	bool _filenameSet;
};

#endif//PLOT_DOCUMENT_H
