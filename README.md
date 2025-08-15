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

`Tokenizer` 的核心是一个 `next()` 方法，每次调用，它就从输入流中读取并返回下一个有效的 `Token`。

```cpp
// tokenizer.hpp

// InputStream 是一个辅助类，封装了字符串和当前读取位置，便于操作
class InputStream { /* ... get(), peek(), eof() ... */ };

class Tokenizer
{
private:
    InputStream stream;
    char current;
    size_t line;
    size_t column;

    // 为了解析 Array，这里需要提前读入下一个 Token
    Token current_token;
    Token next_token;

    // 是否下一个位置输入结束
    bool is_next_end = true;

public:
    Tokenizer(const InputStream& stream) : stream(stream), line(1), column(1) { next(); }

    const Token now() const noexcept { return current_token; }
    const Token forward() const noexcept { return next_token; }

    Token next()
    {
        skip_white_space(); // 第一步：跳过所有无意义的空白符

        if (stream.eof())
        {
            if (is_next_end)
                return is_next_end = false, current_token = next_token;
                // 第一次读完是 next_token，第二次才是真正读完
            else
                return {TokenType::End, ""}; // 如果读完了，返回 End Token
        }

        current = stream.get(); // 读取当前字符
        column++;

        // 每次都需要下一个，因此只要把 current 设为 next 即可
        current_token = next_token;

        // 第二步：根据当前字符，判断下一个 Token 类型
        switch (current)
        {
        case '{':
            next_token = {TokenType::ObjectBegin, "{"};
            break;
        case '}':
            next_token = {TokenType::ObjectEnd, "}"};
            break;
        case '[':
            next_token = {TokenType::ArrayBegin, "["};
            break;
        case ']':
            next_token = {TokenType::ArrayEnd, "]"};
            break;
        case ':':
            next_token = {TokenType::Colon, ":"};
            break;
        case ',':
            next_token = {TokenType::Comma, ","};
            break;
        // 第三步：对于复杂类型，调用专门的解析函数
        case '"':
            next_token = parse_string();
            break;
        case 't':
        case 'f':
            next_token = parse_bool();
            break;
        case 'n':
            next_token = parse_null();
            break;
        default:
            if (isdigit(current) || current == '-')
                next_token = parse_number();
            else
                // 第四步：遇到未知字符，抛出带有位置信息的异常
                throw ParseException(line, column, (std::string) "Unexpected character '" + std::string(1, current) + "'");
            break;
        }

        return current_token;
    }

private:
    // 跳过空格、制表符、换行符
    void skip_white_space()
    { 
        while (
            !stream.eof() &&
            (
                stream.peek() == '\n' ||
                stream.peek() == '\t' ||
                stream.peek() == ' '
            )
        )
        {
            current = stream.get();
            column++;
            if (current == '\n')
                line++, column = 1;
        }
    }

    // 指定读取的下一个字符值，用于处理关键字
    void expect(char nextChar)
    {
        if ((current = stream.get()) != nextChar)
            throw ParseException(
                line, 
                column, 
                (std::string) "Unexpected character '" + std::string(1, current) + "'"
                );
        column++;
    }

    // 从 " 开始，一直读到下一个 "
    Token parse_string()
    {
        std::string _str;
        while (stream.get(current))
        {
            column++;
            if (current == '"')
                break;
            if (current == '\\')
                stream.get(current);
            _str.push_back(current);
        }
        return {TokenType::String, _str};
    }

    // 从数字或'-'开始，读取一个完整的整数或浮点数
    Token parse_number()
    { 
        std::string _str;
        bool is_float = false;

        _str.push_back(current);
        while (
            isdigit(stream.forward()) || 
            stream.forward() == '.' || 
            stream.forward() == '-'
            )
        {
            stream.get(current);
            is_float = is_float || current == '.';
            _str.push_back(current);

            column++;
        }

        if (isdigit(stream.peek()))
            _str.push_back(stream.get());

        return {is_float ? TokenType::Float : TokenType::Integer, _str}
    }

    // 读取并验证 true 或 false
    Token parse_bool() 
    {
        if (current == 't')
        {
            expect('r'), expect('u'), expect('e');
            return {TokenType::Bool, "true"};
        }
        else
        {
            expect('a'), expect('l'), expect('s'), expect('e');
            return {TokenType::Bool, "false"};
        }
    }

    // 读取并验证 null
    Token parse_null()
    {
        expect('u'), expect('l'), expect('l');
        return {TokenType::Null, "null"};
    }
};
```

#### 设计细节剖析：

1. **关注点分离**：<br>
`Tokenizer` 的设计完美体现了“单一职责原则”。<br>
它不关心 JSON 的语法结构（比如一个 key 后面是否必须跟一个冒号），它只关心如何准确地识别出每一个独立的词法单元。<br>
这种解耦使得代码更清晰，也更容易维护。

2. **状态机思想**：<br>
`next()` 方法本质上是一个简单的状态机。<br>
它读取一个字符，然后根据这个字符转换到不同的处理逻辑（返回一个简单的 `Token`，或者进入一个更复杂的 `parse_...` 函数）。

3. 详细的错误报告：<br>
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

#### 5.1 `Parser` 类 - 静态方法的集合

我们的 `Parser` 将是一个纯静态类，因为它不需要存储任何状态，它的工作就是接收一个 `Tokenizer`，然后输出一个 `Element*`

```cpp
// parser.hpp

class Parser
{
public:
    // 总入口（这里的 Ref 是之后实现的指针封装）
    static Ref parse(Tokenizer* tokenizer)
    { return Ref(Parser::parse_value(tokenizer)); }

private:
    // 核心中枢：根据当前 Token 类型，分发到不同的解析函数
    static Element *parse_value(Tokenizer *tokenizer)
    {
        switch (tokenizer->next().type)
        {
        case TokenType::ObjectBegin:
            return Parser::parse_object(tokenizer);
        case TokenType::ArrayBegin:
            return Parser::parse_array(tokenizer);
        case TokenType::Integer:
            return new Value(std::atoi(tokenizer->now().value.c_str()));
        case TokenType::Float:
            try { return new Value((float)std::stod(tokenizer->now().value)); }
            catch (...) { throw Exception("Invalid float: " + tokenizer->now().value); }
        case TokenType::Bool:
            return new Value((!tokenizer->now().value.empty() && tokenizer->now().value[0] == 't'));
        case TokenType::String:
            return new Value(tokenizer->now().value);
        case TokenType::Null:
            return new Value();
        default:
            throw TypeException("Unexpected token type");
        }
    }

    // ... parse_object 和 parse_array 的实现 ...
};
```

`parse_value` 是我们递归的起点和核心。它就像一个交通枢纽，根据 `Tokenizer` 提供的当前 `Token`，决定下一步应该走向何方。

#### 5.2 解析 `Object` 和 `Array` - 递归的魔力

`parse_object` 的任务是解析 `{ "key": value, ... }` 这样的结构。

```cpp
// 在 Parser 类中
static Object* parse_object(Tokenizer* tokenizer) {
    Object* obj = new Object();
    Token tk = tokenizer->next(); // 消耗掉 '{'，获取下一个 Token

    while (tk.type != TokenType::ObjectEnd) { // 循环直到我们遇到 '}'
        // 1. 期待一个字符串作为 key
        if (tk.type != TokenType::String)
            throw Exception("Expected string key in object!");
        std::string key = tk.value;

        // 2. 期待一个冒号 ':'
        tk = tokenizer->next();
        if (tk.type != TokenType::Colon)
            throw Exception("Expected colon after key!");

        // 3. 递归调用 parse_value() 来解析这个 key 对应的值
        obj->insert_raw_ptr(key, parse_value(tokenizer));

        // 4. 处理下一个元素，可能是逗号 ',' 或结束符 '}'
        tk = tokenizer->next();
        if (tk.type == TokenType::Comma)
            tk = tokenizer->next(); // 消耗掉逗号，准备解析下一个键值对
        else if (tk.type != TokenType::ObjectEnd)
            throw Exception("Expected comma or '}' in object");
    }

    return obj;
}
```

`parse_array` 的逻辑与此类似，但更简单，因为它只需要处理值，而不需要处理键。

```cpp
static Array *parse_array(Tokenizer *tokenizer)
{
    Array *arr = new Array();
    Token tk = tokenizer->forward(); // 需要提前看下一个 Token 以平凡化处理空数组的情况
    bool is_empty = true;

    while (tk.type != TokenType::ArrayEnd) // 循环直到下一个 Token 是 ']'
    {
        is_empty = false;
        arr->append_raw_ptr(parse_value(tokenizer)); // 递归调用 parse_value() 来解析这个对象对应的值
        tk = tokenizer->next(); // 消耗掉逗号，准备解析下一个元素
    }

    // 如果是空数组，得多消耗掉 ']'，不然会少消耗一个字符
    if (is_empty)
        tk = tokenizer->next();

    return arr;
}
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

我们成功了！通过 `Tokenizer` 和 `Parser` 的协作，我们已经能将任意复杂的 JSON 字符串，转换成我们精心设计的、在内存中的 `Element` 对象树。

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

### 7.1 核心思想：隐藏 `new` 关键字

工厂函数的核心思想非常简单：<br>
将 `new` 关键字和对象的具体构造过程封装到一个独立的函数里。<br>
用户不再需要关心如何创建对象，只需要调用这个函数，就能得到一个随时可用的成品。

对于我们的库，用户真正关心的类型是 `Ref`，而不是底层的 `Object*`、`Array*` 或 `Value*`。<br>
因此，我们的工厂函数将接收 C++ 的字面量（如 `123`, `"hello"`），并返回一个封装好的 `Ref` 对象。

### 7.2 第一块积木：`make_value`
我们先为最基础的 Value 类型创建工厂函数。这是一组重载函数，每一种都对应一种 C++ 的基础类型。

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

### 7.3 构建容器：`make_array` 和 `make_object`
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

### 7.4 一个重大的设计权衡：内存管理

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

## 七、总结与展望

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

- 功能扩展：增加一个“格式化输出”或“Pretty Print”功能，让 `serialize()` 的输出更具可读性。

希望这次从零到一的旅程，能点燃你对项目开发的激情，并为你未来的编程之路打下坚实的基础。去创造吧！

源码已经放在了 [github 仓库](https://github.com/pjh456/Let_us_build_cpp_json_parser) 中，如果这篇博客对你有帮助，可以给仓库点个 star 支持一下！