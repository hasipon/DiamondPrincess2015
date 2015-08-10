#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <complex>
#include <set>
#include <map>
#include <queue>
#include <future>
using namespace std;
#include "picojson.h"
#include "common.hpp"
#include "main.hpp"
void load(const char* filename, vector<GameConfig>& gcs, vector<pair<int,int>>& seeds) {
	picojson::value v;
	{
		string json;
		ifstream fin(filename);
		getline(fin, json);
		string err = picojson::parse(v, json);
		if (!err.empty()) {
			cerr << err << endl;
			throw 1;
		}
	}
	picojson::array& sourceSeeds = v.get<picojson::object>()["sourceSeeds"].get<picojson::array>();
	for (const auto& x : sourceSeeds) {
		seeds.push_back({(int)gcs.size(), (int)x.get<double>()});
	}
	gcs.push_back(ParseGameConfigJson(v));
}
struct Job {
	const vector<GameConfig>& gcs;
	const vector<pair<int,int>>& seeds;
	const vector<string>& phrases;
	int job_id;
	int num_core;
	long time_limit;
	const char* memory_limit;
	Job(
		vector<GameConfig>& gcs, const vector<pair<int,int>>& seeds, const vector<string>& phrases,
		int job_id, int num_core, long time_limit, const char* memory_limit
	) :
		gcs(gcs), seeds(seeds), phrases(phrases), job_id(job_id), num_core(num_core), time_limit(time_limit), memory_limit(memory_limit)
	{}
	vector<pair<unsigned, string>> result;
	void run() {
		int num_task = 0;
		for (unsigned i = job_id; i < seeds.size(); i += num_core) ++ num_task;
		if (num_task <= 0) return;
		long time_limit_i = time_limit >= 0 ? time_limit / num_task : -1;
		for (unsigned i = job_id; i < seeds.size(); i += num_core) {
			const auto& p = seeds[i];
			result.push_back({i, solve(gcs[p.first], p.second, time_limit_i, memory_limit, phrases)});
		}
	}
	void output(picojson::array& ans) {
		for (const auto& v : result) {
			const auto& p = seeds[v.first];
			picojson::object r;
			r["problemId"] = picojson::value((double)gcs[p.first].id);
			r["seed"] = picojson::value((double)p.second);
			r["solution"] = picojson::value(v.second);
			ans.push_back(picojson::value(r));
		}
	}
};
int main(int argc, char** argv) {
	vector<const char*> filenames;
	vector<string> phrases;
	const char* time_limit = 0;
	const char* memory_limit = 0;
	int num_core = 1;
	for (int i = 1; i < argc; ++ i) {
		if (argv[i] == string("-f")) filenames.push_back(argv[++i]);
		else if (argv[i] == string("-t")) time_limit = argv[++i];
		else if (argv[i] == string("-m")) memory_limit = argv[++i];
		else if (argv[i] == string("-c")) num_core = atoi(argv[++i]);
		else if (argv[i] == string("-p")) phrases.push_back(argv[++i]);
	}
	vector<GameConfig> gcs;
	vector<pair<int,int>> seeds;
	for (unsigned i = 0; i < filenames.size(); ++ i) {
		load(filenames[i], gcs, seeds);
	}
	long time_limit_us = -1;
	if (time_limit) time_limit_us = (long)(atof(time_limit) * 900000);
	vector<future<Job*>> results;
	for (int i = 0; i < num_core; ++ i) {
		auto a = new Job(gcs, seeds, phrases, i, num_core, time_limit_us, memory_limit);
		results.push_back(async(
			launch::async, // async or deferred
			[](Job* job) {
				job->run();
				return job;
			},
			a
		));
	}
	picojson::array ans;
	for (auto& x : results) {
		auto a = x.get();
		a->output(ans);
		delete a;
	}
	cout << picojson::value(ans) << flush;
}
