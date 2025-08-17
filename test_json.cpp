#include <iostream>
#include <functional>
#include <chrono>
#include <cassert>
#include <filesystem>
#include <fstream>

#include "json.hpp"
// #include "json_parser.hpp"

using namespace pjh_std::json;

#define FuncName(name) #name
#define Func(name) functionWrapper(FuncName(name), name)

void functionWrapper(const std::string &func_name, std::function<void()> test_func)
{
    std::cout << func_name + " started!\n";
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    try
    {
        test_func();
        std::cout << func_name + " passed!\n";
    }
    catch (Exception e)
    {
        std::cout << "Exception: " << e.what() << '\n';
    }

    std::chrono::duration<double, std::milli> duration(std::chrono::steady_clock::now() - start);
    std::cout << "Spend " << duration.count() << " ms.\n";
}

void test_value()
{
    // 构造各种类型
    Value v_null;
    Value v_bool(true);
    Value v_int(42);
    Value v_float(3.14f);
    Value v_str("hello");

    // 类型判断
    assert(v_null.is_null());
    assert(v_bool.is_bool());
    assert(v_int.is_int());
    assert(v_float.is_float());
    assert(v_str.is_str());

    // 值访问
    assert(v_bool.as_bool() == true);
    assert(v_int.as_int() == 42);
    assert(v_float.as_float() == 3.14f);
    std::cout << v_str.as_str();
    assert(v_str.as_str() == "hello");
}

void test_array()
{
    Array arr;
    arr.append(1);
    arr.append(2.5f);
    arr.append(true);
    arr.append("world");

    assert(arr.size() == 4);
    assert(Ref(&arr)[0].as_int() == 1);
    assert(Ref(&arr)[1].as_float() == 2.5f);
    assert(Ref(&arr)[2].as_bool() == true);
    assert(Ref(&arr)[3].as_str() == "world");

    arr.erase(2);
    assert(arr.size() == 3);
}

void test_object()
{
    Object obj;
    obj.insert("name", "Alice");
    obj.insert("age", 30);
    obj.insert("height", 1.68f);
    obj.insert("isStudent", false);

    assert(obj.size() == 4);
    assert(Ref(&obj)["name"].as_str() == "Alice");
    assert(Ref(&obj)["age"].as_int() == 30);
    assert(Ref(&obj)["height"].as_float() == 1.68f);
    assert(Ref(&obj)["isStudent"].as_bool() == false);

    obj.insert("age", 31); // 覆盖
    assert(Ref(&obj)["age"].as_int() == 31);
}

void test_parser()
{
    // 一个标准 JSON
    std::string json_text = R"({
        "name": "Bob",
        "age": 25,
        "isStudent": true,
        "scores": [90, 85, 88],
        "profile": {
            "height": 1.75,
            "city": "New York"
        }
    })";

    Parser parser(json_text);
    Ref root = parser.parse();

    assert(root["name"].as_str() == "Bob");
    assert(root["age"].as_int() == 25);
    assert(root["isStudent"].as_bool() == true);

    Ref scores = root["scores"];
    assert(scores.size() == 3);
    assert(scores[0].as_int() == 90);
    assert(scores[2].as_int() == 88);

    Ref profile = root["profile"];
    assert(profile["height"].as_float() == 1.75f);
    assert(profile["city"].as_str() == "New York");

    delete root.get(); // 释放内存
}

void test_factory_build()
{
    Ref json = make_object(
        {
            {"name", make_value("Alice")},
            {"age", make_value(25)},
            {"isStudent", make_value(false)},
            {
                "scores",
                make_array(
                    {
                        make_value(90),
                        make_value(85),
                        make_value(88),
                    }),
            },
            {
                "profile",
                make_object({
                    {"height", make_value(1.68f)},
                    {"city", make_value("New York")},
                }),
            },
        });

    // 开始测试
    assert(json["name"].as_str() == "Alice");
    assert(json["age"].as_int() == 25);
    assert(json["isStudent"].as_bool() == false);

    assert(json["scores"].size() == 3);
    assert(json["scores"][0].as_int() == 90);
    assert(json["scores"][1].as_int() == 85);
    assert(json["scores"][2].as_int() == 88);

    assert(json["profile"]["height"].as_float() == 1.68f);
    assert(json["profile"]["city"].as_str() == "New York");

    std::cout << json;
}

void test_file_io()
{
    const std::filesystem::path path("E:/Projects/blogs/let_us_build_cpp_json_parser/40mb.json");
    std::ifstream ifs(path);
    std::ostringstream oss;
    oss << ifs.rdbuf(); // 一次性读入

    std::string content = oss.str();

    Parser parser(content);
    Ref root = parser.parse();

    // std::cout << root;
    delete root.get();
}

int main()
{
    Func(test_value);
    Func(test_array);
    Func(test_object);
    Func(test_parser);
    Func(test_factory_build);
    Func(test_file_io);
}