#pragma once

#include <expected>
#include <filesystem>
#include <span>
#include <string>

namespace automata::nfa_dfa {

enum class ProgramInputKind {
	K_JSON_AUTOMATON,
	K_REGEX_EXPRESSION,
	K_REGEX_FILE,
};

struct ProgramOptions {
	ProgramInputKind input_kind = ProgramInputKind::K_JSON_AUTOMATON;
	std::filesystem::path input_path;
	std::string regex_text;
	bool show_help = false;
	std::string help_text;
};

// Parse positional arguments: <input.json>.
[[nodiscard]] auto parse_command_line(std::span<const char* const> args)
	-> std::expected<ProgramOptions, std::string>;

// Run the full program pipeline: open JSON, convert epsilon-NFA to NFA when
// needed, construct DFA, then write the formatted result.
[[nodiscard]] auto run(const ProgramOptions& options) -> std::expected<void, std::string>;

}  // namespace automata::nfa_dfa
