#ifndef VIEW_3D_H
#define VIEW_3D_H


#include <QMainWindow>
#include "database.h"

namespace Ui {
	class View3D;
}

class View3D : public QMainWindow
{
	Q_OBJECT
public:
	View3D(QWidget* parent=nullptr);
	~View3D();

	Ui::View3D* ui;

public slots:
	void setCache(const Database::run_cache_t* cache);
	void selectRunIndex(QString index);

signals:
	void visibleChanged(bool);
	void invisibleChanged(bool);

protected:
	virtual void hideEvent(QHideEvent* evt);
	virtual void showEvent(QShowEvent* evt);

private:
	const Database::run_cache_t* _cache;
};

#endif//VIEW_3D_H
