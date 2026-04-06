#include "automata/nfa_dfa/app.hpp"

#include <algorithm>
#include <cctype>
#include <expected>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <cxxopts.hpp>
#include <fmt/format.h>

#include "automata/nfa_dfa/algorithm.hpp"
#include "automata/nfa_dfa/output.hpp"
#include "automata/nfa_dfa/parser.hpp"

namespace automata::nfa_dfa {

namespace {

[[nodiscard]] auto has_json_extension(const std::filesystem::path& path) -> bool {
	auto extension = path.extension().string();
	std::ranges::transform(extension, extension.begin(), [](const unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	return extension == ".json";
}

}  // namespace

auto parse_command_line(std::span<const char* const> args)
	-> std::expected<ProgramOptions, std::string> {
	try {
		cxxopts::Options options(
			"nfa_dfa",
			"Convert a JSON NFA/epsilon-NFA into an equivalent NFA and DFA.");
		options.positional_help("<input.json>");
		options.parse_positional({"input"});
		options.add_options()
			("input", "Input automaton file", cxxopts::value<std::string>())
			("h,help", "Show help");

		const auto parsed = options.parse(static_cast<int>(args.size()), args.data());
		if (!parsed.unmatched().empty()) {
			return std::unexpected("expected exactly one input JSON file");
		}

		ProgramOptions program_options;
		if (parsed.contains("help")) {
			program_options.show_help = true;
			program_options.help_text = options.help();
			return program_options;
		}

		if (!parsed.contains("input")) {
			return std::unexpected("missing input JSON file");
		}

		program_options.input_path = parsed["input"].as<std::string>();
		if (!has_json_extension(program_options.input_path)) {
			return std::unexpected("input file must use the .json extension");
		}

		return program_options;
	} catch (const cxxopts::exceptions::exception& error) {
		return std::unexpected(fmt::format("invalid command line: {}", error.what()));
	}
}

auto run(const ProgramOptions& options) -> std::expected<void, std::string> {
	std::ifstream input_stream(options.input_path);
	if (!input_stream) {
		return std::unexpected(fmt::format("cannot open input file: {}", options.input_path.string()));
	}

	const auto automaton = read_automaton_json(input_stream);
	if (!automaton) {
		return std::unexpected(automaton.error());
	}

	const auto reduced_nfa = reduce_to_nfa(*automaton);
	const auto dfa = build_dfa(*automaton);

	write_result(*automaton, reduced_nfa, dfa, std::cout);
	return {};
}

}  // namespace automata::nfa_dfa
