#pragma once

#include "automata/nfa_dfa/model.hpp"

namespace automata::nfa_dfa {

// Eliminate epsilon transitions while preserving the original state set.
[[nodiscard]] auto reduce_to_nfa(const AutomatonInput& automaton) -> AutomatonInput;

// Build the reachable DFA using epsilon-closure + subset construction.
[[nodiscard]] auto build_dfa(const AutomatonInput& automaton) -> DfaResult;

}  // namespace automata::nfa_dfa
