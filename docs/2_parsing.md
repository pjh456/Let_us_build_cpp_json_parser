# 从零开始用 C++ 打造高性能 JSON 解析器（二）、解析篇

## 前言

在上一章中，我们已经搭建好了 JSON 的数据模型（Element、Value、Object、Array）。现在，我们的内存里已经有了存放数据的“架子”，但如何把一串 JSON 字符串精准地填进这个架子里呢？

这就要涉及到编译器前端的两大核心任务：**词法分析 (Lexical Analysis)** 和 **语法分析 (Syntax Analysis)**。

---

## 一、词法分析：将字符串“打碎”成积木 (Tokenizer)

想象一下，处理 `{"age": 30}` 这种字符串。直接解析很痛苦。我们先把它切分成一个个独立的、有意义的单元，称为 **Token**。

对于 JSON，Token 类型包括：`{`, `}`, `[`, `]`, `:`, `,`, 字符串, 数字, 布尔值, null。

### 1. 定义 Token
```cpp
enum class TokenType {
    ObjectBegin, ObjectEnd, ArrayBegin, ArrayEnd,
    Colon, Comma, String, Integer, Float, Bool, Null, End
};

struct Token {
    TokenType type;
    std::string value;
};
```

### 2. 设计核心：Peek 与 Consume 模式
一个优秀的 `Tokenizer` 不应该只是一次性吐出所有 Token。我们采用**“流式”**设计，提供两个核心接口：
- `peek()`：查看当前指向的 Token，但不移动指针。
- `consume()`：确认处理完当前 Token，指针跳到下一个。

这种“读写分离”的接口设计，是构建健壮解析器的关键，能极大地简化后续 Parser 的逻辑。

```cpp
class Tokenizer {
public:
    const Token& peek() { return m_current_token; }
    void consume() { m_current_token = read_next_token(); }

private:
    Token read_next_token() {
        skip_white_space();
        if (stream.eof()) return {TokenType::End, ""};
        
        char ch = stream.get();
        switch (ch) {
            case '{': return {TokenType::ObjectBegin, "{"};
            case '"': return parse_string(); // 处理字符串转义等复杂逻辑
            case 't': case 'f': return parse_bool();
            // ... 处理数字、null 等
        }
    }
};
```

---

## 二、语法分析：递归下降算法 (Parser)

有了 Token，下一步就是根据 JSON 语法规则把它们组合起来。我们采用最直观的技术：**递归下降 (Recursive Descent)**。

**核心思想：** 为每一种语法结构（Object, Array, Value）编写一个对应的解析函数。如果结构是嵌套的，函数就递归地调用自己。

### 1. parse_value：交通枢纽
`parse_value` 是所有解析的入口。它根据当前的 Token 类型决定去调用谁。

```cpp
Element* Parser::parse_value() {
    const Token& token = peek();
    switch (token.type) {
        case TokenType::ObjectBegin: return parse_object();
        case TokenType::ArrayBegin:  return parse_array();
        case TokenType::Integer: {
            int val = std::stoi(token.value);
            consume(); // 别忘了消耗掉这个 Token
            return new Value(val);
        }
        // ... 其他基础类型
    }
}
```

### 2. parse_object：处理嵌套的奥秘
解析对象时，我们会遇到 `key: value` 对。注意看，`value` 部分又是调用 `parse_value`。

```cpp
Object* Parser::parse_object() {
    consume(); // 消耗 '{'
    Object* obj = new Object();

    if (peek().type == TokenType::ObjectEnd) {
        consume(); return obj; // 空对象 {}
    }

    while (true) {
        // 1. 解析 Key
        auto key = peek().value; consume(); 
        
        // 2. 消耗冒号 ':'
        consume(); 

        // 3. 递归解析 Value！这里是处理无限嵌套的关键
        obj->insert_raw_ptr(key, parse_value());

        // 4. 处理逗号或结束符
        if (peek().type == TokenType::ObjectEnd) {
            consume(); break;
        } else {
            consume(); // 消耗 ','
        }
    }
    return obj;
}
```

---

## 三、为什么这种架构如此优雅？

1. **代码即语法**：
   递归下降最美妙的地方在于，你的代码结构和 JSON 的语法定义几乎是一一对应的。`parse_object` 的逻辑读起来就像在读 JSON 规范。

2. **天然支持嵌套**：
   通过函数调用栈，递归自然地处理了 `{ "a": { "b": { "c": 1 } } }` 这种深层嵌套，无需手动维护复杂的堆栈。

3. **严谨的状态推进**：
   我们严格遵循 **Peek (观察) -> Process (处理) -> Consume (消耗)** 的韵律。这确保了解析器永远单向前进，逻辑清晰，不会出现回溯导致的性能抖动。

---

## 解析篇小结

至此，我们已经打通了从“字符串”到“内存对象树”的全部链路。你现在已经可以写出如下代码：
```cpp
Tokenizer t(input_str);
Parser p(&t);
Element* root = p.parse(); // 大功告成！
```

但是，作为一个好用的库，目前的 API 还是太笨重了——获取一个嵌套字段需要不停地 `as_object()`、`get()`。此外，频繁的 `new` 操作也是我们在第一篇中提到的性能大敌。

**在下一篇文章中，我们将进行“易用性封装”，引入 Ref 代理类和工厂函数，让你的 C++ 代码写起来像 Python 一样丝滑。**

---

*源码仓库：[github 仓库](https://github.com/pjh456/Let_us_build_cpp_json_parser)*
*觉得有启发？点个赞支持一下，你的鼓励是我更新的最大动力！*