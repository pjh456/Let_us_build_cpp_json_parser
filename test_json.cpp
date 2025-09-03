#include <iostream>
#include <functional>
#include <chrono>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>

// 引入 JSON 解析器头文件
#include <pjh_json/json_parser.hpp>
#include <nlohmann/json.hpp>
#include "rapidjson/document.h"

// 使用 JSON 命名空间，以便直接访问相关类
using namespace pjh_std::json;

// 宏定义：将函数名转换为字符串
#define FuncName(name) #name
// 宏定义：包装测试函数，用于计时和异常捕获
#define Func(name) functionWrapper(FuncName(name), name)

/**
 * @brief 测试函数包装器。用于执行测试函数、计时并捕获异常。
 * @param func_name 函数名称字符串。
 * @param test_func 待执行的测试函数（无参，返回 void）。
 */
void functionWrapper(const std::string &func_name, std::function<void()> test_func)
{
    std::cout << func_name + " started!\n";
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    try
    {
        // 1. 执行测试函数
        test_func();
        std::cout << func_name + " passed!\n";
    }
    catch (Exception e)
    {
        // 2. 捕获 JSON 库定义的异常
        std::cout << "Exception: " << e.what() << '\n';
    }

    // 3. 统计并输出执行时间
    std::chrono::duration<double, std::milli> duration(std::chrono::steady_clock::now() - start);
    std::cout << "Spend " << duration.count() << " ms.\n";
}

/**
 * @brief 测试 Value 类的基本功能，包括构造、类型判断和值访问。
 */
void test_value()
{
    std::cout << "Test: Creating and verifying different JSON Value types.\n";

    // 1. 构造各种类型的 Value 对象
    Value v_null;
    Value v_bool(true);
    Value v_int(42);
    Value v_float(3.14f);
    Value v_str("hello");

    // 2. 类型判断
    assert(v_null.is_null());
    assert(v_bool.is_bool());
    assert(v_int.is_int());
    assert(v_float.is_float());
    assert(v_str.is_str());

    // 3. 值访问（使用 as_X() 方法）
    assert(v_bool.as_bool() == true);
    assert(v_int.as_int() == 42);
    assert(v_float.as_float() == 3.14f);
    // std::cout << v_str.as_str(); // 调试输出
    assert(v_str.as_str() == "hello");

    std::cout << "Value tests passed.\n";
}

/**
 * @brief 测试 Array 类的基本功能，包括追加、访问和删除。
 */
void test_array()
{
    std::cout << "Test: Array construction and element access.\n";

    // 1. 创建数组并追加不同类型的值
    Array arr;
    arr.append(1);
    arr.append(2.5f);
    arr.append(true);
    arr.append("world");

    // 2. 验证大小和使用 Ref 进行访问
    assert(arr.size() == 4);
    assert(Ref(&arr)[0].as_int() == 1);
    assert(Ref(&arr)[1].as_float() == 2.5f);
    assert(Ref(&arr)[2].as_bool() == true);
    assert(Ref(&arr)[3].as_str() == "world");

    // 3. 删除元素
    arr.erase(2); // 删除索引为 2 的元素 (true)
    assert(arr.size() == 3);

    std::cout << "Array tests passed.\n";
}

/**
 * @brief 测试 Object 类的基本功能，包括插入和访问。
 */
void test_object()
{
    std::cout << "Test: Object construction and key-value operations.\n";

    // 1. 创建对象并插入键值对
    Object obj;
    obj.insert("name", "Alice");
    obj.insert("age", 30);
    obj.insert("height", 1.68f);
    obj.insert("isStudent", false);

    // 2. 验证大小和使用 Ref 进行访问
    assert(obj.size() == 4);
    assert(Ref(&obj)["name"].as_str() == "Alice");
    assert(Ref(&obj)["age"].as_int() == 30);
    assert(Ref(&obj)["height"].as_float() == 1.68f);
    assert(Ref(&obj)["isStudent"].as_bool() == false);

    // 3. 验证值覆盖
    obj.insert("age", 31); // 覆盖 age 的值
    assert(Ref(&obj)["age"].as_int() == 31);

    std::cout << "Object tests passed.\n";
}

/**
 * @brief 测试 Parser 类的基本功能，解析复杂的 JSON 字符串并进行访问。
 */
void test_parser()
{
    std::cout << "Test: Parsing a complex JSON string.\n";

    // 1. 定义测试 JSON 字符串
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

    // 2. 创建解析器并执行解析
    Parser parser(json_text);
    Ref root = parser.parse();

    // 3. 验证解析结果
    assert(root["name"].as_str() == "Bob");
    assert(root["age"].as_int() == 25);
    assert(root["isStudent"].as_bool() == true);

    // 4. 验证嵌套数组
    Ref scores = root["scores"];
    assert(scores.size() == 3);
    assert(scores[0].as_int() == 90);
    assert(scores[2].as_int() == 88);

    // 5. 验证嵌套对象
    Ref profile = root["profile"];
    assert(profile["height"].as_float() == 1.75f);
    assert(profile["city"].as_str() == "New York");

    // 6. 释放内存 (由于 Ref 内部使用裸指针，需要手动删除根元素)
    delete root.get();

    std::cout << "Parser tests passed.\n";
}

/**
 * @brief 测试工厂函数 (make_object, make_array, make_value) 构建 JSON 结构的功能。
 */
void test_factory_build()
{
    std::cout << "Test: Building JSON structures using factory functions.\n";

    // 1. 使用工厂函数构建一个复杂的 JSON 对象
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

    // 2. 验证构建结果
    assert(json["name"].as_str() == "Alice");
    assert(json["age"].as_int() == 25);
    assert(json["isStudent"].as_bool() == false);

    assert(json["scores"].size() == 3);
    assert(json["scores"][0].as_int() == 90);
    assert(json["scores"][1].as_int() == 85);
    assert(json["scores"][2].as_int() == 88);

    assert(json["profile"]["height"].as_float() == 1.68f);
    assert(json["profile"]["city"].as_str() == "New York");

    // 3. 输出 JSON 结构 (使用 pretty_serialize)
    std::cout << json;

    // 4. 注意：这里没有手动调用 delete json.get()，因为工厂函数创建的 Element 应该由 Ref 的用户负责管理生命周期。
    // 在这个简单的测试中，Ref 指针的生命周期跟随函数结束，但实际应用中需要谨慎管理。

    std::cout << "Factory build tests passed.\n";
}

/**
 * @brief 测试从文件读取大型 JSON 数据并进行解析的性能。
 */
void test_file_io()
{
    // 1. 定义文件路径
    const std::filesystem::path path("E:/Projects/blogs/let_us_build_cpp_json_parser/40mb.json");

    // 2. 检查文件是否存在
    if (!std::filesystem::exists(path))
    {
        std::cout << "Error: File not found at " << path << std::endl;
        return;
    }

    // 3. 打开文件并一次性读取所有内容
    std::ifstream ifs(path);
    std::ostringstream oss;
    oss << ifs.rdbuf();
    std::string content = oss.str();

    // 4. 解析 JSON
    Parser parser(content);
    Ref root = parser.parse();

    // 5. 可以在这里添加断言来验证文件内容，但主要目的是测试性能
    // std::cout << root;

    // 6. 释放内存
    delete root.get();
}

void test_nlohhman_json_file_io()
{
    // 1. 定义文件路径
    const std::filesystem::path path("E:/Projects/blogs/let_us_build_cpp_json_parser/40mb.json");

    // 2. 检查文件是否存在
    if (!std::filesystem::exists(path))
    {
        std::cout << "Error: File not found at " << path << std::endl;
        return;
    }

    // 3. 打开文件并一次性读取所有内容
    std::ifstream ifs(path);
    std::ostringstream oss;
    oss << ifs.rdbuf();
    std::string content = oss.str();

    // 4. 解析 JSON（nlohmann）
    nlohmann::json root = nlohmann::json::parse(content);

    // 5. 可以在这里加一些访问操作验证正确性（可选）
    // std::cout << root.size() << std::endl;
}

void test_rapidjson_file_io()
{
    // 1. 定义文件路径
    const std::filesystem::path path("E:/Projects/blogs/let_us_build_cpp_json_parser/40mb.json");

    // 2. 检查文件是否存在
    if (!std::filesystem::exists(path))
    {
        std::cout << "Error: File not found at " << path << std::endl;
        return;
    }

    // 3. 打开文件并一次性读取所有内容
    std::ifstream ifs(path);
    std::ostringstream oss;
    oss << ifs.rdbuf();
    std::string content = oss.str();

    // 4. 解析 JSON（RapidJSON）
    rapidjson::Document doc;
    if (doc.Parse(content.c_str()).HasParseError())
    {
        std::cout << "Error: Failed to parse JSON with RapidJSON.\n";
        return;
    }
}

/**
 * @brief 主函数，运行所有测试用例。
 */
int main()
{
    // 使用 Func 宏调用并包装每个测试函数
    Func(test_value);
    Func(test_array);
    Func(test_object);
    Func(test_parser);
    Func(test_factory_build);
    Func(test_file_io);
    Func(test_nlohhman_json_file_io);
    Func(test_rapidjson_file_io);
    return 0;
}