
#include <getopt.h>
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <condition_variable>
#include <memory>
#include "utilsconfig.h"
#include "core.h"
#include "analysis.h"

std::mutex mutex_cerr;
std::mutex mutex_workercounter;
size_t g_numWorkers = 0;
bool g_quit = false;

class Job
{
public:
	Job() :
		_analysis(nullptr), _vm(), _isExecutable(false)
	{
	}

	Job(const Job& othr) = delete;

	Job(Job&& other) :
		_analysis(std::move(other._analysis)), _vm(std::move(other._vm)), _isExecutable(other._isExecutable)
	{
	}

	~Job()
	{
	}

	bool setup(std::vector<std::string> argv, std::ostream& output)
	{
		assert(argv.size() > 1);
		std::string name = argv[1];
		if(name[0] == '-') {
			output << argv[0] << ": Invalid option " << name << ", excpected analysis name.\n\n"
			       << "Usage: " << argv[0] << " ANALYSIS [options]\n"
			           "See '" << argv[0] << " --help' for more information" << std::endl;
			return _isExecutable = false;
		}
		std::string analysis_name = argv[1];
		_argv0 = argv[0] + " " + argv[1];
		_cmdline = _argv0;
		for(int i=1; i<argv.size()-1; ++i) {
			argv[i] = argv[i+1];
			_cmdline += " ";
			_cmdline += argv[i];
		}
		argv[0] = _argv0;
		argv.pop_back();
		try {
			_analysis = std::move(core::AnalysisFactory::Instance()->create(analysis_name));
		} catch(std::out_of_range& e) {
			output << "Analysis '" << analysis_name << "' was not found." << std::endl;
			return _isExecutable = false;
		}
		assert(analysis.get());
		try {
			std::vector<const char*> argv_c;
			for(const auto& arg: argv) {
				argv_c.push_back(arg.c_str());
			}
			po::store(po::command_line_parser(argv_c.size(), &argv_c.front())
			          .options(_analysis->getOptionsDescription())
				  .positional(_analysis->getPositionalsDescription())
				  .run(),
			          _vm);
		} catch(std::exception& e) {
			std::cerr << argv[0] << ": " << e.what();
			std::cerr << "\n\n" << _analysis->getUsage(_argv0) << std::endl;
			return _isExecutable = false;
		}
		if(_vm.count("help")) {
			// get usage from analysis class
			auto help = _analysis->getHelp(_argv0);
			if(help != "")
				std::cout << help << "\n\n";
			output << _analysis->getOptionsDescription() << std::endl;
			return _isExecutable = false;
		}
		try {
			po::notify(_vm);
			if(!_analysis->loadConfig(_vm)) {
				return _isExecutable = false;
			}
			if(!_analysis->multirunConsistencyCheck(_argv0, _vm)) {
				return _isExecutable = false;
			}
			_isExecutable = true;
		} catch(po::error& e) {
			output << _argv0 << ": " << e.what() << std::endl;
		} catch(core::CfgParse::parse_error& e) {
			output << _argv0 << ": Error while parsing configuration:\n" << e.what() << std::endl;
		} catch(core::CfgParse::no_variable_error& e) {
			output << _argv0 << ": Configuration Error: " << e.what() << std::endl;
		} catch(core::CfgParse::bad_cast& e) {
			output << _argv0 << ": Configuration Error: " << e.what() << std::endl;
		} catch(std::exception& e) {
			output << _argv0 << ": Exception occured, Aborting! " << e.what() << std::endl;
		}
		return _isExecutable;
	}

	bool isExecutable() const
	{
		return _isExecutable;
	}

	bool execute(std::ostream& output)
	{
		try {
			_analysis->run(_vm);
		} catch(core::CfgParse::parse_error& e) {
			output << _argv0 << ": Error while parsing configuration:\n" << e.what() << std::endl;
			return false;
		} catch(core::CfgParse::no_variable_error& e) {
			output << _argv0 << ": Configuration Error: " << e.what() << std::endl;
			return false;
		} catch(core::CfgParse::bad_cast& e) {
			output << _argv0 << ": Configuration Error: " << e.what() << std::endl;
			return false;
		} catch(std::exception& e) {
			output << _argv0 << ": Exception occured, Aborting! " << e.what() << std::endl;
			return false;
		}
		return true;
	}

	std::string getCommandline() const { return _cmdline; }

private:
	std::unique_ptr<core::Analysis> _analysis;
	po::variables_map _vm;
	bool _isExecutable;
	std::string _argv0;
	std::string _cmdline;
};

std::ostream & operator<<(std::ostream& f, const Job& job)
{
	f << job.getCommandline();
	return f;
}

class JobQueue
{
public:
	JobQueue()
	{
	}

	~JobQueue()
	{
	}

	void push(Job&& job)
	{
		std::lock_guard<std::mutex>(this->_mutex);
		_jobs.push(std::move(job));
		wait_for_work.notify_one();
	}
	Job pop()
	{
		std::lock_guard<std::mutex>(this->_mutex);
		if(_jobs.size() == 0) {
			throw no_jobs_left();
		}
		auto j = std::move(_jobs.front());
		_jobs.pop();
		return j;
	}
	size_t size()
	{
		std::lock_guard<std::mutex>(this->_mutex);
		return _jobs.size();
	}

	class no_jobs_left : public std::exception
	{
	};

	void startWorkers()
	{
		stop_worker = false;
		auto num_threads_str = std::getenv("NUM_THREADS");
		int num_threads = std::thread::hardware_concurrency();
		if(num_threads_str) {
			num_threads = std::stoi(num_threads_str);
			assert(num_threads > 0);
		}
		for(size_t i = 0; i < num_threads; ++i) {
			_workers.push_back(std::thread(JobQueue::workerFunction, i+1, this));
		}
	}

	void stopWorkers()
	{
		std::lock_guard<std::mutex>(this->_mutex);
		while(!_jobs.empty()) {
			_jobs.pop();
		}
		stop_worker = true;
		wait_for_work.notify_one();
		for(auto& w: _workers) {
			w.join();
		}
	}

private:
	static void workerFunction(int workerId, JobQueue* jobQueue)
	{
		/*{
			std::lock_guard<std::mutex> cl(mutex_cerr);
			std::cerr << "Worker " << workerId << " started." << std::endl;
		}*/
		while(!stop_worker) {
			try {
				auto job = std::move(jobQueue->pop());
				auto numJobs = jobQueue->size();
				{
					std::lock_guard<std::mutex> cl(mutex_cerr);
					std::cerr << "Worker " << workerId
					          << " (" << numJobs << " jobs left): '" << job << "'" << std::endl;
				}
				job.execute(std::cout);
			} catch(JobQueue::no_jobs_left& e) {
				std::unique_lock<std::mutex> lk(JobQueue::cv_mutex);
				JobQueue::wait_for_work.wait_for(lk, std::chrono::seconds(2));
			} catch(std::exception& e) {
				std::lock_guard<std::mutex> cl(mutex_cerr);
				std::cerr << "Error occured during execution of Analysis: " << e.what() << std::endl;
			}
		}
	}
	std::queue<Job> _jobs;
	std::mutex _mutex;
	static std::condition_variable wait_for_work;
	static std::mutex cv_mutex;
	static bool stop_worker;
	std::vector<std::thread> _workers;
};

std::condition_variable JobQueue::wait_for_work;
std::mutex JobQueue::cv_mutex;
bool JobQueue::stop_worker = false;


class Listener
{
public:
	Listener()
	 : _socket(0)
	{
		std::string path(BELPHEGOR_SOCKET);
		if(getenv("BELPHEGOR_SOCKET")) {
			path = getenv("BELPHEGOR_SOCKET");
		}
		unlink(path.c_str());
		struct sockaddr_un name;
		_socket = socket(AF_UNIX, SOCK_STREAM, 0);
		if(_socket < 0) {
			throw std::ios_base::failure(std::string("socket: ")+strerror(errno));
		}
		name.sun_family = AF_UNIX;
		strncpy(name.sun_path, path.c_str(), sizeof(name.sun_path));
		name.sun_path[sizeof(name.sun_path)-1] = 0;
		size_t size = (offsetof(struct sockaddr_un, sun_path) + strlen(name.sun_path));
		if(bind(_socket, (struct sockaddr*) &name, size) < 0) {
			throw std::ios_base::failure(std::string("bind: ")+strerror(errno));
		}
	}
	~Listener()
	{
		close(_socket);
	}
	void start()
	{
		_queue.startWorkers();
		listen(_socket, 5);
		int csock = 0;
		const size_t bufsize = 8192;
		char buf[bufsize];
		while(!g_quit) {
			csock = accept(_socket, nullptr, nullptr);
			if(csock == -1) {
				std::cerr << "Listener::listen(): accept: << " << strerror(errno) << std::endl;
				continue;
			}
			size_t size = recv(csock, &buf, bufsize, MSG_PEEK);
			std::string command;
			auto argv = getArgv(std::string(buf, size), command);
			if(command == "WAKE") {
				std::cout << "Belphegor awakens." << std::endl;
			} else if(command == "SACRIFICE") {
				if(argv.size() == 1) {
					std::cout << "Supported analyses:\n";
					for(const auto& name: core::AnalysisFactory::Instance()->getTypes()) {
						std::cout << " " << name << " : " <<
							core::AnalysisFactory::Instance()->getDescription(name) << "\n";
					}
				} else {
					if(argv[1][0] == '-') {
						handleCommand(csock, argv);
					} else {
						std::cout << "Belphegor accepts your sacrifice." << std::endl;
						submit(csock, argv);
					}
				}
			}
			close(csock);
		}
	}

	static std::vector<std::string> getArgv(const std::string& cmdline, std::string& command)
	{
		std::vector<std::string> vec;
		std::string current;
		size_t i = 0;
		for(const auto& c: cmdline) {
			if(c != '\0') {
				current += c;
			} else {
				if(i == 0) {
					command = current;
				} else {
					vec.push_back(current);
				}
				current = "";
				++i;
			}
		}
		if(current.size()) {
			if(i == 0) {
				command = current;
			} else {
				vec.push_back(current);
			}
		}
		return vec;
	}
private:
	void submit(int sock, const std::vector<std::string>& argv)
	{
		Job job;
		std::ostringstream sstr;
		if(job.setup(argv, sstr)) {
			_queue.push(std::move(job));
		}
		auto s = sstr.str();
		send(sock, &s[0], s.size()+1, 0);
	}
	void handleCommand(int sock, const std::vector<std::string>& argv)
	{

		std::ostringstream sstr;
		sstr << "Job Queue size: " << _queue.size() << std::endl;	
		auto s = sstr.str();
		send(sock, &s[0], s.size()+1, 0);
	}
	int _socket;
	JobQueue _queue;
};

void wakeMaster()
{
	std::string path(BELPHEGOR_SOCKET);
	if(getenv("BELPHEGOR_SOCKET")) {
		path = getenv("BELPHEGOR_SOCKET");
	}
	struct sockaddr_un name;
	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sock < 0) {
		throw std::ios_base::failure(std::string("wakeMaster: socket: ")+strerror(errno));
	}
	name.sun_family = AF_UNIX;
	strncpy(name.sun_path, path.c_str(), sizeof(name.sun_path));
	name.sun_path[sizeof(name.sun_path)-1] = 0;
	size_t size = (offsetof(struct sockaddr_un, sun_path) + strlen(name.sun_path));
	if(connect(sock, (struct sockaddr*)& name, sizeof(name)) != 0) {
		throw std::ios_base::failure(std::string("wakeMaster: connect: ")+strerror(errno));
	}
	std::string message("WAKE\0");
	send(sock, message.c_str(), message.size(), 0);
	close(sock);
}


int main(int argc, char* argv[])
{
	core::initClasses();
	Listener listener;
	listener.start();
}

