#pragma once

#include <expected>
#include <iosfwd>
#include <string>

#include "automata/nfa_dfa/model.hpp"

namespace automata::nfa_dfa {

// Parse the text format described in README.md into an in-memory automaton.
[[nodiscard]] auto read_automaton(std::istream& input) -> std::expected<AutomatonInput, std::string>;

// Parse the JSON format described in README.md into an in-memory automaton.
[[nodiscard]] auto read_automaton_json(std::istream& input) -> std::expected<AutomatonInput, std::string>;

}  // namespace automata::nfa_dfa
