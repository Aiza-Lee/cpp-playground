#pragma once

#include <iosfwd>
#include <string_view>

#include "automata/nfa_dfa/model.hpp"

namespace automata::nfa_dfa {

// Render the converted NFA and the computed DFA into the CLI output format.
void write_result(
	const AutomatonInput& source_automaton,
	const AutomatonInput& reduced_nfa,
	const DfaResult& dfa,
	const MinimizedDfaResult& minimized_dfa,
	std::string_view equivalent_regex,
	std::string_view input_regex,
	std::ostream& output);

}  // namespace automata::nfa_dfa
