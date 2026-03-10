# 从零开始用 C++ 打造高性能 JSON 解析器（五）、并发与无锁优化篇

## 前言

欢迎来到本系列的终章。在上一章中，我们通过对象池将内存分配性能优化到了极限。现在的解析器在单核上已经风驰电掣，但面对现代 CPU 的多核心，我们还有一个大招没放：**并行化（Parallelization）**。

今天，我们将为解析器装上“F1 并发引擎”，让词法分析（Tokenizer）和语法分析（Parser）在不同的 CPU 核心上协同工作。

---

## 一、流水线架构：生产者-消费者模型

解析 JSON 的过程可以天然拆分为两个阶段：
1. **生产者 (Tokenizer)**：读取原始字符串，切分成 Token。
2. **消费者 (Parser)**：取走 Token，组装成对象树。

如果能让 Tokenizer 在线程 A 生产，Parser 同时在线程 B 消费，就能实现真正的“流水线并行”。

---

## 二、从有锁到无锁：性能的质变

### 1. 昂贵的安保系统（有锁队列）

起初，我们可能使用 `std::mutex` 和 `std::condition_variable` 来构建通信通道（Channel）。

但测试发现：**由于 Token 非常小，线程在加锁/解锁上的耗时甚至超过了解析本身！** 这种“重型安保”严重拖慢了速度。

### 2. 终极武器：无锁环形缓冲区 (Lock-Free Ring Buffer)

对于“单生产者-单消费者 (SPSC)”场景，我们可以使用 **原子变量** 来实现完全无锁的通信。

```cpp
template <typename T>
class LockFreeRingBuffer {
private:
    std::vector<T> m_buffer;
    // 关键：原子头尾指针
    std::atomic<size_t> m_head{0};
    std::atomic<size_t> m_tail{0};

public:
    bool push(const T& item) {
        size_t tail = m_tail.load(std::memory_order_relaxed);
        size_t next_tail = (tail + 1) % m_buffer.size();
        if (next_tail == m_head.load(std::memory_order_acquire)) 
            return false; // 队列满了

        m_buffer[tail] = item;
        // 魔法：release 语义确保数据写入对消费者可见
        m_tail.store(next_tail, std::memory_order_release);
        return true;
    }

    std::optional<T> pop() {
        size_t head = m_head.load(std::memory_order_relaxed);
        if (head == m_tail.load(std::memory_order_acquire)) 
            return std::nullopt; // 队列空了

        T item = m_buffer[head];
        m_head.store((head + 1) % m_buffer.size(), std::memory_order_release);
        return item;
    }
};
```

**硬核细节：内存序 (Memory Order)**

我们使用了 `std::memory_order_acquire` 和 `std::memory_order_release`。这是一种轻量级的“内存屏障”，它告诉 CPU：不要为了优化而乱序执行我的代码，必须保证生产者写完数据后，消费者才能看到更新后的指针。

---

## 三、总指挥：Parser 的异步化

现在，我们将 Parser 的数据来源从原来的 `Tokenizer` 切换到这个无锁缓冲区。

```cpp
Ref Parser::parse() {
    // 1. 启动生产者线程 (Tokenizer)
    std::thread producer([&]() {
        while (true) {
            Token t = m_tokenizer->peek();
            // 自旋等待，直到缓冲区有空间
            while (!m_buffer.push(t)) std::this_thread::yield();
            if (t.type == TokenType::End) break;
            m_tokenizer->consume();
        }
    });

    // 2. 主线程作为消费者，执行核心解析逻辑
    // 注意：这里的 parse_value 内部调用的 peek/consume 已经被改写为从 m_buffer 读取
    Ref root = Ref(this->parse_value());

    producer.join();
    return root;
}
```

### 抽象的力量
最令人赞叹的是：由于我们在第二章中将核心解析逻辑（`parse_object`, `parse_array`）构建于抽象的 `peek()` 和 `consume()` 之上，这次底层的并行化改造，**竟然不需要改动任何一行核心解析逻辑代码！**

---

## 四、全系列总结：我们的征途

回顾这五篇文章，我们从零开始，亲手完成了一场 C++ 工程实践的马拉松：

1.  **建模篇**：利用 C++ 多态和 `std::variant` 构建了灵活的 JSON 树。
2.  **解析篇**：深入递归下降算法，揭开了编译器前端的神秘面纱。
3.  **API 封装篇**：通过操作符重载和 Proxy 模式，实现了优雅的链式调用。
4.  **内存池篇**：接管 `operator new`，通过 Block 分配器实现了 3 倍的性能提升。
5.  **并发篇**：引入无锁队列，压榨多核 CPU 性能，驶向极致。

### 写在最后

“造轮子”的意义不在于重复发明，而在于 **通过创造去彻底理解**。

当你亲手处理过虚析构导致的内存泄漏、调试过递归嵌套的边界情况、优化过原子变量的内存序，你对 C++ 的理解将不再停留在语法表面，而是深入到了计算机系统的底层律动中。

这个项目虽小，但它涵盖了现代 C++ 开发的精华。希望这个系列能点燃你对底层开发的热情。

**代码没有终点，只有不断的重构与超越。去创造属于你的王国吧！**

---
*完整项目源码已开源，欢迎 Star 交流：[github 仓库](https://github.com/pjh456/Let_us_build_cpp_json_parser)*

*本系列到此结束。如果你有任何疑问或更好的优化思路，欢迎在评论区留言，我们一起进步！*