#include <FEXCore/Utils/Regex.h>

#include <algorithm>
#include <cassert>
#include <iostream>
// Inspiration taken from https://sh4dy.com/2025/05/01/regex_engine/
namespace FEXCore::Utils {
/////////////
// STATE IMPL
/////////////
void State::addEpsilonTransition(State *nextState) {
  assert(nextState && "state needs to be non null for addEpsilonTransition");
  epsilonTransitions.push_back(nextState);
}

void State::addTransition(char c, State *nextState) {
  assert(nextState && "state needs to be non null for addTransition");
  transitions[c].push_back(nextState);
}

/////////////
// NFA IMPL
/////////////
NFA::NFA() {
  fextl::unique_ptr<State> start = fextl::make_unique<State>();
  fextl::unique_ptr<State> accepting = fextl::make_unique<State>(true);
  startState = start.get();
  acceptingState = accepting.get();

  // transfer the ownership to the states vector
  states.push_back(std::move(start));
  states.push_back(std::move(accepting));
}

void NFA::acquireStatesFrom(NFA &other) {
  for (auto &s : other.states)
    this->states.push_back(std::move(s));

  other.states.clear();
}

NFA NFA::createForEpsilon() {
  NFA nfa;
  nfa.startState->addEpsilonTransition(nfa.acceptingState);
  return nfa;
}
NFA NFA::createForDot() {
  NFA nfa;

  // INFO: For now i think let's keep it simple and spawn NFA for the whole
  // alphabet
  //
  // See if performance is acceptable or not and then we can read more dragon
  // book to find optimization
  for (auto ch : alphabet)
    nfa.startState->addTransition(ch, nfa.acceptingState);
  return nfa;
}

NFA NFA::createForChar(char c) {
  NFA nfa;
  nfa.startState->addTransition(c, nfa.acceptingState);
  return nfa;
}

// Dragon book 2nd edition, figure 3.40: NFA for the union of two regular
// expressions
NFA NFA::createForUnion(NFA &nfa1, NFA &nfa2) {
  NFA newNFA;
  nfa1.acceptingState->isAccepting = false;
  nfa2.acceptingState->isAccepting = false;
  newNFA.startState->addEpsilonTransition(nfa1.startState);
  newNFA.startState->addEpsilonTransition(nfa2.startState);
  nfa1.acceptingState->addEpsilonTransition(newNFA.acceptingState);
  nfa2.acceptingState->addEpsilonTransition(newNFA.acceptingState);

  // NOTE: we acquire state here because the use case was that we were gonna
  // leave nfa1 and nfa2 out of scope. Probably need to measure the performance
  // first but it'd be great to move away from making up unique ptr everytime
  // inside a loop
  newNFA.acquireStatesFrom(nfa1);
  newNFA.acquireStatesFrom(nfa2);
  return newNFA;
}

// Dragon book 2nd edition, figure 3.41: NFA for the concat of two regular
// expressions
NFA NFA::createForConcatenation(NFA &nfa1, NFA &nfa2) {
  NFA newNFA;
  nfa1.acceptingState->addEpsilonTransition(nfa2.startState);
  nfa1.acceptingState->isAccepting = false;
  newNFA.startState = nfa1.startState;
  newNFA.acceptingState = nfa2.acceptingState;
  newNFA.acquireStatesFrom(nfa1);
  newNFA.acquireStatesFrom(nfa2);
  return newNFA;
}

// Dragon book 2nd edition, figure 3.42: NFA for the closure of a regular
// expression
NFA NFA::createForKleeneStar(NFA &originalNFA) {
  NFA newNFA;
  newNFA.startState->addEpsilonTransition(originalNFA.startState);
  newNFA.startState->addEpsilonTransition(newNFA.acceptingState);
  originalNFA.acceptingState->addEpsilonTransition(originalNFA.startState);
  originalNFA.acceptingState->addEpsilonTransition(newNFA.acceptingState);
  originalNFA.acceptingState->isAccepting = false;
  newNFA.acquireStatesFrom(originalNFA);
  return newNFA;
}
NFA NFA::createForPlus(NFA &originalNFA) {
  NFA newNFA;
  return newNFA;
}

// Find all the states that can be reached from the current set of states using
// only epsilon transitions
fextl::set<State *> NFA::epsilonClosure(const fextl::set<State *> &states) {
  fextl::stack<State *> stateStack;
  fextl::set<State *> result = states;

  for (State *state : states)
    stateStack.push(state);

  while (!stateStack.empty()) {
    State *currState = stateStack.top();
    stateStack.pop();
    for (State *next : currState->epsilonTransitions) {
      if (result.find(next) == result.end()) {
        stateStack.push(next);
        result.insert(next);
      }
    }
  }
  return result;
}

// Find all the states that can be reached from the current set of states using
// only character transition
fextl::set<State *> NFA::move(const fextl::set<State *> &states, const char c) {
  fextl::set<State *> result;
  for (auto *state : states) {
    const decltype(state->transitions) &transitionMap = state->transitions;
    if (auto itr = transitionMap.find(c); itr != transitionMap.end()) {
      for (auto *transition : itr->second) {
        result.insert(transition);
      }
    }
  }
  return result;
}

/////////////
// REGEX IMPL
/////////////
Regex::Regex(const fextl::string &s) : pattern(s), pos(0) {
  nfa = parseExpression();
}

NFA Regex::parseExpression() { return parseUnion(); }

NFA Regex::parseUnion() {
  NFA result = parseConcatenation();
  while (pos < pattern.size() && pattern[pos] == '|') {
    pos++;
    NFA nfaToMakeUnion = parseConcatenation();
    result = NFA::createForUnion(result, nfaToMakeUnion);
  }
  return result;
}

NFA Regex::parseConcatenation() {
  NFA result = parseStar();
  while (pos < pattern.size() && pattern[pos] != '|' && pattern[pos] != ')') {
    NFA nfaToConcat = parseStar();
    result = NFA::createForConcatenation(result, nfaToConcat);
  }
  return result;
}

NFA Regex::parseStar() {
  NFA result = parseAtom();
  while (pos < pattern.size() && pattern[pos] == '*') {
    pos++;
    result = NFA::createForKleeneStar(result);
  }
  return result;
}
// Algo 3.23: Basis
NFA Regex::parseAtom() {

  if (pos >= pattern.size()) {
    return NFA::createForEpsilon();
  }
  char curChar = pattern[pos++];

  // Move past the opening brace and get the NFA for the expression till a
  // closing brace is found.
  if (curChar == '(') {
    NFA result = parseExpression();
    if (pos < pattern.size() && pattern[pos] == ')') {
      pos++;
    } else if (pos >= pattern.size()) {
      // TODO: Add error prop here somewhere
      LogMan::Msg::EFmt("Expected ')', but has no more character to parse from "
                        "regex for NFA creation\n");
      std::exit(1);
    } else {
      LogMan::Msg::EFmt(
          "Expected ')', but encountered character '{}' while parsing"
          "regex for NFA creation\n",
          pattern[pos]);
      std::exit(1);
    }
    return result;
  }

  if (curChar == '.')
    return NFA::createForDot();
  return NFA::createForChar(curChar);
}

// Dragon book 2nd edition, algorithm 3.22: Simulating an NFA
bool Regex::matches(const fextl::string &target) {
  fextl::set<State *> currentStates = NFA::epsilonClosure({nfa.startState});

  for (const auto c : target) {
    currentStates = NFA::epsilonClosure(NFA::move(currentStates, c));
    if (currentStates.empty()) {
      std::cerr << "Wrong here\n";
      return false;
    }
  }

  return std::ranges::any_of(currentStates, &State::isAccepting);
}

} // namespace FEXCore::Utils
