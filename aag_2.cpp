#ifndef __PROGTEST__

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <variant>
#include <vector>

using State = unsigned int;
using Symbol = uint8_t;

struct NFA {
    std::set<State> m_States;
    std::set<Symbol> m_Alphabet;
    std::map<std::pair<State, Symbol>, std::set<State>> m_Transitions;
    State m_InitialState;
    std::set<State> m_FinalStates;
};

struct DFA {
    std::set<State> m_States;
    std::set<Symbol> m_Alphabet;
    std::map<std::pair<State, Symbol>, State> m_Transitions;
    State m_InitialState;
    std::set<State> m_FinalStates;
};

#endif

DFA unify(const NFA& a, const NFA& b);
DFA intersect(const NFA& a, const NFA& b);

std::set<State> remapSet(const std::set<State> &s, std::map<State, State> &remap) {
  std::set<State> res;
  for (const auto &it : s) {
    res.insert(remap[it]);
  }

  return res;
}

void addToQueue(const std::set<State> &x, std::vector<std::set<State>> &queue,
                DFA &dfa, std::map<std::set<State>, State> &remapStates)
{
  if (remapStates.find(x) != remapStates.end() || x.size() == 0) {
    return;
  }

  queue.push_back(x);
  auto idx = remapStates.size();
  remapStates[x] = idx;

  dfa.m_States.insert(idx);
}

void ToDFA(DFA &dfa, const NFA &x) {
  dfa.m_Alphabet = x.m_Alphabet;
  std::map<std::set<State>, State> remapStates;

  std::vector<std::set<State>> queue;
  addToQueue(std::set<State>({x.m_InitialState}), queue, dfa, remapStates);
  dfa.m_InitialState = 0;

  size_t pos = 0;

  while (pos != queue.size()) {
    for (const auto &ch : x.m_Alphabet) {
      std::set<State> newState;
      for (const auto &it : queue[pos]) {
        const auto &iter = x.m_Transitions.find({ it, ch});
        if (iter != x.m_Transitions.end()) {
          newState.insert(iter->second.begin(), iter->second.end());
        }
      }

      addToQueue(newState, queue, dfa, remapStates);
      if (!newState.empty()) {
        dfa.m_Transitions[{remapStates[queue[pos]], ch}] = remapStates[newState];
      }
    }
    pos++;
  }

  for (const auto &it : remapStates) {
    for (const auto &old : it.first) {
      if (x.m_FinalStates.find(old) != x.m_FinalStates.end()) {
        dfa.m_FinalStates.insert(it.second);
        break;
      }
    }
  }
}

void removeUnreachable(DFA &x) {
  std::set<State> reachableStates;
  std::vector<State> queue;
  queue.push_back(x.m_InitialState);
  reachableStates.insert(x.m_InitialState);

  size_t pos;
  pos = 0;
  while (pos != queue.size()) {
    auto head = queue[pos];

    for (const auto &ch : x.m_Alphabet) {
      const auto &it = x.m_Transitions.find({ head, ch });
      if (it == x.m_Transitions.end()) {
        continue;
      }
      auto st = it->second;
      if (reachableStates.find(st) == reachableStates.end()) {
        queue.push_back(st);
        reachableStates.insert(st);
      }
    }

    pos++;
  }

  std::swap(x.m_States, reachableStates);

  std::set<State> newFinal;

  for (const auto &it : x.m_FinalStates) {
    if (x.m_States.find(it) != x.m_States.end()) {
      newFinal.insert(it);
    }
  }

  std::swap(x.m_FinalStates, newFinal);

  std::map<std::pair<State, Symbol>, State> newTransitions;

  for (const auto &tr : x.m_Transitions) {
    if (   x.m_States.find(tr.second) != x.m_States.end()
        && x.m_States.find(tr.first.first) != x.m_States.end()) {
      newTransitions[tr.first] = tr.second;
    }
  }

  std::swap(x.m_Transitions, newTransitions);
}

void removeRedundant(DFA &x) {
  std::map<State, std::set<State>> backwardEdges;
  for (const auto &tr : x.m_Transitions) {
    backwardEdges[tr.second].insert(tr.first.first);
  }

  std::set<State> usefulStates;
  std::vector<State> queue;

  for (const auto &it : x.m_FinalStates) {
    queue.push_back(it);
    usefulStates.insert(it);
  }
  size_t pos = 0;

  while (pos != queue.size()) {
    auto head = queue[pos];

    for (const auto &it : backwardEdges[head]) {
      if (usefulStates.find(it) == usefulStates.end()) {
        queue.push_back(it);
        usefulStates.insert(it);
      }
    }

    pos++;
  }

  std::swap(x.m_States, usefulStates);

  std::map<std::pair<State, Symbol>, State> newTransitions;

  for (const auto &tr : x.m_Transitions) {

    if (   x.m_States.find(tr.second) != x.m_States.end()
        && x.m_States.find(tr.first.first) != x.m_States.end()) {
      newTransitions[tr.first] = tr.second;
    }
  }

  std::swap(x.m_Transitions, newTransitions);
}

std::pair<std::set<State>, std::set<State>> splitSet(const std::set<State>& src,
                                                     const std::set<State>& div)
{
  std::pair<std::set<State>, std::set<State>> ans;
  for (const auto &st : src) {
    if (div.find(st) != div.end()) {
      ans.first.insert(st);
    } else {
      ans.second.insert(st);
    }
  }

  return ans;
}

void ToMinimalDFA(DFA &x) {
  removeUnreachable(x);
  removeRedundant(x);

  if (x.m_States.empty()) {
    x.m_States.insert(0);
    x.m_InitialState = 0;
    return;
  }

//  P := {F, Q \ F}
//  W := {F, Q \ F}
//  while (W is not empty) do
//       choose and remove a set A from W
//       for each c in Σ do
//            let X be the set of states for which a transition on c leads to a state in A
//            for each set Y in P for which X ∩ Y is nonempty and Y \ X is nonempty do
//                 replace Y in P by the two sets X ∩ Y and Y \ X
//                 if Y is in W
//                      replace Y in W by the same two sets
//                 else
//                      if |X ∩ Y| <= |Y \ X|
//                           add X ∩ Y to W
//                      else
//                           add Y \ X to W


  std::set<std::set<State>> resultSet;
  std::set<std::set<State>> queue;


  auto p = splitSet(x.m_States, x.m_FinalStates);
  resultSet.insert(p.first);
  resultSet.insert(p.second);
  queue.insert(p.first);
  queue.insert(p.second);

  while (queue.size()) {
    auto currSet = *queue.begin();
    queue.erase(queue.begin());

    for (const auto &ch : x.m_Alphabet) {
      std::set<State> from;
      for (const auto &st : x.m_States) {
        const auto &it = x.m_Transitions.find({st, ch});
        if (it == x.m_Transitions.end()) {
          continue;
        }
        if (currSet.find(it->second) != currSet.end()) {
          from.insert(st);
        }
      }

      if (!from.empty()) {
        std::set<std::set<State>> newResSet;
        for (const auto &tmp : resultSet) {
          std::set<State> Y1, Y2;
          auto split = splitSet(tmp, from);

          if (split.first.empty() || split.second.empty()){
            newResSet.insert(tmp);
          } else {
            newResSet.insert(split.first);
            newResSet.insert(split.second);

            const auto &iter = queue.find(tmp);
            if (iter != queue.end()) {
              queue.erase(iter);
              queue.insert(split.first);
              queue.insert(split.second);
            } else {
              if (split.first.size() <= split.second.size()) {
                queue.insert(split.first);
              } else {
                queue.insert(split.second);
              }
            }
          }
        }

        std::swap(newResSet, resultSet);
      }
    }
  }

  std::map<State, State> remap;
  State size = 0;
  for (const auto &setOfStates : resultSet) {
    for (const auto &st : setOfStates) {
      remap[st] = size;
    }
    size++;
  }

  x.m_InitialState = remap[x.m_InitialState];

  x.m_States.clear();
  for (State i = 0; i < size; ++i) {
    x.m_States.insert(i);
  }

  std::set<State> newFinal;

  for (const auto &it : x.m_FinalStates) {
      newFinal.insert(remap[it]);
  }

  std::swap(x.m_FinalStates, newFinal);

  std::map<std::pair<State, Symbol>, State> newTransitions;

  for (const auto &p : x.m_Transitions) {
    newTransitions[{ remap[p.first.first], p.first.second }] = remap[p.second];
  }

  std::swap(x.m_Transitions, newTransitions);
}


DFA unify(const NFA& a, const NFA& b) {
  NFA unionNFA;
  unionNFA.m_States.insert(0);
  unionNFA.m_InitialState = 0;
  unionNFA.m_Alphabet = a.m_Alphabet;
  unionNFA.m_Alphabet.insert(b.m_Alphabet.begin(), b.m_Alphabet.end());
  State newState = 1;
  std::map<State, State> newStatesA;
  for (const auto &it : a.m_States) {
    unionNFA.m_States.insert(newState);
    newStatesA[it] = newState++;
  }
  std::map<State, State> newStatesB;
  for (const auto &it : b.m_States) {
    unionNFA.m_States.insert(newState);
    newStatesB[it] = newState++;
  }

  for (const auto &it : a.m_FinalStates) {
    unionNFA.m_FinalStates.insert(newStatesA[it]);
  }

  for (const auto &it : b.m_FinalStates) {
    unionNFA.m_FinalStates.insert(newStatesB[it]);
  }

  if (   a.m_FinalStates.find(a.m_InitialState) != a.m_FinalStates.end()
      || b.m_FinalStates.find(b.m_InitialState) != b.m_FinalStates.end()) {
    unionNFA.m_FinalStates.insert(0);
  }

  for (const auto &it : a.m_Transitions) {
    const auto s = remapSet(it.second, newStatesA);
    unionNFA.m_Transitions[{ newStatesA[it.first.first], it.first.second }].insert(s.begin(), s.end());

    if (it.first.first == a.m_InitialState) {
      unionNFA.m_Transitions[{ 0, it.first.second }].insert(s.begin(), s.end());
    }
  }

  for (const auto &it : b.m_Transitions) {
    const auto s = remapSet(it.second, newStatesB);
    unionNFA.m_Transitions[{ newStatesB[it.first.first], it.first.second }].insert(s.begin(), s.end());

    if (it.first.first == b.m_InitialState) {
      unionNFA.m_Transitions[{ 0, it.first.second }].insert(s.begin(), s.end());
    }
  }

  DFA dfa;
  ToDFA(dfa, unionNFA);
  ToMinimalDFA(dfa);

  return dfa;
}

DFA intersect(const NFA& a, const NFA& b) {
  NFA res;
  std::map<std::pair<State, State>, State> remap;
  for (const auto &st1 : a.m_States) {
    for (const auto &st2 : b.m_States) {
      auto idx = remap.size();
      remap[{st1, st2}] = idx;
      res.m_States.insert(idx);
    }
  }

  res.m_InitialState = remap[{ a.m_InitialState, b.m_InitialState }];

  std::map<char, std::map<State, std::set<State> >> charSplitedB;
  for (const auto &t : b.m_Transitions) {
    charSplitedB[t.first.second][t.first.first] = t.second;
  }

  for (const auto &tr1 : a.m_Transitions) {
    auto ch = tr1.first.second;
    for (const auto &tr2 : charSplitedB[ch]) {
      const auto idx = remap[{ tr1.first.first, tr2.first}];
      for (const auto &st1 : tr1.second) {
        for (const auto &st2 : tr2.second) {
          res.m_Transitions[{ idx, ch }].insert(remap[{st1, st2}]);
        }
      }
    }
  }

  for (const auto &st1 : a.m_FinalStates) {
    for (const auto &st2 : b.m_FinalStates) {
      res.m_FinalStates.insert(remap[{st1, st2}]);
    }
  }

  res.m_Alphabet = a.m_Alphabet;
  res.m_Alphabet.insert(b.m_Alphabet.begin(), b.m_Alphabet.end());

  DFA dfa;
  ToDFA(dfa, res);
  ToMinimalDFA(dfa);
  return dfa;
}

#ifndef __PROGTEST__

// You may need to update this function or the sample data if your state naming strategy differs.
bool operator==(const DFA& a, const DFA& b)
{
    return std::tie(a.m_States, a.m_Alphabet, a.m_Transitions, a.m_InitialState, a.m_FinalStates) == std::tie(b.m_States, b.m_Alphabet, b.m_Transitions, b.m_InitialState, b.m_FinalStates);
}

int main()
{
    NFA a1{
        {0, 1, 2},
        {'a', 'b'},
        {
            {{0, 'a'}, {0, 1}},
            {{0, 'b'}, {0}},
            {{1, 'a'}, {2}},
        },
        0,
        {2},
    };
    NFA a2{
        {0, 1, 2},
        {'a', 'b'},
        {
            {{0, 'a'}, {1}},
            {{1, 'a'}, {2}},
            {{2, 'a'}, {2}},
            {{2, 'b'}, {2}},
        },
        0,
        {2},
    };
    DFA a{
        {0, 1, 2, 3, 4},
        {'a', 'b'},
        {
            {{0, 'a'}, {1}},
            {{1, 'a'}, {2}},
            {{2, 'a'}, {2}},
            {{2, 'b'}, {3}},
            {{3, 'a'}, {4}},
            {{3, 'b'}, {3}},
            {{4, 'a'}, {2}},
            {{4, 'b'}, {3}},
        },
        0,
        {2},
    };
    assert(intersect(a1, a2) == a);

    NFA b1{
        {0, 1, 2, 3, 4},
        {'a', 'b'},
        {
            {{0, 'a'}, {1}},
            {{0, 'b'}, {2}},
            {{2, 'a'}, {2, 3}},
            {{2, 'b'}, {2}},
            {{3, 'a'}, {4}},
        },
        0,
        {1, 4},
    };
    NFA b2{
        {0, 1, 2, 3, 4},
        {'a', 'b'},
        {
            {{0, 'b'}, {1}},
            {{1, 'a'}, {2}},
            {{2, 'b'}, {3}},
            {{3, 'a'}, {4}},
            {{4, 'a'}, {4}},
            {{4, 'b'}, {4}},
        },
        0,
        {4},
    };
    DFA b{
        {0, 1, 2, 3, 4, 5, 6, 7, 8},
        {'a', 'b'},
        {
            {{0, 'a'}, {1}},
            {{0, 'b'}, {2}},
            {{2, 'a'}, {3}},
            {{2, 'b'}, {4}},
            {{3, 'a'}, {5}},
            {{3, 'b'}, {6}},
            {{4, 'a'}, {7}},
            {{4, 'b'}, {4}},
            {{5, 'a'}, {5}},
            {{5, 'b'}, {4}},
            {{6, 'a'}, {8}},
            {{6, 'b'}, {4}},
            {{7, 'a'}, {5}},
            {{7, 'b'}, {4}},
            {{8, 'a'}, {8}},
            {{8, 'b'}, {8}},
        },
        0,
        {1, 5, 8},
    };
    assert(unify(b1, b2) == b);

    NFA c1{
        {0, 1, 2, 3, 4},
        {'a', 'b'},
        {
            {{0, 'a'}, {1}},
            {{0, 'b'}, {2}},
            {{2, 'a'}, {2, 3}},
            {{2, 'b'}, {2}},
            {{3, 'a'}, {4}},
        },
        0,
        {1, 4},
    };
    NFA c2{
        {0, 1, 2},
        {'a', 'b'},
        {
            {{0, 'a'}, {0}},
            {{0, 'b'}, {0, 1}},
            {{1, 'b'}, {2}},
        },
        0,
        {2},
    };
    DFA c{
        {0},
        {'a', 'b'},
        {},
        0,
        {},
    };
    assert(intersect(c1, c2) == c);

    NFA d1{
        {0, 1, 2, 3},
        {'i', 'k', 'q'},
        {
            {{0, 'i'}, {2}},
            {{0, 'k'}, {1, 2, 3}},
            {{0, 'q'}, {0, 3}},
            {{1, 'i'}, {1}},
            {{1, 'k'}, {0}},
            {{1, 'q'}, {1, 2, 3}},
            {{2, 'i'}, {0, 2}},
            {{3, 'i'}, {3}},
            {{3, 'k'}, {1, 2}},
        },
        0,
        {2, 3},
    };
    NFA d2{
        {0, 1, 2, 3},
        {'i', 'k'},
        {
            {{0, 'i'}, {3}},
            {{0, 'k'}, {1, 2, 3}},
            {{1, 'k'}, {2}},
            {{2, 'i'}, {0, 1, 3}},
            {{2, 'k'}, {0, 1}},
        },
        0,
        {2, 3},
    };
    DFA d{
        {0, 1, 2, 3},
        {'i', 'k', 'q'},
        {
            {{0, 'i'}, {1}},
            {{0, 'k'}, {2}},
            {{2, 'i'}, {3}},
            {{2, 'k'}, {2}},
            {{3, 'i'}, {1}},
            {{3, 'k'}, {2}},
        },
        0,
        {1, 2, 3},
    };
    assert(intersect(d1, d2) == d);
}
#endif
