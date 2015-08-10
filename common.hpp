#ifndef icfpc_common_h
#define icfpc_common_h

#include "picojson.h"
#include <vector>
#include <string>

const std::string MOV_W = "p'!.03";
const std::string MOV_E = "bcefy2";
const std::string MOV_SW = "aghij4";
const std::string MOV_SE = "lmno 5";
const std::string ROT_R = "dqrvz1";
const std::string ROT_L = "kstuwx";

/*
 {p, ', !, ., 0, 3}	move W
 {b, c, e, f, y, 2}	move E
 {a, g, h, i, j, 4}	move SW
 {l, m, n, o, space, 5}    	move SE
 {d, q, r, v, z, 1}	rotate clockwise
 {k, s, t, u, w, x}	rotate counter-clockwise
 \t, \n, \r	(ignored)
*/

struct Cell {
  int x, y; /* x: column, y: row */
};

inline bool operator < (Cell a, Cell b) {
  if (a.x != b.x) return a.x < b.x;
  return a.y < b.y;
}
inline Cell operator + (Cell a, Cell b) {
  return Cell{a.x + b.x, a.y + b.y};
}

struct Unit {
  std::vector<Cell> members; /* The unit members. */
  Cell pivot;  /* The rotation point of the unit. */
};

struct GameConfig {
  int id; /* A unique number identifying the problem */
  int width; /* The number of cells in a row */
  int height; /* The number of rows on the board */
  std::vector<Unit> units; /* The various unit configurations
                       that may appear in this game.
                       There might be multiple entries for the same unit.
                       When a unit is spawned, it will start off in the orientation
                       specified in this field. */
  std::vector<Cell> filled; /* Which cells start filled */
  int sourceLength; /* How many units in the source */
};

GameConfig ParseGameConfigJson(picojson::value input) {
  picojson::object& obj = input.get<picojson::object>();
  GameConfig gc;
  const auto& units = obj["units"].get<picojson::array>();
  const auto& filled = obj["filled"].get<picojson::array>();
  for (auto& e : filled) {
    auto c = e.get<picojson::object>();
    Cell cell;
    cell.x = static_cast<int>(c["x"].get<double>());
    cell.y = static_cast<int>(c["y"].get<double>());
    gc.filled.push_back(std::move(cell));
  }
  for (auto& e : units) {
    auto unit = e.get<picojson::object>();
    Unit u;
    auto members = unit["members"].get<picojson::array>();
    for (auto& m : members) {
      auto member = m.get<picojson::object>();
      Cell cell;
      cell.x = static_cast<int>(member["x"].get<double>());
      cell.y = static_cast<int>(member["y"].get<double>());
      u.members.push_back(std::move(cell));
    }
    auto pivot = unit["pivot"].get<picojson::object>();
    u.pivot.x = static_cast<int>(pivot["x"].get<double>());
    u.pivot.y = static_cast<int>(pivot["y"].get<double>());
    gc.units.push_back(std::move(u));
  }
  gc.height = static_cast<int>(obj["height"].get<double>());
  gc.width = static_cast<int>(obj["width"].get<double>());
  gc.id = static_cast<int>(obj["id"].get<double>());
  gc.sourceLength = static_cast<int>(obj["sourceLength"].get<double>());
  return gc;
}


#endif
