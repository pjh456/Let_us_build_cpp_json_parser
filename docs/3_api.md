# 从零开始用 C++ 打造高性能 JSON 解析器（三）、API 封装篇

## 前言

在上一章中，我们成功实现了从字符串到内存对象的解析。但如果你尝试使用这个解析器，你会发现一个令人崩溃的问题。

假设你想获取 `data.users[0].name`，你不得不写出这样的代码：
```cpp
// 噩梦般的原始调用
Element* root = parser.parse();
std::string name = root->as_object()
                    ->get("data")->as_object()
                    ->get("users")->as_array()
                    ->at(0)->as_object()
                    ->get("name")->as_value()
                    ->as_str();
```
这种代码充满了类型转换和冗长的函数名，既难看又容易出错。今天，我们要通过 **Proxy（代理）模式** 和 **工厂模式**，彻底优化 API 的使用体验。

---

## 一、Ref 类：让链式访问成为可能

为了消灭那一连串的 `as_object()`，我们引入一个轻量级的代理类：`Ref`。它本质上是 `Element*` 的包装器。

### 1. 核心逻辑：重载 `operator[]`
我们让 `Ref` 支持字符串索引（针对对象）和整数索引（针对数组）。

```cpp
class Ref {
private:
    Element* m_ptr; // 包装裸指针

public:
    Ref(Element* p = nullptr) : m_ptr(p) {}

    // 针对 Object 的访问
    Ref operator[](const std::string& key) {
        if (m_ptr && m_ptr->is_object()) {
            return Ref(m_ptr->as_object()->get(key));
        }
        throw TypeException("Not an object");
    }

    // 针对 Array 的访问
    Ref operator[](size_t index) {
        if (m_ptr && m_ptr->is_array()) {
            return Ref(m_ptr->as_array()->at(index));
        }
        throw TypeException("Not an array");
    }

    // 快速取值
    std::string as_str() const { return m_ptr->as_value()->as_str(); }
    int as_int() const { return m_ptr->as_value()->as_int(); }
};
```

### 2. 魔法效果
现在，之前的“噩梦代码”变成了：
```cpp
Ref root = parser.parse();
std::string name = root["data"]["users"][0]["name"].as_str();
```
**设计细节：** 每一次 `[]` 操作都返回一个新的 `Ref` 对象。这种“套娃”式的设计让链式调用变得非常自然。

---

## 二、声明式构建：像写 JSON 一样写 C++

解析 JSON 很重要，但**创建** JSON 同样频繁。如果用 `insert` 和 `append` 一行行写，代码会非常琐碎。

我们希望实现这种效果：
```cpp
Ref root = make_object({
    {"name", make_value("John")},
    {"age", make_value(30)},
    {"skills", make_array({ make_value("C++"), make_value("Python") })}
});
```

### 1. 利用 `std::initializer_list`
C++11 的初始化列表是实现“字面量语法”的神器。

**创建 Value 的工厂：**
```cpp
Ref make_value(int v) { return Ref(new Value(v)); }
Ref make_value(const std::string& v) { return Ref(new Value(v)); }
```

**创建 Object 的工厂：**
```cpp
Ref make_object(std::initializer_list<std::pair<std::string, Ref>> list) {
    Object* obj = new Object();
    for (auto& kv : list) {
        obj->insert_raw_ptr(kv.first, kv.second.get());
    }
    return Ref(obj);
}
```

---

## 三、权衡：易用性 vs 内存安全

细心的读者可能发现了：我们的工厂函数（如 `make_value`）内部调用了 `new`，但返回的是 `Ref`。**谁来负责 `delete`？**

### 1. 我们的设计抉择
在本项目中，我们做了一个明确的权衡：
- **Ref 类不负责销毁内存**：它只是一个“视图”或“引用”。
- **容器负责销毁内存**：`Object` 和 `Array` 在析构时会递归删除子元素。
- **用户责任**：用户只需要 `delete` 根节点的原始指针。

```cpp
Ref root = make_object({ ... });
// 使用 root...
delete root.get(); // 安全释放整棵树
```

**为什么不直接用智能指针？**
1. **教学意义**：手动管理内存能让你彻底搞清楚多态析构和对象所有权。
2. **极致性能**：在下一篇我们要引入**对象池（Object Pool）**，手动管理内存更方便我们直接接管 `new/delete` 的底层行为。

---

## 总结

通过 `Ref` 代理类和工厂函数，我们成功将一个“底层库”进化成了一个“好用的库”。优秀的库设计应该遵循：**把复杂留给自己，把优雅留给用户。**

目前，我们的解析器功能已经完备，API 也足够优雅。但面对 40MB 的大文件，频繁的 `new/delete` 将会成为性能死穴。

**在下一篇文章中，我们将引入“内存分配优化”，通过自定义 Allocator 和对象池，让解析速度提升一个量级！**

---
*本系列源码：[github 仓库](https://github.com/pjh456/Let_us_build_cpp_json_parser)*
*如果你觉得这篇 API 设计对你有帮助，欢迎点赞交流！*