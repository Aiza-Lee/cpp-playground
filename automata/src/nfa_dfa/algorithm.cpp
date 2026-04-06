#include "automata/nfa_dfa/algorithm.hpp"

#include <algorithm>
#include <queue>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace automata::nfa_dfa {

namespace {

// Merge the precomputed single-state epsilon-closures for every bit set in mask.
[[nodiscard]] auto closure_of_mask(const Mask mask, const std::span<const Mask> single_closures) -> Mask {
	Mask result = 0;
	for (std::size_t state = 0; state < single_closures.size(); ++state) {
		if ((mask & state_bit(state)) != 0) {
			result |= single_closures[state];
		}
	}
	return result;
}

// Compute epsilon-closure(state) for every NFA state once. The subset
// construction then reuses these masks instead of repeatedly walking the graph.
[[nodiscard]] auto build_single_closures(const std::span<const Mask> epsilon_transitions)
	-> std::vector<Mask> {
	const auto state_count = epsilon_transitions.size();
	std::vector<Mask> closures(state_count, 0);

	for (std::size_t start_state = 0; start_state < state_count; ++start_state) {
		Mask visited = state_bit(start_state);
		std::queue<std::size_t> pending;
		pending.push(start_state);

		while (!pending.empty()) {
			const auto current_state = pending.front();
			pending.pop();

			const auto epsilon_mask = epsilon_transitions[current_state];
			for (std::size_t next_state = 0; next_state < state_count; ++next_state) {
				if ((epsilon_mask & state_bit(next_state)) == 0 || (visited & state_bit(next_state)) != 0) {
					continue;
				}
				visited |= state_bit(next_state);
				pending.push(next_state);
			}
		}

		closures[start_state] = visited;
	}

	return closures;
}

[[nodiscard]] auto build_effective_single_closures(const AutomatonInput& automaton) -> std::vector<Mask> {
	if (automaton.uses_epsilon()) {
		return build_single_closures(automaton.epsilon_transitions);
	}

	std::vector<Mask> closures(to_size(automaton.state_count), 0);
	for (int state = 0; state < automaton.state_count; ++state) {
		closures[to_size(state)] = state_bit(state);
	}
	return closures;
}

[[nodiscard]] auto move_from_mask(
	const AutomatonInput& automaton,
	const Mask state_mask,
	const std::size_t symbol_index) -> Mask {
	Mask moved = 0;
	for (std::size_t state = 0; state < to_size(automaton.state_count); ++state) {
		if ((state_mask & state_bit(state)) != 0) {
			moved |= automaton.transitions[state][symbol_index];
		}
	}
	return moved;
}

}  // namespace

auto reduce_to_nfa(const AutomatonInput& automaton) -> AutomatonInput {
	const auto single_closures = build_effective_single_closures(automaton);

	AutomatonInput reduced;
	reduced.state_count = automaton.state_count;
	reduced.start_state = automaton.start_state;
	reduced.kind = AutomatonKind::K_NFA;
	reduced.alphabet = automaton.alphabet;
	reduced.epsilon_transitions.assign(to_size(reduced.state_count), 0);
	reduced.transitions.assign(
		to_size(reduced.state_count),
		std::vector<Mask>(reduced.symbol_count(), 0));

	for (int state = 0; state < reduced.state_count; ++state) {
		const auto closure = single_closures[to_size(state)];
		for (std::size_t symbol_index = 0; symbol_index < reduced.symbol_count(); ++symbol_index) {
			const auto moved = move_from_mask(automaton, closure, symbol_index);
			reduced.transitions[to_size(state)][symbol_index] = closure_of_mask(moved, single_closures);
		}
	}

	return reduced;
}

auto build_dfa(const AutomatonInput& automaton) -> DfaResult {
	const auto single_closures = build_effective_single_closures(automaton);

	std::unordered_map<Mask, std::vector<Mask>> transition_rows;
	std::vector<Mask> reachable_states;
	std::vector<Edge> edges;
	std::queue<Mask> pending;
	std::unordered_set<Mask> visited;

	const auto push_state = [&](const Mask state_mask) {
		if (visited.insert(state_mask).second) {
			reachable_states.push_back(state_mask);
			pending.push(state_mask);
		}
	};

	const auto start_state = closure_of_mask(state_bit(automaton.start_state), single_closures);
	push_state(start_state);

	// Standard subset construction: each reachable DFA state is a set of NFA
	// states encoded as a mask. We expand one reachable mask at a time.
	while (!pending.empty()) {
		const auto state_mask = pending.front();
		pending.pop();

		auto row = std::vector<Mask>(automaton.symbol_count(), 0);
		for (std::size_t symbol_index = 0; symbol_index < automaton.symbol_count(); ++symbol_index) {
			const auto moved = move_from_mask(automaton, state_mask, symbol_index);
			const auto next_mask = closure_of_mask(moved, single_closures);
			row[symbol_index] = next_mask;
			edges.push_back({state_mask, next_mask, symbol_index});
			push_state(next_mask);
		}
		transition_rows.emplace(state_mask, std::move(row));
	}

	std::ranges::sort(reachable_states);
	std::ranges::sort(edges, [](const Edge& lhs, const Edge& rhs) {
		if (lhs.from != rhs.from) {
			return lhs.from < rhs.from;
		}
		if (lhs.symbol_index != rhs.symbol_index) {
			return lhs.symbol_index < rhs.symbol_index;
		}
		return lhs.to < rhs.to;
	});

	std::vector<std::vector<Mask>> transition_table;
	transition_table.reserve(reachable_states.size());
	for (const auto state_mask : reachable_states) {
		transition_table.push_back(transition_rows.at(state_mask));
	}

	return {
		.start_state = start_state,
		.reachable_states = std::move(reachable_states),
		.transition_table = std::move(transition_table),
		.edges = std::move(edges),
	};
}

}  // namespace automata::nfa_dfa
