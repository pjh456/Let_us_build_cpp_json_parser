#ifndef INCLUDE_JSON_ELEMENT
#define INCLUDE_JSON_ELEMENT

#include "json_definition.hpp"
#include "json_exception.hpp"

namespace pjh_std
{
    namespace json
    {
        /**
         * @class Element
         * @brief 所有 JSON 元素类型（Value, Array, Object）的抽象基类。
         *        定义了所有 JSON 元素的通用接口，以支持多态。
         */
        class Element
        {
        public:
            /// @brief 虚析构函数。
            virtual ~Element() {}

            /// @brief 判断当前元素是否为简单值 (Value)。
            virtual bool is_value() const noexcept { return false; }
            /// @brief 判断当前元素是否为数组 (Array)。
            virtual bool is_array() const noexcept { return false; }
            /// @brief 判断当前元素是否为对象 (Object)。
            virtual bool is_object() const noexcept { return false; }

            /// @brief 将当前元素转换为 Value 类型指针，若类型不匹配则抛出异常。
            virtual Value *as_value() { throw TypeException("Invalid base type!"); }
            /// @brief 将当前元素转换为 Array 类型指针，若类型不匹配则抛出异常。
            virtual Array *as_array() { throw TypeException("Invalid base type!"); }
            /// @brief 将当前元素转换为 Object 类型指针，若类型不匹配则抛出异常。
            virtual Object *as_object() { throw TypeException("Invalid base type!"); }

            /// @brief 清空元素内容，对于复合类型会递归删除子元素。
            virtual void clear() {}
            /// @brief 创建并返回当前元素的一个深拷贝。
            virtual Element *copy() const = 0;
            /// @brief 将当前元素序列化为紧凑的 JSON 字符串。
            virtual string_t serialize() const noexcept { return ""; }
            /// @brief 将当前元素序列化为带缩进的美化 JSON 字符串。
            virtual string_t pretty_serialize(size_t = 0, char = '\t') const noexcept { return ""; }

            /// @brief 比较两个元素是否相等。
            virtual bool operator==(const Element &other) const noexcept { return false; }
            /// @brief 比较两个元素是否不相等。
            virtual bool operator!=(const Element &other) const noexcept { return true; }
        };
    }
}

#endif // INCLUDE_JSON_ELEMENT