#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

using namespace std::string_literals;

namespace {
	std::mutex print_lock;
	unsigned line = 1;
	long long total_bytes = 0;

	auto
	Color(int color_code, std::string str)
	{
		return "\33[38;5;" + std::to_string(color_code) + 'm' + str + "\33[0m";
	}

	auto
	Line(int l)
	{
		int line_dec = l - line;
		line = l;

		return "\r"+(line_dec<0?"\33["s+std::to_string(-line_dec)
		+'A':std::string(line_dec, '\n')+"\33[2K");
	}

	auto
	Basename(std::string url)
	{
		return url.substr(url.find_last_of("/\\")+1);
	}

	auto
	HumanBytes(double bytes)
	{
		std::string prefixes = " kMGTPE";
		std::stringstream stream;
		unsigned i;
		for (i=0; bytes>1000 && i<prefixes.size();i++) bytes /= 1000;
		stream << std::fixed << std::setprecision(2) << bytes << " " << prefixes[i] << "B";
		return stream.str();
	}

	void
	Download(std::string url, unsigned current_line)
	{
		curlpp::Easy handler;
		auto filename = Basename(url);
		std::ofstream file(filename);

		handler.setOpt(curlpp::Options::Url(url));
		handler.setOpt(curlpp::Options::FollowLocation(true));
		handler.setOpt(curlpp::Options::WriteFunction(
		[&]
		(char *data, size_t size, size_t nmemb, auto ...)
		{
			size_t real_size = nmemb*size;

			file.write(data, real_size);
			total_bytes += real_size;
			return real_size;
		}));
		handler.setOpt(curlpp::Options::NoProgress(false));
		handler.setOpt(curlpp::Options::ProgressFunction(
		[&]
		(size_t total, size_t done, auto ...)
		{
			print_lock.lock();
			std::cout << Line(current_line) << Color(63, filename + ": ") << HumanBytes(done)
			<< " of " << HumanBytes(total) << " downloaded (" << (total?int(done*100.0/total):0)
			<< "%)" << std::flush;
			print_lock.unlock();

			return 0;
		}));

		try {
			handler.perform();
		} catch (curlpp::RuntimeError &e) { 
			print_lock.lock();
			std::cout << Line(current_line) << Color(63, filename + ": ") << Color(196, e.what()) << std::flush;
			print_lock.unlock();
		}
	}

	int
	ReadUrls(std::string filename, std::vector<std::string> &vec)
	{
		std::ifstream file(filename);
		
		if (!file) return -1;

		for (std::string line; std::getline(file, line);) vec.push_back(line);

		return 0;
	}

	int
	ReadUrls(std::istream &stream, std::vector<std::string> &vec)
	{
		for (std::string line; std::getline(stream, line);) vec.push_back(line);
		if (vec.empty()) return -1;
		return 0;
	}
}

int
main(int argc, char **argv)
{
	std::string param;
	std::vector<std::thread> threads;
	std::vector<std::string> urls;
	unsigned current_line = 1;

	curlpp::initialize();

	if (argc<2) {
		if (ReadUrls(std::cin, urls)<0) {
			std::cerr << "Usage: " << argv[0] << " [-v] <file with URLS>" << std::endl;
			return -1;
		}
	} else param = argv[1];

	if (param == "-h") {
		std::cout << "pd - parallel download\n"
		<< "Downloads multiple files simultaneosly using multithreading.\n\n"
		<< "Usage: " << argv[0] << " [-h] <file with URLs>" << std::endl;
		return 0;
	}

	if (!param.empty()) {
		if (ReadUrls(param, urls)<0) {
			std::cerr << "Couldn't open file \"" << param << "\"" << std::endl;
			return -1;
		}
	}

	for (auto url : urls) threads.emplace_back(Download, url, current_line++);
	for (auto &thread : threads) thread.join();

	std::cout << Line(current_line) << Color(27, HumanBytes(total_bytes))
	<< " downloaded" << std::endl;

	return 0;
}
