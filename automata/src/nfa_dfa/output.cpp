#include "automata/nfa_dfa/output.hpp"

#include <algorithm>
#include <map>
#include <ostream>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace automata::nfa_dfa {

namespace {

[[nodiscard]] auto group_edge_labels(const AutomatonInput& automaton, const DfaResult& dfa)
	-> std::map<std::pair<Mask, Mask>, std::vector<std::string_view>> {
	std::map<std::pair<Mask, Mask>, std::vector<std::string_view>> grouped_edges;
	for (const auto& edge : dfa.edges) {
		grouped_edges[{edge.from, edge.to}].push_back(automaton.alphabet[edge.symbol_index]);
	}
	return grouped_edges;
}

[[nodiscard]] auto group_edge_labels(const AutomatonInput& automaton, const MinimizedDfaResult& dfa)
	-> std::map<std::pair<int, int>, std::vector<std::string_view>> {
	std::map<std::pair<int, int>, std::vector<std::string_view>> grouped_edges;
	for (const auto& edge : dfa.edges) {
		grouped_edges[{edge.from, edge.to}].push_back(automaton.alphabet[edge.symbol_index]);
	}
	return grouped_edges;
}

[[nodiscard]] auto merged_state_label(const std::vector<Mask>& states, const int source_state_count) -> std::string {
	std::vector<std::string> labels;
	labels.reserve(states.size());
	for (const auto state_mask : states) {
		labels.push_back(fmt::format(
			"{} {}",
			mask_to_binary(state_mask, source_state_count),
			mask_to_set(state_mask, source_state_count)));
	}
	return fmt::format("[{}]", fmt::join(labels, ", "));
}

void write_nfa_transition_table(const AutomatonInput& nfa, std::ostream& output) {
	output << "Equivalent NFA Transition Table\n";
	output << "State";
	for (const auto& symbol : nfa.alphabet) {
		output << '\t' << symbol;
	}
	output << '\n';

	for (int state = 0; state < nfa.state_count; ++state) {
		output << state;
		for (const auto next_mask : nfa.transitions[to_size(state)]) {
			output << '\t' << mask_to_set(next_mask, nfa.state_count);
		}
		output << '\n';
	}
}

}  // namespace

void write_result(
	const AutomatonInput& source_automaton,
	const AutomatonInput& reduced_nfa,
	const DfaResult& dfa,
	const MinimizedDfaResult& minimized_dfa,
	const std::string_view equivalent_regex,
	const std::string_view input_regex,
	std::ostream& output) {
	if (!input_regex.empty()) {
		output << fmt::format("Input Regex: {}\n", input_regex);
	}
	output << fmt::format("Input Mode: {}\n", kind_name(source_automaton.kind));
	output << fmt::format("Equivalent NFA Start State: {}\n", reduced_nfa.start_state);
	output << fmt::format(
		"Equivalent NFA Accepting States: {}\n",
		mask_to_set(reduced_nfa.accept_states, reduced_nfa.state_count));
	write_nfa_transition_table(reduced_nfa, output);
	output << '\n';

	output << fmt::format(
		"Start DFA State: {} {}\n",
		mask_to_binary(dfa.start_state, source_automaton.state_count),
		mask_to_set(dfa.start_state, source_automaton.state_count));
	output << fmt::format("Reachable DFA states: {}\n", dfa.reachable_states.size());
	for (const auto state_mask : dfa.reachable_states) {
		output << fmt::format(
			"{} {}\n",
			mask_to_binary(state_mask, source_automaton.state_count),
			mask_to_set(state_mask, source_automaton.state_count));
	}
	output << fmt::format("Accepting DFA states: {}\n", std::count(dfa.accepting_states.begin(), dfa.accepting_states.end(), true));
	for (std::size_t state_index = 0; state_index < dfa.reachable_states.size(); ++state_index) {
		if (!dfa.accepting_states[state_index]) {
			continue;
		}
		output << fmt::format(
			"{} {}\n",
			mask_to_binary(dfa.reachable_states[state_index], source_automaton.state_count),
			mask_to_set(dfa.reachable_states[state_index], source_automaton.state_count));
	}
	output << '\n';

	output << "DFA Transition Table\n";
	output << "State";
	for (const auto& symbol : source_automaton.alphabet) {
		output << '\t' << symbol;
	}
	output << '\n';

	for (std::size_t row_index = 0; row_index < dfa.reachable_states.size(); ++row_index) {
		output << mask_to_binary(dfa.reachable_states[row_index], source_automaton.state_count);
		for (const auto next_mask : dfa.transition_table[row_index]) {
			output << '\t' << mask_to_binary(next_mask, source_automaton.state_count);
		}
		output << '\n';
	}
	output << '\n';

	output << "DFA Graph Edges\n";
	output << "from\tto\tlabel\n";
	for (const auto& [endpoints, labels] : group_edge_labels(source_automaton, dfa)) {
		const auto& [from, to] = endpoints;
		output << fmt::format(
			"{}\t{}\t{}\n",
			mask_to_binary(from, source_automaton.state_count),
			mask_to_binary(to, source_automaton.state_count),
			fmt::join(labels, ","));
	}
	output << '\n';

	output << fmt::format("Minimized DFA Start State: q{}\n", minimized_dfa.start_state);
	output << fmt::format("Minimized DFA states: {}\n", minimized_dfa.merged_states.size());
	for (std::size_t state_index = 0; state_index < minimized_dfa.merged_states.size(); ++state_index) {
		output << fmt::format(
			"q{}{} = {}\n",
			state_index,
			minimized_dfa.accepting_states[state_index] ? " [accept]" : "",
			merged_state_label(minimized_dfa.merged_states[state_index], source_automaton.state_count));
	}
	output << '\n';

	output << "Minimized DFA Transition Table\n";
	output << "State";
	for (const auto& symbol : source_automaton.alphabet) {
		output << '\t' << symbol;
	}
	output << '\n';

	for (std::size_t row_index = 0; row_index < minimized_dfa.transition_table.size(); ++row_index) {
		output << 'q' << row_index;
		for (const auto next_state : minimized_dfa.transition_table[row_index]) {
			output << "\tq" << next_state;
		}
		output << '\n';
	}
	output << '\n';

	output << "Minimized DFA Graph Edges\n";
	output << "from\tto\tlabel\n";
	for (const auto& [endpoints, labels] : group_edge_labels(source_automaton, minimized_dfa)) {
		const auto& [from, to] = endpoints;
		output << fmt::format("q{}\tq{}\t{}\n", from, to, fmt::join(labels, ","));
	}
	output << '\n';
	output << fmt::format("Equivalent Regular Expression: {}\n", equivalent_regex);
}

}  // namespace automata::nfa_dfa
