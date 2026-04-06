#include <iostream>
#include <span>
#include <vector>

#include "automata/nfa_dfa/app.hpp"

int main(int argc, char** argv) {
	std::ios::sync_with_stdio(false);
	std::cin.tie(nullptr);

	std::vector<const char*> args(argv, argv + argc);
	const auto options = automata::nfa_dfa::parse_command_line(std::span<const char* const>(args));
	if (!options) {
		std::cerr << "Error: " << options.error() << '\n';
		return 1;
	}

	if (options->show_help) {
		std::cout << options->help_text << '\n';
		return 0;
	}

	const auto result = automata::nfa_dfa::run(*options);
	if (!result) {
		std::cerr << "Error: " << result.error() << '\n';
		return 1;
	}

	return 0;
}
