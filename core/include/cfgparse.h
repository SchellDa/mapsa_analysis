#ifndef CFG_PARSE_H
#define CFG_PARSE_H

#include <stdexcept>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <boost/lexical_cast.hpp>
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE
#endif
#include <regex.h>

namespace core {

/**
 * \brief Parser for very simple configuration files
 *
 * The configuration syntax allows for simple assignments, comments and variable substitutions.
 * Effectively, it is the typical ini-syntax without sections.
 *
 * \section Syntax
 * - A comment is started with a #-symbol, the rest of the line is ignored
 * - A variable name can be anything not containing =, # or whitespace
 * - Values can be partialy substituted by other values using the \@var\@ statement
 *
 * \section Example
 * \code{.cfg}
data_path = /path/to/some/directory
run_db = @data_path@/run.database
run_data_file = @data_path@/run@RunNumber@.data # RunNumber is set by software via setVariable()
\endcode
 */
class CfgParse
{
public:
	CfgParse();

	/** \brief Load configuration file
	 *
	 * \param filename Name of the file to load
	 * \sa parse(const std::istream& config, const std::string& filename)
	 * \throw std::ios_base::failure if file could not be opened
	 * \throw parse_error An error occured during parsing
	 */
	void load(const std::string& filename);

	/** \brief Parse configuration from a string
	 *
	 * \param config String containing the configuration
	 * \param filename Filename that is used for error and warning messages
	 * \sa parse(const std::istream& config, const std::string& filename)
	 * \throw parse_error An error occured during parsing
	 */
	void parse(const std::string& config, const std::string& filename="string");

	/** \brief Parse configuration from a general std::istream
	 *
	 * Used internaly by the CfgParse.
	 *
	 * \param config Stream to read config from
	 * \param filename Filename that is used for error and warning messages
	 * \throw parse_error An error occured during parsing
	 */
	void parse(std::istream& config, const std::string& filename="string");

	/// Set a variable to a specific value
	template<typename T>
	void setVariable(const std::string& var, const T& value)
	{
		_variables[var] = std::to_string(value);
	}

	void setVariable(const std::string& var, const std::string& value)
	{
		_variables[var] = value;
	}

	/**\brief Query a variable and perform replacements as required
	 *
	 * \throw no_variable_error The requested variable name was not found or a substitution variable was not found
	 * \throw recursion_error Substituting parts of the variable lead to infinite recursion
	 */
	std::string getVariable(const std::string& var) const { return getVariable(var, 0, var); }

	/**\brief Queries config variable and casts the value into requested type.
	 *
	 * The function queries the variable using getVariable()
	 *
	 * \throw no_variable_error The requested variable name was not found or a substitution variable was not found
	 * \throw recursion_error Substituting parts of the variable lead to infinite recursion
	 * \throw bad_cast Variable cannot be casted into requested type.
	 */
	template<typename T>
	T get(const std::string& var) const
	{
		auto value = getVariable(var);
		try {
			return boost::lexical_cast<T>(value);
		} catch(boost::bad_lexical_cast& e) {
			throw bad_cast(var, typeid(T).name(), value);
		}
	}

	/**\brief Queries a config variable and returns multiple entries as vector.
	 *
	 * The function queries the variable using getVariable(), then uses an std::istringstream to get individual
	 * entries from the string and casts them to the requessted type.
	 *
	 * \throw no_variable_error The requested variable name was not found or a substitution variable was not found
	 * \throw recursion_error Substituting parts of the variable lead to infinite recursion
	 * \throw bad_cast Variable cannot be casted into requested type.
	 */
	template<typename T>
	std::vector<T> getVector(const std::string& var) const
	{
		std::istringstream is(getVariable(var));
		std::vector<T> vec;
		while(is.good()) {
			std::string value;
			is >> value;
			if(value.size() == 0) {
				continue;
			}
			try {
				vec.push_back(boost::lexical_cast<T>(value));
			} catch(boost::bad_lexical_cast& e) {
				throw bad_cast(var, typeid(T).name(), value);
			}
		}
		return vec;
	}

	/** \brief Return a vector with the names of all defined variables
	 */
	std::vector<std::string> getDefinedVariables() const;

	/// \brief Value casting error
	class bad_cast : public std::invalid_argument {
	public:
		bad_cast(const std::string& variable, const std::string& target_type, const std::string& value)
		 : std::invalid_argument(""), msg()
		{
			std::ostringstream sstr;
			sstr << "Cannot cast variable '" << variable << "' with value '" << value
			     << "' to type " << target_type;
			msg = sstr.str();
		}
		virtual const char* what() const noexcept
		{
			return msg.c_str();
		}

	private:
		std::string msg;
	};

	/// \brief Infinite variable substitution recursion exception
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

	/// \brief Undefined ariable access exception
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

	/// \brief Parsing error exception
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
	std::string getVariable(const std::string& var, size_t depth, const std::string& original) const;
	std::map<std::string, std::string> _variables;
	regex_t regexSubstitution;
};

} // namespace core

#endif
