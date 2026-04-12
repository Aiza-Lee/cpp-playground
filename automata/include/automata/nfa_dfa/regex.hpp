#pragma once

#include <expected>
#include <string>
#include <string_view>

#include "automata/nfa_dfa/model.hpp"

namespace automata::nfa_dfa {

// Build an epsilon-NFA from a regular expression using Thompson construction.
[[nodiscard]] auto build_epsilon_nfa_from_regex(std::string_view regex_text)
	-> std::expected<AutomatonInput, std::string>;

// Apply a small set of algebraic simplifications and render a canonical regex.
[[nodiscard]] auto simplify_regex(std::string_view regex_text)
	-> std::expected<std::string, std::string>;

// Convert a minimized DFA into an equivalent regular expression via state elimination.
[[nodiscard]] auto build_regex_from_minimized_dfa(
	const AutomatonInput& source_automaton,
	const MinimizedDfaResult& minimized_dfa) -> std::string;

}  // namespace automata::nfa_dfa
