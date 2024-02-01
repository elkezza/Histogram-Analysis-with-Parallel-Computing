/**
 * @author Parallel Computing, Uni Wien
 * @comment READ-ONLY FILE 
 */

#include <thread>
#include <vector>
#include <iostream>
#include <random>
#include <chrono>
#include <numeric>

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
	histogram(int count) : data(count)
	{
	}
	void add(int i)
	{
		++data[i];
	}
	int& get(int i)
	{
		return data[i];
	}
	void print(std::ostream& str)
	{
		for (size_t i = 0; i < data.size(); ++i) str << i << ":" << data[i] << endl;
		str << "total:" << accumulate(data.begin(), data.end(), 0) << endl;
	}
private:
	vector<int> data;
};

struct worker
{
	worker(int sample_count, histogram& h, const generator_cfg& cfg) : sample_count(sample_count), h(h), cfg(cfg) {}
	void operator()()
	{
		generator gen(cfg);
		while (sample_count--)
		{
			int next = gen();
			h.add(next);
		}
	}
private:
	int sample_count;
	histogram& h;
	const generator_cfg& cfg;
};

int main(int argc, char **argv)
{
	int N = 10;
	int sample_count = 42000000;

	int num_threads = std::thread::hardware_concurrency();
	int MB = 1; // needed only for v4
	int print_level = 1; // 0 nothing, 1 histogram, 2 histogram + config
	parse_args(argc, argv, num_threads, N, sample_count, print_level, MB);
	
	generator_cfg cfg(N);
	histogram h(N+1);
	
	auto t1 = chrono::high_resolution_clock::now();

	/* static work distribution */
	for (int t = 0; t < num_threads; ++t)
	{
		int from = (sample_count*t / num_threads);
		int to = (sample_count*(t+1) / num_threads);
		worker(to - from, h, cfg)(); // <-- execute this in a thread
	}
	// wait for threads to end here (before the end time is measured)
	/* insert code here */

	auto t2 = chrono::high_resolution_clock::now();
	if ( print_level >= 2 ) cout << "N: " << N << ", sample size: " << sample_count << ", threads: " << num_threads << ", MB: " << MB << endl;
	if ( print_level >= 1 ) h.print(cout);
	cout << chrono::duration<double>(t2 - t1).count() << endl;
}
