
#ifndef TRACK_ANALYSIS_H
#define TRACK_ANALYSIS_H

#include "analysis.h"
#include "trackstreamreader.h"
#include "basesensorstreamreader.h"
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
class TrackAnalysis : public Analysis
{
public:
	TrackAnalysis();
	virtual ~TrackAnalysis() {}

	enum callback_stop_t {
		CS_ALWAYS,
		CS_TRACK
	};
	typedef std::function<void()> init_callback_t;
	typedef std::function<void()> run_init_callback_t;
	typedef std::function<bool(const TrackStreamReader::event_t&,
	                           const BaseSensorStreamReader::event_t&)> run_callback_t;
	typedef std::function<void()> run_post_callback_t;
	typedef std::function<void()> post_callback_t;
	struct process_t {
		std::string name;
		callback_stop_t mode;
		init_callback_t init;
		run_init_callback_t run_init;
		run_callback_t run;
		run_post_callback_t run_post;
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

	virtual std::string getRunIdPadded(int id) const;
	
	virtual bool multirunConsistencyCheck(const std::string& argv0, const po::variables_map& vm);

protected:
	void addProcess(const process_t& proc);
	void addProcess(const std::string& name,
	                const callback_stop_t& mode,
	                const init_callback_t& init,
			const run_init_callback_t& run_init,
	                const run_callback_t& run,
	                const run_post_callback_t& run_post,
			const post_callback_t& stop)
	{
		addProcess({name, mode, init, run_init, run, run_post, stop});
	}
	void setDataOffset(int dataOffset);
	int getDataOffset() const { return _dataOffset; }
	void rerun();
	size_t getRerunNumber() const { return _rerunNumber; }

	QuickRunlistReader _runlist;
	MpaTransform _mpaTransform;

	const std::vector<int>& getAllRunIds() const { return _allRunIds; }
	int getCurrentRunId() const { return _currentRunId; }

private:
	struct run_read_pair_t
	{
		int runId;
		std::shared_ptr<core::BaseSensorStreamReader> pixelreader;
		core::TrackStreamReader trackreader;
	};

	void executeProcess(const std::vector<run_read_pair_t>& reader,
                            const process_t& proc);

	std::vector<process_t> _processes;
	int _dataOffset;
	bool _analysisRunning;
	bool _rerunProcess;
	size_t _rerunNumber;
};

}// namespace core

#endif//TRACK_ANALYSIS_H
