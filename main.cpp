#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

template <typename ItT, typename FcnT>
class mapped_iterator {
public:
  explicit mapped_iterator(ItT it, FcnT fcn)
      : m_it{std::move(it)},
        m_fcn{std::move(fcn)} {}

  mapped_iterator operator++() {
    ++m_it;
    return *this;
  }

  auto &operator*() {
    return m_fcn(*m_it);
  }

  bool operator!=(const mapped_iterator<ItT, FcnT> &it) {
    return m_it != it.m_it;
  }

private:
  ItT m_it;
  FcnT m_fcn;
};

template <typename ItT>
class range {
public:
  explicit range(ItT it, ItT end)
      : m_it{std::move(it)},
        m_end{std::move(end)} {}

  auto begin() { return m_it; }
  auto end() { return m_end; }

private:
  ItT m_it, m_end;
};

template <typename ItT> auto make_range(ItT it, ItT end) {
  return range<ItT>{std::move(it), std::move(end)};
}

template <typename ItT, typename FcnT> auto map_iterator(ItT it, FcnT fcn) {
  return mapped_iterator<ItT, FcnT>{std::move(it), std::move(fcn)};
}

template <typename ContainerT, typename FcnT>
auto map_range(const ContainerT &c, FcnT fcn) {
  return make_range(map_iterator(begin(c), fcn), map_iterator(end(c), fcn));
}

template <typename ItT> auto make_second_iterator(ItT it) {
  return map_iterator(std::move(it), [](auto &p) -> add_lvalue_reference_t<decltype(p.second)> { return p.second; });
}

class Predicate {
  friend class PredicateUniquer;

  explicit Predicate(long repr)
      : m_repr{repr} {}

public:
  auto repr() const { return m_repr; }

private:
  long m_repr;
};

class PredicateUniquer {
public:
  Predicate *get(long repr) {
    if (auto it = preds.find(repr); it != std::end(preds)) {
      return &it->second;
    }
    auto [it, inserted] = preds.emplace(repr, Predicate{repr});
    return &it->second;
  }

  auto size() const { return std::size(preds); }
  auto begin() { return make_second_iterator(std::begin(preds)); }
  auto end() { return make_second_iterator(std::end(preds)); }

private:
  unordered_map<long, Predicate> preds;
};

class Pattern {
  friend class PatternUniquer;

  explicit Pattern(string &&name, unordered_set<Predicate *> &&preds)
      : m_name{move(name)},
        m_preds{std::move(preds)} {}

public:
  const string &name() const { return m_name; }
  auto &preds() const { return m_preds; }

private:
  string m_name;
  unordered_set<Predicate *> m_preds;
};

class PatternUniquer {
public:
  Pattern *get(string name, unordered_set<Predicate *> preds) {
    if (auto it = patterns.find(name); it != std::end(patterns)) {
      return &it->second;
    }
    Pattern pattern{move(name), move(preds)};
    auto [it, inserted] = patterns.emplace(pattern.name(), move(pattern));
    return &it->second;
  }

  auto size() const { return std::size(patterns); }
  auto begin() { return make_second_iterator(std::begin(patterns)); }
  auto end() { return make_second_iterator(std::end(patterns)); }

private:
  unordered_map<string, Pattern> patterns;
};

class ReferenceNode {
public:
  explicit ReferenceNode(ReferenceNode *parent, Pattern *diff)
      : m_parent{parent},
        m_diff{diff} {}

  auto *parent() const { return m_parent; }
  auto *diff() const { return m_diff; }
  auto &refs() { return m_refs; }

private:
  ReferenceNode *m_parent;
  Pattern *m_diff;
  unordered_set<Predicate *> m_refs;
};

void rlcs(PredicateUniquer &preds, PatternUniquer &patterns) {
  size_t K{};
  for (auto &pattern : patterns) {
    K += size(pattern.preds());
  }

  vector<ReferenceNode> nodes;
  nodes.reserve(K);

  auto *root = &nodes.emplace_back(nullptr, nullptr);
  unordered_map<Predicate *, ReferenceNode *> occur;
  occur.reserve(size(preds));
  for (auto &pred : preds) {
    occur.emplace(&pred, root);
    root->refs().insert(&pred);
  }

  for (auto &pattern : patterns) {
    unordered_map<ReferenceNode *, ReferenceNode *> cache;
    for (auto *pred : pattern.preds()) {
      auto curIt = occur.find(pred);
      auto *cur = curIt->second;

      auto it = cache.find(cur);
      ReferenceNode *diff;
      if (it == end(cache)) {
        diff = &nodes.emplace_back(cur, &pattern);
        cache.emplace(cur, diff);
      } else {
        diff = it->second;
      }
      occur.insert_or_assign(curIt, pred, diff);
      cur->refs().erase(pred);
      diff->refs().insert(pred);
    }
  }

  for (auto it = nodes.rbegin(), e = nodes.rend(); it != e; ++it) {
    vector<Pattern *> trace;
    auto *cur = &*it;
    if (cur->refs().empty()) {
      continue;
    }
    while (cur->diff()) {
      trace.push_back(cur->diff());
      cur = cur->parent();
    }

    cout << "( ";
    for (auto *pattern : trace) {
      cout << pattern->name() << " ";
    }
    cout << "): [ ";
    for (auto *pred : it->refs()) {
      cout << static_cast<char>(pred->repr()) << " ";
    }
    cout << "]" << endl;
  }
}

int main() {
  PredicateUniquer preds;
  auto *A = preds.get('A');
  auto *B = preds.get('B');
  auto *C = preds.get('C');
  auto *D = preds.get('D');
  auto *E = preds.get('E');

  PatternUniquer patterns;
  auto *P1 = patterns.get("P1", {A, B, C});
  auto *P2 = patterns.get("P2", {A, B, D});
  auto *P3 = patterns.get("P3", {C, D, E});

  rlcs(preds, patterns);
}
