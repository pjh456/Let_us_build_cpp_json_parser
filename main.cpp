#include "json.h"

void testJsonPrimitive() {
	Json::Utils::functionWrapper("Value test", [] {
		Json::Value p1(10);
		Json::Value p2(10);
		Json::Value p3(20);
		Json::Value p4(10.0f);
		Json::Value p5("Hello");
		Json::Value p6("Hello");

		if (!(p1 == p2)) throw Json::Exception(p1.serialize() + " should euqal to " + p2.serialize());

		if (p1 == p3) throw Json::Exception(p1.serialize() + " should not euqal to " + p3.serialize());

		if (p1 == p4) throw Json::Exception(p1.serialize() + " should not euqal to " + p4.serialize());

		if (!(p5 == p6)) throw Json::Exception(p5.serialize() + " should not euqal to " + p6.serialize());
		});

	puts("");
}

void testJsonArray() {
	Json::Utils::functionWrapper("Array test", [] {
		Json::Value* p1 = new Json::Value(10);
		Json::Value* p2 = new Json::Value(20);
		Json::Value* p3 = new Json::Value(30);

		Json::Array arr1;
		arr1.append_ptr(p1);
		arr1.append_ptr(p2);
		arr1.append_ptr(p3);

		if (arr1.size() != 3) throw Json::Exception(arr1.size() + " should euqal to " + 3);

		if (!arr1.contains(p1)) throw Json::Exception(arr1.serialize() + " should contain " + p1->serialize());
		if (!arr1.contains(p2)) throw Json::Exception(arr1.serialize() + " should contain " + p2->serialize());
		if (!arr1.contains(p3)) throw Json::Exception(arr1.serialize() + " should contain " + p3->serialize());

		if (!(arr1.get(0)->as_primitive()->serialize() == p1->serialize())) throw Json::Exception(arr1.get(0)->as_primitive()->serialize() + " should euqal to " + p1->serialize());
		if (!(arr1.get(1)->as_primitive()->serialize() == p2->serialize())) throw Json::Exception(arr1.get(0)->as_primitive()->serialize() + " should euqal to " + p2->serialize());
		if (!(arr1.get(2)->as_primitive()->serialize() == p3->serialize())) throw Json::Exception(arr1.get(0)->as_primitive()->serialize() + " should euqal to " + p3->serialize());
		});

	puts("");
}

void testJsonObject() {
	Json::Utils::functionWrapper("Object tests", [] {

		Json::Value p1(10);
		Json::Value p2("Hello");

		// 创建 Object 对象，并添加属性
		Json::Object obj;
		obj.insert("key1", p1);
		obj.insert("key2", p2);

		// 测试属性是否正确添加
		if (!(obj["key1"]->as_primitive()->serialize() == p1.serialize())) throw Json::Exception(obj["key1"]->as_primitive()->serialize() + " should euqal to " + p1.serialize());
		if (!(obj["key2"]->as_primitive()->serialize() == p2.serialize())) throw Json::Exception(obj["key2"]->as_primitive()->serialize() + " should euqal to " + p2.serialize());

		// 测试比较操作
		Json::Object obj2;
		obj2.insert("key1", p1);
		obj2.insert("key2", p2);

		//std::cout << obj.serialize() << std::endl;
		//std::cout << obj2.serialize() << std::endl;

		// 两个相同的 Object 应该相等
		if (obj.serialize() != obj2.serialize()) throw Json::Exception(obj.serialize() + " should equal to " + obj2.serialize());

		});

	puts("");
}

void dataStructureTest()
{
	testJsonPrimitive();
	testJsonArray();
	testJsonObject();
}

void testTokenizer() {
	Json::Utils::functionWrapper("Tokenizer test", [] {
		std::string json = R"({
						"name": "John",
						"age": 30,
						"score": 75.3,
						"isStudent": false,
						"courses": ["Math", "Science"]
					})";

		Json::InputStream input(&json);
		Json::Tokenizer tokenizer(input);

		// 测试逐个获取 token
		Json::Token token = tokenizer.next();
		assert(token.type == TokenType::ObjectBegin); // 检查 { 对应的 token

		token = tokenizer.next();
		assert(token.type == TokenType::String && token.value == "name");

		token = tokenizer.next();
		assert(token.type == TokenType::Colon);

		token = tokenizer.next();
		assert(token.type == TokenType::String && token.value == "John");

		token = tokenizer.next();
		assert(token.type == TokenType::Comma);

		token = tokenizer.next();
		assert(token.type == TokenType::String && token.value == "age");

		token = tokenizer.next();
		assert(token.type == TokenType::Colon);

		token = tokenizer.next();
		assert(token.type == TokenType::Integer && token.value == "30");

		token = tokenizer.next();
		assert(token.type == TokenType::Comma);

		token = tokenizer.next();
		assert(token.type == TokenType::String && token.value == "score");

		token = tokenizer.next();
		assert(token.type == TokenType::Colon);

		token = tokenizer.next();
		assert(token.type == TokenType::Float && token.value == "75.3");

		token = tokenizer.next();
		assert(token.type == TokenType::Comma);

		token = tokenizer.next();
		assert(token.type == TokenType::String && token.value == "isStudent");

		token = tokenizer.next();
		assert(token.type == TokenType::Colon);

		token = tokenizer.next();
		assert(token.type == TokenType::Bool && token.value == "false");

		token = tokenizer.next();
		assert(token.type == TokenType::Comma);

		token = tokenizer.next();
		assert(token.type == TokenType::String && token.value == "courses");

		token = tokenizer.next();
		assert(token.type == TokenType::Colon);

		token = tokenizer.next();
		assert(token.type == TokenType::ArrayBegin);

		token = tokenizer.next();
		assert(token.type == TokenType::String && token.value == "Math");

		token = tokenizer.next();
		assert(token.type == TokenType::Comma);

		token = tokenizer.next();
		assert(token.type == TokenType::String && token.value == "Science");

		token = tokenizer.next();
		assert(token.type == TokenType::ArrayEnd);

		token = tokenizer.next();
		assert(token.type == TokenType::ObjectEnd);
	});
	puts("");
}

void testParser() {
	Json::Utils::functionWrapper("Parser test", [] {
		std::string json = R"({
					"name": "John",
					"age": 30,
					"isStudent": false,
					"courses": ["Math", "Science"],
					"fuckJson": [{"fuckJson1":[{"json?":1}]}]
				})";

		Json::InputStream input(&json);
		Json::Tokenizer* tokenizer = new Json::Tokenizer(input);

		//Tokenizer test_tokenizer = *tokenizer;

		//test_tokenizer.next();
		//test_tokenizer.next();
		//assert(test_tokenizer.now().value != tokenizer->now().value);
		//Parser parser(new Tokenizer(input));

		Json::Element* jsonElement = Json::Parser::parse(tokenizer);

		// 确认解析的 JSON 元素类型是 JsonObject
		assert(jsonElement != nullptr);
		// 测试解析的内容
		Json::Element* nameElement = jsonElement->as_object()->get_ptr("name");
		assert(nameElement != nullptr && nameElement->serialize() == "\"John\"");

		Json::Element* ageElement = jsonElement->as_object()->get_ptr("age");
		assert(ageElement != nullptr && ageElement->serialize() == "30");

		Json::Element* isStudentElement = jsonElement->as_object()->get_ptr("isStudent");
		assert(isStudentElement != nullptr && isStudentElement->serialize() == "false");

		Json::Element* coursesElement = jsonElement->as_object()->get_ptr("courses");
		assert(coursesElement != nullptr);

		Json::Array coursesArray = *(coursesElement->as_array());

		assert(coursesArray.size() == 2);
		std::cout << coursesArray.serialize() << std::endl;
		std::cout << coursesArray[0]->serialize() << std::endl;
		//std::cout<<coursesArray->
		assert(coursesArray[0]->serialize() == "\"Math\"");
		assert(coursesArray[1]->serialize() == "\"Science\"");

		delete jsonElement;  // 清理内存
		delete tokenizer;
		});

	puts("");
}

void testParseJson() {
	Json::Element* element = nullptr;
	//std::stringstream fileStream;	// 这个要放外面，要不然引用会被释放
	Json::Tokenizer* tokenizer;
	std::string origin_str;
	Json::InputStream* stream;

	Json::Utils::functionWrapper("File to tokenizer test", [&origin_str, &tokenizer, &stream] {
		//std::string filename = "Tests/test.json";
		std::string filename = "Tests/001・アーサー王ver1.07.ks.json";
		origin_str = Json::Utils::readFileStream(filename).str();
		stream = new Json::InputStream(&origin_str);
		tokenizer = new Json::Tokenizer(*stream);
		});
	puts("");

	//element = Parser::parse(tokenizer);

	Json::Utils::functionWrapper("Parse tokenizer test", [&element, &tokenizer] {
		element = Json::Parser::parse(tokenizer);
		});
	puts("");

	Json::Utils::functionWrapper("Element to string test", [&element] {
		element->serialize();
		});
	puts("");

	delete element;
}

void tokenizerTest()
{
	testTokenizer();
	testParser();
	testParseJson();
}

int main()
{
	dataStructureTest();

	tokenizerTest();

	getchar();
}