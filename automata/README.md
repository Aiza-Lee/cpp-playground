# Automata Playground

从单个 JSON 输入构造：

- epsilon-NFA 的等价 NFA
- NFA / epsilon-NFA 对应的 DFA

源码：

- `include/automata/nfa_dfa/model.hpp`：核心数据结构和公共工具
- `include/automata/nfa_dfa/parser.hpp`：JSON / 文本输入解析
- `include/automata/nfa_dfa/algorithm.hpp`：epsilon 消除、epsilon-closure 和子集构造法
- `include/automata/nfa_dfa/output.hpp`：文本输出格式化
- `include/automata/nfa_dfa/app.hpp`：CLI 和程序入口逻辑
- `src/nfa_dfa/*.cpp`：对应实现
- `tests/nfa_dfa_tests.cpp`：doctest 回归测试

命令行只接收一个 JSON 输入文件：

```bash
cmake --preset gcc14-debug
cmake --build --preset gcc14-debug
./build/gcc14-debug/bin/automata/nfa_dfa automata/nfa_dfa_epsilon.json
./build/gcc14-debug/bin/automata/nfa_dfa --help
ctest --preset gcc14-debug --output-on-failure
```

JSON 结构如下：

```json
{
  "state_count": 4,
  "alphabet": ["a", "b"],
  "start_state": 0,
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
- `epsilon_transitions[i]` 是状态 `i` 的 epsilon 转移列表
- `use_epsilon = false` 时，`epsilon_transitions` 可以传空数组

输出内容：

- 等价 NFA 的起始状态和转移表
- 可达 DFA 状态
- DFA 转移表
- DFA 图边列表：`from to label`
  相同 `from -> to` 的多条边会合并成一行，多个转移符号用逗号分隔
