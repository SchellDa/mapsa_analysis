
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <string>
#include <vector>
#include <cstdlib>
#include <cassert>

std::mutex mutex_cerr;

class JobQueue
{
public:
	JobQueue()
	{
	}

	~JobQueue()
	{
	}

	void push(const std::string& job)
	{
		std::lock_guard<std::mutex>(this->_mutex);
		_jobs.push(job);
	}
	std::string pop()
	{
		std::lock_guard<std::mutex>(this->_mutex);
		if(_jobs.size() == 0) {
			throw std::runtime_error("No jobs left");
		}
		auto j = _jobs.front();
		_jobs.pop();
		return j;
	}
	size_t size()
	{
		std::lock_guard<std::mutex>(this->_mutex);
		return _jobs.size();
	}

private:
	std::queue<std::string> _jobs;
	std::mutex _mutex;
};

void workerFunction(int workerId, JobQueue* jobQueue)
{
	{
		std::lock_guard<std::mutex> cl(mutex_cerr);
		std::cerr << "Worker " << workerId << " started." << std::endl;
	}
	try {
		while(true) {
			auto job = jobQueue->pop();
			auto numJobs = jobQueue->size();
			{
				std::lock_guard<std::mutex> cl(mutex_cerr);
				std::cerr << "Worker " << workerId
				          << " (" << numJobs << " jobs left): '" << job << "'" << std::endl;
			}
			system(job.c_str());
		}
	} catch(const std::runtime_error& e) {
		std::lock_guard<std::mutex> cl(mutex_cerr);
		std::cerr << "Worker " << workerId
		          << " has no jobs left and exists." << std::endl;
	}
}

int main(int argc, char* argv[])
{
	JobQueue jobQueue;
	auto num_threads_str = std::getenv("NUM_THREADS");
	int num_threads = std::thread::hardware_concurrency();
	if(num_threads_str) {
		num_threads = std::stoi(num_threads_str);
		assert(num_threads > 0);
	}
	if(argc > 1) {
		for(size_t i=1; i<argc; ++i) {
			jobQueue.push(argv[i]);
		}
	} else {
		std::string line;
		while(!std::cin.fail()) {
			std::getline(std::cin, line);
			jobQueue.push(line);
		}
	}
	std::vector<std::thread> workers;
	std::cerr << "Start " << num_threads
	          << " worker threads." << std::endl;
	for(size_t i = 0; i < num_threads; ++i) {
		workers.push_back(std::thread(workerFunction, i+1, &jobQueue));
	}
	for(auto& w: workers) {
		w.join();
	}
	std::cerr << "My busineeeeess ... ! is done" << std::endl;
	return 0;
}
