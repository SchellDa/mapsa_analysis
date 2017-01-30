#ifndef VIEWPORT_H
#define VIEWPORT_H


#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <Eigen/Dense>
#include "mpatransform.h"
#include "database.h"

class Viewport : public QOpenGLWidget
{
	Q_OBJECT
public:
	explicit Viewport(QWidget* parent=nullptr);
	virtual ~Viewport();

	virtual void paintGL();
	virtual void resizeGL(int w, int h);
	virtual void initializeGL();

public slots:
	void setParameterX(double par);
	void setParameterY(double par);
	void setParameterZ(double par);
	void setParameterPhi(double par);
	void setParameterTheta(double par);
	void setParameterOmega(double par);
	void setData(int mpaIndex, Eigen::Vector3d trackStart, Eigen::Vector3d trackEnd);
	void setCache(const Database::run_cache_t* cache);
	void setRunId(int run_id);
	void setEventIndex(int idx);

protected:
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void mousePressEvent(QMouseEvent* event);
	static Eigen::Matrix4d perspective(double fovy, double aspect, double zNear, double zFar);
	static Eigen::Matrix4d lookAt(Eigen::Vector3d eye, Eigen::Vector3d center, Eigen::Vector3d up);

private:
	void drawCoordinateSystem();
	void drawMpa();
	void drawTrack();

	Eigen::Vector2d _prevMousePos;
	core::MpaTransform _transform;
	Eigen::Vector3d _rotation;
	int _mpaIndex;
	Eigen::Vector3d _trackStart;
	Eigen::Vector3d _trackEnd;
	double _camYaw;
	double _camPitch;
	double _camDist;
	const Database::run_cache_t* _cache;
	QVector<Database::cache_entry_t> _dataset;
	int _eventIndex;
};

#endif//VIEWPORT_H
