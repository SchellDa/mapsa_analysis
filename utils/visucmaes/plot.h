#ifndef _PLOT_H_
#define _PLOT_H_

#include "qcustomplot.h"
#include "plotdocument.h"

class PlotDocument;

class Plot : public QCustomPlot
{
	Q_OBJECT
public:
	struct curve_t
	{
		QCPGraph* graph;
		QCPStatisticalBox* statbox;
		const PlotDocument::curve_config_t* config;
	};
	explicit Plot(QWidget* parent, size_t id, PlotDocument* doc);
	~Plot();
	
	size_t id() const { return _id; }
	void setId(size_t id) { _id = id; }
	bool setConfig(const PlotDocument::global_config_t config);

	virtual void focusOutEvent(QFocusEvent* event) override;

	const curve_t* selectedCurve() const { return _selectedCurve; }

	virtual QSize minimumSizeHint() const;

	virtual void mousePressEvent(QMouseEvent* event);

public slots:
	void refresh();
	void forcedRefresh();

	void setParameterSelectionX(double par);
	void setParameterSelectionY(double par);
	void setParameterSelectionZ(double par);
	void setParameterSelectionPhi(double par);
	void setParameterSelectionTheta(double par);
	void setParameterSelectionOmega(double par);

	void setParameterSelection(double val, PlotDocument::axis_parameter_t par);
	void emitParameterSelection(double val, PlotDocument::axis_parameter_t par);

signals:
	void selectionChanged(bool selectedSmth);
	void selectedCurve(size_t plot_id, size_t curve_id);
	void curveProcessed();

	void selectedParameterX(double par);
	void selectedParameterY(double par);
	void selectedParameterZ(double par);
	void selectedParameterPhi(double par);
	void selectedParameterTheta(double par);
	void selectedParameterOmega(double par);

private slots:
	void checkSelections();

private:
	Database::data_t getData(size_t curveId);
	void checkCacheInvalidation(PlotDocument::global_config_t newConfig);
	void invalidateAllCaches();
	void invalidateCache(size_t curve_id);
	void updateSelectionLines();
	size_t _id;
	PlotDocument* _doc;
	const PlotDocument::plot_config_t* _config;
	PlotDocument::global_config_t _global;
	QVector<curve_t> _curves;
	const curve_t* _selectedCurve;
	QMap<size_t, Database::data_t> _cache;
	QVector<double> _selectedParameters;
	QCPItemLine* _xSelectionLine;
	QCPItemLine* _ySelectionLine;
};

#endif//_PLOT_H_
