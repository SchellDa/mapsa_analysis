
#include "cfgparse.h"
#include <fstream>
#include <cctype>
#include "trim.h"

using namespace core;

CfgParse::CfgParse()
{
	int ret = regcomp(&regexSubstitution, "@([^@]+)@", REG_EXTENDED);
	if(ret) {
		throw std::runtime_error("Cannot compile regex");
	}
}

void CfgParse::load(const std::string& filename)
{
	std::ifstream fin(filename);
	//fin.exceptions(fin.failbit);
	parse(fin, filename);
}

void CfgParse::parse(const std::string& config, const std::string& filename)
{
	std::istringstream in(config);
	parse(in, filename);
}

void CfgParse::parse(std::istream& config, const std::string& filename)
{
	std::string line;
	int row = 0;
	while(std::getline(config, line)) {
		row++;
		auto tokens = tokenize(line);
		if(tokens.size() == 0) {
			continue;
		}
		if(tokens[0] == "=") {
			throw parse_error("Excpected identifier before token '='", row, -1, filename);
		}
		if(tokens.size() < 2) {
			throw parse_error("Malformed expression", row, -1, filename);
		}
		if(tokens[1] != "=") {
			throw parse_error("Excepted '=' token after identifier", row, -1, filename);
		}
		if(tokens.size() < 3) {
			setVariable(tokens[0], "");
		} else {
			if(tokens[2] == "=") {
				throw parse_error("Excepted value not '=' token", row, -1, filename);
			}
			if(tokens.size() > 3) {
				throw parse_error("Additional values after assignment", row, -1, filename);
			}
			setVariable(tokens[0], tokens[2]);
		}
	}
}

void CfgParse::setVariable(const std::string& var, const std::string& value)
{
	_variables[var] = value;
}

std::string CfgParse::getVariable(const std::string& var, size_t depth, const std::string& original)
{
	if(_variables.find(var) == _variables.end()) {
		throw no_variable_error(var, depth, original);
	}
	if(depth > _variables.size())
	{
		throw recursion_error(original);
	}
	std::string value = _variables[var];
	int ret;
	regmatch_t m[2];
	while((ret = regexec(&regexSubstitution, value.c_str(), 2, m, 0)) != REG_NOMATCH) {
		auto sub_var = value.substr(m[1].rm_so, m[1].rm_eo - m[1].rm_so);
		auto sub_value = getVariable(sub_var, depth+1, original);
		value = value.replace(m[0].rm_so, m[0].rm_eo - m[0].rm_so, sub_value);
	}
	return value;
}

std::vector<std::string> CfgParse::getDefinedVariables() const
{
	std::vector<std::string> vars;
	for(const auto& items: _variables)
		vars.push_back(items.first);
	return vars;
}

std::vector<std::string> CfgParse::tokenize(const std::string& line) const
{
	std::vector<std::string> tokens;
	std::string current_token("");
	for(const char& c: line) {
		bool add_token = false;
		if(c == '=' || c == '#' || current_token == "=") {
			add_token = true;
		}
		if(add_token) {
			if(current_token.length() > 0) {
				tokens.push_back(trim(current_token));
			}
			current_token = "";
		}
		bool is_comment = c == '#';
		if(!is_comment && (
		   current_token.length() > 0 ||
		   (!std::isspace(c) && current_token.length() == 0))) {
			current_token += c;
		}
		if(c == '#') {
			break;
		}
	}
	if(current_token.length() > 0) {
		tokens.push_back(trim(current_token));
	}
	return tokens;
}

