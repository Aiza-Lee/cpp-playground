#include "automata/nfa_dfa/parser.hpp"

#include <cctype>
#include <istream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

namespace automata::nfa_dfa {

namespace {

using Json = nlohmann::json;

[[nodiscard]] auto is_blank_line(const std::string_view line) -> bool {
	return std::ranges::all_of(line, [](const char ch) {
		return std::isspace(static_cast<unsigned char>(ch)) != 0;
	});
}

[[nodiscard]] auto read_non_blank_line(std::istream& input) -> std::expected<std::string, std::string> {
	std::string line;
	while (std::getline(input, line)) {
		if (!is_blank_line(line)) {
			return line;
		}
	}
	return std::unexpected("failed to read header line");
}

// Each unit is stored as: k v1 v2 ... vk. We immediately fold it into a mask,
// which keeps the rest of the algorithm compact and cache-friendly.
[[nodiscard]] auto read_mask_unit(std::istream& input, const int state_count)
	-> std::expected<Mask, std::string> {
	int transition_count = 0;
	if (!(input >> transition_count)) {
		return std::unexpected("failed to read transition count");
	}
	if (transition_count < 0) {
		return std::unexpected("transition count must be non-negative");
	}

	Mask mask = 0;
	for (int index = 0; index < transition_count; ++index) {
		int target_state = 0;
		if (!(input >> target_state)) {
			return std::unexpected("failed to read transition target");
		}
		if (target_state < 0 || target_state >= state_count) {
			return std::unexpected("transition target out of range");
		}
		mask |= state_bit(target_state);
	}
	return mask;
}

[[nodiscard]] auto read_state_mask_from_json(
	const Json& value,
	const int state_count,
	const std::string_view context) -> std::expected<Mask, std::string> {
	if (!value.is_array()) {
		return std::unexpected(fmt::format("{} must be an array of state indices", context));
	}

	Mask mask = 0;
	for (const auto& entry : value) {
		if (!entry.is_number_integer()) {
			return std::unexpected(fmt::format("{} must contain integer state indices", context));
		}

		const auto target_state = entry.get<int>();
		if (target_state < 0 || target_state >= state_count) {
			return std::unexpected(fmt::format("{} contains out-of-range state {}", context, target_state));
		}
		mask |= state_bit(target_state);
	}
	return mask;
}

[[nodiscard]] auto read_use_epsilon_flag(const Json& root) -> std::expected<bool, std::string> {
	if (!root.contains("use_epsilon")) {
		return false;
	}

	const auto& value = root.at("use_epsilon");
	if (value.is_boolean()) {
		return value.get<bool>();
	}
	if (value.is_number_integer()) {
		const auto flag = value.get<int>();
		if (flag == 0) {
			return false;
		}
		if (flag == 1) {
			return true;
		}
	}

	return std::unexpected("use_epsilon must be a boolean or 0/1 integer");
}

[[nodiscard]] auto read_accept_states(const Json& root, const int state_count)
	-> std::expected<Mask, std::string> {
	if (!root.contains("accept_states")) {
		return Mask{0};
	}

	return read_state_mask_from_json(root.at("accept_states"), state_count, "accept_states");
}

}  // namespace

auto read_automaton(std::istream& input) -> std::expected<AutomatonInput, std::string> {
	const auto header_line = read_non_blank_line(input);
	if (!header_line) {
		return std::unexpected(header_line.error());
	}

	int state_count = 0;
	int symbol_count = 0;
	int use_epsilon = 0;
	std::stringstream header_stream(*header_line);
	if (!(header_stream >> state_count >> symbol_count)) {
		return std::unexpected("header must contain n and m");
	}
	if (!(header_stream >> use_epsilon)) {
		use_epsilon = 0;
	}

	if (state_count <= 0 || state_count > K_MAX_STATE_COUNT) {
		return std::unexpected(fmt::format("n must satisfy 1 <= n <= {}", K_MAX_STATE_COUNT));
	}
	if (symbol_count < 0) {
		return std::unexpected("m must be non-negative");
	}
	if (use_epsilon != 0 && use_epsilon != 1) {
		return std::unexpected("use_epsilon must be 0 or 1");
	}

	AutomatonInput automaton;
	automaton.state_count = state_count;
	automaton.kind = use_epsilon == 1 ? AutomatonKind::K_EPSILON_NFA : AutomatonKind::K_NFA;
	automaton.alphabet.resize(to_size(symbol_count));

	for (auto& symbol : automaton.alphabet) {
		if (!(input >> symbol)) {
			return std::unexpected("failed to read alphabet symbols");
		}
	}

	if (!(input >> automaton.start_state)) {
		return std::unexpected("failed to read start state");
	}
	if (automaton.start_state < 0 || automaton.start_state >= automaton.state_count) {
		return std::unexpected("start state out of range");
	}

	automaton.epsilon_transitions.assign(to_size(state_count), 0);
	if (automaton.uses_epsilon()) {
		for (int state = 0; state < state_count; ++state) {
			const auto epsilon_targets = read_mask_unit(input, state_count);
			if (!epsilon_targets) {
				return std::unexpected(fmt::format("epsilon transitions: {}", epsilon_targets.error()));
			}
			automaton.epsilon_transitions[to_size(state)] = *epsilon_targets;
		}
	}

	automaton.transitions.assign(to_size(state_count), std::vector<Mask>(to_size(symbol_count), 0));
	for (int state = 0; state < state_count; ++state) {
		for (int symbol = 0; symbol < symbol_count; ++symbol) {
			const auto targets = read_mask_unit(input, state_count);
			if (!targets) {
				return std::unexpected(fmt::format(
					"state {}, symbol {}: {}",
					state,
					symbol,
					targets.error()));
			}
			automaton.transitions[to_size(state)][to_size(symbol)] = *targets;
		}
	}

	return automaton;
}

auto read_automaton_json(std::istream& input) -> std::expected<AutomatonInput, std::string> {
	try {
		const auto root = Json::parse(input);
		if (!root.is_object()) {
			return std::unexpected("JSON root must be an object");
		}

		if (!root.contains("state_count") || !root.at("state_count").is_number_integer()) {
			return std::unexpected("state_count must be an integer");
		}
		if (!root.contains("start_state") || !root.at("start_state").is_number_integer()) {
			return std::unexpected("start_state must be an integer");
		}
		if (!root.contains("alphabet") || !root.at("alphabet").is_array()) {
			return std::unexpected("alphabet must be an array of strings");
		}
		if (!root.contains("transitions") || !root.at("transitions").is_array()) {
			return std::unexpected("transitions must be a 2D array");
		}

		AutomatonInput automaton;
		automaton.state_count = root.at("state_count").get<int>();
		automaton.start_state = root.at("start_state").get<int>();

		if (automaton.state_count <= 0 || automaton.state_count > K_MAX_STATE_COUNT) {
			return std::unexpected(fmt::format("state_count must satisfy 1 <= n <= {}", K_MAX_STATE_COUNT));
		}
		if (automaton.start_state < 0 || automaton.start_state >= automaton.state_count) {
			return std::unexpected("start_state out of range");
		}

		const auto use_epsilon = read_use_epsilon_flag(root);
		if (!use_epsilon) {
			return std::unexpected(use_epsilon.error());
		}
		automaton.kind = *use_epsilon ? AutomatonKind::K_EPSILON_NFA : AutomatonKind::K_NFA;

		const auto accept_states = read_accept_states(root, automaton.state_count);
		if (!accept_states) {
			return std::unexpected(accept_states.error());
		}
		automaton.accept_states = *accept_states;

		for (const auto& symbol : root.at("alphabet")) {
			if (!symbol.is_string()) {
				return std::unexpected("alphabet must contain only strings");
			}
			automaton.alphabet.push_back(symbol.get<std::string>());
		}

		const auto symbol_count = automaton.symbol_count();
		const auto& transitions = root.at("transitions");
		if (transitions.size() != to_size(automaton.state_count)) {
			return std::unexpected("transitions row count must equal state_count");
		}

		automaton.epsilon_transitions.assign(to_size(automaton.state_count), 0);
		if (automaton.uses_epsilon()) {
			if (!root.contains("epsilon_transitions") || !root.at("epsilon_transitions").is_array()) {
				return std::unexpected("epsilon_transitions must be provided as an array when use_epsilon is true");
			}

			const auto& epsilon_transitions = root.at("epsilon_transitions");
			if (epsilon_transitions.size() != to_size(automaton.state_count)) {
				return std::unexpected("epsilon_transitions row count must equal state_count");
			}

			for (int state = 0; state < automaton.state_count; ++state) {
				const auto epsilon_targets = read_state_mask_from_json(
					epsilon_transitions.at(to_size(state)),
					automaton.state_count,
					fmt::format("epsilon_transitions[{}]", state));
				if (!epsilon_targets) {
					return std::unexpected(epsilon_targets.error());
				}
				automaton.epsilon_transitions[to_size(state)] = *epsilon_targets;
			}
		}

		automaton.transitions.assign(
			to_size(automaton.state_count),
			std::vector<Mask>(symbol_count, 0));

		for (int state = 0; state < automaton.state_count; ++state) {
			const auto& row = transitions.at(to_size(state));
			if (!row.is_array()) {
				return std::unexpected(fmt::format("transitions[{}] must be an array", state));
			}
			if (row.size() != symbol_count) {
				return std::unexpected(fmt::format(
					"transitions[{}] must contain exactly {} symbol entries",
					state,
					symbol_count));
			}

			for (std::size_t symbol = 0; symbol < symbol_count; ++symbol) {
				const auto targets = read_state_mask_from_json(
					row.at(symbol),
					automaton.state_count,
					fmt::format("transitions[{}][{}]", state, symbol));
				if (!targets) {
					return std::unexpected(targets.error());
				}
				automaton.transitions[to_size(state)][symbol] = *targets;
			}
		}

		return automaton;
	} catch (const Json::parse_error& error) {
		return std::unexpected(fmt::format("invalid JSON: {}", error.what()));
	} catch (const Json::exception& error) {
		return std::unexpected(fmt::format("invalid JSON automaton: {}", error.what()));
	}
}

}  // namespace automata::nfa_dfa
