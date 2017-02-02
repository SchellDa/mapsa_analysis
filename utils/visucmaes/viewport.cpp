#include "viewport.h"
#include <QMouseEvent>
#include <QDebug>
#include <cmath>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <GL/gl.h>
#include <Eigen/Geometry>

Viewport::Viewport(QWidget* parent) :
 QOpenGLWidget(parent), _rotation({0, 0, 0}), _mpaIndex(0), _trackStart({0, 0, 0}), _trackEnd({0, 0, 1}),
 _camYaw(45*3.141/180), _camPitch(-20*3.141/180.0), _camDist(std::log(10)), _cache(nullptr), _filter(efAllEvents)
{
	setParameterOmega(0.0);
}

Viewport::~Viewport()
{
}

void Viewport::paintGL()
{
	auto f = QOpenGLContext::currentContext()->functions();
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	Eigen::Matrix3d m;
	m = Eigen::AngleAxisd(_camYaw, Eigen::Vector3d::UnitY())
	  * Eigen::AngleAxisd(_camPitch, Eigen::Vector3d::UnitX());
	Eigen::Vector3d midMpa = _transform.pixelCoordToGlobal(Eigen::Vector2d{8.0, 1.5});
	Eigen::Vector3d camPos = m*Eigen::Vector3d::UnitZ() * exp(_camDist);
	Eigen::Vector3d camUp = m*Eigen::Vector3d::UnitY();
	auto camera = lookAt(camPos + midMpa,
	                     midMpa,
			     camUp);
	glLoadMatrixd(camera.data());
	glLineWidth(1.0);
	drawMpa();
	drawTrack();
	drawCoordinateSystem();
}

void Viewport::resizeGL(int w, int h)
{
	auto projection = perspective(45.0, static_cast<double>(w)/h, 0.01, 10000.0);
	auto f = QOpenGLContext::currentContext()->functions();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glLoadMatrixd(projection.data());
}

void Viewport::initializeGL()
{
	auto f = QOpenGLContext::currentContext()->functions();
	glClearColor(0.8f, 0.8f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
}

void Viewport::setParameterX(double par)
{
	Eigen::Vector3d offset = _transform.getOffset();
	offset(0) = par;
	_transform.setOffset(offset);
	setRunId(_runId);
	update();
}

void Viewport::setParameterY(double par)
{
	Eigen::Vector3d offset = _transform.getOffset();
	offset(1) = par;
	_transform.setOffset(offset);
	setRunId(_runId);
	update();
}

void Viewport::setParameterZ(double par)
{
	Eigen::Vector3d offset = _transform.getOffset();
	offset(2) = par;
	_transform.setOffset(offset);
	setRunId(_runId);
	update();
}

void Viewport::setParameterPhi(double par)
{
	_rotation(0) = par;
	_transform.setRotation(_rotation);
	setRunId(_runId);
	update();
}

void Viewport::setParameterTheta(double par)
{
	_rotation(1) = par;
	_transform.setRotation(_rotation);
	setRunId(_runId);
	update();
}

void Viewport::setParameterOmega(double par)
{
	_rotation(2) = par;
	_transform.setRotation(_rotation);
	setRunId(_runId);
	update();
}

void Viewport::setData(int mpaIndex, Eigen::Vector3d trackStart, Eigen::Vector3d trackEnd)
{
	_mpaIndex = mpaIndex;
	_trackStart = trackStart;
	_trackEnd = trackEnd;
	update();
}

void Viewport::setCache(const Database::run_cache_t* cache)
{
	_cache = cache;
}

void Viewport::setRunId(int run_id)
{
	if(_cache == nullptr) {
		return;
	}
	qDebug() << "Viewport::setRunId(" << run_id << ")";
	try {
		_runId = run_id;
		_dataset = (*_cache)[run_id];
		filterDataset();
		setEventIndex(1);
	} catch(std::exception& e) {
		_dataset.clear();
	}
	emit numEventsChanged(_dataset.size());
}

void Viewport::setEventIndex(int idx)
{
	if(_dataset.empty()) {
		return;
	}
	qDebug() << "Viewport::setEventIndex(" << idx << ")";
	_eventIndex = idx - 1;
	_mpaIndex = _dataset.at(_eventIndex).mpa_idx;
	_trackStart = _dataset.at(_eventIndex).a;
	_trackEnd = _dataset.at(_eventIndex).b;
	qDebug() << " -> cache entry" << _mpaIndex
	         << _trackStart(0)
	         << _trackStart(1)
	         << _trackStart(2)
	         << _trackEnd(0)
	         << _trackEnd(1)
	         << _trackEnd(2);
	update();
}

void Viewport::setEventFilter(event_filter_t filter)
{
	_filter = filter;
	setRunId(_runId);
}

void Viewport::mouseMoveEvent(QMouseEvent* event)
{
	if(event->buttons() == Qt::LeftButton) {
		_camYaw -= static_cast<double>(event->x() - _prevMousePos(0)) * 0.01;
		_camPitch -= static_cast<double>(event->y() - _prevMousePos(1)) * 0.01;
		update();
	} else if(event->buttons() == Qt::RightButton) {
		_camDist += static_cast<double>(event->y() - _prevMousePos(1)) * 0.01;
		update();
	}
	_prevMousePos(0) = event->x();
	_prevMousePos(1) = event->y();
}

void Viewport::mousePressEvent(QMouseEvent* event)
{
	_prevMousePos(0) = event->x();
	_prevMousePos(1) = event->y();
}

Eigen::Matrix4d Viewport::perspective(double fovy, double aspect, double zNear, double zFar)
{
	assert(aspect > 0);
	assert(zFar > zNear);
	double radf = fovy*3.141/180.0;
	double tanHalfFovy = tan(radf/2.0);
	Eigen::Matrix4d res = Eigen::Matrix4d::Zero();
	res(0,0) = 1.0 / (aspect * tanHalfFovy);
	res(1,1) = 1.0 / (tanHalfFovy);
	res(2,2) = - (zFar + zNear) / (zFar - zNear);
	res(3,2) = - 1.0;
	res(2,3) = - (2.0 * zFar * zNear) / (zFar - zNear);
	return res;
}

Eigen::Matrix4d Viewport::lookAt(Eigen::Vector3d eye, Eigen::Vector3d center, Eigen::Vector3d up)
{
	Eigen::Vector3d f = (center - eye).normalized();
	Eigen::Vector3d u = up.normalized();
	Eigen::Vector3d s = f.cross(u).normalized();
	u = s.cross(f);
	Eigen::Matrix4d res;
	res << s.x(),s.y(),s.z(),-s.dot(eye),
	       u.x(),u.y(),u.z(),-u.dot(eye),
  	       -f.x(),-f.y(),-f.z(),f.dot(eye),
  	       0,0,0,1;
	return res;
}

void Viewport::drawCoordinateSystem()
{
	glLineWidth(3.0);
	glBegin(GL_LINES);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(1.0f, 0.0f, 0.0f);
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 1.0f, 0.0f);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 1.0f);
	glEnd();
}

void Viewport::drawMpa()
{
	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_LINES);
	for(int px = 0; px < _transform.num_pixels_x; ++px) {
		auto a = _transform.pixelCoordToGlobal({px, 0}, false);
		auto b = _transform.pixelCoordToGlobal(Eigen::Vector2d{
					                static_cast<double>(px),
		                                        static_cast<double>(_transform.num_pixels_y)-0.01});
		glVertex3f(a(0), a(1), a(2));
		glVertex3f(b(0), b(1), b(2));
	}
	auto a = _transform.pixelCoordToGlobal(Eigen::Vector2d{-0.01+_transform.num_pixels_x, 0.0});
	auto b = _transform.pixelCoordToGlobal(Eigen::Vector2d{
			                        static_cast<double>(_transform.num_pixels_x)-0.01,
	                                        static_cast<double>(_transform.num_pixels_y)-0.01});
	glVertex3f(a(0), a(1), a(2));
	glVertex3f(b(0), b(1), b(2));
	for(int py = 0; py < _transform.num_pixels_y; ++py) {
		auto a = _transform.pixelCoordToGlobal({0, py}, false);
		auto b = _transform.pixelCoordToGlobal(Eigen::Vector2d{
		                                        static_cast<double>(_transform.num_pixels_x)-0.01,
					                static_cast<double>(py)});
		glVertex3f(a(0), a(1), a(2));
		glVertex3f(b(0), b(1), b(2));
	}
	a = _transform.pixelCoordToGlobal(Eigen::Vector2d{0.0, -0.01+_transform.num_pixels_y});
	b = _transform.pixelCoordToGlobal(Eigen::Vector2d{
	                                        static_cast<double>(_transform.num_pixels_x)-0.01,
			                        static_cast<double>(_transform.num_pixels_y)-0.01});
	glVertex3f(a(0), a(1), a(2));
	glVertex3f(b(0), b(1), b(2));
	glEnd();

	Eigen::Vector2d pc = _transform.translatePixelIndex(_mpaIndex).cast<double>();
	bool mpaHit = false;
	size_t extrapIdx = 0;
	Eigen::Vector2d epc;
	try {
		Eigen::Vector3d extrapPoint = _transform.mpaPlaneTrackIntersect({{0, 1}, {_trackStart, _trackEnd}});
		extrapIdx = _transform.getPixelIndex(extrapPoint);
		mpaHit = true;
		epc = _transform.translatePixelIndex(extrapIdx).cast<double>();
	} catch (std::out_of_range& e) {
	}
	Eigen::Vector3d pos;
	glColor3f(0.7, 0.7, 0.0);
	glBegin(GL_QUADS);
	pos = _transform.pixelCoordToGlobal(Eigen::Vector2d{pc + Eigen::Vector2d{0., 0.}});
	glVertex3f(pos(0), pos(1), pos(2));
	pos = _transform.pixelCoordToGlobal(Eigen::Vector2d{pc + Eigen::Vector2d{0.99, 0.}});
	glVertex3f(pos(0), pos(1), pos(2));
	pos = _transform.pixelCoordToGlobal(Eigen::Vector2d{pc + Eigen::Vector2d{0.99, 0.99}});
	glVertex3f(pos(0), pos(1), pos(2));
	pos = _transform.pixelCoordToGlobal(Eigen::Vector2d{pc + Eigen::Vector2d{0., 0.99}});
	glVertex3f(pos(0), pos(1), pos(2));
	glEnd();
	if(mpaHit && extrapIdx != _mpaIndex) {
		glColor3f(0.7, 0.7, 0.7);
		glBegin(GL_QUADS);
		pos = _transform.pixelCoordToGlobal(Eigen::Vector2d{epc + Eigen::Vector2d{0., 0.}});
		glVertex3f(pos(0), pos(1), pos(2));
		pos = _transform.pixelCoordToGlobal(Eigen::Vector2d{epc + Eigen::Vector2d{0.99, 0.}});
		glVertex3f(pos(0), pos(1), pos(2));
		pos = _transform.pixelCoordToGlobal(Eigen::Vector2d{epc + Eigen::Vector2d{0.99, 0.99}});
		glVertex3f(pos(0), pos(1), pos(2));
		pos = _transform.pixelCoordToGlobal(Eigen::Vector2d{epc + Eigen::Vector2d{0., 0.99}});
		glVertex3f(pos(0), pos(1), pos(2));
		glEnd();
	}
}

void Viewport::drawTrack()
{
	glLineWidth(2);
	Eigen::Vector3d b = _transform.mpaPlaneTrackIntersect({{0, 1}, {_trackStart, _trackEnd}});
	glBegin(GL_LINES);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3d(_trackStart(0), _trackStart(1), _trackStart(2));
	glVertex3d(_trackEnd(0), _trackEnd(1), _trackEnd(2));
	glColor3f(1.0f, 1.0f, 0.0f);
	glVertex3d(_trackEnd(0), _trackEnd(1), _trackEnd(2));
	glVertex3d(b(0), b(1),  b(2));
	glEnd();
	qDebug() << "Track Z positions" << _trackStart(2) << _trackEnd(2) << b(2) << _transform.getOffset()(2);
}

void Viewport::filterDataset()
{
	if(_filter == efAllEvents) {
		return;
	}
	for(size_t i = 0; i < _dataset.size(); ++i) {
		qDebug() << "Test idx" << i;
		const auto& entry = _dataset[i];
		try {
			Eigen::Vector3d extrapPoint = _transform.mpaPlaneTrackIntersect({{0, 1}, {entry.a, entry.b}});
			_transform.getPixelIndex(extrapPoint);
			if(_filter == efFiducialMiss) {
				_dataset.remove(i);
				--i;
			}
		} catch (std::out_of_range& e) {
			if(_filter == efFiducialHit) {
				_dataset.remove(i);
				--i;
			}
		}
	}
}
