#include "runlistreader.h"
#include <fstream>
#include <sstream>

using namespace core;


RunlistReader::RunlistReader(CfgParse& cfg) :
 _cfg(cfg)
{
}

void RunlistReader::load(std::string filename)
{
	std::ifstream fin;
	fin.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fin.open(filename);
	fin.exceptions(std::ifstream::goodbit);
	std::vector<std::string> keys;
	std::map<int, std::vector<std::string>> values;
	size_t lineno = 1;
	for(std::string line; std::getline(fin, line); lineno++) {
		auto tokens = split(line);
		if(lineno == 1) {
			for(size_t tok = 0; tok < tokens.size(); ++tok) {
				if(tokens[tok].size() == 0) {
					throw parse_error("Empty column names are invalid!", lineno, -1, filename);
				}
				if(tok == 0 && tokens[tok] != "MpaRun") {
					throw parse_error("First column must be named 'MpaRun'!", lineno, -1, filename);
				}
				keys.push_back(tokens[tok]);
			}
			continue;
		}
		if(line.size() == 0) {
			continue;
		}
		if(line[0] == '#') {
			continue;
		}
		if(tokens.size() != keys.size()) {
			throw parse_error("Invalid number of columns", lineno, -1, filename);
		}
		try {
			values[std::stoi(tokens[0])] = tokens;
		} catch(std::exception& e) {
			throw parse_error("First column 'MpaRun' must be integer value", lineno, -1, filename);
		}
	}
	_keys = keys;
	_values = values;
}

bool RunlistReader::loadRun()
{
	return loadRun(_cfg.get<int>("MpaRun"));
}

bool RunlistReader::loadRun(int runId)
{
	if(_keys.size() == 0 || _values.size() == 0) {
		return true;
	}
	if(_values.find(runId) == _values.end()) {
		return false;
	}
	for(size_t i = 0; i < _keys.size(); ++i) {
		auto key = _keys[i];
		auto val = _values[runId][i];
		_cfg.setVariable(key, val);
	}
	return true;
}

std::vector<std::string> RunlistReader::split(std::string line)
{
	std::vector<std::string> tokens;
	std::istringstream ss(line);
	while(!ss.eof()) {
		std::string x;
		std::getline(ss, x, ';');
		tokens.push_back(x);
	}
	return tokens;
}
