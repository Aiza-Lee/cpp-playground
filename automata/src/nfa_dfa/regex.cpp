#include "automata/nfa_dfa/regex.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace automata::nfa_dfa {

namespace {

enum class RegexTokenKind {
	K_LITERAL,
	K_EPSILON,
	K_EMPTY,
	K_UNION,
	K_CONCAT,
	K_STAR,
	K_LPAREN,
	K_RPAREN,
};

struct RegexToken {
	RegexTokenKind kind = RegexTokenKind::K_LITERAL;
	std::string text;
};

struct RegexFragment {
	int start = 0;
	int accept = 0;
};

struct RegexAst;
using RegexAstPtr = std::shared_ptr<RegexAst>;

enum class RegexValueKind {
	K_EMPTY,
	K_EPSILON,
	K_NORMAL,
};

struct RegexValue {
	RegexValueKind kind = RegexValueKind::K_EMPTY;
	std::string text;
	int precedence = 4;
	RegexAstPtr ast;
};

enum class RegexAstKind {
	K_LITERAL,
	K_EPSILON,
	K_EMPTY,
	K_UNION,
	K_CONCAT,
	K_STAR,
};

struct RegexAst {
	RegexAstKind kind = RegexAstKind::K_EMPTY;
	std::string literal;
	std::vector<RegexAstPtr> children;
};

[[nodiscard]] auto make_ast(RegexAstKind kind) -> RegexAstPtr;
[[nodiscard]] auto make_literal_ast(std::string literal) -> RegexAstPtr;
[[nodiscard]] auto make_composite_ast(RegexAstKind kind, std::vector<RegexAstPtr> children) -> RegexAstPtr;
[[nodiscard]] auto render_regex_ast(const RegexAstPtr& ast, int parent_precedence = 0) -> std::string;
[[nodiscard]] auto simplify_regex_ast(const RegexAstPtr& ast) -> RegexAstPtr;

[[nodiscard]] auto token_can_end_concat(const RegexTokenKind kind) -> bool {
	return kind == RegexTokenKind::K_LITERAL || kind == RegexTokenKind::K_EPSILON ||
		kind == RegexTokenKind::K_EMPTY || kind == RegexTokenKind::K_RPAREN ||
		kind == RegexTokenKind::K_STAR;
}

[[nodiscard]] auto token_can_begin_concat(const RegexTokenKind kind) -> bool {
	return kind == RegexTokenKind::K_LITERAL || kind == RegexTokenKind::K_EPSILON ||
		kind == RegexTokenKind::K_EMPTY || kind == RegexTokenKind::K_LPAREN;
}

[[nodiscard]] auto precedence_of(const RegexTokenKind kind) -> int {
	switch (kind) {
		case RegexTokenKind::K_UNION:
			return 1;
		case RegexTokenKind::K_CONCAT:
			return 2;
		case RegexTokenKind::K_STAR:
			return 3;
		default:
			return 0;
	}
}

[[nodiscard]] auto is_identifier_symbol(const std::string_view symbol) -> bool {
	if (symbol.empty() || symbol == "eps" || symbol == "empty") {
		return false;
	}
	return std::ranges::all_of(symbol, [](const unsigned char ch) {
		return std::isalnum(ch) != 0 || ch == '_';
	});
}

[[nodiscard]] auto quote_symbol(const std::string_view symbol) -> std::string {
	std::string quoted = "'";
	for (const char ch : symbol) {
		if (ch == '\\' || ch == '\'') {
			quoted.push_back('\\');
		}
		quoted.push_back(ch);
	}
	quoted.push_back('\'');
	return quoted;
}

[[nodiscard]] auto format_symbol_regex(const std::string_view symbol) -> std::string {
	if (is_identifier_symbol(symbol)) {
		return std::string(symbol);
	}
	return quote_symbol(symbol);
}

[[nodiscard]] auto regex_empty() -> RegexValue {
	return {
		.kind = RegexValueKind::K_EMPTY,
		.text = "empty",
		.precedence = 4,
		.ast = make_ast(RegexAstKind::K_EMPTY),
	};
}

[[nodiscard]] auto regex_epsilon() -> RegexValue {
	return {
		.kind = RegexValueKind::K_EPSILON,
		.text = "eps",
		.precedence = 4,
		.ast = make_ast(RegexAstKind::K_EPSILON),
	};
}

[[nodiscard]] auto regex_literal(const std::string_view symbol) -> RegexValue {
	return {
		.kind = RegexValueKind::K_NORMAL,
		.text = format_symbol_regex(symbol),
		.precedence = 4,
		.ast = make_literal_ast(std::string(symbol)),
	};
}

[[nodiscard]] auto maybe_parenthesize(const RegexValue& value, const int needed_precedence) -> std::string {
	if (value.precedence < needed_precedence) {
		return fmt::format("({})", value.text);
	}
	return value.text;
}

[[nodiscard]] auto regex_union(RegexValue lhs, RegexValue rhs) -> RegexValue {
	if (lhs.kind == RegexValueKind::K_EMPTY) {
		return rhs;
	}
	if (rhs.kind == RegexValueKind::K_EMPTY) {
		return lhs;
	}
	if (lhs.text == rhs.text) {
		return lhs;
	}
	if (rhs.text < lhs.text) {
		std::swap(lhs, rhs);
	}
	return {
		.kind = RegexValueKind::K_NORMAL,
		.text = fmt::format("{}|{}", maybe_parenthesize(lhs, 1), maybe_parenthesize(rhs, 1)),
		.precedence = 1,
		.ast = make_composite_ast(RegexAstKind::K_UNION, {lhs.ast, rhs.ast}),
	};
}

[[nodiscard]] auto regex_concat(const RegexValue& lhs, const RegexValue& rhs) -> RegexValue {
	if (lhs.kind == RegexValueKind::K_EMPTY || rhs.kind == RegexValueKind::K_EMPTY) {
		return regex_empty();
	}
	if (lhs.kind == RegexValueKind::K_EPSILON) {
		return rhs;
	}
	if (rhs.kind == RegexValueKind::K_EPSILON) {
		return lhs;
	}
	return {
		.kind = RegexValueKind::K_NORMAL,
		.text = fmt::format("{}{}", maybe_parenthesize(lhs, 2), maybe_parenthesize(rhs, 2)),
		.precedence = 2,
		.ast = make_composite_ast(RegexAstKind::K_CONCAT, {lhs.ast, rhs.ast}),
	};
}

[[nodiscard]] auto regex_star(const RegexValue& value) -> RegexValue {
	if (value.kind == RegexValueKind::K_EMPTY || value.kind == RegexValueKind::K_EPSILON) {
		return regex_epsilon();
	}
	if (value.precedence == 3 && !value.text.empty() && value.text.back() == '*') {
		return value;
	}
	return {
		.kind = RegexValueKind::K_NORMAL,
		.text = fmt::format("{}*", maybe_parenthesize(value, 3)),
		.precedence = 3,
		.ast = make_composite_ast(RegexAstKind::K_STAR, {value.ast}),
	};
}

[[nodiscard]] auto read_quoted_symbol(const std::string_view regex_text, std::size_t& pos)
	-> std::expected<std::string, std::string> {
	++pos;
	std::string value;
	while (pos < regex_text.size()) {
		const auto ch = regex_text[pos++];
		if (ch == '\\') {
			if (pos >= regex_text.size()) {
				return std::unexpected("dangling escape inside quoted symbol");
			}
			value.push_back(regex_text[pos++]);
			continue;
		}
		if (ch == '\'') {
			return value;
		}
		value.push_back(ch);
	}
	return std::unexpected("unterminated quoted symbol");
}

[[nodiscard]] auto tokenize_regex(const std::string_view regex_text)
	-> std::expected<std::vector<RegexToken>, std::string> {
	std::vector<RegexToken> tokens;

	const auto push_token = [&](RegexToken token) {
		if (!tokens.empty() && token_can_end_concat(tokens.back().kind) &&
			token_can_begin_concat(token.kind)) {
			tokens.push_back({.kind = RegexTokenKind::K_CONCAT, .text = {}});
		}
		tokens.push_back(std::move(token));
	};

	for (std::size_t pos = 0; pos < regex_text.size();) {
		const auto ch = static_cast<unsigned char>(regex_text[pos]);
		if (std::isspace(ch) != 0) {
			++pos;
			continue;
		}

		if (std::isalnum(ch) != 0 || ch == '_') {
			const auto start = pos;
			while (pos < regex_text.size()) {
				const auto current = static_cast<unsigned char>(regex_text[pos]);
				if (std::isalnum(current) == 0 && current != '_') {
					break;
				}
				++pos;
			}

			auto symbol = std::string(regex_text.substr(start, pos - start));
			if (symbol == "eps") {
				push_token({.kind = RegexTokenKind::K_EPSILON, .text = {}});
			} else if (symbol == "empty") {
				push_token({.kind = RegexTokenKind::K_EMPTY, .text = {}});
			} else {
				push_token({.kind = RegexTokenKind::K_LITERAL, .text = std::move(symbol)});
			}
			continue;
		}

		if (regex_text[pos] == '\'') {
			auto symbol = read_quoted_symbol(regex_text, pos);
			if (!symbol) {
				return std::unexpected(symbol.error());
			}
			push_token({.kind = RegexTokenKind::K_LITERAL, .text = std::move(*symbol)});
			continue;
		}

		if (regex_text[pos] == '\\') {
			++pos;
			if (pos >= regex_text.size()) {
				return std::unexpected("dangling escape at end of regex");
			}
			push_token({.kind = RegexTokenKind::K_LITERAL, .text = std::string(1, regex_text[pos++])});
			continue;
		}

		switch (regex_text[pos]) {
			case '|':
				tokens.push_back({.kind = RegexTokenKind::K_UNION, .text = {}});
				++pos;
				break;
			case '*':
				tokens.push_back({.kind = RegexTokenKind::K_STAR, .text = {}});
				++pos;
				break;
			case '(':
				push_token({.kind = RegexTokenKind::K_LPAREN, .text = {}});
				++pos;
				break;
			case ')':
				tokens.push_back({.kind = RegexTokenKind::K_RPAREN, .text = {}});
				++pos;
				break;
			default:
				return std::unexpected(fmt::format("unsupported regex character '{}'", regex_text[pos]));
		}
	}

	if (tokens.empty()) {
		return std::unexpected("regex must not be empty");
	}
	return tokens;
}

[[nodiscard]] auto to_postfix(const std::vector<RegexToken>& tokens)
	-> std::expected<std::vector<RegexToken>, std::string> {
	std::vector<RegexToken> output;
	std::vector<RegexToken> operators;
	bool expect_operand = true;

	for (const auto& token : tokens) {
		switch (token.kind) {
			case RegexTokenKind::K_LITERAL:
			case RegexTokenKind::K_EPSILON:
			case RegexTokenKind::K_EMPTY:
				if (!expect_operand) {
					return std::unexpected("missing operator between regex terms");
				}
				output.push_back(token);
				expect_operand = false;
				break;
			case RegexTokenKind::K_LPAREN:
				if (!expect_operand) {
					return std::unexpected("missing operator before '('");
				}
				operators.push_back(token);
				break;
			case RegexTokenKind::K_RPAREN:
				if (expect_operand) {
					return std::unexpected("unexpected ')'");
				}
				while (!operators.empty() && operators.back().kind != RegexTokenKind::K_LPAREN) {
					output.push_back(operators.back());
					operators.pop_back();
				}
				if (operators.empty()) {
					return std::unexpected("unmatched ')'");
				}
				operators.pop_back();
				expect_operand = false;
				break;
			case RegexTokenKind::K_STAR:
				if (expect_operand) {
					return std::unexpected("'*' must follow a regex term");
				}
				output.push_back(token);
				expect_operand = false;
				break;
			case RegexTokenKind::K_UNION:
			case RegexTokenKind::K_CONCAT:
				if (expect_operand) {
					return std::unexpected("binary operator is missing its left operand");
				}
				while (!operators.empty() && operators.back().kind != RegexTokenKind::K_LPAREN &&
					precedence_of(operators.back().kind) >= precedence_of(token.kind)) {
					output.push_back(operators.back());
					operators.pop_back();
				}
				operators.push_back(token);
				expect_operand = true;
				break;
		}
	}

	if (expect_operand) {
		return std::unexpected("regex cannot end with an operator");
	}

	while (!operators.empty()) {
		if (operators.back().kind == RegexTokenKind::K_LPAREN) {
			return std::unexpected("unmatched '('");
		}
		output.push_back(operators.back());
		operators.pop_back();
	}
	return output;
}

[[nodiscard]] auto create_state(std::vector<Mask>& epsilon_transitions) -> std::expected<int, std::string> {
	if (epsilon_transitions.size() >= to_size(K_MAX_STATE_COUNT)) {
		return std::unexpected(fmt::format("regex-generated automaton exceeds {} states", K_MAX_STATE_COUNT));
	}
	epsilon_transitions.push_back(0);
	return static_cast<int>(epsilon_transitions.size() - 1);
}

[[nodiscard]] auto make_ast(RegexAstKind kind) -> RegexAstPtr {
	return std::make_shared<RegexAst>(RegexAst{
		.kind = kind,
		.literal = {},
		.children = {},
	});
}

[[nodiscard]] auto make_literal_ast(std::string literal) -> RegexAstPtr {
	return std::make_shared<RegexAst>(RegexAst{
		.kind = RegexAstKind::K_LITERAL,
		.literal = std::move(literal),
		.children = {},
	});
}

[[nodiscard]] auto make_composite_ast(RegexAstKind kind, std::vector<RegexAstPtr> children) -> RegexAstPtr {
	return std::make_shared<RegexAst>(RegexAst{
		.kind = kind,
		.literal = {},
		.children = std::move(children),
	});
}

[[nodiscard]] auto ast_precedence(const RegexAst& ast) -> int {
	switch (ast.kind) {
		case RegexAstKind::K_UNION:
			return 1;
		case RegexAstKind::K_CONCAT:
			return 2;
		case RegexAstKind::K_STAR:
			return 3;
		default:
			return 4;
	}
}

[[nodiscard]] auto render_regex_ast(const RegexAstPtr& ast, const int parent_precedence) -> std::string {
	std::string rendered;
	switch (ast->kind) {
		case RegexAstKind::K_LITERAL:
			rendered = format_symbol_regex(ast->literal);
			break;
		case RegexAstKind::K_EPSILON:
			rendered = "eps";
			break;
		case RegexAstKind::K_EMPTY:
			rendered = "empty";
			break;
		case RegexAstKind::K_STAR:
			rendered = fmt::format("{}*", render_regex_ast(ast->children.front(), ast_precedence(*ast)));
			break;
		case RegexAstKind::K_CONCAT:
			for (const auto& child : ast->children) {
				rendered += render_regex_ast(child, ast_precedence(*ast));
			}
			break;
		case RegexAstKind::K_UNION: {
			bool first = true;
			for (const auto& child : ast->children) {
				if (!first) {
					rendered += '|';
				}
				rendered += render_regex_ast(child, ast_precedence(*ast));
				first = false;
			}
			break;
		}
	}

	if (ast_precedence(*ast) < parent_precedence) {
		return fmt::format("({})", rendered);
	}
	return rendered;
}

[[nodiscard]] auto decompose_concat(const RegexAstPtr& ast) -> std::vector<RegexAstPtr> {
	if (ast->kind == RegexAstKind::K_CONCAT) {
		return ast->children;
	}
	return {ast};
}

[[nodiscard]] auto regex_ast_key(const RegexAstPtr& ast) -> std::string {
	return render_regex_ast(ast);
}

[[nodiscard]] auto regex_ast_equal(const RegexAstPtr& lhs, const RegexAstPtr& rhs) -> bool {
	return regex_ast_key(lhs) == regex_ast_key(rhs);
}

[[nodiscard]] auto simplify_regex_ast(const RegexAstPtr& ast) -> RegexAstPtr {
	switch (ast->kind) {
		case RegexAstKind::K_LITERAL:
		case RegexAstKind::K_EPSILON:
		case RegexAstKind::K_EMPTY:
			return ast;

		case RegexAstKind::K_STAR: {
			const auto child = simplify_regex_ast(ast->children.front());
			if (child->kind == RegexAstKind::K_EMPTY || child->kind == RegexAstKind::K_EPSILON) {
				return make_ast(RegexAstKind::K_EPSILON);
			}
			if (child->kind == RegexAstKind::K_STAR) {
				return child;
			}
			return make_composite_ast(RegexAstKind::K_STAR, {child});
		}

		case RegexAstKind::K_CONCAT: {
			std::vector<RegexAstPtr> factors;
			for (const auto& child : ast->children) {
				const auto simplified = simplify_regex_ast(child);
				if (simplified->kind == RegexAstKind::K_EMPTY) {
					return make_ast(RegexAstKind::K_EMPTY);
				}
				if (simplified->kind == RegexAstKind::K_EPSILON) {
					continue;
				}
				if (simplified->kind == RegexAstKind::K_CONCAT) {
					factors.insert(factors.end(), simplified->children.begin(), simplified->children.end());
				} else {
					factors.push_back(simplified);
				}
			}

			std::vector<RegexAstPtr> compacted;
			for (const auto& factor : factors) {
				if (!compacted.empty() &&
					compacted.back()->kind == RegexAstKind::K_STAR &&
					factor->kind == RegexAstKind::K_STAR &&
					regex_ast_equal(compacted.back()->children.front(), factor->children.front())) {
					continue;
				}
				compacted.push_back(factor);
			}

			if (compacted.empty()) {
				return make_ast(RegexAstKind::K_EPSILON);
			}
			if (compacted.size() == 1) {
				return compacted.front();
			}
			return make_composite_ast(RegexAstKind::K_CONCAT, std::move(compacted));
		}

		case RegexAstKind::K_UNION: {
			std::vector<RegexAstPtr> terms;
			for (const auto& child : ast->children) {
				const auto simplified = simplify_regex_ast(child);
				if (simplified->kind == RegexAstKind::K_EMPTY) {
					continue;
				}
				if (simplified->kind == RegexAstKind::K_UNION) {
					terms.insert(terms.end(), simplified->children.begin(), simplified->children.end());
				} else {
					terms.push_back(simplified);
				}
			}

			std::map<std::string, RegexAstPtr> unique_terms;
			for (const auto& term : terms) {
				unique_terms.emplace(regex_ast_key(term), term);
			}

			bool has_star_term = false;
			for (const auto& [key, term] : unique_terms) {
				(void)key;
				has_star_term = has_star_term || term->kind == RegexAstKind::K_STAR;
			}
			if (has_star_term) {
				unique_terms.erase("eps");
			}

			std::vector<std::string> erased_keys;
			for (const auto& [key, term] : unique_terms) {
				(void)key;
				if (term->kind != RegexAstKind::K_STAR) {
					continue;
				}
				erased_keys.push_back(regex_ast_key(term->children.front()));
			}
			for (const auto& key : erased_keys) {
				unique_terms.erase(key);
			}

			std::vector<RegexAstPtr> simplified_terms;
			simplified_terms.reserve(unique_terms.size());
			for (auto& [key, term] : unique_terms) {
				(void)key;
				simplified_terms.push_back(std::move(term));
			}

			if (simplified_terms.empty()) {
				return make_ast(RegexAstKind::K_EMPTY);
			}
			if (simplified_terms.size() == 1) {
				return simplified_terms.front();
			}

			auto common_prefix = decompose_concat(simplified_terms.front());
			for (std::size_t term_index = 1; term_index < simplified_terms.size() && !common_prefix.empty(); ++term_index) {
				const auto factors = decompose_concat(simplified_terms[term_index]);
				std::size_t matched = 0;
				while (matched < common_prefix.size() && matched < factors.size() &&
					regex_ast_equal(common_prefix[matched], factors[matched])) {
					++matched;
				}
				common_prefix.resize(matched);
			}
			if (!common_prefix.empty()) {
				bool every_term_has_remainder = true;
				std::vector<RegexAstPtr> remainders;
				remainders.reserve(simplified_terms.size());
				for (const auto& term : simplified_terms) {
					const auto factors = decompose_concat(term);
					if (factors.size() == common_prefix.size()) {
						every_term_has_remainder = false;
						break;
					}
					std::vector<RegexAstPtr> remainder(
						factors.begin() + static_cast<std::ptrdiff_t>(common_prefix.size()),
						factors.end());
					remainders.push_back(simplify_regex_ast(make_composite_ast(RegexAstKind::K_CONCAT, std::move(remainder))));
				}
				if (every_term_has_remainder) {
					auto suffix_union = simplify_regex_ast(make_composite_ast(RegexAstKind::K_UNION, std::move(remainders)));
					auto factored = common_prefix;
					factored.push_back(suffix_union);
					return simplify_regex_ast(make_composite_ast(RegexAstKind::K_CONCAT, std::move(factored)));
				}
			}

			auto common_suffix = decompose_concat(simplified_terms.front());
			for (std::size_t term_index = 1; term_index < simplified_terms.size() && !common_suffix.empty(); ++term_index) {
				const auto factors = decompose_concat(simplified_terms[term_index]);
				std::size_t matched = 0;
				while (matched < common_suffix.size() && matched < factors.size() &&
					regex_ast_equal(
						common_suffix[common_suffix.size() - 1 - matched],
						factors[factors.size() - 1 - matched])) {
					++matched;
				}
				common_suffix.erase(
					common_suffix.begin(),
					common_suffix.begin() + static_cast<std::ptrdiff_t>(common_suffix.size() - matched));
			}
			if (!common_suffix.empty()) {
				bool every_term_has_remainder = true;
				std::vector<RegexAstPtr> remainders;
				remainders.reserve(simplified_terms.size());
				for (const auto& term : simplified_terms) {
					const auto factors = decompose_concat(term);
					if (factors.size() == common_suffix.size()) {
						every_term_has_remainder = false;
						break;
					}
					std::vector<RegexAstPtr> remainder(
						factors.begin(),
						factors.end() - static_cast<std::ptrdiff_t>(common_suffix.size()));
					remainders.push_back(simplify_regex_ast(make_composite_ast(RegexAstKind::K_CONCAT, std::move(remainder))));
				}
				if (every_term_has_remainder) {
					auto prefix_union = simplify_regex_ast(make_composite_ast(RegexAstKind::K_UNION, std::move(remainders)));
					std::vector<RegexAstPtr> factored;
					factored.push_back(prefix_union);
					factored.insert(factored.end(), common_suffix.begin(), common_suffix.end());
					return simplify_regex_ast(make_composite_ast(RegexAstKind::K_CONCAT, std::move(factored)));
				}
			}

			std::ranges::sort(simplified_terms, [](const auto& lhs, const auto& rhs) {
				return regex_ast_key(lhs) < regex_ast_key(rhs);
			});
			return make_composite_ast(RegexAstKind::K_UNION, std::move(simplified_terms));
		}
	}

	return ast;
}

[[nodiscard]] auto parse_regex_ast(const std::string_view regex_text)
	-> std::expected<RegexAstPtr, std::string> {
	const auto tokens = tokenize_regex(regex_text);
	if (!tokens) {
		return std::unexpected(tokens.error());
	}

	const auto postfix = to_postfix(*tokens);
	if (!postfix) {
		return std::unexpected(postfix.error());
	}

	std::vector<RegexAstPtr> stack;
	for (const auto& token : *postfix) {
		switch (token.kind) {
			case RegexTokenKind::K_LITERAL:
				stack.push_back(make_literal_ast(token.text));
				break;
			case RegexTokenKind::K_EPSILON:
				stack.push_back(make_ast(RegexAstKind::K_EPSILON));
				break;
			case RegexTokenKind::K_EMPTY:
				stack.push_back(make_ast(RegexAstKind::K_EMPTY));
				break;
			case RegexTokenKind::K_STAR:
				if (stack.empty()) {
					return std::unexpected("invalid regex: star is missing its operand");
				}
				stack.push_back(make_composite_ast(RegexAstKind::K_STAR, {stack.back()}));
				stack.erase(stack.end() - 2);
				break;
			case RegexTokenKind::K_CONCAT:
			case RegexTokenKind::K_UNION: {
				if (stack.size() < 2) {
					return std::unexpected("invalid regex: binary operator is missing operands");
				}
				const auto rhs = stack.back();
				stack.pop_back();
				const auto lhs = stack.back();
				stack.pop_back();
				stack.push_back(make_composite_ast(
					token.kind == RegexTokenKind::K_CONCAT ? RegexAstKind::K_CONCAT : RegexAstKind::K_UNION,
					{lhs, rhs}));
				break;
			}
			case RegexTokenKind::K_LPAREN:
			case RegexTokenKind::K_RPAREN:
				break;
		}
	}

	if (stack.size() != 1) {
		return std::unexpected("invalid regex: expression did not reduce to a single syntax tree");
	}
	return stack.back();
}

}  // namespace

auto build_epsilon_nfa_from_regex(const std::string_view regex_text)
	-> std::expected<AutomatonInput, std::string> {
	const auto tokens = tokenize_regex(regex_text);
	if (!tokens) {
		return std::unexpected(tokens.error());
	}

	const auto postfix = to_postfix(*tokens);
	if (!postfix) {
		return std::unexpected(postfix.error());
	}

	std::vector<std::string> alphabet;
	std::map<std::string, std::size_t, std::less<>> symbol_to_index;
	for (const auto& token : *tokens) {
		if (token.kind != RegexTokenKind::K_LITERAL || symbol_to_index.contains(token.text)) {
			continue;
		}
		symbol_to_index.emplace(token.text, alphabet.size());
		alphabet.push_back(token.text);
	}

	std::vector<Mask> epsilon_transitions;
	std::vector<std::tuple<int, int, std::size_t>> symbol_edges;
	std::vector<RegexFragment> stack;

	for (const auto& token : *postfix) {
		switch (token.kind) {
			case RegexTokenKind::K_LITERAL: {
				const auto start = create_state(epsilon_transitions);
				if (!start) {
					return std::unexpected(start.error());
				}
				const auto accept = create_state(epsilon_transitions);
				if (!accept) {
					return std::unexpected(accept.error());
				}
				symbol_edges.push_back({*start, *accept, symbol_to_index.at(token.text)});
				stack.push_back({.start = *start, .accept = *accept});
				break;
			}
			case RegexTokenKind::K_EPSILON: {
				const auto start = create_state(epsilon_transitions);
				if (!start) {
					return std::unexpected(start.error());
				}
				const auto accept = create_state(epsilon_transitions);
				if (!accept) {
					return std::unexpected(accept.error());
				}
				epsilon_transitions[to_size(*start)] |= state_bit(*accept);
				stack.push_back({.start = *start, .accept = *accept});
				break;
			}
			case RegexTokenKind::K_EMPTY: {
				const auto start = create_state(epsilon_transitions);
				if (!start) {
					return std::unexpected(start.error());
				}
				const auto accept = create_state(epsilon_transitions);
				if (!accept) {
					return std::unexpected(accept.error());
				}
				stack.push_back({.start = *start, .accept = *accept});
				break;
			}
			case RegexTokenKind::K_CONCAT: {
				if (stack.size() < 2) {
					return std::unexpected("invalid regex: concat is missing operands");
				}
				const auto rhs = stack.back();
				stack.pop_back();
				const auto lhs = stack.back();
				stack.pop_back();
				epsilon_transitions[to_size(lhs.accept)] |= state_bit(rhs.start);
				stack.push_back({.start = lhs.start, .accept = rhs.accept});
				break;
			}
			case RegexTokenKind::K_UNION: {
				if (stack.size() < 2) {
					return std::unexpected("invalid regex: union is missing operands");
				}
				const auto rhs = stack.back();
				stack.pop_back();
				const auto lhs = stack.back();
				stack.pop_back();
				const auto start = create_state(epsilon_transitions);
				if (!start) {
					return std::unexpected(start.error());
				}
				const auto accept = create_state(epsilon_transitions);
				if (!accept) {
					return std::unexpected(accept.error());
				}
				epsilon_transitions[to_size(*start)] |= state_bit(lhs.start) | state_bit(rhs.start);
				epsilon_transitions[to_size(lhs.accept)] |= state_bit(*accept);
				epsilon_transitions[to_size(rhs.accept)] |= state_bit(*accept);
				stack.push_back({.start = *start, .accept = *accept});
				break;
			}
			case RegexTokenKind::K_STAR: {
				if (stack.empty()) {
					return std::unexpected("invalid regex: star is missing its operand");
				}
				const auto operand = stack.back();
				stack.pop_back();
				const auto start = create_state(epsilon_transitions);
				if (!start) {
					return std::unexpected(start.error());
				}
				const auto accept = create_state(epsilon_transitions);
				if (!accept) {
					return std::unexpected(accept.error());
				}
				epsilon_transitions[to_size(*start)] |= state_bit(operand.start) | state_bit(*accept);
				epsilon_transitions[to_size(operand.accept)] |= state_bit(operand.start) | state_bit(*accept);
				stack.push_back({.start = *start, .accept = *accept});
				break;
			}
			case RegexTokenKind::K_LPAREN:
			case RegexTokenKind::K_RPAREN:
				break;
		}
	}

	if (stack.size() != 1) {
		return std::unexpected("invalid regex: expression did not reduce to a single automaton");
	}

	AutomatonInput automaton;
	automaton.state_count = static_cast<int>(epsilon_transitions.size());
	automaton.start_state = stack.back().start;
	automaton.kind = AutomatonKind::K_EPSILON_NFA;
	automaton.alphabet = std::move(alphabet);
	automaton.accept_states = state_bit(stack.back().accept);
	automaton.epsilon_transitions = std::move(epsilon_transitions);
	automaton.transitions.assign(
		to_size(automaton.state_count),
		std::vector<Mask>(automaton.symbol_count(), 0));

	for (const auto& [from, to, symbol_index] : symbol_edges) {
		automaton.transitions[to_size(from)][symbol_index] |= state_bit(to);
	}

	return automaton;
}

auto simplify_regex(const std::string_view regex_text) -> std::expected<std::string, std::string> {
	const auto ast = parse_regex_ast(regex_text);
	if (!ast) {
		return std::unexpected(ast.error());
	}
	return render_regex_ast(simplify_regex_ast(*ast));
}

auto build_regex_from_minimized_dfa(
	const AutomatonInput& source_automaton,
	const MinimizedDfaResult& minimized_dfa) -> std::string {
	if (minimized_dfa.merged_states.empty() ||
		std::ranges::find(minimized_dfa.accepting_states, true) == minimized_dfa.accepting_states.end()) {
		return "empty";
	}

	const auto state_count = static_cast<int>(minimized_dfa.merged_states.size());
	const auto augmented_state_count = state_count + 2;
	const auto synthetic_start = state_count;
	const auto synthetic_accept = state_count + 1;

	std::vector<std::vector<RegexValue>> labels(
		to_size(augmented_state_count),
		std::vector<RegexValue>(to_size(augmented_state_count), regex_empty()));

	labels[to_size(synthetic_start)][to_size(minimized_dfa.start_state)] = regex_epsilon();
	for (int state = 0; state < state_count; ++state) {
		if (minimized_dfa.accepting_states[to_size(state)]) {
			labels[to_size(state)][to_size(synthetic_accept)] = regex_union(
				labels[to_size(state)][to_size(synthetic_accept)],
				regex_epsilon());
		}
		for (std::size_t symbol_index = 0; symbol_index < minimized_dfa.transition_table[to_size(state)].size();
			 ++symbol_index) {
			const auto next_state = minimized_dfa.transition_table[to_size(state)][symbol_index];
			labels[to_size(state)][to_size(next_state)] = regex_union(
				labels[to_size(state)][to_size(next_state)],
				regex_literal(source_automaton.alphabet[symbol_index]));
		}
	}

	for (int eliminated = 0; eliminated < state_count; ++eliminated) {
		for (int from = 0; from < augmented_state_count; ++from) {
			if (from == eliminated) {
				continue;
			}
			for (int to = 0; to < augmented_state_count; ++to) {
				if (to == eliminated) {
					continue;
				}

				labels[to_size(from)][to_size(to)] = regex_union(
					labels[to_size(from)][to_size(to)],
					regex_concat(
						regex_concat(
							labels[to_size(from)][to_size(eliminated)],
							regex_star(labels[to_size(eliminated)][to_size(eliminated)])),
						labels[to_size(eliminated)][to_size(to)]));
			}
		}

		for (int index = 0; index < augmented_state_count; ++index) {
			labels[to_size(index)][to_size(eliminated)] = regex_empty();
			labels[to_size(eliminated)][to_size(index)] = regex_empty();
		}
	}

	const auto& result = labels[to_size(synthetic_start)][to_size(synthetic_accept)];
	if (result.ast != nullptr) {
		return render_regex_ast(simplify_regex_ast(result.ast));
	}
	return result.text;
}

}  // namespace automata::nfa_dfa
