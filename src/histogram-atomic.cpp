/**
 * @author Saleh Elkaza
 * @id 1622901
 */

#include <thread>
#include <vector>
#include <iostream>
#include <random>
#include <chrono>
#include <numeric>
#include <atomic>

using namespace std;

// Parsing arguments
// Function to parse the command line arguments.
void parse_args(int argc, char **argv, int& num_threads, int &buckets, int &sample_size, int& print_level) {
    std::string usage("Usage: --num-threads <integer> --N <integer> --samlpe-size <integer> --print-level <integer>");
	// Parsing command line arguments and storing values.
    for (int i = 1; i < argc; ++i) {
        if ( std::string(argv[i]).compare("--num-threads") == 0 ) {
            num_threads=std::stoi(argv[++i]);
        } else if ( std::string(argv[i]).compare("--N") == 0 ) {
            buckets=stoi(argv[++i]);
        } else if ( std::string(argv[i]).compare("--sample-size") == 0 ) {
            sample_size=stoi(argv[++i]);
        } else if ( std::string(argv[i]).compare("--print-level") == 0 ) {
            print_level=stoi(argv[++i]);
		} else if (  std::string(argv[i]).compare("--help") == 0 ) {
			// Print usage message and exit if "--help" is passed.
            std::cout << usage << std::endl;
            exit(-1);
        }
    }
};

struct generator_cfg {
	// Configurations for generating random numbers.
	generator_cfg(int max) : max(max) {}
	int max;
};

struct generator {
	// let's generate random number like this for consistent tests
	// Random number generator.
	generator(const generator_cfg& cfg) : dist(0, cfg.max) {	}
	int operator()() {
		return dist(engine);
	}
private:
	// Random number generation tools.
	minstd_rand engine; 
	uniform_int_distribution<int> dist;
};

struct histogram {
	 // Histogram to store and count occurrences.
	histogram(int count) : data(count) { }

	void add(int i) {
		//++data[i];
		data[i].fetch_add(1, std::memory_order_relaxed);
	}

	int get(int i)	{
		// return data[i];
		return data[i].load();
	}

	void print(std::ostream& str) {
		// Print histogram data.
		for (size_t i = 0; i < data.size(); ++i) str << i << ":" << data[i] << endl;
		str << "total:" << accumulate(data.begin(), data.end(), 0) << endl;
	}

private:
	// Data storage for the histogram.
	// vector<int> data;
	vector<atomic<int>> data;
};

struct worker
{	// Worker structure to perform computations.
	worker(int sample_count, histogram& h, const generator_cfg& cfg) 
		: sample_count(sample_count), h(h), cfg(cfg) 
	{ }
	// Generate samples and update histogram.
	long do_work() {
		long count = 0.0;

		generator gen(cfg);
		while (sample_count--) {
			int next = gen();
			h.add(next);
			count++;
		}

		return count;
	}
private:
	int sample_count;
	histogram& h;
	const generator_cfg& cfg;
};

int main(int argc, char **argv)
{
	int N = 10000;
	int sample_count = 30000000;

	//int num_threads = 3;//std::thread::hardware_concurrency();
    int num_threads = std::thread::hardware_concurrency();
	int print_level = 2; // 0 exec time only, 1 histogram, 2 histogram + config
	// Parse command line arguments.
	parse_args(argc, argv, num_threads, N, sample_count, print_level);
	
	generator_cfg cfg(N);
	histogram h(N+1);

	vector<thread> threads;  // <--------------------
	atomic<long> global_processed_count(0);

	auto t1 = chrono::high_resolution_clock::now();

	for (int t = 0; t < num_threads; ++t)
	{
		int from = (sample_count*t / num_threads);
		int to = (sample_count*(t+1) / num_threads);

		threads.push_back(thread([from, to, &h, &cfg, &global_processed_count]() {
			long processed_count = worker(to - from, h, cfg).do_work();
			global_processed_count.fetch_add(processed_count, std::memory_order_relaxed);
		}));
	}

	for (auto& th : threads) {
		th.join();
	}

	auto t2 = chrono::high_resolution_clock::now();

	if ( print_level >= 2 ) 
		cout << "N: " << N << ", sample size: " << sample_count << ", threads: " << num_threads << ", items processed: " << global_processed_count.load() << endl;
	if ( print_level >= 1 ) 
		h.print(cout);
	cout << chrono::duration<double>(t2 - t1).count() << endl;
}