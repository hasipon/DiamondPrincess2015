typedef complex<int> P;

namespace std {
	bool operator < (const P& a, const P& b) {
		return real(a) != real(b) ? real(a) < real(b) : imag(a) < imag(b);
	}
}

P cp(Cell c) {
	return P(c.x - c.y / 2, c.y);
}

vector<P> rotate(vector<P> sp) {
	vector<P> r;
	for (auto x : sp) {
		r.push_back(P(-x.imag(), x.real() + x.imag()));
	}
	return r;
}

enum UnitCommand {
	W, E, SW, SE, R, L
};
const vector<string> scmds = {
	MOV_W, MOV_E, MOV_SW, MOV_SE, ROT_R, ROT_L
};
const pair<P,int> cmds[] = {
	{P(-1, 0), 0},
	{P(1, 0), 0},
	{P(-1, 1), 0},
	{P(0, 1), 0},
	{P(0, 0), 1},
	{P(0, 0), -1},
};

UnitCommand to_uc(char c) {
	for (int i = 0; i < 6; ++ i) {
		if (scmds[i].find(c) != string::npos) return (UnitCommand)i;
	}
	throw 1;
}

pair<P, int> get_cmd(char c) {
	return cmds[to_uc(c)];
}

struct SolverState {
	int uidx, u;
	P pos;
	int rot;
	vector<vector<bool>> board;
	int score, ls_old;
	SolverState(const vector<vector<bool>>& initBoard) : uidx(0), u(-1), board(initBoard), score(0), ls_old(0) {}
};

struct BaseSolver {
	const GameConfig& config;
	int seed;
	int H, W;
	vector<vector<vector<P>>> units;
	vector<P> initPos;
	vector<int> units_idx;
	vector<vector<bool>> initBoard;
	vector<vector<tuple<int, int, vector<vector<bool>>>>> unit_area;

	BaseSolver(const GameConfig& config, int seed) : config(config), seed(seed) {
		H = config.height;
		W = config.width;
		initBoard = vector<vector<bool>>(H, vector<bool>(W));
		for (auto a : config.filled) {
			initBoard[a.y][a.x] = true;
		}
		uint32_t s = seed;
		for (int i = 0; i < config.sourceLength; ++ i) {
			units_idx.push_back(((s>>16)&0x7FFF) % config.units.size());
			s = s * 1103515245 + 12345;
		}
		for (const auto& u : config.units) {
			int y0 = -(1<<30);
			P pv = cp(u.pivot);
			vector<P> sp;
			for (const auto& m : u.members) {
				P q = cp(m) - pv;
				y0 = max(y0, -q.imag());
				sp.push_back(q);
			}
			sort(sp.begin(), sp.end());
			vector<vector<P>> vsp(1, sp);
			for (int i = 1; i < 6; ++ i) {
				vector<P> r = rotate(vsp[i-1]);
				sort(r.begin(), r.end());
				if (r == sp) break;
				vsp.push_back(r);
			}
			units.push_back(vsp);
			
			int x0 = 1<<30;
			int x1 = -(1<<30);
			for (const auto& p : sp) {
				int X = p.real();
				int Y = p.imag() + y0;
				if (Y < 0) throw 1;
				int Z = X + Y/2;
				x0 = min(x0, Z);
				x1 = max(x1, Z);
			}
			int x2 = W - (x1-x0 + 1);
			initPos.push_back(P(x2/2 - x0, y0));
			vector<set<P>> s(units.back().size());
			unit_area_dfs(s, (int)units.size()-1, initPos.back(), 0);
			unit_area.push_back({});
			for (const auto& sp : s) {
				int min_real = 1<<30;
				int max_real = -(1<<30);
				int min_imag = 1<<30;
				int max_imag = -(1<<30);
				for (auto p : sp) {
					min_real = min(min_real, p.real());
					max_real = max(max_real, p.real());
					min_imag = min(min_imag, p.imag());
					max_imag = max(max_imag, p.imag());
				}
				vector<vector<bool>> a(max_real-min_real+1, vector<bool>(max_imag-min_imag+1));
				for (auto p : sp) {
					a[p.real() - min_real][p.imag() - min_imag] = true;
				}
				unit_area.back().push_back(make_tuple(min_real, min_imag, a));
			}
		}
	}
	bool is_inside(int u, P pos, int r) const {
		for (auto p : units[u][r]) {
			P q = p + pos;
			int X = q.real();
			int Y = q.imag();
			if (Y < 0 || Y >= H) return false;
			if (X + Y/2 < 0 || X + Y/2 >= W) return false;
		}
		return true;
	}
	void unit_area_dfs(vector<set<P>>& s, int u, const P& p, int r) {
		if (!s[r].insert(p).second) return;
		for (int i = 0; i < 6; ++ i) {
			auto npos = p + cmds[i].first;
			int nrot = (r + cmds[i].second + units[u].size()) % units[u].size();
			if (is_inside(u, npos, nrot)) {
				unit_area_dfs(s, u, npos, nrot);
			}
		}
	}

	void printBoard(const vector<vector<bool>>& board) const {
		bool odd = false;
		for (auto r : board) {
			if (odd) cout << " ";
			for (auto x : r) cout << (x ? "# " : "_ ");
			cout << endl;
			odd = !odd;
		}
	}
	bool canSet(const vector<vector<bool>>& a, int u, P pos, int r) const {
		const auto& area = unit_area[u][r];
		int i = pos.real() - get<0>(area);
		int j = pos.imag() - get<1>(area);
		const auto& ua = get<2>(area);
		if (!(0 <= i && i < (int)ua.size() && 0 <= j && j < (int)ua[i].size() && ua[i][j])) return false;
		for (auto p : units[u][r]) {
			P q = p + pos;
			int X = q.real();
			int Y = q.imag();
			//cout << q << X+Y/2 << ":" << Y;
			if (Y < 0 || Y >= H) return false;
			if (X + Y/2 < 0 || X + Y/2 >= W) return false;
			if (a[Y][X + Y/2]) return false;
		}
		return true;
	}
	void setUnit(vector<vector<bool>>& a, int u, P pos, int r) const {
		for (auto p : units[u][r]) {
			P q = p + pos;
			int X = q.real();
			int Y = q.imag();
			if (Y < 0) throw 1;
			if (X + Y/2 < 0 || X + Y/2 >= W) throw 1;
			a[Y][X + Y/2] = true;
		}
	}
	vector<pair<pair<P,int>,string>> getNext(const vector<vector<bool>>& board, int u) const {
		vector<pair<pair<P,int>,string>> ch;
		set<pair<P,int>> visited;
		queue<pair<pair<P,int>,string>> Q;
		visited.insert({initPos[u], 0});
		Q.push({{initPos[u], 0}, ""});
		while (!Q.empty()) {
			auto p = Q.front().first;
			auto s = Q.front().second;
			Q.pop();
			bool fix = false;
			for (int i = 0; i < 6; ++ i) {
				auto npos = p.first + cmds[i].first;
				int nrot = (p.second + cmds[i].second + units[u].size()) % units[u].size();
				if (canSet(board, u, npos, nrot)) {
					if (visited.insert({npos, nrot}).second) {
						Q.push({{npos, nrot}, s + scmds[i][0]});
					}
				} else {
					if (!fix) {
						ch.push_back({p, s + scmds[i][0]});
						fix = true;
					}
				}
			}
		}
		return ch;
	}
	void setNext(vector<vector<bool>>& board, int& score, int& ls_old, string& result, int u, const pair<pair<P,int>,string>& elem) const {
		setUnit(board, u, elem.first.first, elem.first.second);
		for (int i = H-1; i >= 0; -- i) {
			for (bool x : board[i]) if (!x) goto next;
			board.erase(board.begin() + i);
			next:;
		}
		int ls = H - board.size();
		if (ls > 0) {
			board.insert(board.begin(), ls, vector<bool>(W));
		}
		result += elem.second;

		int points = units[u][0].size() + 100 * (1 + ls) * ls / 2;
		int line_bonus = ls_old > 1 ? (ls_old - 1) * points / 10 : 0;
		int move_score = points + line_bonus;
		score += move_score;
		ls_old = ls;
	}
	vector<pair<P,int>> getLockCandidates(const vector<vector<bool>>& board, int u) const {
		vector<pair<P,int>> ch;
		vector<vector<vector<bool>>> visited;
		for (const auto& info : unit_area[u]) {
			visited.push_back(vector<vector<bool>>(get<2>(info).size(), vector<bool>(get<2>(info)[0].size())));
		}
		queue<pair<P,int>> Q;
		visited[0][initPos[u].real() - get<0>(unit_area[u][0])][initPos[u].imag() - get<1>(unit_area[u][0])] = true;
		Q.push({initPos[u], 0});
		while (!Q.empty()) {
			auto p = Q.front(); Q.pop();
			bool fix = false;
			for (int i = 0; i < 6; ++ i) {
				auto npos = p.first + cmds[i].first;
				int nrot = (p.second + cmds[i].second + units[u].size()) % units[u].size();
				if (canSet(board, u, npos, nrot)) {
					int ii = npos.real() - get<0>(unit_area[u][nrot]);
					int jj = npos.imag() - get<1>(unit_area[u][nrot]);
					if (!visited[nrot][ii][jj]) {
						visited[nrot][ii][jj] = true;
						Q.push({npos, nrot});
					}
				} else {
					if (!fix) {
						ch.push_back(p);
						fix = true;
					}
				}
			}
		}
		return ch;
	}
	int setLock(vector<vector<bool>>& board, int u, const pair<P,int>& elem) const {
		setUnit(board, u, elem.first, elem.second);
		for (int i = H-1; i >= 0; -- i) {
			for (bool x : board[i]) if (!x) goto next;
			board.erase(board.begin() + i);
			next:;
		}
		int ls = H - board.size();
		if (ls > 0) {
			board.insert(board.begin(), ls, vector<bool>(W));
		}
		return ls;
	}
	int calcScore(int u, int ls, int ls_old) const {
		int points = units[u][0].size() + 100 * (1 + ls) * ls / 2;
		int line_bonus = ls_old > 1 ? (ls_old - 1) * points / 10 : 0;
		int move_score = points + line_bonus;
		return move_score;
	}
	bool inputState(SolverState& state, UnitCommand uc) const {
		if (state.u < 0) {
			state.u = units_idx[state.uidx++];
			state.pos = initPos[state.u];
			state.rot = 0;
		}
		auto npos = state.pos + cmds[uc].first;
		int nrot = (state.rot + cmds[uc].second + units[state.u].size()) % units[state.u].size();
		if (canSet(state.board, state.u, npos, nrot)) {
			state.pos = npos;
			state.rot = nrot;
		} else {
			setUnit(state.board, state.u, state.pos, state.rot);
			for (int i = H-1; i >= 0; -- i) {
				for (bool x : state.board[i]) if (!x) goto next;
				state.board.erase(state.board.begin() + i);
				next:;
			}
			int ls = H - state.board.size();
			if (ls > 0) {
				state.board.insert(state.board.begin(), ls, vector<bool>(W));
			}
			int points = units[state.u][0].size() + 100 * (1 + ls) * ls / 2;
			int line_bonus = state.ls_old > 1 ? (state.ls_old - 1) * points / 10 : 0;
			int move_score = points + line_bonus;
			state.score += move_score;
			state.ls_old = ls;
			state.u = -1;
		}
		return true;
	}
};
