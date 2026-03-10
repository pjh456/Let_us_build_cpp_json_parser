# 从零开始用 C++ 打造高性能 JSON 解析器（一）、建模篇

## 一、需求分析与技术栈

JSON 支持六种数据类型：`object`, `array`, `string`, `number`, `boolean`, `null`。它们可以互相嵌套，形成树状结构。

### 技术选择 (C++ 17)
- **`std::variant`**：类型安全的 `union`，用于存储多种基础类型。
- **`std::vector` & `std::map`**：实现 Array 和 Object 的天然容器。
- **面向对象多态**：通过虚函数处理递归嵌套结构。

---

## 二、构建 JSON 的数据王国：建模

JSON 的本质是分层的。为了实现“万物皆可互嵌”，我们需要设计一个通用的基类 `Element`。

### 1. 抽象基类：多态的艺术

```cpp
class Element {
public:
    virtual ~Element() {} // 黄金法则：虚析构函数防止内存泄漏

    // 类型识别：默认都是 false，由子类重写
    virtual bool is_value() const noexcept { return false; }
    virtual bool is_array() const noexcept { return false; }
    virtual bool is_object() const noexcept { return false; }

    // 安全转换：失败直接抛出异常（Fail-fast 设计）
    virtual Value* as_value() { throw TypeException("Invalid base type!"); }
    virtual Array* as_array() { throw TypeException("Invalid base type!"); }
    virtual Object* as_object() { throw TypeException("Invalid base type!"); }

    virtual Element* copy() const = 0; // 强制子类实现深拷贝
    virtual std::string serialize() const noexcept { return ""; }
};
```

### 2. 实现“原子”：Value 类

利用 `std::variant`，我们可以优雅地在一个类里存储多种基础类型。

```cpp
using value_t = std::variant<std::nullptr_t, bool, int, float, std::string>;

class Value : public Element {
private:
    value_t m_value;

public:
    Value(int v) : m_value(v) {}
    Value(const std::string& v) : m_value(v) {}

    bool is_value() const noexcept override { return true; }
    Value* as_value() override { return this; }
    
    Element* copy() const noexcept override { return new Value(*this); }

    std::string serialize() const noexcept override {
        return std::visit([](auto&& arg) -> std::string {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::string>) return "\"" + arg + "\"";
            else if constexpr (std::is_same_v<T, std::nullptr_t>) return "null";
            // ... 处理其他类型
        }, m_value);
    }
};
```

### 3. 实现容器：Array 与 Object

由于需要存储任意类型的 `Element`，容器必须存储基类指针：`Element*`。

**注意：** 这里我们选择**手动内存管理**。虽然 `std::unique_ptr` 更安全，但手动处理 `new/delete` 能让你更深刻地理解对象所有权（Ownership）。

```cpp
class Array : public Element {
private:
    std::vector<Element*> m_arr;

public:
    ~Array() { clear(); }
    
    void clear() override {
        for (auto it : m_arr) delete it; // 释放拥有的子元素
        m_arr.clear();
    }

    void append_raw_ptr(Element* child) { m_arr.push_back(child); }

    Element* copy() const noexcept override {
        Array* new_arr = new Array();
        for (const auto& elem : m_arr) new_arr->append_raw_ptr(elem->copy());
        return new_arr;
    }
};
```

---

## 三、设计决策剖析

1. **为什么 `is_xxx` 写在基类？**
   这建立了一条“默认规则”：除非子类特意声明（重写），否则它什么都不是。这避免了在每个子类中写冗长的判断逻辑。
   
2. **内存管理的挑战**
   当 `Array` 拥有 `Element*` 时，它就成了这块内存的“主人”。在析构或 `clear` 时，必须递归调用 `delete`。这正是为什么基类必须有 `virtual ~Element()` —— 确保 `delete` 时能顺着继承链一路拆解。

3. **深拷贝的必要性**
   JSON 树状结构中，简单的指针拷贝会导致多个容器指向同一内存，产生“双重释放”崩溃。`copy()` 纯虚函数强制每个子类实现自己的递归深拷贝逻辑。

---

## 第一阶段总结

到这里，我们已经为整个 JSON 解析器搭建好了骨架。虽然还没有解析任何字符串，但我们已经确立了：
- **多态体系**：如何统一处理不同类型的数据。
- **内存模型**：谁申请，谁释放，如何递归释放。
- **类型安全**：如何从通用的基类指针安全地转回具体类型。

**在下一篇文章中，我们将进入核心环节：实现词法分析器 (Tokenizer) 与语法分析器 (Parser)，揭开字符串变对象的神秘面纱。**

---

*如果你对本项目感兴趣，可以关注我的后续更新，源码已在 [github 仓库](https://github.com/pjh456/Let_us_build_cpp_json_parser) 给出。欢迎在评论区探讨 C++ 内存管理的细节！*
