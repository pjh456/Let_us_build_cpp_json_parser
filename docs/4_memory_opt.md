# 从零开始用 C++ 打造高性能 JSON 解析器（四）、内存池优化篇

## 前言

在前面的章节中，我们已经拥有了一个功能完备、API 优雅的 JSON 解析器。但在处理大型文件（如 40MB+）时，你会发现程序变得迟钝。

通过性能分析（Profiling），你会发现瓶颈不在于逻辑，而在于 **`new` 和 `delete`**。

在 JSON 解析过程中，我们会创建成千上万个 `Value`、`Object` 和 `Array`。每一次 `new` 都要向操作系统申请内存，涉及内核态切换和复杂的寻址算法。今天，我们要通过 **对象池 (Object Pool)** 技术，把内存申请的开销降到几乎为零。

---

## 一、核心思想：批发代替零售

对象池的原理很简单：
1. **预先批发**：一次性向系统申请一大块内存（Block）。
2. **内部零售**：当 Parser 需要 `new` 一个对象时，直接从这一大块内存中切出一小块给它。这只是移动一下指针的操作，速度极快。
3. **统一释放**：当解析任务结束，直接释放整个大块内存。

---

## 二、解耦设计：Allocator 策略模式

不同的场景需要不同的分配策略。我们定义一个抽象的 `Allocator` 接口，实现多种分配策略。

### 1. 区块分配器 (BlockAllocator) —— 解析器的最爱
这种分配器只进不出，非常适合“解析一次，使用完就全扔掉”的场景。

```cpp
template <typename T, size_t BlockSize = 1024>
class BlockAllocator {
private:
    std::vector<void*> blocks;
    T* currentBlock = nullptr;
    size_t remaining = 0;

public:
    T* allocate() {
        if (remaining == 0) {
            // 当前块用完了，批发一块新的
            void* block = ::malloc(sizeof(T) * BlockSize);
            blocks.push_back(block);
            currentBlock = reinterpret_cast<T*>(block);
            remaining = BlockSize;
        }
        remaining--;
        return currentBlock++; // 指针偏移，极快！
    }
    
    void deallocate(T* ptr) {
        // 解析期间不释放，最后由析构函数统一 ::free 掉 blocks
    }
};
```

---

## 三、黑魔法：重载 `operator new/delete`

有了对象池，我们该如何让 `new Value()` 自动使用它，而不是去调系统的堆内存呢？

C++ 提供了一个非常强大的特性：**类成员 `new` 重载**。

```cpp
// json_value.hpp
class Value : public Element {
private:
    // 为 Value 类定义一个静态对象池
    static ObjectPool<Value, BlockAllocator<Value>> pool;

public:
    // 当执行 new Value(...) 时，编译器会自动调用这个函数
    void* operator new(std::size_t n) {
        return pool.allocate();
    }

    // 当执行 delete value_ptr 时调用
    void operator delete(void* ptr) {
        pool.deallocate(static_cast<Value*>(ptr));
    }
};

// C++17 inline 静态成员初始化
inline ObjectPool<Value, BlockAllocator<Value>> Value::pool;
```

### 设计细节：
- **透明性**：我们完全不需要修改 `Parser` 里的任何代码。原本的 `new Value()` 依然保留，但底层的内存分配逻辑已经被我们“偷梁换柱”了。
- **编译期多态**：通过模板参数 `BlockAllocator<Value>`，我们在编译期就确定了分配策略，避免了运行时的虚函数调用开销。

---

## 四、性能飞跃：为什么变快了？

1. **减少系统调用**：从“成千上万次 `malloc`”变成了“几十次 `malloc`”。
2. **内存局部性（Locality）**：对象池分配的对象在物理内存上是连续存放的。这意味着 CPU 缓存命中率会大大提高。
3. **消除碎片**：大块申请内存避免了大量小对象导致的堆碎片。

还记得我们在第一篇里展示的 Benchmark 吗？
> **PJH (本项目): 242ms** vs **nlohmann/json: 840ms**

这 **3 倍** 的性能领先，绝大部分归功于这一章所做的内存池优化。

---

## 总结

内存管理是 C++ 的灵魂，也是性能优化的“兵家必争之地”。通过接管 `operator new`，我们不仅实现了性能的跨越，还保持了业务代码的整洁。

目前我们的解析器已经是“单核战神”了。但在多核 CPU 时代，如何让词法分析和语法分析并行，进一步压榨机器性能？

**在下一篇文章（也是本系列的终章）中，我们将引入“并发解析”，通过无锁环形缓冲区（Lock-Free Ring Buffer）打造真正的并发解析引擎。**

---
*本系列源码：[github 仓库](https://github.com/pjh456/Let_us_build_cpp_json_parser)*
*觉得干货够多？点个赞或收藏，这是对我最大的支持！*