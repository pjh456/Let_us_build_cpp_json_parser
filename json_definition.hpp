#ifndef INCLUDE_JSON_DEFINITIONS
#define INCLUDE_JSON_DEFINITIONS

#include <string>
#include <vector>
#include <map>
#include <variant>

namespace pjh_std
{
    namespace json
    {
        // Json values
        class Ref;
        class Element;

        class Value;
        class Object;
        class Array;

        using string_t = std::string;

        using value_t = std::variant<
            std::nullptr_t,
            bool,
            int,
            float,
            string_t>;

        template <typename T>
        using array_t = std::vector<T>;

        template <typename T>
        using object_t = std::map<string_t, T>;

        // Exceptions
        class Exception;

        class ParseException;
        class TypeException;
        class OutOfRangeException;
        class InvalidKeyException;
        class SerializationException;
    }
}
#endif // INCLUDE_JSON_DEFINITIONS