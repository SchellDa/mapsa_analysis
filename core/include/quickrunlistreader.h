
#ifndef QUICK_RUNLIST_READER_H
#define QUICK_RUNLIST_READER_H

#include <string>
#include <vector>

namespace core {

/** \brief Read runlist CSV files and interpret the most important information in it.
 *
 * Runlist files contain crucial per-run information required for analyzing the testbeam data. The
 * QuickRunlistReader class is a quick-and-dirty implementation of a tab seperated values file. Several
 * methods provide easy access to the most important information stored in the file.
 *
 * A better implementation would use a "real" CSV/TSV file interpreter as underlying parser.
 */
class QuickRunlistReader
{
public:
	/** \brief Important data from a single line in the runlist CSV */
	struct run_t
	{
		/// MPA run ID
		int mpa_run;
		/// Telescope run ID
		int telescope_run;
		/// Offset of the MPA data relative to the telescope data.
		/// \sa Analysis::setDataOffset
		int data_offset;
		/// Angle of the DUT in degrees
		double angle;
		/// Applied bias voltage in V
		double bias_voltage;
		/// Measured bias current in uA
		double bias_current;
		/// Threshold voltage in V
		double threshold;
	};
	typedef std::vector<run_t> run_vec_t;

	/** \brief Import run data from file
	 *
	 * Repeatedly calling read appends the data from each read, so there is potential doublication of run
	 * IDs! Be careful if you do that. Or call clear() first.
	 *
	 * \warning No error is reported if the value in a column cannot be converted or is empty! A default
	 * value of 0 is used in that case. This behaviour underlines the quick-and-dirty aspect of the
	 * QuickRunlistReader
	 * \param filename Name of the file to read
	 * \throw std::ios_base::failure File was not found
	 * \sa clear
	 */
	void read(const std::string& filename);

	/** \brief Return the MPA run corresponding to a certain telescope run
	 *
	 * \param telRun Telescope run ID
	 * \return MPA run id
	 * \throw std::invalid_argument MPA run ID not specified in loaded runlist data
	 */
	int getMpaRunByTelRun(int telRun) const;
	
	/** \brief Return the telescope run corresponding to a certain MPA run
	 *
	 * \param mpaRun MPA run id
	 * \return Telescope run ID
	 * \throw std::invalid_argument Telescope run ID not specified in loaded runlist data
	 */
	int getTelRunByMpaRun(int mpaRun) const;

	/** \return Get all interpreted information for specified MPA run
	 *
	 * \param mpaRun MPA run ID
	 * \return Run-specific information
	 * \throw std::invalid_argument Run ID was not specified in loaded runlist data
	 */
	const run_t& getByMpaRun(int mpaRun) const;
	
	/** \return Get all interpreted information for specified telescope run
	 *
	 * \param telRun Telescope run ID
	 * \return Run-specific information
	 * \throw std::invalid_argument Run ID was not specified in loaded runlist data
	 */
	const run_t& getByTelRun(int telRun) const;

	/** \brief Clear all loaded runlist data
	 */
	void clear() { _runs.clear(); }
	/** \brief Returns number loaded runs */
	size_t size() { return _runs.size(); }

	/** \brief Get a vector to all loaded run data */
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
