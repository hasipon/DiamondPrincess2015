#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <complex>
#include <set>
#include <map>
#include <queue>
#include <ctime>

#include "picojson.h"
#include "common.hpp"

using namespace std;
#include "sim.hpp"

const vector<string> powers = {
	"ei!",
	"ia! ia!",
	"cthulhu fhtagn!",
	"r'lyeh",
	"yuggoth",
	"in his house at r'lyeh dead cthulhu waits dreaming.",
	"yogsothoth",
	"planet 10",
};

struct Simulator : public BaseSolver {
	Simulator(const GameConfig& config, int seed) : BaseSolver(config, seed) {}

	int run(string solution) const {
		SolverState state(initBoard);
		int uidx = -1;
		set<pair<P,int>> visited;
		for (char c : solution) {
			if (state.u < 0) {
				if (state.uidx >= (int)units_idx.size()) return -3;
				int u = units_idx[state.uidx];
				if (!canSet(state.board, u, initPos[u], 0)) {
					return -4;
				}
			}
			if (!inputState(state, to_uc(c))) return -1;
			if (state.u >= 0) {
				if (uidx != state.uidx) {
					uidx = state.uidx;
					visited.clear();
					visited.insert({initPos[state.u], 0});
				}
				if (!visited.insert({state.pos, state.rot}).second) return -2;
			}
		}
		return state.score;
	}
};

int main(int argc, char** argv) {
	if (argc < 3) {
		printf("%s TAG INPUT_DIR OUTPUT_DIR < OUTPUT_JSON\n", argv[0]);
		return 1;
	}
	const char* tag = argv[1];
	const char* input_dir = argv[2];
	const char* output_dir = argv[3];
	int remaining = 1439208000 - time(0);

	string output_json;
	getline(cin, output_json);

	picojson::value v;
	string err = picojson::parse(v, output_json);
	if (!err.empty()) {
		cerr << err << endl;
		return 1;
	}
	for (auto& x : v.get<picojson::array>()) {
		picojson::object& y = x.get<picojson::object>();
		int problemId = (int)y["problemId"].get<double>();
		int seed = (int)y["seed"].get<double>();
		string solution = y["solution"].get<string>();
		string input_json;
		{
			ifstream fin(input_dir + string("/problem_") + to_string(problemId) + ".json");
			getline(fin, input_json);
		}
		picojson::value w;
		string err = picojson::parse(w, input_json);
		if (!err.empty()) {
			cerr << err << endl;
			return 1;
		}
		int old_score = -1;
		{
			ifstream fin(output_dir + string("/") + to_string(problemId) + "_" + to_string(seed));
			if (fin) {
				int n;
				if (fin >> n) {
					old_score = n;
				}
			}
		}
		Simulator sim(ParseGameConfigJson(w), seed);
		for (char& c : solution) {
			if ('A' <= c && c <= 'Z') {
				c += 'a' - 'A';
			}
		}
		int new_score = sim.run(solution);
		if (new_score >= 0) for (string spell : powers) {
			int reps = 0;
			string::size_type pos = 0;
			for (;;) {
				pos = solution.find(spell, pos);
				if (pos == string::npos) break;
				++ reps;
				++ pos;
			}
			if (reps > 0) {
				new_score += 2 * spell.length() * reps + 300;
			}
		}
		cout << problemId << "\t" << seed << "\t" << new_score << endl;
		if (new_score > old_score) {
			ofstream fout(output_dir + string("/") + to_string(problemId) + "_" + to_string(seed));
			fout << new_score << "\t" << tag << remaining << "\t" << solution << endl;
		}
	}
	cout << "save: " << tag << remaining << endl;
}
