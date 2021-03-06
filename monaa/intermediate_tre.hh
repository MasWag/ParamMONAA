#pragma once

#include <unordered_set>

#include "interval.hh"
#include "tre.hh"

struct SyntacticDecision {
  enum class Decision {
    Constant,
    Mixed
  };
  Decision tag;
  //! @note chars must be sorted
  std::vector<Alphabet> chars;
  SyntacticDecision(const Decision &tag, const std::vector<Alphabet> &chars) : tag(tag), chars(chars) {}
  void concat(std::shared_ptr<SyntacticDecision> in);
  inline bool isMixed() const {
    return tag == Decision::Mixed && !chars.empty();
  }
  inline bool isConstant(char &singleC) {
    if (chars.size() == 1) {
      singleC = chars[0];
      return true;
    } else {
      return false;
    }
  }
};

class SingletonTRE {
public:
  Alphabet c;
  //! @todo this is too naive implemenation. this must be slow.
  std::vector<std::shared_ptr<Interval>> intervals;
  SingletonTRE(Alphabet c, std::vector<std::shared_ptr<Interval>> intervals = {std::make_shared<Interval>()}) : c(c), intervals(std::move(intervals)) {}
};

class AtomicTRE {
public:
  enum class op {
    singleton,
    epsilon,
    concat,
    plus,
    within
  };

  AtomicTRE() : tag(op::epsilon) {}
  AtomicTRE(const Alphabet c) : tag(op::singleton), singleton(std::make_shared<SingletonTRE>(c)){}
  AtomicTRE(const std::shared_ptr<SingletonTRE> singleton) : tag(op::singleton), singleton(singleton) {}
  AtomicTRE(const std::list<std::shared_ptr<AtomicTRE>> &left, const std::shared_ptr<AtomicTRE> mid, const std::list<std::shared_ptr<AtomicTRE>> &right) : tag(op::concat), list() {
    list = left;
    list.push_back(mid);
    list.insert(list.end(), right.begin(), right.end());
    }
  AtomicTRE(const std::shared_ptr<AtomicTRE> left, const std::shared_ptr<AtomicTRE> right) : tag(op::concat), list() {
    if (left->tag == op::concat) {
      list = left->list;
    } else if (left->tag != op::epsilon) {
      list = {left};
    }
    if (right->tag == op::concat) {
      list.insert(list.end(), right->list.begin(), right->list.end());
    } else if (right->tag != op::epsilon) {
      list.push_back(right);
    }
    // When the list is not concat anymore, use the original one
    if (list.size() == 0) {
      tag = op::epsilon;
      list.~list();
    } else if (list.size() == 1) {
      auto origExpr = list.front();
      tag = origExpr->tag;
      list.~list();
      switch (tag) {
      case op::singleton: {
        singleton = origExpr->singleton;
        break;
      }
      case op::epsilon: {
        break;
      }
      case op::concat: {
        list = origExpr->list;
        break;
      }
      case op::plus: {
        expr = origExpr->expr;
        break;
      }
      case op::within: {
        within = origExpr->within;
        break;
      }
      }
    }
  }
  AtomicTRE(const std::shared_ptr<DNFTRE> expr) : tag(op::plus), expr(expr) {}
  AtomicTRE(const std::shared_ptr<AtomicTRE> atomic, const std::shared_ptr<Interval> interval) :tag (atomic->tag == op::singleton ? op::singleton : op::within) {
    if (atomic->tag == op::singleton) {
      new (&singleton) std::shared_ptr<SingletonTRE>(std::make_shared<SingletonTRE>(*(atomic->singleton)));
      land(singleton->intervals, *interval);
    } else {
      new (&within) std::pair<std::shared_ptr<AtomicTRE>, std::shared_ptr<Interval>>(atomic, interval);
    }
  }

  ~AtomicTRE() {
    switch(tag) {
    case op::epsilon:
      break;
    case op::singleton:
      singleton.reset();
      break;
    case op::plus:
      expr.reset();
      break;
    case op::within:
      within.~pair<std::shared_ptr<AtomicTRE>, std::shared_ptr<Interval>>();
      break;
    case op::concat:
      list.~list();
      break;
    }
  }

  op tag;

  union {
    std::shared_ptr<SingletonTRE> singleton;
    std::shared_ptr<DNFTRE> expr;
    std::pair<std::shared_ptr<AtomicTRE>, std::shared_ptr<Interval>>  within;
    std::list<std::shared_ptr<AtomicTRE>> list;
  };
  std::shared_ptr<SyntacticDecision> decision;
  void toNormalForm();
  bool makeSNF(const char singleC);
  void toSignalTA(TimedAutomaton& out) const;
};

class DNFTRE {
public:
  std::list<std::list<std::shared_ptr<AtomicTRE>>> list;
  DNFTRE() {}
  DNFTRE(const std::list<std::shared_ptr<AtomicTRE>> &conjunctions) : list({conjunctions}){}
  DNFTRE(const std::shared_ptr<const TRE> tre);
  std::shared_ptr<SyntacticDecision> decision;
  void toNormalForm();
  bool makeSNF(const char singleC);
  void toSignalTA(TimedAutomaton& out) const;
};
