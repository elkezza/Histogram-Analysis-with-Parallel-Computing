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
#include <future>
#include <mutex>
#include <atomic>

using namespace std;

/**
 * Parsing arguments
 * 
*/
void parse_args(int argc, char **argv, int& num_threads, int &buckets, int &sample_size, int& print_level, int &M) {
    std::string usage("Usage: --num-threads <integer> --N <integer> --samlpe-size <integer> --print_level --MB");

    for (int i = 1; i < argc; ++i) {
        if ( std::string(argv[i]).compare("--num-threads") == 0 ) {
            num_threads=std::stoi(argv[++i]);
        } else if ( std::string(argv[i]).compare("--N") == 0 ) {
            buckets=stoi(argv[++i]);
        } else if ( std::string(argv[i]).compare("--MB") == 0 ) {
            M=stoi(argv[++i]);
        } else if ( std::string(argv[i]).compare("--sample-size") == 0 ) {
            sample_size=stoi(argv[++i]);
        } else if ( std::string(argv[i]).compare("--print-level") == 0 ) {
            print_level=stoi(argv[++i]);
		} else if (  std::string(argv[i]).compare("--help") == 0 ) {
            std::cout << usage << std::endl;
            exit(-1);
        }
    }
};

struct generator_cfg
{
	generator_cfg(int max) : max(max) {}
	int max;
};

struct generator
{
	generator(const generator_cfg& cfg) : dist(0, cfg.max), engine((rd())) {	}
	int operator()()
	{
		return dist(engine);
	}
private:
	std::random_device rd;
	mt19937 engine;
	uniform_int_distribution<int> dist;
};

struct histogram
{
	 histogram(int count) : data(count), locks(count) { }
	    void add(int i) {
        if (data.size() < 100) { 
            std::lock_guard<std::mutex> lock(locks[i]);
            ++data[i];
        } else { 
            data[i].fetch_add(1, std::memory_order_relaxed);
        }
    }
    int get(int i) {
        return data[i].load(std::memory_order_relaxed);
    }

	void print(std::ostream& str)
	{
		for (size_t i = 0; i < data.size(); ++i) str << i << ":" << data[i] << endl;
		str << "total:" << accumulate(data.begin(), data.end(), 0) << endl;
	}
private:
	vector<atomic<int>> data;
    vector<std::mutex> locks; 
};

struct worker {
    int from, to;
    const generator_cfg& cfg;

    worker(int from, int to, const generator_cfg& cfg) 
        : from(from), to(to), cfg(cfg) {}

    std::vector<int> operator()() {
        std::vector<int> local_histogram(cfg.max + 1, 0);
        generator gen(cfg);
        for (int i = from; i < to; ++i) {
            int number = gen();
            local_histogram[number]++;
        }
        return local_histogram;
    }
};

int main(int argc, char **argv)
{
	int N = 10;
	int sample_count = 30000000;

	int num_threads = std::thread::hardware_concurrency();
	int MB = 1; 
	int print_level = 1; 
	parse_args(argc, argv, num_threads, N, sample_count, print_level, MB);
	
	generator_cfg cfg(N);
	histogram h(N+1);
	std::vector<std::future<std::vector<int>>> futures;
	auto t1 = chrono::high_resolution_clock::now();

 

    for (int t = 0; t < num_threads; ++t) {
        int from = (sample_count * t / num_threads);
        int to = (sample_count * (t + 1) / num_threads);
        futures.push_back(std::async(std::launch::async, worker(from, to, cfg)));
    }

    std::vector<int> global_histogram(N+1, 0);
    for (auto &f : futures) {
        auto local_histogram = f.get();
        for (size_t i = 0; i < local_histogram.size(); ++i) {
            global_histogram[i] += local_histogram[i];
        }
    }


	auto t2 = chrono::high_resolution_clock::now();
	if ( print_level >= 2 ) cout << "N: " << N << ", sample size: " << sample_count << ", threads: " << num_threads << ", MB: " << MB << endl;


	if (print_level >= 1) {
        for (size_t i = 0; i < global_histogram.size(); ++i) {
            cout << i << ":" << global_histogram[i] << endl;
        }
        cout << "total:" << accumulate(global_histogram.begin(), global_histogram.end(), 0) << endl;
    }
	cout << chrono::duration<double>(t2 - t1).count() << endl;
}
