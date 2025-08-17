# 从零开始：手把手教你用 C++ 打造一个自己的 JSON 解析器 (项目构建实战)

## 前言

你好，C++ 开发者！你是否已经熟悉了 C++ 的语法，却在面对“如何从零开始一个完整项目”时感到迷茫？你是否想深入理解一个工具的底层原理，而不仅仅是停留在调用 API 的层面？

这篇博客就是为你准备的。我们将以一个大家既熟悉又实用的目标——JSON 解析器——为例，完整地走过一次从概念设计到代码实现的全过程。

本文不是对一份现有代码的解释，而是一份详细的施工蓝图和步骤指南。我们将一步步提出问题，然后用代码给出解决方案，并深入探讨“为什么”要这么设计。

### 本文包含的技术实践：
- 面向对象编程：如何通过抽象基类和继承来设计一套灵活的数据结构。
- 手动内存管理：深入理解 `new` 和 `delete` 的责任与时机（这是C++开发者的重要一课）。
- 自定义错误处理：如何设计一套清晰、具体的异常类来提升代码的健壮性。
- 词法与语法分析：实现一个简单的 `Tokenizer` 和递归下降解析器，揭开“解析”的神秘面纱。
- 易用性API设计：如何封装底层实现，提供优雅、易用的链式调用接口。

### 该文章不包含的技术内容：
- 高性能多线程解析
- C++ 容器的特性讲解
- 智能指针的使用实践
- 底层并行优化实现
- JSON 语法校验与自修正逻辑

读完本文，你将收获的不仅仅是一个可用的 JSON 库，更重要的是：

- **项目化思维**：学习如何将一个模糊的需求，逐步拆解、设计、并实现为具体的代码模块。
- **设计决策的权衡**：理解在特定场景下，为什么选择这种数据结构，为什么采用这种内存管理方式。
- **从底层构建的成就感**：亲手创造一个实用工具，彻底掌握它的每一个细节。

准备好了吗？清空你的编辑器，让我们从第一行代码开始，构建属于我们自己的 JSON 王国。

---

## 一、项目背景：为什么我们要“造轮子”？

JSON 作为一种轻量级、高可读性的数据交换格式，早已成为现代软件开发的“通用语言”。

然而，C++ 标准库至今仍未将其纳入标准。在实际开发中，我们通常会引入 `nlohmann/json` 或 `rapidjson` 等优秀的第三方库。

既然有现成的轮子，我们为什么还要自己造一个呢？

1. **深度学习**：自己动手实现一遍，能让你彻底理解 JSON 的解析原理、内存布局以及各种边界情况的处理，这些是单纯使用库无法获得的。
2. **轻量与可控**：对于一些小型项目，引入一个庞大的第三方库可能有些“杀鸡用牛刀”。一个单头文件的实现，意味着极简的依赖和部署，你可以完全掌控每一行代码。
3. **面试加分项**：在求职面试中，一个能清晰阐述设计思路并展示完整实现的项目，无疑会成为你技术能力的有力证明。

因此，这个项目不仅是一个工具，更是一个绝佳的学习和实践平台。

---

## 二、需求与技术栈

在动手之前，我们先明确目标。

### 功能性需求

- 解析 (反序列化)：将 JSON 格式的字符串，转换为 C++ 内的对象。
- 生成 (序列化)：将 C++ 内的对象，转换为 JSON 格式的字符串。
- 数据访问：提供方便的接口来访问和修改解析后的数据。
- 错误处理：在解析或访问出错时，能抛出包含具体信息的异常。
- 支持类型：完整支持 JSON 的六种数据类型（`object`, `array`, `string`, `number`, `boolean`, `null`）。

### 技术选择 (C++ 17)
- `std::variant`：用于优雅地存储 `string`, `int`, `float`, `bool`, `nullptr` 等多种基础类型的值。
- `std::string`：用于处理字符串。
- `std::vector`：用于实现 JSON Array。
- `std::map`：用于实现 JSON Object。
- `std::runtime_error`：作为我们自定义异常类的基类。

## 三、蓝图设计：构建 JSON 的数据王国

JSON 的世界本质上是分层的。一个 JSON 文档的根节点，要么是一个对象 (`{}`), 要么是一个数组 (`[]`)。而对象和数组内部，又可以包含其他对象、数组或者基础类型的值。这种递归的结构，天然适合用面向对象的方式来建模。

### 第一步：为 JSON 数据建模 - 抽象与多态的艺术

在写任何解析代码之前，我们必须先回答一个核心问题：当一个 JSON 字符串被解析后，我们应该用什么 C++ 数据结构来存储它？

JSON 有六种数据类型：`object`, `array`, `string`, `number`, `boolean`, 和 `null`。这些类型可以互相嵌套，形成一个树状结构。例如：

```json
{
    "name": "John Doe",
    "age": 30,
    "isStudent": false,
    "courses": ["C++", "Python"],
    "address": null
}
```

这个结构中，根是一个 **对象(`Object`)**，它包含 **字符串(`String`)**、**数字(`Number`)**、**布尔(`Boolean`)**、**空值(`Null`)**，以及一个 **数组(`Array`)**。

这种“万物皆可互嵌”的特性，完美地指向了 C++ 的一个核心思想：**多态**。我们可以设计一个通用的基类，让它代表“JSON 世界里的一切元素”，然后让 `Object`、`Array`、`Value` 等具体类型都继承自它。

让我们来创建这个基类，就叫 `Element` 吧。

```cpp
// json_element.hpp
using string_t = std::string;

class Element {
public:
    virtual ~Element() {} // 1. 虚析构函数

    // 2. 类型识别
    virtual bool is_value() const noexcept { return false; }
    virtual bool is_array() const noexcept { return false; }
    virtual bool is_object() const noexcept { return false; }

    // 3. 类型安全转换
    virtual Value* as_value() { throw TypeException("Invalid base type!"); }
    virtual Array* as_array() { throw TypeException("Invalid base type!"); }
    virtual Object* as_object() { throw TypeException("Invalid base type!"); }

    // 4. 通用操作
    virtual Element* copy() const = 0; // 纯虚函数
    virtual std::string serialize() const noexcept { return ""; }
    virtual void clear() {}

    // 5. 比较操作
    virtual bool operator==(const Element& other) const noexcept { return false; }
    virtual bool operator!=(const Element& other) const noexcept { return true; }
};
```

#### 设计细节剖析：

1. **`virtual ~Element()`**：<br>
这是多态基类的“黄金法则”。<br>
当我们通过基类指针 `Element*` 删除一个派生类对象时，如果析构函数不是虚函数，就只会调用基类的析构函数，导致派生类的资源（如 Array 里的元素）无法被释放，造成内存泄漏。将其设为虚函数，可以确保正确的析构函数链被调用。

2. **`is_...()` 成员函数**：<br>
我们为什么要把这三个函数直接实现在基类里，并且都返回 false？<br>
这是一个非常精巧的设计。它建立了一条“默认规则”：任何一个 `Element` 对象，默认情况下既不是 `Value`，也不是 `Array`，也不是 `Object`。<br>
只有当一个子类，比如 `Array`，想要表明自己的身份时，它才需要去 `override is_array()` 函数并返回 `true`。这避免了在每个子类中都要实现全部三个 `is_...` 函数的麻烦。

3. **`as_...()` 成员函数**：<br>
这里的逻辑与 `is_...()` 类似，但更加严格。<br>
它建立的默认规则是：“任何试图将一个基类指针转换为具体类型的操作，默认都是非法的”。<br>
因此，基类版本直接抛出异常。只有 `Array` 类才有资格 `override as_array()` 并安全地返回 `this` 指针。<br>
这种“fail-fast”（快速失败）的设计，让类型转换的错误能被立即捕获，而不是产生一个悬空的 `nullptr`，把问题留到后面。

4. **`virtual Element* copy() const = 0;`**：<br>
这是一个纯虚函数。它意味着 `Element` 类自身是抽象的，不能被实例化。<br>
更重要的是，它强制所有继承自 `Element` 的具体子类都必须提供自己的 `copy()` 实现。<br>
我们无法在基类层面知道该如何“深拷贝”一个对象（是拷贝一个 `Value` 还是一个 `Array`？），所以这个责任必须被下放到子类。

通过这一个基类，我们已经为整个 JSON 数据王国搭建好了骨架。

### 第二步：实现“原子” - `Value` 类

现在我们有了骨架，需要填充最基础的砖块：`string`, `number`, `bool`, `null`。我们可以把它们统一看作“值(`Value`)”。问题是，如何在 C++ 的一个类里，同时持有这些不同类型的数据？

C++17 给了我们一个完美的工具：`std::variant`。它是一个类型安全的 `union`。

```cpp
// json_value.hpp

// ... using 语句和异常类定义 ...
using value_t = std::variant<
    std::nullptr_t,
    bool,
    int,
    float,
    string_t>;

class Value : public Element
{
private:
    value_t m_value;

public:
    // 构造函数，支持各种基础类型
    Value() : m_value(nullptr) {}
    Value(bool p_value) : m_value(p_value) {}
    Value(int p_value) : m_value(p_value) {}
    Value(float p_value) : m_value(p_value) {}
    Value(const string_t& p_value) : m_value(p_value) {}
    Value(char* p_value) : Value((string_t)p_value) {}

    // --- 实现基类的约定 ---
    bool is_value() const noexcept override { return true; }
    Value* as_value() override { return this; }
    Element* copy() const noexcept override { return new Value(*this); }

    // --- 序列化 ---
    std::string serialize() const noexcept override
    {
        if (is_int())
            return std::to_string(as_int());
        else if (is_float())
            return std::to_string(as_float());
        else if (is_str())
            return "\"" + as_str() + "\""; // 字符串需要加引号
        else if (is_bool())
            return as_bool() ? "true" : "false";
        else if (is_null())
            return "null";
        else
            return "";
    }

    // --- 类型判断和获取 ---
private:
    template <typename T>
    bool is_T() const noexcept { return std::holds_alternative<T>(m_value); }

public:
    bool is_null() const noexcept { return is_T<std::nullptr_t>(); }
    bool is_bool() const noexcept { return is_T<bool>(); }
    // ... is_int(), is_float(), is_str() ...

private:
    template <typename T>
    T as_T() const { return std::get<T>(m_value); }

public:
    int as_int() const {
        if (is_int()) return as_T<int>();
        throw TypeException("Not int type!");
    }
    // ... as_bool(), as_float(), as_str() ...
};
```

#### 设计细节剖析：

1. **继承与重写**：<br>
`Value` 类继承自 `Element`，并立刻重写了 `is_value()` 和 `as_value()`，明确了自己的“身份”。<br>
这是对基类设计模式的直接应用。

2. **std::variant 的使用**：<br>
`std::holds_alternative<T>(m_value)` 用于安全地检查 `m_value` 当前持有的类型是不是 `T`。<br>
`std::get<T>(m_value)` 用于获取 `m_value` 中类型为 `T` 的值。如果类型不匹配，它会抛出 `std::bad_variant_access` 异常。我们的 `as_...` 函数通过先检查后获取的方式，提供了更具体的错误信息。

3. **`copy()` 实现**：`copy` 函数简单地通过拷贝构造函数 `new Value(*this)` 创建了一个新的 `Value` 对象，实现了深拷贝。<br>

4. **``serialize()`` 实现**：这个函数是解析的逆过程。它通过 `is_...()` 判断当前值的具体类型，并将其转换为符合 JSON 规范的字符串。<br>
**注意，字符串类型的值在序列化时，前后必须加上双引号**。

### 第三步：实现“容器” - `Array` 和 `Object` 类

有了原子，我们就可以构建分子了。JSON 的 `Array` 和 `Object` 都是容器，它们装载的是任意 `Element`。

#### 3.1 `Array` 类

JSON 数组是有序的元素集合，`std::vector` 是最自然的选择。

我们应该存储什么呢？为了实现多态，我们必须存储基类指针：`std::vector<Element*>`。

```cpp
// json_array.hpp
template<typename T>
using array_t = std::vector<T>;

class Array : public Element
{
private:
    array_t<Element*> m_arr;

public:
    Array() : m_arr() {}
    ~Array() override { clear(); } // 关键点1：析构时清理内存

    // --- 实现基类的约定 ---
    bool is_array() const noexcept override { return true; }
    Array* as_array() override { return this; }
    Element* copy() const noexcept override; // 后面实现

    // --- 核心功能 ---
    void clear() override {
        for (const auto& it : m_arr) {
            if (it != nullptr) delete it; // 关键点2：手动释放内存
        }
        m_arr.clear();
    }

    void append(int p_value) { append_raw_ptr(new Value(p_value)); }
    // ... 其他 append 重载 ...
    void append_raw_ptr(Element* child) { m_arr.push_back(child); }

    Element* at(size_t p_index) { /* ... 边界检查 ... */ return m_arr[p_index]; }

    // ... 其他功能如 size(), operator[] 等 ...
};
```

#### 3.2 `Object` 类

JSON 对象是键值对的无序集合，`std::map<std::string, Element*>` 是理想的实现。

```cpp
// json_object.hpp
using object_t = std::map<string_t, T>;

class Object : public Element
{
private:
    object_t<Element*> m_obj;

public:
    Object() { m_obj.clear(); }
    ~Object() override { clear(); }

    // --- 实现基类的约定 ---
    bool is_object() const noexcept override { return true; }
    Object* as_object() override { return this; }
    Element* copy() const noexcept override; // 后面实现

    // --- 核心功能 ---
    void clear() override {
        for (auto const& [key, val] : m_obj) {
            if (val != nullptr) delete val;
        }
        m_obj.clear();
    }

    void insert(const string_t& p_key, int p_value) { insert_raw_ptr(p_key, new Value(p_value)); }
    // ... 其他 insert 重载 ...
    void insert_raw_ptr(const string_t& p_key, Element* child) {
        // ... 处理 key 已存在的情况 ...
        m_obj[p_key] = child;
    }

    Element* get(const string_t& p_key) { /* ... 查找并返回值或抛异常 ... */ }
};
```

#### 设计细节剖析与重大决策：手动内存管理
1. **`new` 与 `delete` 的责任**：<br>
当我们调用 `my_array.append(10)` 时，内部执行了 `new Value(10)`。这在 **堆(heap)** 上分配了一块内存。<br>
C++ 的一个核心原则是：谁 `new` 了，谁就得 `delete`。<br>
在这里，`Array` 和 `Object` 作为容器，它们“拥有”了这些 `new` 出来的 `Element` 对象。<br>
因此，它们必须承担起在自己生命周期结束时（即调用析构函数时），或者在调用 `clear()` 清空内容时，将这些内存 `delete` 掉的责任。<br>
这就是 `clear()` 函数中 `for` 循环 `delete` 的由来。

2. **为什么不使用智能指针？** <br>
这绝对是一个合理的问题。在现代 C++ 中，使用 `std::vector<std::unique_ptr<Element>>` 会是更安全、更符合 RAII (资源获取即初始化) 思想的做法。<br>
智能指针可以自动管理内存，我们就不必手动写 `delete`，从而避免了大量潜在的内存泄漏风险。<br>
**然而，本项目选择手动管理，是出于教学目的**。对于初学者来说，亲手处理 `new` 和 `delete`，能让你对 C++ 的内存模型、对象所有权(ownership)、以及 RAII 的重要性有更深刻、更本质的理解。<br>
当你完全掌握了这里的逻辑后，用 `std::unique_ptr` 来重构它，将会是一次非常有价值的进阶练习。

3. **`copy()` 的深度拷贝**：现在我们可以实现 `Array` 和 `Object` 的 `copy` 函数了。简单的指针拷贝是“浅拷贝”，会导致两个容器指向同一份内存，当一个析构时，另一个就变成了野指针。我们必须实现“深拷贝”：

```cpp
// 在 Array 类中
Element* Array::copy() const noexcept {
    Array* new_arr = new Array();
    for (const auto& elem : m_arr) {
        // 多态调用每个子元素的 copy() 函数，实现递归深拷贝
        new_arr->append_raw_ptr(elem->copy());
    }
    return new_arr;
}

// Object::copy() 逻辑类似
```

至此，我们已经拥有了一套功能完备的、能够在内存中表示任意 JSON 结构的数据模型！

---

## 四、数据解析：实现 JSON 与数据之间的转换

通过以上步骤，我们已经成功在 C++ 中搭建起来了 JSON 数据的模型，但是这只完成了我们目标的一半。

在实际运用中，如何将 JSON 字符串转化为可以处理的格式，也是同等甚至更加重要的一个内容。在接下来的教程中，我们会一步步实现一个最基础的，从 JSON 字符串到我们的 C++ 模型的处理过程。

### 第四步：词法分析器 - 将字符串“打碎”成有意义的积木 (`Tokenizer`)

想象一下，我们拿到一长串 JSON 字符串。直接去解析它，就像是让我们直接阅读一篇没有空格和标点的文章，会非常困难。

一个更明智的做法是，先把文章切分成一个个独立的单词和标点符号。

在编程语言处理中，这个过程被称为 **词法分析 (Lexical Analysis)**，执行这个任务的工具就是 **词法分析器 (`Tokenizer`)**。

它的唯一职责就是读取输入的字符串流，然后把它分解成一连串有意义的最小单元，我们称之为 `Token`。

对于 JSON 来说，`Token` 有哪些呢？

- `{` (ObjectBegin), `}` (ObjectEnd)
- `[` (ArrayBegin), `]` (ArrayEnd)
- `: `(Colon), `,` (Comma)
- 字符串 (e.g., `"name"`)
- 数字 (e.g., `123`, `12.72`)
- 布尔值 (`true`, `false`)
- `null`

#### 4.1 定义 `Token`

首先，我们用一个 `enum class` 来定义所有可能的 `Token` 类型，再用一个 `struct` 来封装一个具体的 `Token`：

```cpp
// token.hpp

enum class TokenType {
    ObjectBegin, // '{'
    ObjectEnd,   // '}'
    ArrayBegin,  // '['
    ArrayEnd,    // ']'
    Colon,       // ':'
    Comma,       // ','
    String,      // "string"
    Integer,     // 123
    Float,       // 12.72
    Bool,        // true or false
    Null,        // null
    End          // End of input
};

struct Token {
    TokenType type;
    std::string value;
};
```

#### 4.2 实现 `Tokenizer`

`Tokenizer` 的核心是暴露给外部的 `peek()` 和 `consume()` 接口。它内部像一个“预读”的机器，时刻准备好下一个将要被解析的 `Token`。

```cpp
// tokenizer.hpp
// InputStream 是一个辅助类，封装了字符串和当前读取位置，便于操作
class InputStream { /* ... get(), peek(), eof() ... */ };

class Tokenizer {
private:
    InputStream stream;
    char current_ch;
    size_t line;
    size_t column;
    Token m_current_token; // 核心：内部缓冲的“当前”Token

public:
    // 构造时，立刻读取第一个 Token 待命
    Tokenizer(const InputStream& stream) : stream(stream), line(1), column(1) {
        consume();
    }

    // 接口1：“窥视” (Peek) - 查看下一个 Token 是什么，不改变状态
    const Token& peek() const noexcept { return m_current_token; }

    // 接口2：“消耗” (Consume) - 确认当前 Token 已处理，前进到下一个
    void consume() { m_current_token = read_next_token(); }

private:
    // 真正读取并解析下一个 Token 的内部实现
    Token read_next_token()
    {
        skip_white_space(); // 跳过空白符
        // 输入结束
        if (stream.eof()) { return {TokenType::End, ""}; }

        current_ch = stream.get();
        column++;

        switch (current_ch) {
            case '{': return {TokenType::ObjectBegin, "{"};
            // ... 其他简单符号 ...
            case '"': return parse_string();
            case 't':
            case 'f': return parse_bool();
            case 'n': return parse_null();
            default:
                if (isdigit(current_ch) || current_ch == '-')
                    return parse_number();
                else
                    throw ParseException(line, column, "Unexpected character");
        }
    }
    // ... parse_string(), parse_number() 等辅助函数 ...
};
```

#### 设计细节剖析：

1. **清晰的“契约”**：<br>
这个 `Tokenizer` 的设计向它的使用者 (`Parser`) 提供了一个非常清晰的契约：<br>
- `peek()`：一个只读操作。你可以无限次调用它，它总是告诉你下一个将要处理的 `Token` 是什么，而不会改变 `Token` 流的状态。<br>
- `consume()`：一个只写操作。它像一个确认按钮，告诉 `Tokenizer`：“我已经处理完 `peek()` 看到的那个 `Token` 了，请准备好下一个”。<br>
这种“读写分离”的接口设计，是构建健壮解析器的关键，它极大地简化了 `Parser` 的逻辑。

2. **状态机思想**：<br>
`next()` 方法本质上是一个简单的状态机。<br>
它读取一个字符，然后根据这个字符转换到不同的处理逻辑（返回一个简单的 `Token`，或者进入一个更复杂的 `parse_...` 函数）。

3. **详细的错误报告**：<br>
当遇到一个无法识别的字符时，我们不能简单地失败。<br>
通过追踪当前的 `line` 和 `column`，`ParseException` 可以告诉用户错误发生的确切位置。<br>
这对于调试不规范的 JSON 输入至关重要，是提升工具可用性的关键一步。

4. **`InputStream` 辅助类**：<br>
虽然可以直接操作 `std::string` 和一个索引 `size_t pos`，<br>
但将其封装成一个 `InputStream` 类（包含 `get()` 获取并前进, `peek()` 只看不前进, `eof()` 判断末尾等方法）是一种良好的实践。<br>
它能让 `Tokenizer` 的代码更具可读性，逻辑更清晰。

现在，我们已经有了一台能把原始字符串加工成标准化“积木”（`Token`）的机器。下一步，就是用这些积木来搭建我们宏伟的 `Element` 大厦了。

### 第五步：语法分析器 - 组合积木，构建对象 (`Parser`)

我们有了一堆 `Token`，现在需要根据 JSON 的语法规则 (Grammar) 来将它们组合成有意义的结构。这个过程就是 **语法分析 (Syntax Analysis)**。

我们将采用一种非常直观且易于实现的解析技术：**递归下降 (Recursive Descent)**。

这个方法的核心思想是：<br>
为语法中的每一种结构（比如 `Value`, `Object`, `Array`）编写一个对应的解析函数。<br>
当一个函数需要解析一个内嵌的结构时，它就去调用那个结构对应的解析函数。

#### 5.1 `Parser` 类 - 一个有状态的建筑师

我们的 `Parser` 将是一个纯静态类，因为它不需要存储任何状态，它的工作就是接收一个 `Tokenizer`，然后输出一个 `Element*`

与 `Tokenizer` 类似，`Parser` 是一个真正的对象。它拥有自己的状态，并封装了完整的解析流程。

```cpp
// json.hpp
class Parser
{
private:
    // Parser 拥有自己的 Tokenizer 实例指针
    Tokenizer* m_tokenizer;

public:
    Parser(Tokenizer* p_tokenizer) : m_tokenizer(p_tokenizer) {}

    // 统一的入口点
    Ref parse() { return Ref(parse_value()); }

private:
    // 便捷的内部接口，封装与 Tokenizer 的交互
    const Token& peek() const noexcept { return m_tokenizer->peek(); }
    void consume() { m_tokenizer->consume(); }

    // 核心的递归下降函数
    Element* parse_value();
    Object* parse_object();
    Array* parse_array();
};
```

#### 设计细节剖析：

1. **代码结构镜像语法结构**：<br>
递归下降最美妙的地方在于，代码的结构几乎就是 JSON 语法的直接翻译。<br>
`parse_object` 的 `while` 循环完美地对应了对象中“一个接一个的键值对”的结构。<br>
这种直观性使得代码易于理解和调试。

2. **递归的体现**：<br>
当 `parse_object` 需要一个值时，它并不关心这个值是简单的 `123` 还是一个复杂的嵌套对象。<br>
它只是简单地把这个任务委托给 `parse_value`。<br>
而 `parse_value` 如果发现下一个 Token 是 `{`，又会反过来调用 `parse_object`。<br>
这个过程就形成了递归，完美地处理了 JSON 无限嵌套的能力。

3. **`Parser` 与 `Tokenizer` 的协作**：<br>
`Parser` 是消费者，`Tokenizer` 是生产者。<br>
`Parser` 不断地向 `Tokenizer` 请求 `next()` 下一个 `Token`，并根据 `Token` 的类型来推进自己的解析逻辑。<br>
这是一个经典的生产者-消费者模式。<br>

#### 5.2 递归下降的实现

现在，我们来实现具体的解析函数，感受一下“代码即语法”的魅力。

`parse_value()`：交通枢纽

```cpp
// 在 Parser 类中
Element* Parser::parse_value() 
{
    const Token& token = peek(); // 1. Peek

    switch (token.type) {
        case TokenType::ObjectBegin:
            return parse_object(); // 2. Process (递归)
        case TokenType::ArrayBegin:
            return parse_array();  // 2. Process (递归)
        
        // 对于基础类型，直接处理
        case TokenType::Integer: 
        {
            int val = std::atoi(token.value.c_str());
            consume();             // 3. Consume
            return new Value(val);
        }
        // ... 其他基础类型的处理逻辑完全相同 ...
        default:
            throw TypeException("Unexpected token for a value");
    }
}
```

`parse_object()`：构建复杂结构

```cpp
// 在 Parser 类中
Object* Parser::parse_object() 
{
    consume(); // 前提：已知当前是 '{', 消耗掉它

    Object* obj = new Object();
    
    // 处理空对象 {} 的情况
    if (peek().type == TokenType::ObjectEnd) 
    {
        consume(); // 消耗 '}'
        return obj;
    }

    while (true) 
    {
        // 1. 期待 Key
        const Token& key_token = peek();
        if (key_token.type != TokenType::String) throw Exception("Expected string key");
        std::string key = key_token.value;
        consume(); // 消耗 Key

        // 2. 期待 Colon
        if (peek().type != TokenType::Colon) throw Exception("Expected colon");
        consume(); // 消耗 ':'

        // 3. 递归解析 Value。把复杂的任务委托出去！
        obj->insert_raw_ptr(key, parse_value());

        // 4. 检查下一个是逗号还是结束符
        const Token& next_token = peek();
        if (next_token.type == TokenType::ObjectEnd) 
        {
            consume(); // 消耗 '}'
            break;     // 循环结束
        }
        else if (next_token.type == TokenType::Comma)
            consume(); // 消耗 ','，准备下一个键值对
        else throw Exception("Expected comma or '}'");
    }
    return obj;
}
```

`parse_array` 的实现也遵循着完全相同的严谨逻辑。

#### 最终设计剖析：

1.  **代码结构镜像语法结构**：<br>
递归下降最美妙的地方在于，代码的结构几乎就是 JSON 语法的直接翻译。<br>
`parse_object` 的 `while` 循环完美地对应了对象中“一个接一个的键值对”的结构。这种直观性使得代码易于理解和调试。

2.  **递归的体现**：<br>
当 `parse_object` 需要一个值时，它并不关心这个值是简单的 `123` 还是一个复杂的嵌套对象。它只是简单地把这个任务委托给 `parse_value`。<br>
而 `parse_value` 如果发现下一个 Token 是 `{`，又会反过来调用 `parse_object`。<br>
这个过程就形成了递归，完美地处理了 JSON 无限嵌套的能力。

3.  **严谨的状态推进**：<br>
`Parser` 严格遵循 **Peek -> Process -> Consume** 的韵律。<br>
每个 Token 在被 `peek()` 确认其角色后，都会被立即 `consume()`。<br>
这确保了解析器总是在单向、无回溯地前进，逻辑清晰，不会出错。

我们成功了！通过一个清晰的 `Tokenizer` 和一个有状态的 `Parser` 的协作，我们已经能将任意复杂的 JSON 字符串，转换成我们精心设计的、在内存中的 `Element` 对象树。



## 五、锦上添花 - 便捷的 API 封装 (`Ref`) 与总结

我们的库现在功能已经完备，但还不够“好用”。

想象一下，用户想获取 `data.users[0].name` 这个值，他需要写出这样的代码：

```cpp
// 极其繁琐和不安全的代码
Element* root = Parser::parse(...)->get();
std::string name = 
    root->as_object()
    ->get("data")->as_object()
    ->get("users")->as_array()
    ->at(0)->as_object()
    ->get("name")->as_value()
    ->as_str();
```

这简直是一场灾难！充满了丑陋的类型转换和函数调用。我们需要一层优雅的封装来简化这个过程。

### 5.1 `Ref` 类 - 用户的友好界面

`Ref` 类是一个轻量级的代理（Proxy），它包装了底层的 `Element*` 指针，并重载了 `operator[]`，从而实现了优雅的链式调用。

```cpp
// json_ref.hpp
class Ref {
private:
    Element* m_ptr;

public:
    Ref(Element* p_ptr = nullptr) : m_ptr(p_ptr) {}

    // 核心：重载 operator[] 以支持链式访问
    Ref operator[](const std::string& p_key) {
        if (!m_ptr) throw NullPointerException("Null reference");
        if (auto obj = dynamic_cast<Object*>(m_ptr))
            return Ref(obj->get(p_key)); // 返回一个新的 Ref 包装子元素
        throw TypeException("Not an object");
    }

    Ref operator[](size_t index) {
        if (!m_ptr) throw NullPointerException("Null reference");
        if (auto arr = dynamic_cast<Array*>(m_ptr))
            return Ref(arr->at(index));
        throw TypeException("Not an array");
    }

    // 提供便捷的取值方法，隐藏 as_value() 和 as_T() 的细节
    int as_int() const {
        if (m_ptr && m_ptr->is_value())
            return m_ptr->as_value()->as_int();
        throw TypeException("Not an int value");
    }
    std::string as_str() const { /* ... 类似实现 ... */ }
    // ... 其他 as_... 方法 ...

    // 允许用户获取原始指针，以备不时之需
    Element* get() { return m_ptr; }
};
```

有了 `Ref` 类，之前的灾难代码现在可以写成：

```cpp
Ref root = Parser::parse(...);
std::string name = root["data"]["users"][0]["name"].as_str();
```

#### 设计细节剖析：

1. **接口与实现分离**：<br>
`Ref` 类是典型的接口层。<br>
它为用户提供了一个稳定、简洁、易用的 API，同时将底层复杂的指针操作、`dynamic_cast` 和类型检查完全隐藏了起来。<br>
这是优秀库设计的核心思想。

2. **链式调用**：<br>
`operator[]` 的返回值是另一个 `Ref` 对象，而不是 `Element*`。<br>
这正是实现 `root["data"]["users"]` 这种链式调用的关键。<br>
每一次 `[]` 操作，都会返回一个包装了下一层元素的 `Ref` 对象，使得下一次 `[]` 操作可以继续进行。

## 六、优雅地构建 - 工厂函数的妙用

我们已经拥有了一个功能强大的 JSON 库，但还剩下一个痛点没有解决：**创建 JSON 对象的过程依然很笨拙**。

看看我们现在是怎么做的：

```cpp
// “之前”的痛苦方式
auto obj = Object();
obj.insert("name", "John");
obj.insert("age", 30);

auto arr = new Array();
arr->append("C++");
arr->append("Python");
obj->insert_raw_ptr("courses", arr); // 手动管理 arr 指针

Ref root(&obj);
// ... 使用 root ...
/* 在 obj 被释放之后，
 arr 变为了已经释放内存的指针，不可使用！*/
```

这个过程充满了 `new`、`insert`、`append` 的调用，不仅繁琐，而且要求用户需要关心任何父子关系之间的释放顺序和调用顺序，极易出错。

我们的目标是让构建过程像书写 JSON 字面量一样自然。我们希望的代码是这样的：

```cpp
// 我们想要的“之后”的优雅方式
Ref root = make_object({
    {"name", make_value("John")},
    {"age", make_value(30)},
    {"courses", make_array({
        make_value("C++"),
        make_value("Python")
    })},
    {"address", make_value(nullptr)}
});
// ... 使用 root ...
delete root.get();
// 由于对外只暴露了一个 root，因此不存在除 root 以外的指针被调用的风险，只需要把 root 释放掉即可放心。
```

这种声明式的语法清晰地反映了数据的结构，可读性极高。

为了实现它，我们将引入一系列 **工厂函数 (Factory Functions)**。

### 6.1 核心思想：隐藏 `new` 关键字

工厂函数的核心思想非常简单：<br>
将 `new` 关键字和对象的具体构造过程封装到一个独立的函数里。<br>
用户不再需要关心如何创建对象，只需要调用这个函数，就能得到一个随时可用的成品。

对于我们的库，用户真正关心的类型是 `Ref`，而不是底层的 `Object*`、`Array*` 或 `Value*`。<br>
因此，我们的工厂函数将接收 C++ 的字面量（如 `123`, `"hello"`），并返回一个封装好的 `Ref` 对象。

### 6.2 第一块积木：`make_value`
我们先为最基础的 `Value` 类型创建工厂函数。这是一组重载函数，每一种都对应一种 C++ 的基础类型。

```cpp
// json_ref.hpp (新添加的工厂函数)
Ref make_value(std::nullptr_t) { return Ref(new Value(nullptr)); }
Ref make_value(bool p_val) { return Ref(new Value(p_val)); }
Ref make_value(int p_val) { return Ref(new Value(p_val)); }
Ref make_value(float p_val) { return Ref(new Value(p_val)); }
Ref make_value(const string_t& p_val) { return Ref(new Value(p_val)); }
Ref make_value(char* p_val) { return Ref(new Value((string_t)p_val)); }
```

#### 设计细节剖析：
1. **封装**：<br>
`new Value(...)` 这个动作被完美地隐藏在了函数内部。<br>
用户现在只需要和 `make_value` 打交道，代码变得更干净。

2. **统一接口**：<br>
无论你想创建哪种类型的 `Value`，函数名都是 `make_value`。<br>
C++ 的函数重载机制会根据你传入的参数类型，自动选择正确的版本。

### 6.3 构建容器：`make_array` 和 `make_object`
现在，我们利用 `std::initializer_list` 来为容器类型创建工厂。

**对于 `Array`**：<br>
我们需要一个函数，它能接收一个由 `Ref` 对象组成的初始化列表 `{...}`。

```cpp
// json_ref.hpp
Ref make_array(std::initializer_list<Ref> p_list)
{
    auto arr = new Array();
    for (const auto& v : p_list) {
        // 从 Ref 中获取原始指针，并添加到新 Array 中
        arr->append_raw_ptr(v.get());
    }
    return Ref(arr);
}
```

**对于 `Object`**：<br>
逻辑类似，但初始化列表的元素变成了键值对 `{"key", value}`，在 C++ 中对应 `std::pair`。

```cpp
// json_ref.hpp
Ref make_object(std::initializer_list<std::pair<string_t, Ref>> p_list)
{
    auto obj = new Object();
    for (auto& kv : p_list) {
        // 从 key-value pair 中获取 key 和 Ref 里的原始指针
        obj->insert_raw_ptr(kv.first, kv.second.get());
    }
    return Ref(obj);
}
```

#### 设计细节剖析：

1. **`std::initializer_list` 的魔力**：<br>
这个 C++ 特性是让 `{...}` 语法工作的关键。<br>
它允许我们向函数传递一个临时的、匿名的元素列表。

2. **组合的力量**：<br>
这套工厂函数可以完美地嵌套组合。<br>
`make_object` 的值可以是另一个 `make_array` 的结果，而 `make_array` 的元素又可以是 `make_value` 的结果。<br>
这种组合能力，让我们能够以声明式的方式构建出任意复杂的 JSON 树。

### 6.4 一个重大的设计权衡：内存管理

我们的新 API 非常优雅，但它引入了一个必须高度警惕的问题。

让我们再看一下 `make_object` 的实现：<br>
`auto obj = new Object();`<br>
这个函数在堆上创建了一个对象，然后返回一个**只包含裸指针 (`Element*`)** 的 `Ref`。

**`Ref` 类本身并不知道它需要管理这个指针的生命周期！**

这意味着什么？当你这样写代码时：

```cpp
void create_and_leak_json() {
    Ref root = make_object({ {"key", make_value("value")} });
    /* 当函数结束，root 对象被销毁。
       但是，它内部的那个 Element* 指针所指向的内存，
       从未被 delete！这里发生了内存泄漏！*/
}
```

**这是一个非常严肃的问题。** 这个设计选择将内存管理的责任完全交还给了**最终用户**。

正确的使用姿势是：

```cpp
// 1. 创建 JSON 树
Ref root = make_object({
    {"name", make_value("John")},
    {"age", make_value(30)}
});

// 2. 使用 JSON 对象
std::cout << root["name"].as_str() << std::endl;

// 3. **手动释放整个树的内存！**
delete root.get();
```

你只需要 `delete` 根节点的指针。<br>
因为我们之前设计的 `Object` 和 `Array` 的析构函数是虚函数，<br>
并且会递归地 `delete` 它们所拥有的所有子元素，<br>
所以 `delete root.get()` 会安全地清理掉整个 JSON 树所占用的所有内存。

**总结这个权衡**：

- **优点**：<br>我们获得了一个极其简洁、富有表现力的构建 API。<br>
`Ref` 类保持了其作为轻量级“引用”或“视图”的简单性。<br>
同时还避免了误用已释放指针、忘记释放子节点内存这两种常见的内存管理问题。

- **代价**：<br>
牺牲了自动内存管理 (RAII)。<br>
库的使用者必须时刻记住，他们通过工厂函数创建的任何 JSON 对象树，都必须在用完后手动 `delete`。

对于一个追求极致安全的现代 C++ 库来说，这可能不是最终的完美方案<br>
（一个更健壮的方案是让 `Ref` 变成一个类似 `std::unique_ptr` 的智能指针包装器）。

但作为一个设计决策，它清晰地展示了在 **“易用性”** 和 **“安全性”** 之间存在的微妙平衡。

通过理解这一点，你对软件设计的理解又深入了一层。

## 七、性能的飞跃 - 为你的 JSON 王国建造高速公路 (对象池)

我们的 JSON 库到目前为止，功能已经非常完备。但在一个追求性能的世界里，“能用”只是起点，“够快”才是王道。现在，我们将着手进行一次激动人心的性能优化，其效果立竿见影，并能让你的库达到工业级的性能水准。

### 7.1 发现瓶颈：`new` 和 `delete` 的无情开销

想象一下解析一个包含成千上万个键值对的大型 JSON 文件。我们的 `Parser` 每遇到一个数字、一个字符串、一个数组或一个对象，都会执行一次 `new` 操作。

```cpp
new Value(123);
new Object();
new Array();
```

`new` 操作看起来很简单，但它背后隐藏着一系列重量级的动作：<br>
它需要向操作系统请求堆内存，这个过程可能涉及到线程锁定、复杂的内存寻址算法，对于频繁创建和销毁大量小对象的场景（比如我们的 JSON 解析器），这会成为一个巨大的性能瓶颈。

我们能否绕过这个瓶颈？答案是肯定的。我们将采用一种经典的高性能内存管理模式：对象池 (`ObjectPool`)。

### 7.2 解决方案：批发代替零售 (对象池原理)

对象池的思想非常直观：

1. **预先批发**：我们不再在需要时才向系统零散地申请内存，而是一开始就申请一大块内存，或者按照固定的“块(Block)”来批量申请。

2. **内部零售**：当需要一个新对象时，我们从预先申请好的内存中快速切出一小块来使用。这个过程通常只是移动一下指针，速度极快，完全避免了与操作系统的昂贵交互。

3. **循环利用**：当一个对象被 `delete` 时，我们不把它归还给操作系统，而是将其回收，放回到池中，以备下次分配时循环使用。

### 7.3 第一步：解耦设计 - 灵活的 `Allocator` 策略

不同的使用场景可能需要不同的内存分配策略。<br>
比如，解析器可能最适合一次性分配、永不回收的“区块分配器”，而其他场景可能需要能高效回收和重用内存的“自由列表分配器”。

为了实现这种灵活性，我们首先定义一个抽象的 `Allocator` 接口，然后创建几种具体的实现。这正是设计模式中的 **策略模式 (Strategy Pattern)**。

```cpp
// allocator.hpp
// 1. 定义策略接口 (这里使用了虚函数实现动态多态)
template <typename T>
class Allocator 
{
public:
    virtual ~Allocator() = default;

    virtual T* allocate(size_t n) = 0;
    virtual void deallocate(T* ptr) = 0;
};

// 2. 策略A：默认分配器 (MallocAllocator)
// 一般情况下 C++ 内部的默认实现
template <typename T>
class MallocAllocator : public Allocator<T> 
{
public:
    T *allocate(size_t n)
    {
        void *raw = ::malloc(n);
        if (!raw)
            throw std::bad_alloc();
        return reinterpret_cast<T *>(raw);
    }

    void deallocate(T *ptr)
    {
        ::free(ptr);
    }
};

// 3. 策略B：区块分配器 (BlockAllocator)
// 这是最高效的策略之一，适合解析这种一次性大量分配的场景
template <typename T, size_t BlockSize = 1024>
class BlockAllocator : public Allocator<T> 
{
private:
    std::vector<void*> blocks;
    T* currentBlock = nullptr;
    size_t remaining = 0;

public:
    ~BlockAllocator() 
    {
         // 释放所有 blocks
        for (void *block : blocks)
            ::free(block);
    }

    T* allocate(size_t n) override
    {
        if (remaining == 0)
        {
            // 当前块用完了，就去批发一大块新的
            void* block = ::malloc(sizeof(T) * BlockSize);
            blocks.push_back(block);
            currentBlock = reinterpret_cast<T*>(block);
            remaining = BlockSize;
        }

        // 从当前块中快速切出一个对象的位置
        T* obj = currentBlock++;
        --remaining;
        return obj;
    }

    void deallocate(T* ptr) override
    {
        // 在这个策略里，我们通常什么都不做，等到最后统一释放所有 blocks
    }
};

// 4. 策略C：自由列表分配器 (FreeListAllocator)
// 这个策略能高效地回收和重用被释放的对象
template <typename T>
class FreeListAllocator : public Allocator<T> 
{
    struct Node
    {
        Node *next;
    };
    Node *freeList = nullptr;

public:
    ~FreeListAllocator()
    {
        // 等到析构的时候再统一释放链表内的内存
        while (freeList)
        {
            Node *tmp = freeList;
            freeList = freeList->next;
            ::free(tmp);
        }
    }

    T *allocate(size_t n)
    {
        // 对已经回收的内存链表再利用
        if (freeList)
        {
            Node *node = freeList;
            freeList = node->next;
            return reinterpret_cast<T *>(node);
        }
        void *raw = ::malloc(n);
        if (!raw)
            throw std::bad_alloc();
        return reinterpret_cast<T *>(raw);
    }

    void deallocate(T *ptr)
    {
        // 每次回收内存并不释放，而是加入链表
        Node *node = reinterpret_cast<Node *>(ptr);
        node->next = freeList;
        freeList = node;
    }
};
```

#### 设计细节剖析：

- 抽象与实现分离：<br>
`Allocator` 基类定义了“做什么”（分配/释放），<br>
而 `BlockAllocator` 等子类则定义了“怎么做”。<br>
这使得我们的上层代码（对象池）可以与具体的分配策略解耦。

- `BlockAllocator` 的智慧：<br>
这个分配器是性能优化的关键。<br>
它的 `allocate` 函数几乎只做了几次指针运算，没有任何复杂的逻辑。<br>
它的 `deallocate` 是空操作，因为它的哲学是“只进不退”，所有内存都在最后统一释放，这完美匹配了 JSON 解析器的生命周期。

### 7.4 第二步：构建 `ObjectPool`

`ObjectPool` 是一个简单的封装，它持有一个我们选择的 `Allocator` 策略实例，并向外提供分配/释放的接口。

```cpp
// object_pool.hpp
template <typename T, typename Alloc = BlockAllocator<T>>
class ObjectPool
{
private:
    Alloc m_allocator;

public:
    T* allocate(size_t n) { return m_allocator.allocate(n); }
    void deallocate(void* ptr) 
    { m_allocator.deallocate(static_cast<T*>(ptr)); }
};
```

#### 设计细节剖析：

- 编译期策略选择：<br>
通过模板参数 `Alloc = BlockAllocator<T>`，我们在编译期就决定了对象池将使用哪种分配策略。<br>
这比在运行时传入一个 `Allocator*` 指针效率更高，因为它避免了虚函数调用，为编译器内联优化打开了大门。

实际上，为了彻底避免虚函数调用，`Allocator` 基类完全可以省去。<br>
它的教学作用已经完成，当时在子类里写上的 `override` 可以借助编译器的检查来避免虚函数接口参数对不上。<br>
其他类型由于拥有自己的 `allocate` 和 `deallocate` 函数，即使删去基类也能在编译期自动确定下策略。

### 7.5 第三步：无缝集成 - `operator new/delete` 的重载魔法

现在我们有了 `ObjectPool`，如何让 `new Value(...)` 这样的代码自动地从我们的池中获取内存，而不是从全局堆中获取呢？<br>
答案是：重载 `operator new` 和 `operator delete`。

这是 C++ 赋予我们的强大能力，可以直接改变一个类的内存分配行为。

```cpp
// json_value.hpp
// 在 Value 类中
class Value : public Element {
private:
    // 关键1：每个类拥有一个静态的、自己的对象池
    static ObjectPool<Value> pool;
    value_t m_value;

public:
    // ... 构造函数等 ...

    // 关键2：重载 new 和 delete
    void* operator new(std::size_t n) {
        // 当写 new Value(...) 时，这个函数会被调用来获取内存
        return Value::pool.allocate(n);
    }

    void operator delete(void* ptr) {
        // 当写 delete value_ptr 时，这个函数会被调用来释放内存
        Value::pool.deallocate(ptr);
    }
};
// 关键3：在类外定义并初始化静态成员
inline ObjectPool<Value> Value::pool;

// 对 Array 和 Object 类做完全相同的改造
/* ... */
```

#### 设计细节剖析：

- **透明的魔法**：<br>
这次重构最美妙的地方在于它的透明性。<br>
我们不需要去修改 `Parser` 内部成百上千处 `new Value(...)` 的代码。我们只是改变了 `new` 这个关键字对于 `Value`、`Array` 和 `Object` 的底层含义。<br>
现在，`new Value` 不再是请求系统内存，而是向 `Value::pool` 请求一个对象。

- **静态对象池**：<br>
每个 `Element` 子类（`Value`, `Array`, `Object`）都拥有一个属于自己的 `static` 对象池。<br>
这意味着所有的 `Value` 对象都从同一个池中分配，所有的 `Array` 对象从另一个池中分配。<br>
这种分离管理使得内存布局更规整，也更符合对象池的设计初衷。

- **`inline` 关键字 (C++17+)**：<br>
在头文件中直接定义并初始化一个静态成员变量，需要使用 `inline` 关键字，以防止在多个编译单元中包含此头文件时出现链接错误。<br>
这是一个现代 C++ 的便利特性。

### 7.6 成果与展望

我们成功了！通过引入一套灵活的 Allocator 策略，并利用 ObjectPool 和 operator new/delete 重载，我们将 JSON 库的性能提升到了一个全新的高度。

#### 我们获得了什么？

1. **极高的性能**：<br>
解析和构建过程中的内存分配速度得到了数量级的提升。

2. **优雅的实现**：<br>
性能的提升几乎没有“污染”我们的核心解析逻辑代码，`Parser` 依然可以干净地写着 `new Value`。

3. **高度的灵活性**：<br>
如果未来我们发现 `BlockAllocator` 不适合某个场景，我们只需要在 `ObjectPool` 的模板参数里换上 `FreeListAllocator`，就可以轻松切换内存管理策略，而无需改动任何其他代码。

这次重构不仅仅是一次性能优化，它更是一次深刻的架构设计实践。<br>
它向我们展示了如何通过抽象、策略模式和 C++ 的底层语言特性，来构建一个既高性能又易于维护的复杂系统。

我们的 JSON 王国，现在不仅宏伟，而且拥有了四通八达的高速公路。

## 八、最终前沿 - 驶向并发，构建多线程解析器

我们的 JSON 王国至今已是宏伟壮丽，性能卓越。但在追求极致的道路上，我们还有最后一张王牌尚未打出——并发。

在单核性能已近极限的今天，解锁多核 CPU 的并行处理能力，是让我们的库产生质的飞跃的关键。

在这一章，我们将进行最激动人心的改造：<br>
将 `Tokenizer` (生产者) 和 `Parser` (消费者) 分置于不同的线程，让它们像一条高效的工厂流水线，协同完成 JSON 的解析工作。

### 8.1 蓝图：生产者-消费者流水线

我们的并发模型基于一个经典而强大的设计模式——**生产者-消费者 (Producer-Consumer)**。

1. **生产者 (The Producer)**：<br>
一个专门的 `Tokenizer` 线程。<br>
它的职责只有一个：心无旁骛地读取原始字符串，将其切割成 `Token` 这种“半成品”。

2. **传送带 (The Conveyor Belt)**：<br>
一个 **线程安全** 的 `Channel`。<br>
它负责将生产好的 `Token` 从生产者安全、有序地运送到消费者那里。

3. **消费者 (The Consumer)**：<br>
主线程或另一个专门的 `Parser` 线程。<br>
它的职责是从传送带上取下 `Token`，并将其组装成最终的 `Element` 成品。

这种设计的巨大优势在于，当 `Parser` 正在进行 CPU 密集型的树结构组建时 (`switch`, `new`, `insert`)，`Tokenizer` 可以在另一个 CPU 核心上同时进行词法分析，两者互不等待，极大地提升了整体的吞吐量。

### 8.2 传送带的设计：**线程安全** 的 `Channel`

要让流水线安全运转，我们需要一个特制的“传送带”，它必须保证在任何时候只有一个工人（线程）能操作它。<br>
这就是线程安全的 `Channel` (有时也叫 **阻塞队列**)。

它的核心是使用 `std::mutex` (互斥锁) 和 `std::condition_variable` (条件变量)。

```cpp
// channel.hpp
template <typename T>
class Channel
{
private:
    size_t m_capacity;
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond_not_empty;
    std::condition_variable m_cond_not_full;

public:
    // 构造函数，析构函数...

    void push(const T& p_item)
    {
        std::unique_lock<std::mutex> lock(m_mutex); // 1. 上锁
        // ... 如果队列有容量限制，则解锁并休眠，在这里等待 ...（m_capacity 为 0 时不限制队列内元素数）
        m_cond_not_full.wait(
            lock, [&]()
            { return m_capacity == 0 || m_queue.size() < m_capacity; });
        m_queue.push(p_item);
        lock.unlock(); // 2. 解锁
        m_cond_not_empty.notify_one(); // 3. 唤醒可能在等待的消费者
    }

    void pop()
    {
        std::unique_lock<std::mutex> lock(m_mutex); // 1. 上锁
        // ... 如果队列为空，则解锁并休眠，直到被唤醒 ...
        m_cond_not_empty.wait(
            lock, [&]()
            { return !m_queue.empty(); });
        m_queue.pop();
        lock.unlock(); // 2. 解锁
        m_cond_not_full.notify_one(); // 3. 唤醒可能在等待的生产者
    }
    
    // 我们还需要一个 peek 方法，用于“偷看”但不取出
    T peek()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond_not_empty.wait(lock, [&](){ return !m_queue.empty(); });
        T item = m_queue.front(); // 注意：这里返回的是拷贝
        lock.unlock(); // 立即解锁
        return item;
    }
};
```

#### 设计细节剖析：

- **`std::mutex`**：<br>
像是一把钥匙。任何线程想要操作队列，必须先拿到这把钥匙。

- **`std::condition_variable`**：<br>
一个高效的“等待室”。<br>
当消费者发现队列是空的，它不会疯狂地空转浪费 CPU，而是进入这个等待室“睡觉”，直到生产者 `push` 了新东西后通过 `notify_one()` 把它“叫醒”。

- **`peek()` 的实现**：<br>
为了绝对的安全，避免返回一个可能很快就失效的内部引用，我们的 `peek()` 方法返回一个 `Token` 的拷贝。<br>
对于 `Token` 这样的小对象，这个开销是值得的。

### 8.3 架构演进：`Parser` 成为并发“总指挥”

要使用 `Channel`，我们的 `Parser` 对象的角色必须再次进化。

它不再直接与 `Tokenizer` 对话，而是作为“总指挥”，通过 `Channel` 与在另一个线程的 `Tokenizer` 协同工作。

```cpp
// parser.hpp (Parser 的新设计)
class Parser 
{
private:
    // Parser 现在拥有自己的“传送带” (Channel)
    Channel<Token> m_channel; 
    Tokenizer* m_tokenizer; // 它需要知道数据源来启动生产者

public:
    Parser(Tokenizer* p_tokenizer, size_t capacity = 128)
        : m_tokenizer(p_tokenizer), m_channel(capacity) {}

    Ref parse() { /* ... 启动并协同线程 ... */ }

private:
    // 内部的 peek/consume 现在直接与传送带交互
    Token peek() { return m_channel.peek(); }
    void consume() { m_channel.pop(); }

    // 核心解析逻辑
    Element* parse_value() { /* ... */ }
};
```

#### 设计细节剖析：

1.  **状态的内聚**：<br>
`Parser` 对象现在封装了并发解析所需的**全部状态**：`m_tokenizer` (数据源) 和 `m_channel` (线程间通信媒介)。<br>
这使得并发的复杂性被控制在 `Parser` 内部，对外部调用者透明。

2.  **数据源的切换**：<br>
`Parser` 内部的 `peek()` 和 `consume()` 方法，现在的数据源从 `m_tokenizer` **无缝切换** 到了 `m_channel`。<br>
这是至关重要的一步，它意味着 `Parser` 的核心解析逻辑，现在开始消费一个来自 **另一个线程** 的、异步的数据流。

### 8.4 `parse()` 方法：启动并协同两个线程

`Parser` 的 `parse()` 成员函数是整个并发大戏的“开场导演”。

它负责创建生产者线程，并让自己（主线程）成为消费者，然后确保两者能正确地协同工作直到结束。

```cpp
// 在 Parser 类中
Ref parse() 
{
    // 第一幕：创建并启动生产者线程
    std::thread producer_thread(
        [&]() 
        {
        while (true) 
        {
            const Token& token = m_tokenizer->peek();
            m_channel.push(token); // 生产一个 Token，放入传送带

            if (token.type == TokenType::End) break;
            
            m_tokenizer->consume(); // 生产者推进自己的状态
        }
    });

    // 第二幕：主线程作为消费者，开始解析工作
    Ref root = Ref(parse_value());

    // 最终幕：等待生产者线程结束
    producer_thread.join();

    return root;
}
```

#### 设计细节与正确性剖析：

1. **职责的严格分离 (至关重要!)**：<br>
这是并发正确的基石。<br>
生产者 (`producer_thread`) 只与 `m_tokenizer` 和 `m_channel.push` 交互。<br>
消费者 (`Parser` 主线程) 只与 `m_channel` 的 `peek/pop` 交互。<br>
两者各司其职，互不干涉对方的状态推进逻辑。

2. **`End` Token：流水线的“关机信号”**：<br>
生产者必须将 `End` Token 也推入 `Channel`。<br>
这是它告诉 `Parser` “所有货物都已送达，准备关机”的唯一信号。

3. **`thread.join()`：确保善始善终**：<br>
在 `parse()` 函数的末尾调用 `producer_thread.join()` 是必须的。<br>
它会阻塞主线程（消费者），直到生产者线程完全执行完毕。<br>
这可以防止 `parse()` 函数提前返回，导致 `Parser` 对象（及其成员 `m_channel` 和 `m_tokenizer`）被销毁，而此时生产者线程可能还在访问它们，从而引发程序崩溃。

4. 接口的优雅不变性：<br>
这次惊心动魄的并发改造，最令人赞叹的一点是什么？是我们几乎没有修改 `parse_value()`、`parse_object()`、`parse_array()` 这些核心解析函数的内部逻辑！<br>
因为我们之前将它们构建于 `peek()/consume()` 这个抽象接口之上，现在我们只是改变了 `peek()/consume()` 的底层实现（从直接访问 `Tokenizer` 变为访问 `Channel`），而上层的语法解析逻辑因为与数据源解耦，得以保持稳定。<br>
这是优秀架构设计的力量的完美体现。

### 总结

我们成功了！通过引入一个线程安全的 `Channel`，我们将 `Parser` 重构为了一个真正的并发协调器。

这次改造不仅仅是为了追求极致的性能，它更是一次深刻的软件架构实践，教会了我们：

- 如何将一个复杂的单线程流程，分解为可并行执行的、职责单一的单元。
- 如何通过线程安全的队列，在不同执行单元之间建立可靠的通信。
- 如何设计具有前瞻性的接口 (`peek/consume`)，使得系统在面对重大的底层变更（从同步到异步）时，核心逻辑依然能保持稳定。

我们的 JSON 王国，现在不仅宏伟、高效，更拥有了处理未来海量数据的、可扩展的并行处理能力。然而，在追求极致性能的道路上，这只是第一步。

## 九、为并发引擎减负 - 从阻塞到飞跃 (无锁队列)

我们的并发流水线已经能正确运转了。但如果你进行性能测试，可能会惊讶地发现：**它甚至比单线程版本还要慢！**

问题出在哪里？答案是：我们的“传送带” (`Channel`) 虽然安全，但太“重”了。

### 9.1 性能诊断：昂贵的“安保系统”

`Channel` 内部的 `std::mutex` 和 `std::condition_variable` 就像一个极其负责但手续繁琐的“安保系统”。

`Tokenizer` 每生产一个小小的 `Token`，就要经历一次“加锁 -> 等待 -> 推送 -> 解锁 -> 通知”的完整流程。<br>
当 `Token` 的生产和消费速度非常快时，线程的大部分时间都耗费在了等待锁、线程上下文切换和内核交互上，而不是实际的解析工作。

对于 **单生产者-单消费者 (SPSC)** 这个特定场景（这正是我们的场景！），我们可以使用一种终极武器来彻底抛弃这个笨重的安保系统——**无锁环形缓冲区 (Lock-Free Ring Buffer)**。

### 9.2 终极武器：`LockFreeRingBuffer`

无锁队列是并发编程的圣杯之一。

它不使用任何锁，只通过 CPU 保证的原子操作 (atomic operations) 来同步线程。这意味着线程之间几乎没有阻塞和等待，数据传递的延迟可以达到纳秒级别。

```cpp
// lock_free_ring_buffer
template <typename T>
class LockFreeRingBuffer
{
private:
    size_t m_capacity;
    std::vector<T> m_buffer;
    // **关键：头尾指针都是原子变量**
    std::atomic<size_t> m_head{0};
    std::atomic<size_t> m_tail{0};

public:
    LockFreeRingBuffer(size_t p_capacity = 256) 
        : m_capacity(p_capacity), m_buffer(p_capacity) {}

    // 生产者只操作 tail
    bool push(const T& p_item)
    {
        size_t tail = m_tail.load(std::memory_order_relaxed);
        size_t next_tail = (tail + 1) % m_capacity;
        // 如果尾指针追上了头指针，说明队列满了
        if (next_tail == m_head.load(std::memory_order_acquire))
            return false; // 非阻塞，立即返回失败
        m_buffer[tail] = p_item;
        // 魔法：通过 release 语义，确保写入的数据对消费者可见
        m_tail.store(next_tail, std::memory_order_release);
        return true;
    }

    // 消费者只操作 head
    bool pop() { /* ... 通过原子操作移动 head ... */ }
    std::optional<T> peek() { /* ... 通过原子操作读取 head 位置的数据 ... */ }
};
```

#### 设计细节剖析：

-   **无锁 (Lock-Free)**：<br>
整个过程中没有任何 `mutex`。<br>
线程之间通过 `std::atomic` 变量直接在硬件层面进行同步，避免了昂贵的操作系统内核调用。

-   **内存序 (Memory Ordering)**：<br>
`acquire` 和 `release` 是这里的核心魔法。<br>
它是一种内存屏障，精确地告诉 CPU 如何同步不同核心之间的缓存，确保生产者写入的数据能被消费者正确地、不乱序地看到。

-   **非阻塞与忙等**：<br>
无锁队列的 `push` 和 `pop` 通常是 **非阻塞** 的。<br>
如果队列满了或空了，它们会立即返回失败。<br>
这意味着使用者需要在一个循环中“忙等” (Busy-Waiting)，不断尝试操作直到成功。

#### 9.3 最后的改造：为 `Parser` 换上“F1 引擎”

现在，我们将 `Parser` 内部的“传送带”从 `Channel` 替换为 `LockFreeRingBuffer`。

```cpp
// parser.hpp (Parser 的最终形态)
class Parser 
{
private:
    // **传送带升级！**
    LockFreeRingBuffer<Token> m_buffer; 
    Tokenizer* m_tokenizer;

public:
    Parser(Tokenizer* p_tokenizer, size_t capacity = 256)
        : m_tokenizer(p_tokenizer), m_buffer(capacity) {}

    Ref parse() {
        std::thread producer_thread([&]()
        {
            while (true) {
                Token token = m_tokenizer->peek();
                // 关键修正1：处理非阻塞 push
                // 如果 push 失败，就循环等待，直到成功
                while (!m_buffer.push(token)) std::this_thread::yield(); // 让出时间片
                if (token.type == TokenType::End) break;
                m_tokenizer->consume();
            }
        });

        Ref root = Ref(this->parse_value());
        producer_thread.join();
        return root;
    }
private:
    // 关键修正2：处理非阻塞 peek 和 pop
    Token peek()
    {
        std::optional<Token> token;
        // 如果 peek 失败 (缓冲区为空)，就循环等待
        while (!(token = m_buffer.peek()).has_value()) std::this_thread::yield(); // 让出时间片
        return token.value();
    }
    void consume() 
    {
        // 如果 pop 失败 (缓冲区为空)，就循环等待
        while (!m_buffer.pop()) std::this_thread::yield(); 
    }
    // ... 核心解析逻辑 parse_value 等完全不变！
};
```

#### 设计细节剖析：

1. **处理非阻塞**：<br>
从阻塞队列切换到非阻塞队列，最大的改变就是我们需要在调用点手动处理操作失败的情况。<br>
通过 `while` 循环和 `yield` (或更高效的 `sleep_for`)，我们实现了一种“自旋等待”，在不引入锁的情况下实现了线程同步。

2. **接口的胜利**：<br>
再次强调，由于我们核心的 `parse_value`, `parse_object`, `parse_array` 函数只依赖于 `peek()` 和 `consume()` 这两个我们自己定义的接口，<br>
这次从有锁到无锁的巨大底层变更，依然没有对它们产生任何冲击。这就是抽象的力量！

### 总结

我们完整地走过了一条从单线程到并发，再到高性能并发的演进之路。

这次旅程不仅仅是代码的升级，更是设计思想的升华：

1. **并发初探**：通过有锁的 **Channel**，我们成功地搭建了生产者-消费者模型，理解了线程同步、互斥锁和条件变量的基本原理。

2. **性能瓶颈**：我们亲身体会到，不恰当的并发控制（过多的锁竞争）反而会降低性能，并学会了定位瓶颈。

3. **终极形态**：通过引入无锁队列，我们掌握了在特定场景下（SPSC）实现极致性能的尖端技术，并理解了原子操作和内存序等底层概念。

你的 JSON 库现在已经脱胎换骨，它不仅功能完备、架构优美，更拥有了一颗真正为现代多核处理器而生的、风驰电掣的“F1 引擎”。

## 十、总结与展望

恭喜你！我们从零开始，完整地经历了一个 JSON 解析器的诞生全过程：

1. **数据建模**：通过抽象基类 `Element` 和多态，设计了一套能够表示任意 JSON 结构的 C++ 对象模型。
2. **内存管理**：深入实践了手动内存管理，理解了 `new/delete` 的所有权和责任。
3. **词法分析**：实现了 `Tokenizer`，将原始字符串流分解为结构化的 `Token`。
4. **语法分析**：利用递归下降的思想，实现了 `Parser`，将 `Token` 流组装成 `Element` 对象树。
5. **API 封装**：设计了 `Ref` 类，提供了一个安全、优雅、易用的用户接口。
6. **决策设计**：用更安全的、只需要管理根节点内存的工厂方法取代了原来手动构建的方式，体验了易用性和安全性设计之间的取舍问题。

这个项目虽小，但它涵盖了从底层设计到上层封装的方方面面。它不仅仅是一个工具，更是一次宝贵的项目实践。

### 下一步可以做什么？

- 升级内存管理：将项目中的裸指针 `Element*` 全部替换为 `std::unique_ptr<Element>`，拥抱现代 C++ 的 RAII 思想，消除内存泄漏的风险。

- 性能优化：`parse_number` 和 `parse_string` 等函数可以通过更底层的算法进行优化，避免过多的字符串拷贝。

- 并行化算法：`tokenizer`、`Parser` 和 `InputStream` 之间的逻辑分离，可以放在不同的线程里并行加速。

希望这次从零到一的旅程，能点燃你对项目开发的激情，并为你未来的编程之路打下坚实的基础。去创造吧！

源码已经放在了 [github 仓库](https://github.com/pjh456/Let_us_build_cpp_json_parser) 中，如果这篇博客对你有帮助，可以给仓库点个 star 支持一下！