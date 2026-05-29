#include "Types.h"

std::string monsterName(MonsterKind kind) {
  switch (kind) {
  case MonsterKind::Goblin:
    return "哥布林";
  case MonsterKind::Skeleton:
    return "骷髅";
  case MonsterKind::Orc:
    return "兽人";
  case MonsterKind::Troll:
    return "巨魔";
  case MonsterKind::Dragon:
    return "幼龙";
  }
  return "未知怪物";
}

char monsterGlyph(MonsterKind kind) {
  switch (kind) {
  case MonsterKind::Goblin:
    return 'g';
  case MonsterKind::Skeleton:
    return 's';
  case MonsterKind::Orc:
    return 'o';
  case MonsterKind::Troll:
    return 'T';
  case MonsterKind::Dragon:
    return 'D';
  }
  return '?';
}
