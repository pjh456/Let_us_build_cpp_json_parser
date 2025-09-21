#ifndef INCLUDE_JSON_DEFINITIONS
#define INCLUDE_JSON_DEFINITIONS

#include <string>
#include <vector>
#include <unordered_map>
// #include <variant>
#include <sstream>
#include <string_view>
// #include <utils/variant.hpp>
#include <pjh_json/utils/variant.hpp>

namespace pjh_std
{
    namespace json
    {
        // 提前声明 JSON 库中的核心类，以解决类之间的循环引用问题
        class Ref;
        class Element;

        class Value;
        class Object;
        class Array;

        // 为 std::string 定义一个更简洁的别名
        using string_t = std::string;
        // 为 std::string_view 定义一个更简洁的别名，用于高效处理字符串切片
        using string_v_t = std::string_view;

        // 使用 std::variant 定义 JSON 的值类型，它可以持有多种不同的基础数据类型
        using value_t = Variant<
            std::nullptr_t,
            bool,
            int,
            float,
            string_t,
            string_v_t>;

        // JSON 数组的模板别名，底层使用 std::vector
        template <typename T>
        using array_t = std::vector<T>;

        /**
         * @struct StringViewHash
         * @brief 为 std::string_view 提供哈希计算的结构体，用于 unordered_map。
         *        通过启用透明模式(is_transparent)，允许在不创建 std::string 临时对象的情况下，
         *        直接使用 const char* 或 std::string_view 作为键进行查找，提升性能。
         */
        struct StringViewHash
        {
            using is_transparent = void; // 关键！启用透明模式

            size_t operator()(std::string_view sv) const
            {
                return std::hash<std::string_view>{}(sv);
            }
        };

        // JSON 对象的模板别名，底层使用带有自定义哈希的 std::unordered_map
        template <typename T>
        using object_t = std::unordered_map<
            string_v_t,
            T,
            StringViewHash,
            std::equal_to<>>;

        // 提前声明异常类
        class Exception;

        class ParseException;
        class TypeException;
        class OutOfRangeException;
        class InvalidKeyException;
        class SerializationException;

        // 提前声明并发缓冲和内存管理相关类
        template <typename T>
        class Channel;
        template <typename T>
        class LockFreeRingBuffer;
        template <typename T>
        class MallocAllocator;
        template <typename T, size_t BlockSize>
        class BlockAllocator;
        template <typename T>
        class FreeListAllocator;

        template <typename T, typename Allocate>
        class ObjectPool;
    }
}
#endif // INCLUDE_JSON_DEFINITIONS