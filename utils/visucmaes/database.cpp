
#include "database.h"
#include <limits>
#include <QApplication>
#include <QDebug>

class Sqlite3Stmt
{
public:
	Sqlite3Stmt(sqlite3_stmt* n=nullptr) : stmt(n) { }
	~Sqlite3Stmt() { if(stmt) sqlite3_finalize(stmt); }

	sqlite3_stmt* stmt;
};

Database::Database(QObject* parent) :
 QObject(parent), _db(nullptr)
{
}

Database::~Database()
{
	close();
}

void Database::open(QString filename)
{
	close();
	auto result = sqlite3_open_v2(filename.toUtf8().constData(), &_db, SQLITE_OPEN_READONLY, nullptr);
	if(_db == nullptr) {
		throw std::bad_alloc();
	}
	if(result != SQLITE_OK) {
		sqlite3_close(_db);
		_db = nullptr;
		throw database_error(result, "Opening Database");
	}
	emit opened();
	emit opened(true);
	importCache();
}

void Database::close()
{
	if(_db) {
		emit closed();
		emit opened(false);
		sqlite3_close(_db);
		_db = nullptr;
	}
}

bool Database::isOpen() const
{
	return _db != nullptr;
}

void Database::testQuery(QString query, bool jobQuery, bool statbox)
{
	if(!isOpen()) {
		throw no_database();
	}
	Sqlite3Stmt stmt;
	auto utf8Query = query.toUtf8();
	auto result = sqlite3_prepare_v2(_db, utf8Query.constData(), utf8Query.size()+1, &stmt.stmt, nullptr);
	if(result != SQLITE_OK) {
		throw database_error(result, "Preparing Query", sqlite3_errmsg(_db));
	}
	static const QVector<QString> allowed_xy = {"x", "xerr", "y", "yerr"};
	static const QVector<QString> allowed_statbox = {"x, median",
		"first_quartile", "third_quartile", "upper_whisker", "lower_whisker"};
	if(jobQuery) {
/*		for(size_t i = 0; i < sqlite3_column_count(stmt.stmt); ++i) {
			if(QString("job_id") != sqlite3_column_name(stmt.stmt, i)) {
				throw unknown_column(sqlite3_column_name(stmt.stmt, i));
			}
		}*/
		// do whatever you want...
	} else {
		if(statbox) {
			for(size_t i = 0; i < sqlite3_column_count(stmt.stmt); ++i) {
			if(allowed_statbox.indexOf(sqlite3_column_name(stmt.stmt, i)) == -1) {
					throw unknown_column(sqlite3_column_name(stmt.stmt, i));
				}
			}
		} else {
			for(size_t i = 0; i < sqlite3_column_count(stmt.stmt); ++i) {
				if(allowed_xy.indexOf(sqlite3_column_name(stmt.stmt, i)) == -1) {
					throw unknown_column(sqlite3_column_name(stmt.stmt, i));
				}
			}
		}
	}
}

Database::data_t Database::exec(QString query, bool statbox)
{
	testQuery(query, false, statbox);
	Sqlite3Stmt stmt;
	auto utf8Query = query.toUtf8();
	auto result = sqlite3_prepare_v2(_db, utf8Query.constData(), utf8Query.size()+1, &stmt.stmt, nullptr);
	if(result != SQLITE_OK) {
		throw database_error(result, "Preparing Query", sqlite3_errmsg(_db));
	}
	qDebug() << "Read rows for " << query;
	data_t data;
	size_t num_rows = 0;
	do {
		result = sqlite3_step(stmt.stmt);
		if(result != SQLITE_DONE && result != SQLITE_ROW) {
			throw database_error(result, "Executing Query", sqlite3_errmsg(_db));
		}
		if(result == SQLITE_ROW) {
			num_rows++;
			if(num_rows % 1000 == 0) {
				QApplication::processEvents();
			}
			if(num_rows % 10000 == 0) {
				qDebug() << num_rows << "rows read.";
			}
			for(size_t i = 0; i < sqlite3_column_count(stmt.stmt); ++i) {
				QString name(sqlite3_column_name(stmt.stmt, i));
				double val = sqlite3_column_double(stmt.stmt, i);
				if(statbox) {
					if(name == "x") {
						data.x.push_back(val);
					} else if(name == "median") {
						data.median.push_back(val);
					} else if(name == "first_quartile") {
						data.first_quartile.push_back(val);
					} else if(name == "third_quartile") {
						data.third_quartile.push_back(val);
					} else if(name == "lower_whisker") {
						data.lower_whisker.push_back(val);
					} else if(name == "upper_whisker") {
						data.upper_whisker.push_back(val);
					} else {
						throw unknown_column(name);
					}
				} else {
					if(name == "x") {
						data.x.push_back(val);
					} else if(name == "y") {
						data.y.push_back(val);
					} else if(name == "xerr") {
						data.x_err.push_back(val);
					} else if(name == "yerr") {
						data.y_err.push_back(val);
					} else {
						throw unknown_column(name);
					}
				}
			}
		}
	} while(result != SQLITE_DONE);
	qDebug() << "Read all rows";
	return data;
}

void Database::importCache()
{
	_cache.clear();
	Sqlite3Stmt stmt;
	QString query("SELECT run_id, mpa_index, ax, ay, az, bx, by, bz FROM hitpairs;");
	auto utf8Query = query.toUtf8();
	auto result = sqlite3_prepare_v2(_db, utf8Query.constData(), utf8Query.size()+1, &stmt.stmt, nullptr);
	if(result != SQLITE_OK) {
		throw database_error(result, "Preparing Query", sqlite3_errmsg(_db));
	}
	data_t data;
	size_t num_rows = 0;
	qDebug() << "Start reading cache…";
	do {
		result = sqlite3_step(stmt.stmt);
		if(result != SQLITE_DONE && result != SQLITE_ROW) {
			throw database_error(result, "Executing Query", sqlite3_errmsg(_db));
		}
		if(result == SQLITE_ROW) {
			num_rows++;
			if(num_rows % 100 == 0) {
				QApplication::processEvents();
			}
			if(num_rows % 1000 == 0) {
				qDebug() << num_rows << "rows read.";
			}
			int run_id = sqlite3_column_int(stmt.stmt, 0);
			int mpa_index = sqlite3_column_int(stmt.stmt, 1);
			Eigen::Vector3d a;
			Eigen::Vector3d b;
			a(0) = sqlite3_column_double(stmt.stmt, 2);
			a(1) = sqlite3_column_double(stmt.stmt, 3);
			a(2) = sqlite3_column_double(stmt.stmt, 4);
			b(0) = sqlite3_column_double(stmt.stmt, 5);
			b(1) = sqlite3_column_double(stmt.stmt, 6);
			b(2) = sqlite3_column_double(stmt.stmt, 7);
			_cache[run_id].push_back({mpa_index, a, b});
		}
	} while(result != SQLITE_DONE);
	qDebug() << "… done!";
	qDebug() << "Run IDs" << _cache.keys();
	emit cacheChanged(&_cache);
}
