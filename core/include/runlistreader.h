#ifndef RUNLIST_READER_H
#define RUNLIST_READER_H

#include "cfgparse.h"
#include <map>
#include <vector>
#include <stdexcept>

namespace core
{

class RunlistReader
{
public:
	RunlistReader(CfgParse& cfg);
	void load(std::string filename);
	bool loadRun();
	bool loadRun(int runId);

	class parse_error : public std::runtime_error {
	public:
		parse_error(const std::string& message, const int& row, const int& col, const std::string& file)
		 : std::runtime_error(""), msg()
		{
			std::ostringstream sstr;
			if(file.length() > 0)
				sstr << file << ":";
			sstr << row << ":";
			if(col != -1)
				sstr << col << ":";
			sstr << " " << message;
			msg = sstr.str();
		}
		virtual const char* what() const noexcept
		{
			return msg.c_str();
		}

	private:
		std::string msg;
	};

private:
	std::vector<std::string> split(std::string line);
	CfgParse& _cfg;
	std::vector<std::string> _keys;
	std::map<int, std::vector<std::string>> _values;
};

} // namespace core

#endif//RUNLIST_READER_H
