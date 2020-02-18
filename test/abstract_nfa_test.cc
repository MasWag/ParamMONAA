#include <boost/test/unit_test.hpp>

#include "../libmonaa/abstract_nfa.hh"

struct NFAState : public AbstractNFAState<NFAState> {};

struct NFA : public AbstractNFA<NFAState> {};

BOOST_AUTO_TEST_CASE(removeDeadStatesTest)
{
  // Input
  NFA A;
  A.states.resize(3);
  for (auto &state: A.states) {
    state = std::make_shared<NFAState>();
  }
  A.initialStates.push_back(A.states[0]);

  A.states[0]->isMatch = false;
  A.states[1]->isMatch = false;
  A.states[2]->isMatch = true;

  // Transitions
  A.states[0]->next['a'].push_back(A.states[1]);
  A.states[0]->next['a'].push_back(A.states[2]);

  // Run
  A.removeDeadStates();

  // Comparison
  BOOST_TEST (A.stateSize() == 2);
  BOOST_TEST (A.initialStates.size() == 1);
}

BOOST_AUTO_TEST_CASE(epsilonClosureTest)
{
  // Input
  NFA A;
  A.states.resize(3);
  for (auto &state: A.states) {
    state = std::make_shared<NFAState>();
  }
  A.initialStates.push_back(A.states[0]);

  A.states[0]->isMatch = false;
  A.states[1]->isMatch = false;
  A.states[2]->isMatch = true;

  // Transitions
  A.states[0]->next[0].push_back(A.states[0]);
  A.states[0]->next[0].push_back(A.states[1]);
  A.states[0]->next['a'].push_back(A.states[2]);

  std::unordered_set<std::shared_ptr<NFAState>> closure;
  closure.insert(A.states[0]);

  // Run
  epsilonClosure(closure);

  // Comparison
  BOOST_TEST (closure.size() == 2);
}
