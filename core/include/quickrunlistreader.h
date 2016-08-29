
#ifndef QUICK_RUNLIST_READER_H
#define QUICK_RUNLIST_READER_H

#include <string>
#include <vector>

namespace core {

class QuickRunlistReader
{
public:
	struct run_t
	{
		int mpa_run;
		int telescope_run;
		int data_offset;
		double angle;
	};
	typedef std::vector<run_t> run_vec_t;

	void read(const std::string& filename);

	int getMpaRunByTelRun(int telRun) const;
	int getTelRunByMpaRun(int mpaRun) const;

	void clear() { _runs.clear(); }
	size_t size() { return _runs.size(); }

	run_vec_t& getRuns() { return _runs; }
	const run_vec_t& getRuns() const { return _runs; }

	run_vec_t::const_iterator begin() const { return _runs.begin(); }
	run_vec_t::iterator begin() { return _runs.begin(); }
	run_vec_t::const_iterator end() const { return _runs.end(); }
	run_vec_t::iterator end() { return _runs.end(); }
private:
	run_vec_t _runs;
};

} // namespace core

#endif//QUICK_RUNLIST_READER_H
