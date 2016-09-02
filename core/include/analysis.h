
#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <boost/program_options.hpp>
#include "cfgparse.h"
#include "abstractfactory.h"
#include "trackstreamreader.h"
#include "mpastreamreader.h"
#include "quickrunlistreader.h"
#include "mpatransform.h"

namespace po = boost::program_options;

namespace core {

/** \brief Abstract analysis interface
 *
 * Sub-classes of Analysis should implement the run() method to perform analysis tasks. The sub-classes should
 * register themselves with #REGISTER_ANALYSIS_TYPE(type, description) to the factory class. An executable
 * can then select which analysis object to create and run.
 *
 * On construction several command line options are registered. The sub-class may add additional options using
 * the boost::program_options::option_description object returned by getOptionsDescription().
 *
 * The loadConfig() implementation of Analysis attempts to load a configuration file if passed with the -c
 * command line option and applies any additional variables from the -D command line option.
 *
 * The functions getUsage() and getHelp() display default help messages if the -h option is specified or an
 * error in the command line arguments are found. A sub-class should reimplement them if additional options
 * are implemented. Note that getHelp() only displays a free-text help message and not a option-by-option
 * documentation. Each option is autodocumented by Boost.
 */
class Analysis
{
public:
	Analysis();
	virtual ~Analysis() {}

	enum callback_stop_t {
		CS_ALWAYS,
		CS_TRACK
	};
	typedef std::function<bool(const TrackStreamReader::event_t&,
	                           const MPAStreamReader::event_t&)> run_callback_t;
	typedef std::function<void()> post_callback_t;
	struct process_t {
		callback_stop_t mode;
		run_callback_t run;
		post_callback_t post;
	};

	/** \brief Load configuration from file and from command line
	 *
	 * Loads configuration file specified by -c option and executes any string given by -D as additional
	 * configuration text.
	 * \throw std::ios_base::failure Configuration file cannot be found
	 * \throw core::CfgParse::parse_error Configuration by file or command line cannot be parsed
	 */
	virtual bool loadConfig(const po::variables_map& vm);

	virtual void init(const po::variables_map& vm) {};

	/** \brief Perform analysis. Must be reimplemented.
	 *
	 * This is the work-horse to perform any actual analysis.
	 */
	virtual void run(const po::variables_map& vm);

	/** \brief Get boost::program_options::options_description object to add further command line
	 * arguments
	 */
	const po::options_description& getOptionsDescription() const { return _options; }
	po::options_description& getOptionsDescription() { return _options; }

	/** \brief Get boost::program_options::positional_options_description object to add further
	 * positional command line arguments
	 */
	const po::positional_options_description& getPositionalsDescription() const { return _positionals; }
	po::positional_options_description& getPositionalsDescription() { return _positionals; }

	/** \brief Get string describing the tool usage from the command line
	 *
	 * Sub-classes should reimplement this to return useful information. The default implementation merely
	 * advices to run -h for further help.
	 *
	 * The usage string usualy displays a command line with the different possible short-hand arguments,
	 * e.g. \verbatim ./analysis MyAnalysis [-h] [-c cfgfile] [-Dvar=val ...] [-o outfile] [--verbose] \endverbatim
	 *
	 * \return Usage string
	 * \param argv0 The command to call the analysis. Usually something like
	 * \verbatim ./command AnalysisType \endverbatim
	 */
	virtual std::string getUsage(const std::string& argv0) const;
	/** \brief Return help message to display before the option-by-option documentation
	 *
	 * \param argv0 The command to call the analysis. Usually something like
	 * \verbatim ./command AnalysisType \endverbatim
	 * Can be used for displaying example invokations.
	 * \return A free-text help message describing what the analysis does. Displayed before the
	 * option-by-option documentation when showing the help text via -h. Default implementation returns
	 * an empty string.
	 */
	virtual std::string getHelp(const std::string& argv0) const;

	static std::string getPaddedIdString(int id, unsigned int width);

	virtual std::string getMpaIdPadded(int id);
	virtual std::string getRunIdPadded(int id);

	virtual std::string getName() const;
	virtual std::string getRootFilename(const std::string& suffix="") const;
	virtual std::string getFilename(const std::string& suffix="") const;

protected:
	void addProcess(const process_t& proc);
	void addProcess(const callback_stop_t& mode,
	                const run_callback_t& run,
			const post_callback_t& stop=std::function<void()>())
	{
		addProcess({mode, run, stop});
	}
	void setDataOffset(int dataOffset);
	int getDataOffset() const { return _dataOffset; }
	void rerun();

	CfgParse _config;
	QuickRunlistReader _runlist;
	MpaTransform _mpaTransform;

private:
	void executeProcess(core::MPAStreamReader& mpareader,
	                    core::TrackStreamReader& trackreader,
                            const process_t& proc);

	po::options_description _options;
	po::positional_options_description _positionals;
	std::vector<process_t> _processes;
	int _dataOffset;
	bool _analysisRunning;
	bool _rerunProcess;
};

/** \brief short-hand type for the factory class for core::Analysis. */
typedef AbstractFactory<Analysis, std::string> AnalysisFactory;

/** \brief Register new analysis type
 *
 * Register an new analysis type. It must be a subclass of core::Analysis. Call this makro from the implementation
 * file of your analysis class.
 * \param type C++ identifier of the analysis to register.
 * \param descr String with a short description of the analysis.
 */ 
#define REGISTER_ANALYSIS_TYPE(type, descr) REGISTER_FACTORY_TYPE_WITH_DESCR(core::Analysis, type, descr)

}// namespace core

#endif//ANALYSIS_H
