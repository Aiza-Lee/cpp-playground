# Automata Playground

从单个 JSON 输入构造：

- epsilon-NFA 的等价 NFA
- NFA / epsilon-NFA 对应的 DFA
- DFA 的最小化结果
- 自动机对应的正则表达式
- 正则表达式对应的 epsilon-NFA / DFA / 最小化 DFA

源码：

- `include/automata/nfa_dfa/model.hpp`：核心数据结构和公共工具
- `include/automata/nfa_dfa/parser.hpp`：JSON / 文本输入解析
- `include/automata/nfa_dfa/algorithm.hpp`：epsilon 消除、epsilon-closure 和子集构造法
- `include/automata/nfa_dfa/regex.hpp`：正则解析、Thompson 构造、状态消除
- `include/automata/nfa_dfa/output.hpp`：文本输出格式化
- `include/automata/nfa_dfa/app.hpp`：CLI 和程序入口逻辑
- `src/nfa_dfa/*.cpp`：对应实现
- `tests/nfa_dfa_tests.cpp`：doctest 回归测试

命令行一次只接收一个输入源，可以是 JSON 自动机、命令行正则、或者正则文件：

```bash
cmake --preset gcc14-debug
cmake --build --preset gcc14-debug
./build/gcc14-debug/bin/automata/nfa_dfa automata/nfa_dfa_epsilon.json
./build/gcc14-debug/bin/automata/nfa_dfa --regex "a(b|c)*"
./build/gcc14-debug/bin/automata/nfa_dfa --regex-file automata/regex_sample.txt
./build/gcc14-debug/bin/automata/nfa_dfa --help
ctest --preset gcc14-debug --output-on-failure
```

JSON 结构如下：

```json
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
```

其中：

- `transitions[i][j]` 是状态 `i` 在第 `j` 个符号下可到达的状态列表
- `accept_states` 是接受态列表；若省略则默认没有接受态
- `epsilon_transitions[i]` 是状态 `i` 的 epsilon 转移列表
- `use_epsilon = false` 时，`epsilon_transitions` 可以传空数组

输出内容：

- 等价 NFA 的起始状态和转移表
- 可达 DFA 状态与接受态
- DFA 转移表
- DFA 图边列表：`from to label`
  相同 `from -> to` 的多条边会合并成一行，多个转移符号用逗号分隔
- 最小化 DFA 的状态分组、转移表和图边列表
- 自动机对应的等价正则表达式

正则语法：

- 并：`|`
- 闭包：`*`
- 连接：隐式，例如 `a(b|c)`、`a b`、`foo'+'bar`
- 多字符单个符号：直接写成一个裸标识符，例如 `ab12`；若含特殊字符或空格则用引号，例如 `'two words'`
- 括号：`(` `)`
- 空串：`eps`
- 空语言：`empty`
- 字面符号：
  - 裸标识符：`a`、`token_1`、`ab12`
  - 单引号包裹：`'+'`、`'->'`、`'two words'`
  - 反斜杠转义单字符：`\(`、`\|`、`\*`

优先级和结合方式：

- 最高：`*`
  只作用在它左边紧邻的一个项上，例如 `ab*` 会解析成 `a(b*)`
- 其次：连接
  例如 `ab|c` 会解析成 `(ab)|c`
- 最低：`|`
  例如 `a|bc` 会解析成 `a|(bc)`
- `*` 可以连续出现，例如 `a**` 会按 `((a*)*)` 处理
- 连接和并都按左结合处理
  例如 `abc` 在词法拆分成多个项的情况下会按 `((ab)c)`，`a|b|c` 会按 `((a|b)|c)`
- 括号优先级最高，用于显式改变分组
  例如 `(a|b)c` 会解析成 `((a|b)c)`

说明：

- 正则输入通过 Thompson 构造生成 epsilon-NFA
- 自动机转正则会先走可达 DFA 和最小化，再用状态消除输出等价正则
- 最终输出的等价正则会再经过一轮轻量代数化简
  目前会处理去重、去掉 `empty/eps` 冗余、消除重复闭包，以及对形如 `ab|ac`、`ba|ca` 的公共前后缀提取
