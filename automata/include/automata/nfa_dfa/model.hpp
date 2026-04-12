#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace automata::nfa_dfa {

using Mask = std::uint64_t;
inline constexpr int K_MAX_STATE_COUNT = 64;

enum class AutomatonKind : std::uint8_t {
	K_NFA,
	K_EPSILON_NFA,
};

struct Edge {
	Mask from = 0;
	Mask to = 0;
	std::size_t symbol_index = 0;
};

struct AutomatonInput {
	int state_count = 0;
	int start_state = 0;
	AutomatonKind kind = AutomatonKind::K_NFA;
	std::vector<std::string> alphabet;
	Mask accept_states = 0;
	std::vector<Mask> epsilon_transitions;
	std::vector<std::vector<Mask>> transitions;

	[[nodiscard]] auto uses_epsilon() const -> bool {
		return kind == AutomatonKind::K_EPSILON_NFA;
	}

	[[nodiscard]] auto symbol_count() const -> std::size_t {
		return alphabet.size();
	}
};

struct DfaResult {
	Mask start_state = 0;
	std::vector<Mask> reachable_states;
	std::vector<std::vector<Mask>> transition_table;
	std::vector<bool> accepting_states;
	std::vector<Edge> edges;
};

struct IndexedEdge {
	int from = 0;
	int to = 0;
	std::size_t symbol_index = 0;
};

struct MinimizedDfaResult {
	int start_state = 0;
	std::vector<std::vector<Mask>> merged_states;
	std::vector<std::vector<int>> transition_table;
	std::vector<bool> accepting_states;
	std::vector<IndexedEdge> edges;
};

[[nodiscard]] constexpr auto to_size(int value) -> std::size_t {
	return static_cast<std::size_t>(value);
}

[[nodiscard]] constexpr auto state_bit(std::size_t state_index) -> Mask {
	return Mask{1} << state_index;
}

[[nodiscard]] constexpr auto state_bit(int state_index) -> Mask {
	return state_bit(to_size(state_index));
}

// Human-readable labels used by CLI output.
[[nodiscard]] auto kind_name(AutomatonKind kind) -> std::string_view;

// DFA states are represented as bitmasks; these helpers convert them into
// stable textual forms for tables and graph edges.
[[nodiscard]] auto mask_to_binary(Mask mask, int state_count) -> std::string;
[[nodiscard]] auto mask_to_set(Mask mask, int state_count) -> std::string;

}  // namespace automata::nfa_dfa
