
#include <QDialog>

namespace Ui {
	class CurveSettings;
};

class PlotDocument;

class CurveSettings : public QDialog
{
	Q_OBJECT
public:
	CurveSettings(QWidget* parent, PlotDocument* doc, size_t plotId, size_t curveId);
	virtual ~CurveSettings();

	Ui::CurveSettings* ui;

public slots:
	void accept();
	void apply();
	void chooseColor();
	void updateColor();
	void test();

private:
	QString getFullQuery() const;

	PlotDocument* _doc;
	size_t _plotId;
	size_t _curveId;
	QColor _color;
};
