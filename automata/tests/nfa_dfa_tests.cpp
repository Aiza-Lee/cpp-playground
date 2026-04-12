#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <array>
#include <span>
#include <sstream>
#include <string>
#include <vector>

#include "automata/nfa_dfa/algorithm.hpp"
#include "automata/nfa_dfa/app.hpp"
#include "automata/nfa_dfa/model.hpp"
#include "automata/nfa_dfa/output.hpp"
#include "automata/nfa_dfa/parser.hpp"
#include "automata/nfa_dfa/regex.hpp"

namespace automata::nfa_dfa {
namespace {

auto count_occurrences(const std::string& text, const std::string& needle) -> std::size_t {
	std::size_t count = 0;
	std::size_t pos = 0;
	while ((pos = text.find(needle, pos)) != std::string::npos) {
		++count;
		pos += needle.size();
	}
	return count;
}

}  // namespace
}  // namespace automata::nfa_dfa

TEST_CASE("parser reads plain NFA format") {
	using namespace automata::nfa_dfa;

	std::istringstream input{
		"4 3\n"
		"0 1 2\n"
		"0\n"
		"2 0 1 2 0 2 2 0 2\n"
		"2 0 3 0 1 2\n"
		"0 2 1 3 2 1 2\n"
		"2 2 3 1 3 1 0\n"};

	const auto automaton = read_automaton(input);
	REQUIRE(automaton.has_value());

	CHECK(automaton->state_count == 4);
	CHECK(automaton->start_state == 0);
	CHECK(automaton->kind == AutomatonKind::K_NFA);
	CHECK(automaton->alphabet == std::vector<std::string>{"0", "1", "2"});
	CHECK(automaton->transitions[0][0] == (state_bit(0) | state_bit(1)));
	CHECK(automaton->transitions[1][0] == (state_bit(0) | state_bit(3)));
	CHECK(automaton->transitions[3][2] == state_bit(0));
}

TEST_CASE("parser reports invalid transition target") {
	using namespace automata::nfa_dfa;

	std::istringstream input{
		"2 1\n"
		"a\n"
		"0\n"
		"1 2\n"
		"0\n"};

	const auto automaton = read_automaton(input);
	REQUIRE_FALSE(automaton.has_value());
	CHECK(automaton.error().find("transition target out of range") != std::string::npos);
}

TEST_CASE("parser reads epsilon NFA from json") {
	using namespace automata::nfa_dfa;

	std::istringstream input{R"json(
{
  "state_count": 4,
  "alphabet": ["a", "b"],
  "start_state": 0,
  "accept_states": [3],
  "use_epsilon": true,
  "epsilon_transitions": [[1], [2], [], []],
  "transitions": [
    [[0], []],
    [[], [3]],
    [[], []],
    [[], []]
  ]
}
)json"};

	const auto automaton = read_automaton_json(input);
	REQUIRE(automaton.has_value());

	CHECK(automaton->kind == AutomatonKind::K_EPSILON_NFA);
	CHECK(automaton->alphabet == std::vector<std::string>{"a", "b"});
	CHECK(automaton->accept_states == state_bit(3));
	CHECK(automaton->epsilon_transitions[0] == state_bit(1));
	CHECK(automaton->epsilon_transitions[1] == state_bit(2));
	CHECK(automaton->transitions[1][1] == state_bit(3));
}

TEST_CASE("subset construction handles epsilon closure") {
	using namespace automata::nfa_dfa;

	std::istringstream input{
		"4 2 1\n"
		"a b\n"
		"0\n"
		"1 1\n"
		"1 2\n"
		"0\n"
		"0\n"
		"1 0 0\n"
		"0 1 3\n"
		"0 0\n"
		"0 0\n"};

	const auto automaton = read_automaton(input);
	REQUIRE(automaton.has_value());

	const auto dfa = build_dfa(*automaton);
	CHECK(dfa.start_state == (state_bit(0) | state_bit(1) | state_bit(2)));
	CHECK(dfa.reachable_states == std::vector<Mask>{0, 7, 8});
	REQUIRE(dfa.transition_table.size() == 3);
	CHECK(dfa.transition_table[1] == std::vector<Mask>{7, 8});
}

TEST_CASE("epsilon NFA can be reduced to an equivalent NFA") {
	using namespace automata::nfa_dfa;

	std::istringstream input{R"json(
{
  "state_count": 4,
  "alphabet": ["a", "b"],
  "start_state": 0,
  "accept_states": [3],
  "use_epsilon": true,
  "epsilon_transitions": [[1], [2], [], []],
  "transitions": [
    [[0], []],
    [[], [3]],
    [[], []],
    [[], []]
  ]
}
)json"};

	const auto automaton = read_automaton_json(input);
	REQUIRE(automaton.has_value());

	const auto reduced_nfa = reduce_to_nfa(*automaton);
	CHECK(reduced_nfa.kind == AutomatonKind::K_NFA);
	CHECK(reduced_nfa.accept_states == state_bit(3));
	CHECK(reduced_nfa.epsilon_transitions == std::vector<Mask>(4, 0));
	CHECK(reduced_nfa.transitions[0][0] == (state_bit(0) | state_bit(1) | state_bit(2)));
	CHECK(reduced_nfa.transitions[0][1] == state_bit(3));
	CHECK(reduced_nfa.transitions[1][0] == 0);
	CHECK(reduced_nfa.transitions[1][1] == state_bit(3));
}

TEST_CASE("epsilon reduction lifts accepting states through epsilon closure") {
	using namespace automata::nfa_dfa;

	std::istringstream input{R"json(
{
  "state_count": 4,
  "alphabet": ["a"],
  "start_state": 0,
  "accept_states": [2],
  "use_epsilon": true,
  "epsilon_transitions": [[1], [2], [], []],
  "transitions": [
    [[]],
    [[]],
    [[]],
    [[]]
  ]
}
)json"};

	const auto automaton = read_automaton_json(input);
	REQUIRE(automaton.has_value());

	const auto reduced_nfa = reduce_to_nfa(*automaton);
	CHECK(reduced_nfa.accept_states == (state_bit(0) | state_bit(1) | state_bit(2)));
}

TEST_CASE("output merges labels for identical from/to edges") {
	using namespace automata::nfa_dfa;

	std::istringstream input{
		"4 3\n"
		"0 1 2\n"
		"0\n"
		"2 0 1 2 0 2 2 0 2\n"
		"2 0 3 0 1 2\n"
		"0 2 1 3 2 1 2\n"
		"2 2 3 1 3 1 0\n"};

	const auto automaton = read_automaton(input);
	REQUIRE(automaton.has_value());

	const auto reduced_nfa = reduce_to_nfa(*automaton);
	const auto dfa = build_dfa(*automaton);
	const auto minimized_dfa = minimize_dfa(dfa);
	const auto equivalent_regex = build_regex_from_minimized_dfa(*automaton, minimized_dfa);
	std::ostringstream output;
	write_result(*automaton, reduced_nfa, dfa, minimized_dfa, equivalent_regex, {}, output);

	const auto rendered = output.str();
	CHECK(rendered.find("Equivalent NFA Transition Table") != std::string::npos);
	CHECK(rendered.find("0001\t0101\t1,2") != std::string::npos);
	CHECK(rendered.find("1101\t1111\t0,1") != std::string::npos);
	CHECK(count_occurrences(rendered, "0001\t0101\t") == 1);
	CHECK(rendered.find("Minimized DFA Transition Table") != std::string::npos);
	CHECK(rendered.find("Equivalent Regular Expression:") != std::string::npos);
}

TEST_CASE("dfa minimization merges equivalent reachable states") {
	using namespace automata::nfa_dfa;

	AutomatonInput automaton;
	automaton.state_count = 4;
	automaton.start_state = 0;
	automaton.kind = AutomatonKind::K_NFA;
	automaton.alphabet = {"a", "b"};
	automaton.accept_states = state_bit(3);
	automaton.epsilon_transitions.assign(4, 0);
	automaton.transitions = {
		{state_bit(1), state_bit(2)},
		{state_bit(1), state_bit(3)},
		{state_bit(1), state_bit(3)},
		{state_bit(3), state_bit(3)},
	};

	const auto dfa = build_dfa(automaton);
	REQUIRE(dfa.reachable_states == std::vector<Mask>{1, 2, 4, 8});
	REQUIRE(dfa.accepting_states == std::vector<bool>{false, false, false, true});

	const auto minimized_dfa = minimize_dfa(dfa);
	REQUIRE(minimized_dfa.merged_states.size() == 3);
	CHECK(minimized_dfa.start_state == 0);
	CHECK(minimized_dfa.accepting_states == std::vector<bool>{false, false, true});
	CHECK(minimized_dfa.merged_states[0] == std::vector<Mask>{1});
	CHECK(minimized_dfa.merged_states[1] == std::vector<Mask>{2, 4});
	CHECK(minimized_dfa.merged_states[2] == std::vector<Mask>{8});
	CHECK(minimized_dfa.transition_table[0] == std::vector<int>{1, 1});
	CHECK(minimized_dfa.transition_table[1] == std::vector<int>{1, 2});
	CHECK(minimized_dfa.transition_table[2] == std::vector<int>{2, 2});
}

TEST_CASE("regex Thompson construction builds epsilon NFA") {
	using namespace automata::nfa_dfa;

	const auto automaton = build_epsilon_nfa_from_regex("a(b|c)*");
	REQUIRE(automaton.has_value());

	CHECK(automaton->kind == AutomatonKind::K_EPSILON_NFA);
	CHECK(automaton->alphabet == std::vector<std::string>{"a", "b", "c"});
	CHECK(automaton->state_count == 10);
	CHECK(automaton->start_state == 0);
	CHECK(automaton->accept_states == state_bit(9));
	CHECK(automaton->transitions[0][0] == state_bit(1));
	CHECK(automaton->epsilon_transitions[1] == state_bit(8));
	CHECK(automaton->epsilon_transitions[6] == (state_bit(2) | state_bit(4)));
	CHECK(automaton->epsilon_transitions[8] == (state_bit(6) | state_bit(9)));
}

TEST_CASE("automaton can be converted to a simple regex") {
	using namespace automata::nfa_dfa;

	AutomatonInput automaton;
	automaton.state_count = 2;
	automaton.start_state = 0;
	automaton.kind = AutomatonKind::K_NFA;
	automaton.alphabet = {"a"};
	automaton.accept_states = state_bit(1);
	automaton.epsilon_transitions.assign(2, 0);
	automaton.transitions = {
		{state_bit(1)},
		{0},
	};

	const auto minimized_dfa = minimize_dfa(build_dfa(automaton));
	CHECK(build_regex_from_minimized_dfa(automaton, minimized_dfa) == "a");
}

TEST_CASE("regex simplifier factors common prefixes and removes redundant terms") {
	using namespace automata::nfa_dfa;

	CHECK(simplify_regex("a b|a c").value() == "a(b|c)");
	CHECK(simplify_regex("b a|c a").value() == "(b|c)a");
	CHECK(simplify_regex("a|a*").value() == "a*");
	CHECK(simplify_regex("eps|a*").value() == "a*");
	CHECK(simplify_regex("a**").value() == "a*");
}

TEST_CASE("automaton to regex uses structural simplification on generated concatenations") {
	using namespace automata::nfa_dfa;

	AutomatonInput automaton;
	automaton.state_count = 4;
	automaton.start_state = 0;
	automaton.kind = AutomatonKind::K_NFA;
	automaton.alphabet = {"a", "b", "c"};
	automaton.accept_states = state_bit(3);
	automaton.epsilon_transitions.assign(4, 0);
	automaton.transitions = {
		{state_bit(1), 0, 0},
		{0, state_bit(3), state_bit(3)},
		{0, 0, 0},
		{0, 0, 0},
	};

	const auto minimized_dfa = minimize_dfa(build_dfa(automaton));
	CHECK(build_regex_from_minimized_dfa(automaton, minimized_dfa) == "a(b|c)");
}

TEST_CASE("command line accepts exactly one input source") {
	using namespace automata::nfa_dfa;

	{
		const std::array<const char*, 2> argv = {"nfa_dfa", "automata/nfa_dfa_epsilon.json"};
		const auto options = parse_command_line(std::span<const char* const>(argv));
		REQUIRE(options.has_value());
		CHECK(options->input_kind == ProgramInputKind::K_JSON_AUTOMATON);
		CHECK(options->input_path == "automata/nfa_dfa_epsilon.json");
	}

	{
		const std::array<const char*, 3> argv = {"nfa_dfa", "--regex", "a(b|c)*"};
		const auto options = parse_command_line(std::span<const char* const>(argv));
		REQUIRE(options.has_value());
		CHECK(options->input_kind == ProgramInputKind::K_REGEX_EXPRESSION);
		CHECK(options->regex_text == "a(b|c)*");
	}

	{
		const std::array<const char*, 3> argv = {"nfa_dfa", "--regex-file", "automata/regex_sample.txt"};
		const auto options = parse_command_line(std::span<const char* const>(argv));
		REQUIRE(options.has_value());
		CHECK(options->input_kind == ProgramInputKind::K_REGEX_FILE);
		CHECK(options->input_path == "automata/regex_sample.txt");
	}

	{
		const std::array<const char*, 1> argv = {"nfa_dfa"};
		const auto options = parse_command_line(std::span<const char* const>(argv));
		REQUIRE_FALSE(options.has_value());
		CHECK(options.error().find("provide exactly one") != std::string::npos);
	}

	{
		const std::array<const char*, 2> argv = {"nfa_dfa", "automata/nfa_dfa.in"};
		const auto options = parse_command_line(std::span<const char* const>(argv));
		REQUIRE_FALSE(options.has_value());
		CHECK(options.error().find(".json extension") != std::string::npos);
	}

	{
		const std::array<const char*, 4> argv = {"nfa_dfa", "automata/nfa_dfa_epsilon.json", "--regex", "a"};
		const auto options = parse_command_line(std::span<const char* const>(argv));
		REQUIRE_FALSE(options.has_value());
		CHECK(options.error().find("mutually exclusive") != std::string::npos);
	}
}
