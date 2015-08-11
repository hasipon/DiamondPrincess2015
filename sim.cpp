#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <complex>
#include <set>
#include <map>
#include <queue>

#include "picojson.h"
#include "common.hpp"

using namespace std;
#include "sim.hpp"

struct Solver : public BaseSolver {
	Solver(const GameConfig& config, int seed) : BaseSolver(config, seed) {}

	void solve() const {
		auto board = initBoard;
		int score = 0, ls_old = 0;
		string result;
		for (unsigned uidx = 0; uidx < units_idx.size(); ++ uidx) {
			int u = units_idx[uidx];
			if (!canSet(board, u, initPos[u], 0)) break;
			vector<pair<pair<P,int>,string>> ch = getNext(board, u);
			int sel = 0;
			bool ok = false;
			for (;;) {
				cout << result << endl;
				cout << ch[sel].second << endl;

				auto b = board;
				setUnit(b, u, ch[sel].first.first, ch[sel].first.second);
				printBoard(b);

				cout << score << " pt. " << sel+1 << "/" << ch.size() << " : (s)elect (n)ext (p)rev ? " << flush;
				char c;
				if (!(cin >> c)) break;
				if (c == 'n') {
					sel = (sel + 1) % ch.size();
				} else if (c == 'p') {
					sel = (sel + ch.size() - 1) % ch.size();
				} else if (c == 's') {
					setNext(board, score, ls_old, result, u, ch[sel]);
					ok = true;
					break;
				}
			}
			if (!ok) break;
		}
		cout << result << endl;
	}
	void run(const char* solution) const {
		int uidx = 0;
		int u = -1;
		P pos;
		int rot = 0;
		auto board = initBoard;
		int score = 0, ls_old = 0;
		for (int i = 0; solution[i]; ++ i) {
			cout << "cmd: " << solution[i] << endl;
			if (u < 0) {
				u = units_idx[uidx++];
				pos = initPos[u];
				rot = 0;
			}
			auto b = board;
			setUnit(b, u, pos, rot);
			printBoard(b);
			cout << endl;
			auto c = get_cmd(solution[i]);
			auto npos = pos + c.first;
			int nrot = (rot + c.second + units[u].size()) % units[u].size();
			if (canSet(board, u, npos, nrot)) {
				cout << "move" << endl;
				pos = npos;
				rot = nrot;
			} else {
				cout << "not move" << endl;
				setUnit(board, u, pos, rot);
				for (int i = H-1; i >= 0; -- i) {
					for (bool x : board[i]) if (!x) goto next;
					board.erase(board.begin() + i);
					next:;
				}
				int ls = H - board.size();
				if (ls > 0) {
					board.insert(board.begin(), ls, vector<bool>(W));
				}
				int points = units[u][0].size() + 100 * (1 + ls) * ls / 2;
				int line_bonus = ls_old > 1 ? (ls_old - 1) * points / 10 : 0;
				int move_score = points + line_bonus;
				score += move_score;
				ls_old = ls;
				u = -1;
				cout << "move_score: " << move_score << endl;
				cout << score << " pt." << endl;
			}
		}
	}
};

int main(int argc, char** argv) {
	const char* filename = 0;
	int seed = 0;
	const char* solution = 0;
	for (int i = 1; i < argc; ++ i) {
		if (argv[i] == string("-f")) filename = argv[++i];
		else if (argv[i] == string("-s")) seed = atoi(argv[++i]);
		else if (argv[i] == string("-c")) solution = argv[++i];
	}
	ifstream fin(filename);
	string json;
	getline(fin, json);
	picojson::value v;
	string err = picojson::parse(v, json);
	if (! err.empty()) {
		cerr << err << endl;
	}
	Solver solver(ParseGameConfigJson(v), seed);
	if (solution) {
		solver.run(solution);
	} else {
		solver.solve();
	}
}
