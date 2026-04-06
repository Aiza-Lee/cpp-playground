#include "automata/nfa_dfa/model.hpp"

#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace automata::nfa_dfa {

auto kind_name(const AutomatonKind kind) -> std::string_view {
	return kind == AutomatonKind::K_EPSILON_NFA ? "epsilon-NFA" : "NFA";
}

auto mask_to_binary(const Mask mask, const int state_count) -> std::string {
	auto result = std::string(to_size(state_count), '0');
	for (std::size_t state = 0; state < to_size(state_count); ++state) {
		if ((mask & state_bit(state)) != 0) {
			result[result.size() - 1 - state] = '1';
		}
	}
	return result;
}

auto mask_to_set(const Mask mask, const int state_count) -> std::string {
	std::vector<int> states;
	states.reserve(to_size(state_count));
	for (int state = 0; state < state_count; ++state) {
		if ((mask & state_bit(state)) != 0) {
			states.push_back(state);
		}
	}
	return fmt::format("{{{}}}", fmt::join(states, ","));
}

}  // namespace automata::nfa_dfa
