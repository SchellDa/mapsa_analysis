#ifndef CFG_PARSE_H
#define CFG_PARSE_H

#include <stdexcept>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE
#endif
#include <regex.h>

class CfgParse
{
public:
	CfgParse();

	/// Load configuration file
	void load(const std::string& filename);

	/// Set a variable to a specific value
	void setVariable(const std::string& var, const std::string& value);

	/// Query a variable and perform replacements as required
	/// Exceptions: NotFound
	std::string getVariable(const std::string& var) { return getVariable(var, 0, var); }
	
	class recursion_error : public std::runtime_error {
	public:
		recursion_error(const std::string& variable)
		 : std::runtime_error(""), msg()
		{
			std::ostringstream sstr;
			sstr << "Recursion detected in variable substitution for  '" << variable << "'";
			msg = sstr.str();
		}
		virtual const char* what() const noexcept
		{
			return msg.c_str();
		}

	private:
		std::string msg;
	};

	class no_variable_error : public std::runtime_error {
	public:
		no_variable_error(const std::string& variable, int depth, const std::string& original)
		 : std::runtime_error(""), msg()
		{
			std::ostringstream sstr;
			sstr << "Variable '" << variable << "' not found";
			if(depth > 0) {
				sstr << " while applying substitutions to '" << original << "'";
			}
			msg = sstr.str();
		}
		virtual const char* what() const noexcept
		{
			return msg.c_str();
		}

	private:
		std::string msg;
	};

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
	std::vector<std::string> tokenize(const std::string& line) const;
	std::string getVariable(const std::string& var, size_t depth, const std::string& original);
	std::map<std::string, std::string> _variables;
	regex_t regexSubstitution;
};

#endif
