#include "automata/nfa_dfa/app.hpp"

#include <algorithm>
#include <cctype>
#include <expected>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include <cxxopts.hpp>
#include <fmt/format.h>

#include "automata/nfa_dfa/algorithm.hpp"
#include "automata/nfa_dfa/output.hpp"
#include "automata/nfa_dfa/parser.hpp"
#include "automata/nfa_dfa/regex.hpp"

namespace automata::nfa_dfa {

namespace {

[[nodiscard]] auto has_json_extension(const std::filesystem::path& path) -> bool {
	auto extension = path.extension().string();
	std::ranges::transform(extension, extension.begin(), [](const unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	return extension == ".json";
}

[[nodiscard]] auto trim_copy(std::string text) -> std::string {
	const auto first = text.find_first_not_of(" \t\r\n");
	if (first == std::string::npos) {
		return {};
	}
	const auto last = text.find_last_not_of(" \t\r\n");
	return text.substr(first, last - first + 1);
}

}  // namespace

auto parse_command_line(std::span<const char* const> args)
	-> std::expected<ProgramOptions, std::string> {
	try {
		cxxopts::Options options(
			"nfa_dfa",
			"Convert automata and regular expressions between epsilon-NFA, DFA, minimized DFA, and regex.");
		options.positional_help("<input.json>");
		options.parse_positional({"input"});
		options.add_options()
			("input", "Input automaton file", cxxopts::value<std::string>())
			("regex", "Input regex expression", cxxopts::value<std::string>())
			("regex-file", "Input regex file", cxxopts::value<std::string>())
			("h,help", "Show help");

		const auto parsed = options.parse(static_cast<int>(args.size()), args.data());
		if (!parsed.unmatched().empty()) {
			return std::unexpected("unexpected extra positional arguments");
		}

		ProgramOptions program_options;
		if (parsed.contains("help")) {
			program_options.show_help = true;
			program_options.help_text = options.help();
			return program_options;
		}

		const auto input_source_count = static_cast<int>(parsed.contains("input")) +
			static_cast<int>(parsed.contains("regex")) +
			static_cast<int>(parsed.contains("regex-file"));
		if (input_source_count == 0) {
			return std::unexpected("provide exactly one of <input.json>, --regex, or --regex-file");
		}
		if (input_source_count > 1) {
			return std::unexpected("input JSON, --regex, and --regex-file are mutually exclusive");
		}

		if (parsed.contains("input")) {
			program_options.input_kind = ProgramInputKind::K_JSON_AUTOMATON;
			program_options.input_path = parsed["input"].as<std::string>();
			if (!has_json_extension(program_options.input_path)) {
				return std::unexpected("input file must use the .json extension");
			}
		} else if (parsed.contains("regex")) {
			program_options.input_kind = ProgramInputKind::K_REGEX_EXPRESSION;
			program_options.regex_text = parsed["regex"].as<std::string>();
		} else {
			program_options.input_kind = ProgramInputKind::K_REGEX_FILE;
			program_options.input_path = parsed["regex-file"].as<std::string>();
		}

		return program_options;
	} catch (const cxxopts::exceptions::exception& error) {
		return std::unexpected(fmt::format("invalid command line: {}", error.what()));
	}
}

auto run(const ProgramOptions& options) -> std::expected<void, std::string> {
	AutomatonInput source_automaton;
	std::string input_regex;

	switch (options.input_kind) {
		case ProgramInputKind::K_JSON_AUTOMATON: {
			std::ifstream input_stream(options.input_path);
			if (!input_stream) {
				return std::unexpected(fmt::format("cannot open input file: {}", options.input_path.string()));
			}

			const auto automaton = read_automaton_json(input_stream);
			if (!automaton) {
				return std::unexpected(automaton.error());
			}
			source_automaton = *automaton;
			break;
		}
		case ProgramInputKind::K_REGEX_EXPRESSION:
			input_regex = trim_copy(options.regex_text);
			break;
		case ProgramInputKind::K_REGEX_FILE: {
			std::ifstream input_stream(options.input_path);
			if (!input_stream) {
				return std::unexpected(fmt::format("cannot open input file: {}", options.input_path.string()));
			}
			input_regex.assign(
				std::istreambuf_iterator<char>(input_stream),
				std::istreambuf_iterator<char>());
			input_regex = trim_copy(std::move(input_regex));
			break;
		}
	}

	if (options.input_kind != ProgramInputKind::K_JSON_AUTOMATON) {
		const auto automaton = build_epsilon_nfa_from_regex(input_regex);
		if (!automaton) {
			return std::unexpected(automaton.error());
		}
		source_automaton = *automaton;
	}

	const auto reduced_nfa = reduce_to_nfa(source_automaton);
	const auto dfa = build_dfa(source_automaton);
	const auto minimized_dfa = minimize_dfa(dfa);
	const auto equivalent_regex = build_regex_from_minimized_dfa(source_automaton, minimized_dfa);

	write_result(
		source_automaton,
		reduced_nfa,
		dfa,
		minimized_dfa,
		equivalent_regex,
		input_regex,
		std::cout);
	return {};
}

}  // namespace automata::nfa_dfa
