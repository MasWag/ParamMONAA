#pragma once
/*! 
  @file parametric_monaa.hh
  
  @section Parametric Timed Pattern Matching

  @subsection NNC_Polyhedra

  The dimension of NNC_Polyhedra is used as follows.

  - 0: t (beginning of the trimming)
  - 1 -- A.A.paramDimensions: pi (parameters)
  - A.paramDimensions + 1 -- A.paramDimensions + A.clockDimensions: xi (clock variables)
  - A.paramDimensions + A.clockDimensions + : t' (end of the trimming)

  However, when we output, the dimension is translated as follows

  - 0: t (beginning of the trimming)
  - 1 -- A.A.paramDimensions: pi (parameters)
  - A.paramDimensions + 1: t' (end of the trimming)
*/ 

#include <iostream>
#include <chrono>
#include <cmath>
#include <climits>
#include <unordered_set>
#include <boost/variant.hpp>

#include "parametric_timed_automaton.hh"
#include "word_container.hh"
#include "sunday_skip_value.hh"
#include "kmp_skip_value.hh"
#include "ans_vec.hh"
#include "linear_expression.hh"
#include "polyhedron_zone_automaton.hh"
#include "pta2pza.hh"
#include "parametric_intersection.hh"
#include "parametric_kmp_skip_value.hh"

#include "utils.hh"

#ifdef ENABLE_PARAMETRIC_SKIPPING
#define ENABLE_QUICK_SEARCH
#define ENABLE_PARAMETRIC_KMP
#undef  ENABLE_KMP
#endif

#ifdef ENABLE_SKIPPING
#define ENABLE_QUICK_SEARCH
#undef  ENABLE_PARAMETRIC_KMP
#define ENABLE_KMP
#endif

#ifdef DISABLE_SKIPPING
#undef ENABLE_QUICK_SEARCH
#undef ENABLE_PARAMETRIC_KMP
#undef ENABLE_KMP
#endif

namespace parametric_monaa {
  bool enable_quick_search = false;
  bool enable_parametric_kmp = false;
  bool enable_kmp = false;
}

namespace {
  /*
    @brief Internal state of the BFS in the parametric timed pattern matching
    @note This struct is used only internally. Users cannot use this.
  */
  template<class T>
  struct InternalState {
    using Variables = char;
    //! @brief The current state in the PTA
    const PTAState *s;
    /*!
      @brief The vector for the latest reset of each clock variable.
      @note If the clock variable is not reest yet, the value is 0.
    */
    std::vector<T> resetTime;
    /*!
      @brief The constraint on the beginnng time t and the parameters.
    */
    Parma_Polyhedra_Library::NNC_Polyhedron constraint;
    InternalState(const PTAState *s, const std::vector<T> &resetTime, const Parma_Polyhedra_Library::NNC_Polyhedron &constraint) : s(std::move(s)), resetTime(std::move(resetTime)), constraint(std::move(constraint)) {}
  };
}

using PSundaySkipValue = SundaySkipValueTemplate<ParametricTimedAutomaton, typename Parma_Polyhedra_Library::NNC_Polyhedron>;
using PKMPSkipValue = KMPSkipValueTemplate<ParametricTimedAutomaton, typename Parma_Polyhedra_Library::NNC_Polyhedron>;

/*!
  @brief Execute the parametric timed FJS algorithm.
  @param [in] word Container of a timed word representing a log.
  @param [in] A Parametric timed automaton used as a pattern.
  @param [out] ans Container for the answer polyhedra.
*/
template <class InputContainer, class OutputContainer>
void parametricMonaa(WordContainer<InputContainer> word,
                     ParametricTimedAutomaton A,
                     AnsContainer<OutputContainer> &ans)
{
  // Construct Ap.
  // Ap is the PTA that is basically a copy of A but the accepting states are the states having a transition to the original accepting states. Ap is used for skip value computation.
  ParametricTimedAutomaton Ap;
  Ap.states.reserve(A.states.size());
  Ap.initialStates.reserve(A.initialStates.size());
  Ap.clockDimensions = A.clockDimensions;
  Ap.paramDimensions = A.paramDimensions;
  // Mapping from States of A to States of Ap
  std::unordered_map<const PTAState*, std::shared_ptr<PTAState>> ptrConv;

  for (std::shared_ptr<PTAState> s: A.states) {
    ptrConv[s.get()] = std::make_shared<PTAState>(*s);
    Ap.states.push_back(ptrConv[s.get()]);
    if (std::binary_search(A.initialStates.begin(), A.initialStates.end(), s)) {
      Ap.initialStates.push_back(ptrConv[s.get()]);
    }
  }

  for (std::shared_ptr<PTAState> s: Ap.states) {
    if (s->next.find('$') != s->next.end()) {
      s->isMatch = true;
      s->next['$'].clear();
      s->next.erase('$');
    }
    for(auto &transitionsPair: s->next) {
      for(auto &transition: transitionsPair.second) {
        transition.target = ptrConv[transition.target].get();
      }
    }
  }

  // Sunday's Skip value
  // Char -> Skip Value
  int m;
  const PSundaySkipValue delta = PSundaySkipValue(Ap);
  std::unordered_set<Alphabet> endChars;
  if (parametric_monaa::enable_quick_search) {
    m = delta.getM();
    delta.getEndChars(endChars);
  } else {
    m = 1;
  }

  // KMP-Type Skip value
  // A.State -> SkipValue
  const ParametricKMPSkipValue betaParamKMP(Ap, m);
  const PKMPSkipValue betaKMP(Ap, m);
  std::unordered_map<PTAState*, std::size_t> betaNoSkip;
  for (std::shared_ptr<PTAState> s: A.states) {
    betaNoSkip[s.get()] = 1;
  }

  // main computation
  // We assume there is no epsilon transitions.
  std::size_t i = 0;
  std::vector<InternalState<typename InputContainer::TimeStamp>> CStates;
  std::vector<InternalState<typename InputContainer::TimeStamp>> LastStates;
  const Parma_Polyhedra_Library::Variable beginningTimeVariable(0);
  std::vector<Parma_Polyhedra_Library::Variable> paramVariables;
  paramVariables.reserve(A.paramDimensions);
  for (ClockVariables p = 0; p < A.paramDimensions; p++ ) {
    paramVariables.emplace_back(Parma_Polyhedra_Library::Variable(p + 1));
  }
  std::vector<Parma_Polyhedra_Library::Variable> clockVariables;
  clockVariables.reserve(A.clockDimensions);
  for (ClockVariables x = 0; x < A.clockDimensions; x++ ) {
    clockVariables.emplace_back(Parma_Polyhedra_Library::Variable(A.paramDimensions + x + 1));
  }
  const Parma_Polyhedra_Library::Constraint zeroBounds = beginningTimeVariable >= 0;
  
  // When there can be immidiate accepting
  // @todo This optimization is not yet when we have epsilon transitions
#if 0
  std::vector<std::pair<std::pair<double,bool>,std::pair<double,bool> > > init;

  if (m == 1) {
    for (const auto initState: A.initialStates) {
      for (char c = 0; c < CHAR_MAX; c++) {
        for (const auto &edge: initState->next[c]) {
          if (edge.target.lock()->isMatch) {
            // solve delta
            IntermediateZone zone = Zone::zero(2);
            for (const auto &constraint: edge.guard) {
              zone.tighten(1, constraint);
            }
            if (zone.isSatisfiableCanonized()) {
              init[c].push_back(std::move(zone));
            }
          }
        }
      }
    }
  }
#endif

  ans.clear();
  std::size_t j;
  while (word.fetch(i + m - 1)) {
    if (parametric_monaa::enable_quick_search) {
      bool tooLarge = false;
      // Sunday Shift
      if (m > 1 && word.fetch(i + m - 1)) {
        while (endChars.find(word[i + m - 1].first) == endChars.end() ) {
          if (!word.fetch(i + m)) {
            tooLarge = true;
            break;
          }
          // increment i
          i += delta[ word[i + m].first ];
          word.setFront(i - 1);
          if (!word.fetch(i + m - 1))  {
            tooLarge = true;
            break;
          }
        }
      }

      if (tooLarge) break;
    }

    // KMP like Matching
    CStates.clear ();
    CStates.reserve(A.initialStates.size());
    if (word.fetch(i)) {
      // Construct the initial configuration
      const std::vector<typename InputContainer::TimeStamp> zeroResetTime(A.clockDimensions, 0);
      // The clock variable for the beginning time.
      const Parma_Polyhedra_Library::Constraint lowerConstraint = (i <= 0) ? zeroBounds : LinearExpression(beginningTimeVariable) >= word[i-1].second;
      const Parma_Polyhedra_Library::Constraint upperConstraint = LinearExpression(beginningTimeVariable) < word[i].second;
      // Dimension of the constraint is the 1 (for beginningTimeVariable) + paramDimensions
      Parma_Polyhedra_Library::NNC_Polyhedron constraint = Parma_Polyhedra_Library::NNC_Polyhedron(A.paramDimensions + 1);
      constraint.add_constraint(lowerConstraint);
      constraint.add_constraint(upperConstraint);
      InternalState<typename InputContainer::TimeStamp> istate = {nullptr,
                              zeroResetTime,
                              constraint};

      CStates.resize(A.initialStates.size(), istate);
      for (std::size_t k = 0; k < A.initialStates.size(); k++) {
        CStates[k].s = A.initialStates[k].get();
      }
    } else {
      break;
    }
    j = i;
    while (!CStates.empty () && word.fetch(j)) {
      const Alphabet c = word[j].first;
      const typename InputContainer::TimeStamp currentTime = word[j].second;

      // try to go to an accepting state
      for (const auto &config : CStates) {
        const PTAState *s = config.s;
        auto it = s->next.find('$');
        if (it == s->next.end()) {
          continue;
        }
        for (const auto &edge : it->second) {
          auto target = edge.target;
          if (!target || !target->isMatch) {
            continue;
          }
          Parma_Polyhedra_Library::NNC_Polyhedron constraint = config.constraint;
          assert(constraint.space_dimension() == A.paramDimensions + 1);
          // add dimension for clockVariables
          constraint.add_space_dimensions_and_embed(A.clockDimensions);
          constraint.intersection_assign(edge.guard);
          
          // add dimension for endTimeVariable;
          constraint.add_space_dimensions_and_embed(1);          
          const Parma_Polyhedra_Library::Variable endTimeVariable(A.paramDimensions + A.clockDimensions + 1);

          const Parma_Polyhedra_Library::Constraint upperEndConstraint = LinearExpression(endTimeVariable) <= word[j].second;
          const Parma_Polyhedra_Library::Constraint lowerEndConstraint = (j > 0) ? LinearExpression(endTimeVariable) > word[j-1].second : zeroBounds;
          constraint.add_constraint(upperEndConstraint);
          constraint.add_constraint(lowerEndConstraint);


          for (ClockVariables x = 0; x < A.clockDimensions; x++) {
            if (config.resetTime[x]) {
              // x is reset at config.resetTime[x]
              constraint.add_constraint(Parma_Polyhedra_Library::Constraint(LinearExpression(endTimeVariable - clockVariables[x]) == config.resetTime[x]));
            } else {
              // x is never reset.
              constraint.add_constraint(Parma_Polyhedra_Library::Constraint(LinearExpression(endTimeVariable - beginningTimeVariable) == clockVariables[x]));
            }
          }

          if (constraint.is_empty()) {
            continue;
          }

          if (A.clockDimensions > 0) {
            const Parma_Polyhedra_Library::Variables_Set clockVariablesSet(clockVariables[0], clockVariables[A.clockDimensions - 1]);
            constraint.remove_space_dimensions(clockVariablesSet);
          }

          ans.push_back(std::move(constraint));
        }
      }

      // try observable transitios
      LastStates = std::move(CStates);
      for (const auto &config : LastStates) {
        const PTAState *s = config.s;
        auto it = s->next.find(c);
        if (it == s->next.end()) {
          continue;
        }
        for (auto &edge : it->second) {
          const auto target = edge.target;
          if (!target) {
            continue;
          }

          Parma_Polyhedra_Library::NNC_Polyhedron constraint = config.constraint;
          assert(constraint.space_dimension() == A.paramDimensions + 1);
          // add dimension for clockVariables
          constraint.add_space_dimensions_and_embed(A.clockDimensions);

          for (ClockVariables x = 0; x < A.clockDimensions; x++) {
            if (config.resetTime[x]) {
              // x is reset at config.resetTime[x]
              constraint.add_constraint(Parma_Polyhedra_Library::Constraint(LinearExpression(clockVariables[x]) == static_cast<typename InputContainer::TimeStamp>(currentTime - config.resetTime[x])));
            } else {
              // x is never reset.
              constraint.add_constraint(Parma_Polyhedra_Library::Constraint(LinearExpression(beginningTimeVariable + clockVariables[x]) == currentTime));
            }
          }

          constraint.intersection_assign(edge.guard);
          
          if (constraint.is_empty()) {
            continue;
          }
            
          auto tmpResetTime = config.resetTime;
          for (auto i : edge.resetVars) {
            tmpResetTime[i] = currentTime;
          }
            
          if (A.clockDimensions > 0) {
            const Parma_Polyhedra_Library::Variables_Set clockVariablesSet(clockVariables[0], clockVariables[A.clockDimensions - 1]);
            constraint.remove_space_dimensions(clockVariablesSet);
          }

          CStates.emplace_back(edge.target, std::move(tmpResetTime), std::move(constraint));
        }
      }
      j++;
    }
    if (!word.fetch(j)) {
      // try to go to an accepting state
      for (const auto &config : CStates) {
        const PTAState *s = config.s;
        auto it = s->next.find('$');
        if (it == s->next.end()) {
          continue;
        }
        for (const auto &edge : it->second) {
          auto target = edge.target;
          if (!target || !target->isMatch) {
            continue;
          }
          Parma_Polyhedra_Library::NNC_Polyhedron constraint = config.constraint;
          assert(constraint.space_dimension() == A.paramDimensions + 1);
          // add dimension for clockVariables
          constraint.add_space_dimensions_and_embed(A.clockDimensions);
          constraint.intersection_assign(edge.guard);
          
          // add dimension for endTimeVariable;
          constraint.add_space_dimensions_and_embed(1);          

          const Parma_Polyhedra_Library::Variable endTimeVariable(A.paramDimensions + A.clockDimensions + 1);
            
          const Parma_Polyhedra_Library::Constraint lowerEndConstraint = (j > 0) ? LinearExpression(endTimeVariable) > word[j-1].second : zeroBounds;
          constraint.add_constraint(lowerEndConstraint);

          for (ClockVariables x = 0; x < A.clockDimensions; x++) {
            if (config.resetTime[x]) {
              // x is reset at config.resetTime[x]
              constraint.add_constraint(Parma_Polyhedra_Library::Constraint(LinearExpression(endTimeVariable - clockVariables[x]) == config.resetTime[x]));
            } else {
              // x is never reset.
              constraint.add_constraint(Parma_Polyhedra_Library::Constraint(LinearExpression(endTimeVariable - beginningTimeVariable) == clockVariables[x]));
            }
          }

          if (constraint.is_empty()) {
            continue;
          }

          if (A.clockDimensions > 0) {
            const Parma_Polyhedra_Library::Variables_Set clockVariablesSet(clockVariables[0], clockVariables[A.clockDimensions - 1]);
            constraint.remove_space_dimensions(clockVariablesSet);
          }

          ans.push_back(std::move(constraint));
        }
      }
      LastStates = std::move(CStates);
    }
    // KMP like skip value
    std::size_t greatestN = 1;
    if (parametric_monaa::enable_parametric_kmp) {
    for (const InternalState<typename InputContainer::TimeStamp>& istate: LastStates) {
      greatestN = std::max(betaParamKMP.at(ptrConv[istate.s], istate.constraint), greatestN);
    }
    } else if (parametric_monaa::enable_kmp)
    for (const InternalState<typename InputContainer::TimeStamp>& istate: LastStates) {
      greatestN = std::max(betaKMP[ptrConv[istate.s].get()], greatestN);
    }
    // increment i
    i += greatestN;
    word.setFront(i - 1);
  }
}
