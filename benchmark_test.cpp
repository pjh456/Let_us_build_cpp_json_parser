#include <fstream>
#include <sstream>
#include <string>
#include <random>
#include <iostream>

#include <benchmark/benchmark.h>
#include <pjh_json/parsers/json_parser.hpp>
#include <nlohmann/json.hpp>
#include "rapidjson/document.h"

static std::mt19937 rng(std::random_device{}());

std::string random_string(size_t length)
{
    static const char charset[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789";
    std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; i++)
    {
        result.push_back(charset[dist(rng)]);
    }
    return result;
}

pjh_std::json::Ref random_json(int depth = 0, int max_depth = 5)
{
    using namespace pjh_std::json;

    std::uniform_int_distribution<int> type_dist(0, 5);
    int t = (depth >= max_depth) ? type_dist(rng) % 3 : type_dist(rng);
    // 限制最大深度后只生成基础类型

    switch (t)
    {
    case 0:
        return Ref(new Value());
    case 1:
        return Ref(new Value((bool)(rng() % 2)));
    case 2:
        return Ref(new Value((float)((rng() % 10000) / 10.0)));
    case 3:
        return Ref(new Value(random_string(5 + rng() % 10)));
    case 4:
    { // array
        auto arr = Ref(new Array());
        int n = 1 + (rng() % 5);
        for (int i = 0; i < n; i++)
        {
            arr.get()->as_array()->append_raw_ptr(random_json(depth + 1, max_depth).get());
        }
        return arr;
    }
    case 5:
    { // object
        auto obj = Ref(new Object());
        int n = 1 + (rng() % 5);
        for (int i = 0; i < n; i++)
        {
            obj.get()->as_object()->insert_raw_ptr(random_string(3 + rng() % 5), random_json(depth + 1, max_depth).get());
        }
        return obj;
    }
    }
    return Ref(new Value());
}

void generate_json_file(const std::string &path, size_t target_size, int max_depth = 5)
{
    using namespace pjh_std::json;

    Ref root = random_json(0, max_depth);

    std::string json_str = root.get()->serialize();

    // 如果太小，可以不断往数组里加元素直到接近目标大小
    while (json_str.size() < target_size)
    {
        auto arr = Ref(new Array());
        arr.get()->as_array()->append_raw_ptr(root.get());
        arr.get()->as_array()->append_raw_ptr(random_json(0, max_depth).get());
        root = arr;
        json_str = root.get()->serialize();
    }

    std::ofstream ofs(path);
    ofs << json_str;
}

inline std::string read_file(const std::string &path_str)
{
    const std::filesystem::path path(path_str);

    // 2. 检查文件是否存在
    if (!std::filesystem::exists(path))
    {
        std::cerr << "Error: File not found at " << path << std::endl;
        return "";
    }

    // 3. 打开文件并一次性读取所有内容
    std::ifstream ifs(path);
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

// nlohmann_json
static void BM_Nlohmann_Json_Parse(benchmark::State &state, const std::string &content)
{
    for (auto _ : state)
    {
        auto j = nlohmann::json::parse(content);
        benchmark::DoNotOptimize(j);
    }
}

// RapidJSON
static void BM_Rapid_Json_Parse(benchmark::State &state, const std::string &content)
{
    for (auto _ : state)
    {
        rapidjson::Document d;
        d.Parse(content.c_str());
        benchmark::DoNotOptimize(d);
    }
}

// pjh_json
static void BM_PJH_Json_Parse(benchmark::State &state, const std::string &content)
{
    for (auto _ : state)
    {
        pjh_std::json::Parser parser(content);
        pjh_std::json::Ref root = parser.parse();
        benchmark::DoNotOptimize(root);
        delete root.get();
    }
}

void RegisterBenchmarks()
{
    // std::vector<std::pair<std::string, size_t>> sizes = {
    //     {"1kb.json", 1 * 1024},
    //     {"100kb.json", 100 * 1024},
    //     {"1mb.json", 1024 * 1024},
    //     {"10mb.json", 10 * 1024 * 1024},
    //     {"40mb.json", 40 * 1024 * 1024}};

    std::string path = "E:/Projects/blogs/let_us_build_cpp_json_parser/500mb.json";

    // for (auto &[fname, target_size] : sizes)
    {
        // std::string path = "data/" + fname;
        // std::string path = "data/" + fname;
        // if (!std::filesystem::exists(path))
        // {
        //     std::cout << "Generating " << path << "...\n";
        //     generate_json_file(path, target_size);
        // }

        std::string json_data = read_file(path);

        benchmark::RegisterBenchmark(
            "PJH/",
            BM_PJH_Json_Parse, json_data);
        benchmark::RegisterBenchmark(
            "Nlohmann/",
            BM_Nlohmann_Json_Parse, json_data);
        benchmark::RegisterBenchmark(
            "RapidJSON/",
            BM_Rapid_Json_Parse, json_data);
    }
}

int main(int argc, char **argv)
{
    RegisterBenchmarks();
    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
}