#pragma once
#include "FEXCore/fextl/memory.h"
#include "FEXCore/fextl/string.h"
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/stack.h>
#include <FEXCore/fextl/set.h>
#include <FEXCore/fextl/vector.h>

namespace FEXCore::Utils {

class State {
public:
  fextl::vector<State *> epsilonTransitions;
  fextl::map<char, fextl::vector<State *>> transitions;
  bool isAccepting;
  State(bool accepting = false) : isAccepting(accepting) {}
  void addEpsilonTransition(State *nextState);
  void addTransition(char c, State *nextState);
};
class NFA {
public:
  State *startState;
  State *acceptingState;
  fextl::vector<fextl::unique_ptr<State>> states;

  NFA();
  // Transfers the ownership of the states (unique_ptr) of other NFA to the
  // current NFA.
  void acquireStatesFrom(NFA &other);

  // Functions for creating NFA using the McNaughton-Yamada-Thompson algorithm
  static NFA createForEpsilon();
  static NFA createForChar(char c);
  static NFA createForDot();
  static NFA createForUnion(NFA &nfa1, NFA &nfa2);
  static NFA createForConcatenation(NFA &nfa1, NFA &nfa2);
  static NFA createForKleeneStar(NFA &originalNFA);
  static NFA createForPlus(NFA &originalNFA);
  static NFA createForQuestion(NFA &originalNFA);
  static fextl::set<State *> epsilonClosure(const fextl::set<State *> &states);
  static fextl::set<State *> move(const fextl::set<State *> &states, char c);
};

// TODO: probably an NFA vector would be better instead of State vector inside
// each NFA
class Regex {
  fextl::string pattern;
  int pos;
  NFA nfa;
  bool escaped = false;

  // Top level parser, calls parseUnion
  NFA parseExpression();

  // INFO: "a|b"
  NFA parseUnion();

  // INFO: "ab"
  NFA parseConcatenation();

  // INFO: "a*", ".*"
  NFA parseStarPlusHuhhhh();

  // INFO: "(abc)" or a
  NFA parseAtom();

public:
  static inline fextl::string alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ,./<>?;':\"[]\\{}|1234567890!@#$%^&*()-=_+";
  static inline fextl::string acceptable_escapable = ".?[]\\|";
  Regex(const fextl::string &s);
  bool matches(const fextl::string &s);
};
} // namespace FEXCore::Utils
