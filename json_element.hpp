#ifndef INCLUDE_JSON_ELEMENT
#define INCLUDE_JSON_ELEMENT

#include "json_definition.hpp"
#include "json_exception.hpp"

namespace pjh_std
{
    namespace json
    {
        class Element
        {
        public:
            virtual ~Element() {}

            virtual bool is_value() const noexcept { return false; }
            virtual bool is_array() const noexcept { return false; }
            virtual bool is_object() const noexcept { return false; }

            virtual Value *as_value() { throw TypeException("Invalid base type!"); }
            virtual Array *as_array() { throw TypeException("Invalid base type!"); }
            virtual Object *as_object() { throw TypeException("Invalid base type!"); }

            virtual void clear() {}
            virtual Element *copy() const = 0;
            virtual std::string serialize() const noexcept { return ""; }

            virtual bool operator==(const Element &other) const noexcept { return false; }
            virtual bool operator!=(const Element &other) const noexcept { return true; }
        };
    }
}

#endif // INCLUDE_JSON_ELEMENT