#pragma once

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/optional.hpp>
#include <fstream>
#include <iostream>
#include <sstream>

#include "../libmonaa/parametric_timed_automaton.hh"
#include "constraint_driver.hh"

namespace boost{
  enum vertex_match_t {vertex_match};
  enum edge_label_t {edge_label};
  enum edge_reset_t {edge_reset};
  enum edge_guard_t {edge_guard};
  enum graph_clock_dimensions_t {graph_clock_dimensions};
  enum graph_param_dimensions_t {graph_param_dimensions};

  BOOST_INSTALL_PROPERTY(vertex, match);
  BOOST_INSTALL_PROPERTY(edge, label);
  BOOST_INSTALL_PROPERTY(edge, reset);
  BOOST_INSTALL_PROPERTY(edge, guard);
  BOOST_INSTALL_PROPERTY(graph, clock_dimensions);
  BOOST_INSTALL_PROPERTY(graph, param_dimensions);
}

namespace {
  std::unique_ptr<ConstraintDriver> driverRef;
}

static inline 
std::ostream& operator<<(std::ostream& os, const Constraint::Order& odr) {
  switch (odr) {
  case Constraint::Order::lt:
    os << "<";
    break;
  case Constraint::Order::le:
    os << "<=";
    break;
  case Constraint::Order::ge:
    os << ">=";
    break;
  case Constraint::Order::gt:
    os << ">";
    break;
  }
  return os;
}

static inline 
std::ostream& operator<<(std::ostream& os, const Constraint& p)
{
  os << "x" << int(p.x) << " " << p.odr << " " << p.c;
  return os;
}

template<class T>
static inline 
std::ostream& operator<<(std::ostream& os, const std::vector<T>& guard)
{
  bool first = true;
  os << "{";
  for (const auto &g: guard) {
    if (!first) {
      os << ", ";
    }
    first = false;
    os << g;
  }
  os << "}";
  return os;
}

static inline 
std::istream& operator>>(std::istream& is, Constraint& p)
{
  if (is.get() != 'x') {
    is.setstate(std::ios_base::failbit);
    return is;
  }
  int x;
  is >> x;
  p.x = x;
  if (!is) {
    is.setstate(std::ios_base::failbit);
    return is;
  }

  if (is.get() != ' ') {
    is.setstate(std::ios_base::failbit);
    return is;
  }

  char odr[2];
  is >> odr[0] >> odr[1];

  switch (odr[0]) {
  case '>':
    if (odr[1] == '=') {
      p.odr = Constraint::Order::ge;
      if (is.get() != ' ') {
        is.setstate(std::ios_base::failbit);
        return is;
      }
    } else if (odr[1] == ' ') {
      p.odr = Constraint::Order::gt;
    } else {
      is.setstate(std::ios_base::failbit);
      return is;
    }
    break;
  case '<':
    if (odr[1] == '=') {
      p.odr = Constraint::Order::le;
      if (is.get() != ' ') {
        is.setstate(std::ios_base::failbit);
        return is;
      }
    } else if (odr[1] == ' ') {
      p.odr = Constraint::Order::lt;
    } else {
      is.setstate(std::ios_base::failbit);
      return is;
    }
    break;
  default:
    is.setstate(std::ios_base::failbit);
    return is;
  }

  is >> p.c;
  return is;
}

static inline 
std::ostream& operator<<(std::ostream& os, const std::string& resetVars)
{
  bool first = true;
  os << "{";
  for (const auto &x: resetVars) {
    if (!first) {
      os << ", ";
    }
    first = false;
    os << int(x);
  }
  os << "}";
  return os;
}

static inline 
std::istream& operator>>(std::istream& is, std::vector<ClockVariables>& resetVars)
{
  resetVars.clear();
  if (!is) {
    is.setstate(std::ios_base::failbit);
    return is;
  }

  if (is.get() != '{') {
    is.setstate(std::ios_base::failbit);
    return is;
  }

  if (!is) {
    is.setstate(std::ios_base::failbit);
    return is;
  }

  while (true) {
    int x;
    is >> x;
    resetVars.emplace_back(ClockVariables(x));
    if (!is) {
      is.setstate(std::ios_base::failbit);
      return is;
    }
    char c;
    is >> c;
    if (c == '}') {
      break;
    } else if (c == ',') {
      is >> c;
      if (c != ' ') {
        is.setstate(std::ios_base::failbit);
        return is;
      }
    } else {
      is.setstate(std::ios_base::failbit);
      return is;
    }
  }

  return is;
}

template <class T>
static inline 
std::istream& operator>>(std::istream& is, std::vector<T>& resetVars)
{
  resetVars.clear();
  if (!is) {
    is.setstate(std::ios_base::failbit);
    return is;
  }

  if (is.get() != '{') {
    is.setstate(std::ios_base::failbit);
    return is;
  }

  if (!is) {
    is.setstate(std::ios_base::failbit);
    return is;
  }

  while (true) {
    T x;
    is >> x;
    resetVars.emplace_back(std::move(x));
    if (!is) {
      is.setstate(std::ios_base::failbit);
      return is;
    }
    char c;
    is >> c;
    if (c == '}') {
      break;
    } else if (c == ',') {
      is >> c;
      if (c != ' ') {
        is.setstate(std::ios_base::failbit);
        return is;
      }
    } else {
      is.setstate(std::ios_base::failbit);
      return is;
    }
  }

  return is;
}

namespace boost {
  template <class T>
  static inline 
  std::ostream& operator<<(std::ostream& os, const boost::optional<T>& x)
  {
    if (x) {
      os << x.get();
    }
    else {
      os << "";
    }
    return os;
  }

  template <class T>
  static inline 
  std::istream& operator>>(std::istream& is, boost::optional<T>& x)
  {
    T result;
    if (is >> result) {
      x = result;
    }
    return is;
  }
}

struct ResetVars {
  std::vector<ClockVariables> resetVars;
};

static inline 
std::istream& operator>>(std::istream& is, ResetVars& resetVars)
{
  is >> resetVars.resetVars;
  return is;
}

static inline 
std::ostream& operator<<(std::ostream& os, const ResetVars& resetVars)
{
  os << resetVars.resetVars;
  return os;
}

struct BoostPTAState {
  bool isInit;
  bool isMatch;
};

struct BoostPTATransition {
  Alphabet c;
  //! @note this structure is necessary because of some problem in boost graph
  ResetVars resetVars;
  Parma_Polyhedra_Library::Constraint_System guard;
};

using BoostParametricTimedAutomaton = 
        boost::adjacency_list<
  boost::listS, boost::vecS, boost::directedS,
  BoostPTAState, BoostPTATransition,boost::property<boost::graph_clock_dimensions_t, std::size_t,
                                                    boost::property<boost::graph_param_dimensions_t, std::size_t>>>;

namespace Parma_Polyhedra_Library {
  using Parma_Polyhedra_Library::IO_Operators::operator<<;

  static inline 
  std::istream& operator>>(std::istream& is, Parma_Polyhedra_Library::Constraint_System& cs)
  {
    driverRef->parse(is);
    cs = driverRef->getResult();

    return is;
  }
}

namespace boost {
   template<>
   Parma_Polyhedra_Library::Constraint_System lexical_cast(const std::string& str) {
     std::stringstream stream;
     stream << str;
     Parma_Polyhedra_Library::Constraint_System cs;
     stream >> cs;
     return cs;
    }
   template<>
   std::string lexical_cast(const Parma_Polyhedra_Library::Constraint_System& cs) {
     std::stringstream stream;
     stream << cs;
     std::string str;
     stream >> str;
     return str;
    }
  using Parma_Polyhedra_Library::operator<<;
  using Parma_Polyhedra_Library::operator>>;
}

static inline 
void parseBoostTA(std::istream &file, BoostParametricTimedAutomaton &BoostPTA)
{
  //// read only the dimension at first
  {
    BoostParametricTimedAutomaton tmpGraph;
    std::size_t paramDimensions;
    boost::dynamic_properties dp(boost::ignore_other_properties);
    boost::ref_property_map<BoostParametricTimedAutomaton*, std::size_t> gparam_dimensions(get_property(tmpGraph, boost::graph_param_dimensions));
    dp.property("param_dimensions", gparam_dimensions);

    boost::read_graphviz(file, tmpGraph, dp);
    paramDimensions = boost::get_property(tmpGraph, boost::graph_param_dimensions);

    std::unique_ptr<ConstraintDriver> driver = std::make_unique<ConstraintDriver>(paramDimensions);
    driverRef = std::move(driver);
  }

  //// read other properties
  boost::dynamic_properties dp(boost::ignore_other_properties);
  boost::ref_property_map<BoostParametricTimedAutomaton*, std::size_t> gparam_dimensions(get_property(BoostPTA, boost::graph_param_dimensions));
  dp.property("param_dimensions", gparam_dimensions);
  boost::ref_property_map<BoostParametricTimedAutomaton*, std::size_t> gclock_dimensions(get_property(BoostPTA, boost::graph_clock_dimensions));
  dp.property("clock_dimensions", gclock_dimensions);
  dp.property("match", boost::get(&BoostPTAState::isMatch, BoostPTA));
  dp.property("init",  boost::get(&BoostPTAState::isInit, BoostPTA));
  dp.property("label", boost::get(&BoostPTATransition::c, BoostPTA));
  dp.property("reset", boost::get(&BoostPTATransition::resetVars, BoostPTA));
  dp.property("guard", boost::get(&BoostPTATransition::guard, BoostPTA));

  file.clear();
  file.seekg(0, std::ios_base::beg);
  boost::read_graphviz(file, BoostPTA, dp);
  driverRef.reset();
}

static inline 
void convBoostTA(const BoostParametricTimedAutomaton &BoostPTA, ParametricTimedAutomaton &PTA)
{
  PTA.clockDimensions = boost::get_property(BoostPTA, boost::graph_clock_dimensions);
  PTA.paramDimensions = boost::get_property(BoostPTA, boost::graph_param_dimensions);
  PTA.states.clear();
  PTA.initialStates.clear();
  auto vertex_range = boost::vertices(BoostPTA);
  std::unordered_map<BoostParametricTimedAutomaton::vertex_descriptor, std::shared_ptr<PTAState>> stateConvMap;
  for (auto first = vertex_range.first, last = vertex_range.second; first != last; ++first) {
    BoostParametricTimedAutomaton::vertex_descriptor v = *first;
    stateConvMap[v] = std::make_shared<PTAState>(boost::get(&BoostPTAState::isMatch, BoostPTA, v));
    PTA.states.emplace_back(stateConvMap[v]);
    if (boost::get(&BoostPTAState::isInit, BoostPTA, v)) {
      PTA.initialStates.emplace_back(stateConvMap[v]);
    }
  }

  for (auto first = vertex_range.first, last = vertex_range.second; first != last; ++first) {
    auto edge_range = boost::out_edges(*first, BoostPTA);
    for (auto firstEdge = edge_range.first, lastEdge = edge_range.second; firstEdge != lastEdge; ++firstEdge) {    
      PTATransition transition;
      transition.target = stateConvMap[boost::target(*firstEdge, BoostPTA)].get();
      transition.guard = Parma_Polyhedra_Library::NNC_Polyhedron(boost::get(&BoostPTATransition::guard, BoostPTA, *firstEdge));
      transition.guard.add_space_dimensions_and_embed(PTA.clockDimensions + PTA.paramDimensions + 1 - transition.guard.space_dimension());
      transition.resetVars = boost::get(&BoostPTATransition::resetVars, BoostPTA, *firstEdge).resetVars;
      stateConvMap[*first]->next[boost::get(&BoostPTATransition::c, BoostPTA, *firstEdge)].emplace_back(std::move(transition));
    }
  }
}
