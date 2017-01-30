#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <sqlite3.h>
#include <exception>
#include <Eigen/Dense>

class Database : public QObject
{
	Q_OBJECT
public:
	explicit Database(QObject* parent=nullptr);
	~Database();

	struct data_t
	{
		QVector<double> x;
		QVector<double> x_err;
		QVector<double> y;
		QVector<double> y_err;
		QVector<double> median;
		QVector<double> first_quartile;
		QVector<double> third_quartile;
		QVector<double> upper_whisker;
		QVector<double> lower_whisker;
	};

	struct cache_entry_t
	{
		int mpa_idx;
		Eigen::Vector3d a;
		Eigen::Vector3d b;
	};
	typedef QMap<int, QVector<cache_entry_t>> run_cache_t;

	void open(QString filename);
	void close();
	bool isOpen() const;

	void testQuery(QString query, bool jobQuery, bool statbox);
	data_t exec(QString query, bool statbox);

	const run_cache_t* getCache() const { return &_cache; }

	class no_database : public std::runtime_error {
	public:
		no_database() : std::runtime_error("No database opened") { }
	};
	class database_error : public std::runtime_error
	{
	public:
		database_error(int errcode, QString context, QString message="") :
		 std::runtime_error(""),
		 _context(context.toUtf8().constData()),
		 _txt(context.toUtf8().constData()),
		 _details(message.toUtf8().constData()),
		 _errcode(errcode)
		{
			_txt += ": ";
			_txt += message.size()>0 ? message.toUtf8().constData() : sqlite3_errstr(errcode);
		}
		virtual const char* what() const noexcept { return _txt.c_str(); }
		int errorCode() const noexcept { return _errcode; }
		const char* errorStr() const noexcept { return sqlite3_errstr(_errcode); }
		const char* context() const noexcept { return _context.c_str(); }
		const char* details() const noexcept { return _details.c_str(); }
	private:
		std::string _txt;
		std::string _context;
		std::string _details;
		int _errcode;
	};
	class unknown_column : public std::runtime_error
	{
	public:
		unknown_column(QString colname) :
		 std::runtime_error((std::string("Unknown Column: ")+colname.toUtf8().constData()).c_str()),
		 _column(colname.toUtf8().constData())
		{
		}
		const char* column() const noexcept
		{
			return _column.c_str();
		}
	private:
		std::string _column;
	};

signals:
	void opened();
	void opened(bool);
	void closed();
	void cacheChanged(const run_cache_t* cache);

private:
	void importCache();
	sqlite3* _db;
	run_cache_t _cache;
};

#endif//DATABASE_H
