#pragma once

#include <iosfwd>

#include "automata/nfa_dfa/model.hpp"

namespace automata::nfa_dfa {

// Render the converted NFA and the computed DFA into the CLI output format.
void write_result(
	const AutomatonInput& source_automaton,
	const AutomatonInput& reduced_nfa,
	const DfaResult& dfa,
	std::ostream& output);

}  // namespace automata::nfa_dfa
