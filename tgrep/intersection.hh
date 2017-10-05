#pragma once

#include <boost/unordered_map.hpp>
#include "timed_automaton.hh"

/*
  Specifications
  ==============
  * (s,s') is encoded as s + |S| * s'
  * x = x1 or x2 + |C|
  * in1 and in2 can be same.
*/
void intersectionTA (const TimedAutomaton &in1, const TimedAutomaton &in2, TimedAutomaton &out)
{
  //! (in1.State, in2.State) -> out.State
  boost::unordered_map<std::pair<std::shared_ptr<TAState>, std::shared_ptr<TAState>>, std::shared_ptr<TAState>> toIState;

  // make states
  out.states.reserve(in1.stateSize() * in2.stateSize());
  for (auto s1: in1.states) {
    for (auto s2: in2.states) {
      toIState[std::make_pair(s1,s2)] = std::make_shared<TAState>(s1->isMatch && s2->isMatch);      
      out.states.push_back(toIState[std::make_pair(s1,s2)]);
    }
  }

  // make initial states
  out.initialStates.clear ();
  out.initialStates.reserve (in1.initialStates.size() * in2.initialStates.size());
  for (auto s1: in1.initialStates) {
    for (auto s2: in2.initialStates) {
      out.initialStates.push_back(toIState[std::make_pair(s1,s2)]);
    }
  }

  // make max constraints
  out.maxConstraints.reserve( in1.maxConstraints.size() + in2.maxConstraints.size() ); // preallocate memory
  out.maxConstraints.insert( out.maxConstraints.end(), in1.maxConstraints.begin(), in1.maxConstraints.end() );
  out.maxConstraints.insert( out.maxConstraints.end(), in2.maxConstraints.begin(), in2.maxConstraints.end() );

  // make edges
  for (auto s1: in1.states) {
    for (auto s2: in2.states) {
      // Epsilon transitions
      //! @todo not yet: write a function to do the loop and apply
      for (const auto &e1: s1->next[0]) {
        auto nextS1 = e1.target.lock();
        if (!nextS1) {
          continue;
        }
        TATransition transition;
        transition.target = toIState[std::make_pair(nextS1, s2)];
        
      }
      for (const auto &e2: s2->next[0]) {
        auto nextS2 = e2.target.lock();
        if (!nextS2) {
          continue;
        }
        TATransition transition;
        transition.target = toIState[std::make_pair(s1, nextS2)];
      }

      // Observable transitions
      for (char c = 1; c < CHAR_MAX; c++) {
        for (const auto &e1: s1->next[c]) {
          auto nextS1 = e1.target.lock();
          if (!nextS1) {
              continue;
          }
          for (const auto &e2: s2->next[c]) {
            auto nextS2 = e2.target.lock();
            if (!nextS2) {
              continue;
            }
            TATransition transition;
            transition.target = toIState[std::make_pair(nextS1, nextS2)];

            //concat resetVars
            transition.resetVars.reserve (e1.resetVars.size() + e2.resetVars.size());
            transition.resetVars.insert (transition.resetVars.end(),e1.resetVars.begin(), e1.resetVars.end());
            transition.resetVars.insert (transition.resetVars.end(),e2.resetVars.begin(), e2.resetVars.end());
            std::for_each (transition.resetVars.begin () + e1.resetVars.size(),
                           transition.resetVars.end(),
                           [&] (char &v) { v += in1.clockSize();});

            //concat constraints
            transition.guard.reserve (e1.guard.size() + e2.guard.size());
            transition.guard.insert (transition.guard.end(),e1.guard.begin(), e1.guard.end());
            transition.guard.insert (transition.guard.end(),e2.guard.begin(), e2.guard.end());
            std::for_each (transition.guard.begin () + e1.guard.size(),
                           transition.guard.end(),
                           [&] (Constraint &guard) { guard.x += in1.clockSize();});

            toIState[std::make_pair(nextS1,nextS2)]->next[c].push_back(transition);
          }
        }
      }
    }
  }  
}