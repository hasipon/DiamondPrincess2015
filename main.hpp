#include "sim.hpp"
#include <random>
#include <sys/time.h>
long getTime() {
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}
struct AI {
	mt19937 mt;
	const BaseSolver& sr;
	const vector<string>& phrases;
	AI(const BaseSolver& sr, const vector<string>& phrases) : sr(sr), phrases(phrases) {}
	vector<pair<P, int>> calcLockList(long time_limit, long startTime) {
		int max_score = -1;
		vector<pair<P, int>> best_lock_list;
		int cnt = 0;
		do {
			auto board = sr.initBoard;
			int score = 0, ls_old = 0;
			vector<pair<P, int>> lock_list;
			for (unsigned uidx = 0; uidx < sr.units_idx.size(); ++ uidx) {
				int u = sr.units_idx[uidx];
				if (!sr.canSet(board, u, sr.initPos[u], 0)) break;
				auto locks = sr.getLockCandidates(board, u);
				unsigned ret = 0;
				{
					int maxy = -(1<<30);
					int num = 0;
					for (unsigned i = 0; i < locks.size(); ++ i) {
						const auto& x = locks[i];
						if (x.first.imag() > maxy) {
							maxy = x.first.imag();
							num = 1;
							ret = i;
						} else if (x.first.imag() == maxy && mt() % ++ num == 0) {
							ret = i;
						}
					}
				}
				lock_list.push_back(locks[ret]);
				int ls = sr.setLock(board, u, lock_list.back());
				score += sr.calcScore(u, ls, ls_old);
				ls_old = ls;
			}
			if (score > max_score) {
				max_score = score;
				best_lock_list = lock_list;
			}
			++ cnt;
		} while (getTime() - startTime < time_limit);
		cerr << sr.config.id << " " << sr.seed << " " << max_score << " " << cnt << endl;
		return best_lock_list;
	}
	struct Elichika {
		const BaseSolver& sr;
		const vector<vector<bool>>& board;
		int u;
		map<tuple<int,int,bool>, pair<int, string>> a, b;
		Elichika(const BaseSolver& sr, const vector<vector<bool>>& board, int u) : sr(sr), board(board), u(u) {}
		bool comp_b(int x, int r, bool f, int p) {
			auto it = b.find(make_tuple(x,r,f));
			return it == b.end() || it->second.first < p;
		}
		pair<string, bool> get_path(int y, int x1, int r1, bool f1, int x2, int r2, bool f2) {
			if (x1 == x2 && r1 == r2) {
				if (f1 && f2) return {"", false};
				return {"", true};
			}
			queue<tuple<int,int,string>> Q; // x, r, s
			vector<set<int>> visited(sr.units[u].size()); // [r]{x}
			if (f1) visited[r1].insert(x1 + 1);
			if (f2) visited[r2].insert(x2 + 1);
			if (!visited[r1].insert(x1).second) return {"", false};
			if (visited[r2].count(x2)) return {"", false};
			Q.push(make_tuple(x1, r1, ""));
			while (!Q.empty()) {
				int x = get<0>(Q.front());
				int r = get<1>(Q.front());
				string s = get<2>(Q.front());
				Q.pop();
				for (int i = 0; i < 6; ++ i) {
					if (i == 2 || i == 3) continue;
					P npos = P(x, y) + cmds[i].first;
					int nrot = (r + cmds[i].second + sr.units[u].size()) % sr.units[u].size();
					if (sr.canSet(board, u, npos, nrot)) {
						if (x2 == npos.real() && r2 == nrot) {
							return {s + scmds[i][0], true};
						}
						if (visited[nrot].insert(npos.real()).second) {
							Q.push(make_tuple(npos.real(), nrot, s + scmds[i][0]));
						}
					}
				}
			}
			return {"", false};
		}
		void make_b(int y) {
			b.clear();

			vector<set<int>> active(sr.units[u].size()); // [r]{x}
			for (const auto& kv : a) {
				vector<set<int>> visited(sr.units[u].size()); // [r]{x}
				queue<pair<int, int>> Q; // x, r
				visited[get<1>(kv.first)].insert(get<0>(kv.first));
				Q.push({get<0>(kv.first), get<1>(kv.first)});
				while (!Q.empty()) {
					int x = Q.front().first;
					int r = Q.front().second;
					Q.pop();
					active[r].insert(x);
					for (int i = 0; i < 6; ++ i) {
						if (i == 2 || i == 3) continue;
						P npos = P(x, y) + cmds[i].first;
						int nrot = (r + cmds[i].second + sr.units[u].size()) % sr.units[u].size();
						if (sr.canSet(board, u, npos, nrot)) {
							if (visited[nrot].insert(npos.real()).second) {
								Q.push({npos.real(), nrot});
							}
						}
					}
				}
			}

			for (const auto& kv : a) {
				int x1 = get<0>(kv.first);
				int r1 = get<1>(kv.first);
				bool f1 = get<2>(kv.first);
				for (int r2 = 0; r2 < (int)active.size(); ++ r2) {
					for (int x2 : active[r2]) {
						int p = kv.second.first;
						string s = kv.second.second;
						if (comp_b(x2, r2, false, p)) {
							auto path = get_path(y, x1, r1, f1, x2, r2, false);
							if (path.second) {
								b[make_tuple(x2, r2, false)] = {p, s + path.first};
							}
						}
						if (comp_b(x2, r2, true, p)) {
							auto path = get_path(y, x1, r1, f1, x2, r2, true);
							if (path.second) {
								b[make_tuple(x2, r2, true)] = {p, s + path.first};
							}
						}
					}
				}
			}
		}
		void insert_a(int x, int r, bool b, const pair<int, string>& v, const string& s) {
			int p = v.first + (b ? 1 : 0);
			auto it = a.find(make_tuple(x, r, b));
			if (it == a.end()) {
				a.insert({make_tuple(x, r, b), {p, v.second + s}});
			} else if (it->second.first < p) {
				it->second = {p, v.second + s};
			}
		}
		void make_a(int y) {
			a.clear();
			for (const auto& kv : b) {
				int x = get<0>(kv.first);
				int r = get<1>(kv.first);
				bool can_sw = sr.canSet(board, u, P(x-1, y+1), r);
				bool can_se = sr.canSet(board, u, P(x, y+1), r);
				if (can_sw) insert_a(x-1, r, false, kv.second, "a");
				if (can_se) insert_a(x, r, false, kv.second, "l");
				if (can_sw && can_se && get<2>(kv.first)) {
					bool can_e = sr.canSet(board, u, P(x+1, y), r);
					if (can_e) insert_a(x-1, r, true, kv.second, "ei!");
				}
			}
		}
		string run(const pair<P,int>& target) {
			a[make_tuple(sr.initPos[u].real(), 0, false)] = {0, ""};
			for (int y = sr.initPos[u].imag(); ; ++ y) {
				make_b(y);
				if (y == target.first.imag()) break;
				make_a(y);
			}
			auto i = b.find(make_tuple(target.first.real(), target.second, false));
			auto j = b.find(make_tuple(target.first.real(), target.second, true));
			if (i == b.end() && j == b.end()) throw 1;
			string ret = max(
				i != b.end() ? i->second : make_pair(-1, ""),
				j != b.end() ? j->second : make_pair(-1, "")
			).second;
			return ret;
		}
	};
	void makeCommands(string& ans, const vector<vector<bool>>& board, int u, const pair<P,int>& target) {
		ans += Elichika(sr, board, u).run(target);
		for (int i = 0; i < 6; ++ i) {
			auto npos = target.first + cmds[i].first;
			int nrot = (target.second + cmds[i].second + sr.units[u].size()) % sr.units[u].size();
			if (!sr.canSet(board, u, npos, nrot)) {
				ans += scmds[i][0];
				return;
			}
		}
		throw 1;
	}
};
struct Solver : public BaseSolver {
	Solver(const GameConfig& config, int seed) : BaseSolver(config, seed) {}
	string solve(const vector<string>& phrases, long time_limit, long startTime) const {
		AI ai(*this, phrases);
		vector<pair<P, int>> lock_list = ai.calcLockList(time_limit, startTime);
		string ans;
		{
			auto board = initBoard;
			for (unsigned uidx = 0; uidx < lock_list.size(); ++ uidx) {
				int u = units_idx[uidx];
				ai.makeCommands(ans, board, u, lock_list[uidx]);
				setLock(board, u, lock_list[uidx]);
			}
		}
		return ans;
	}
};
string solve(GameConfig input, double seed, long time_limit, const char* memory_limit, const vector<string>& phrases) {
	long startTime = getTime();
	Solver solver(input, (int)seed);
	return solver.solve(phrases, time_limit >= 0 ? time_limit / 2 : 1000000, startTime);
}
