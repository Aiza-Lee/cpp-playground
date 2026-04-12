#include "automata/nfa_dfa/algorithm.hpp"

#include <algorithm>
#include <map>
#include <queue>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <utility>
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
	reduced.accept_states = automaton.accept_states;
	reduced.epsilon_transitions.assign(to_size(reduced.state_count), 0);
	reduced.transitions.assign(
		to_size(reduced.state_count),
		std::vector<Mask>(reduced.symbol_count(), 0));

	for (int state = 0; state < reduced.state_count; ++state) {
		const auto closure = single_closures[to_size(state)];
		if ((closure & automaton.accept_states) != 0) {
			reduced.accept_states |= state_bit(state);
		}
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
	std::vector<bool> accepting_states;
	accepting_states.reserve(reachable_states.size());
	for (const auto state_mask : reachable_states) {
		transition_table.push_back(transition_rows.at(state_mask));
		accepting_states.push_back((state_mask & automaton.accept_states) != 0);
	}

	return {
		.start_state = start_state,
		.reachable_states = std::move(reachable_states),
		.transition_table = std::move(transition_table),
		.accepting_states = std::move(accepting_states),
		.edges = std::move(edges),
	};
}

auto minimize_dfa(const DfaResult& dfa) -> MinimizedDfaResult {
	if (dfa.reachable_states.empty()) {
		return {};
	}

	std::unordered_map<Mask, int> state_to_index;
	state_to_index.reserve(dfa.reachable_states.size());
	for (std::size_t index = 0; index < dfa.reachable_states.size(); ++index) {
		state_to_index.emplace(dfa.reachable_states[index], static_cast<int>(index));
	}

	std::vector<std::vector<int>> partitions;
	std::vector<int> non_accepting_states;
	std::vector<int> accepting_states;
	for (std::size_t index = 0; index < dfa.reachable_states.size(); ++index) {
		if (dfa.accepting_states[index]) {
			accepting_states.push_back(static_cast<int>(index));
		} else {
			non_accepting_states.push_back(static_cast<int>(index));
		}
	}

	if (!non_accepting_states.empty()) {
		partitions.push_back(non_accepting_states);
	}
	if (!accepting_states.empty()) {
		partitions.push_back(accepting_states);
	}

	while (true) {
		std::vector<int> state_to_partition(dfa.reachable_states.size(), -1);
		for (std::size_t partition_index = 0; partition_index < partitions.size(); ++partition_index) {
			for (const auto state_index : partitions[partition_index]) {
				state_to_partition[to_size(state_index)] = static_cast<int>(partition_index);
			}
		}

		std::vector<std::vector<int>> next_partitions;
		bool changed = false;
		for (const auto& partition : partitions) {
			std::map<std::vector<int>, std::vector<int>> grouped_states;
			for (const auto state_index : partition) {
				std::vector<int> signature;
				signature.reserve(dfa.transition_table[to_size(state_index)].size());
				for (const auto next_mask : dfa.transition_table[to_size(state_index)]) {
					signature.push_back(state_to_partition.at(to_size(state_to_index.at(next_mask))));
				}
				grouped_states[std::move(signature)].push_back(state_index);
			}

			changed = changed || grouped_states.size() > 1;
			for (auto& [signature, group] : grouped_states) {
				(void)signature;
				next_partitions.push_back(std::move(group));
			}
		}

		if (!changed) {
			partitions = std::move(next_partitions);
			break;
		}

		for (auto& partition : next_partitions) {
			std::ranges::sort(partition, [&](const int lhs, const int rhs) {
				return dfa.reachable_states[to_size(lhs)] < dfa.reachable_states[to_size(rhs)];
			});
		}
		std::ranges::sort(next_partitions, [&](const auto& lhs, const auto& rhs) {
			return dfa.reachable_states[to_size(lhs.front())] < dfa.reachable_states[to_size(rhs.front())];
		});
		partitions = std::move(next_partitions);
	}

	const auto start_state_index = state_to_index.at(dfa.start_state);
	std::ranges::sort(partitions, [&](const auto& lhs, const auto& rhs) {
		const auto lhs_is_start = std::ranges::find(lhs, start_state_index) != lhs.end();
		const auto rhs_is_start = std::ranges::find(rhs, start_state_index) != rhs.end();
		if (lhs_is_start != rhs_is_start) {
			return lhs_is_start;
		}
		return dfa.reachable_states[to_size(lhs.front())] < dfa.reachable_states[to_size(rhs.front())];
	});

	std::vector<int> state_to_partition(dfa.reachable_states.size(), -1);
	for (std::size_t partition_index = 0; partition_index < partitions.size(); ++partition_index) {
		for (const auto state_index : partitions[partition_index]) {
			state_to_partition[to_size(state_index)] = static_cast<int>(partition_index);
		}
	}

	MinimizedDfaResult minimized;
	minimized.merged_states.reserve(partitions.size());
	minimized.transition_table.reserve(partitions.size());
	minimized.accepting_states.reserve(partitions.size());
	minimized.edges.reserve(partitions.size() * (dfa.transition_table.empty() ? 0 : dfa.transition_table.front().size()));

	for (std::size_t partition_index = 0; partition_index < partitions.size(); ++partition_index) {
		const auto representative = partitions[partition_index].front();

		std::vector<Mask> merged_states;
		merged_states.reserve(partitions[partition_index].size());
		for (const auto state_index : partitions[partition_index]) {
			merged_states.push_back(dfa.reachable_states[to_size(state_index)]);
		}
		minimized.merged_states.push_back(std::move(merged_states));
		minimized.accepting_states.push_back(dfa.accepting_states[to_size(representative)]);

		std::vector<int> row;
		row.reserve(dfa.transition_table[to_size(representative)].size());
		for (std::size_t symbol_index = 0; symbol_index < dfa.transition_table[to_size(representative)].size(); ++symbol_index) {
			const auto next_mask = dfa.transition_table[to_size(representative)][symbol_index];
			const auto next_state_index = state_to_index.at(next_mask);
			const auto next_partition = state_to_partition.at(to_size(next_state_index));
			row.push_back(next_partition);
			minimized.edges.push_back({
				.from = static_cast<int>(partition_index),
				.to = next_partition,
				.symbol_index = symbol_index,
			});
		}
		minimized.transition_table.push_back(std::move(row));
	}

	minimized.start_state = state_to_partition.at(to_size(state_to_index.at(dfa.start_state)));
	std::ranges::sort(minimized.edges, [](const IndexedEdge& lhs, const IndexedEdge& rhs) {
		if (lhs.from != rhs.from) {
			return lhs.from < rhs.from;
		}
		if (lhs.symbol_index != rhs.symbol_index) {
			return lhs.symbol_index < rhs.symbol_index;
		}
		return lhs.to < rhs.to;
	});

	return minimized;
}

}  // namespace automata::nfa_dfa
