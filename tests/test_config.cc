#include "../zero/config.h"
#include "../zero/log.h"
#include <bits/stdint-uintn.h>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>

zero::Logger::ptr g_logger = ZERO_LOG_ROOT();

class Test_Functor {
public:
    void operator()(const int val) {
        ZERO_LOG_INFO(g_logger) << val;
    }
};

// 仿函数测试
void Test_MyFunctor() {
    Test_Functor()(5);
    Test_Functor tf;
    tf(6);
}

// 简单类型转换
void Test_Lexical() {
    // 仿函数形式使用
    std::string str = zero::LexicalCast<int, std::string>()(5);
    ZERO_LOG_INFO(g_logger) << str;
    int num = zero::LexicalCast<std::string, int>()("5");
    ZERO_LOG_INFO(g_logger) << num;
    // zero::LexicalCast<class F, class T> 不支持复杂类型转换
    std::string str_vec = zero::LexicalCast<std::vector<int>, std::string>()(std::vector<int>{ 23 });
    ZERO_LOG_INFO(g_logger) << str_vec;
}

// 复杂类型转换
void Test_Lexical_Vec() {
    // std::vector<int> vec{1,2,3};
    std::vector<std::string> v = zero::LexicalCast<std::string, std::vector<std::string>>()("[name: Brewers, city: Milwaukee]");
    for (auto& i : v) {
        std::cout << i << std::endl;
    }
    std::string str = zero::LexicalCast<std::vector<std::string>, std::string>()(v);
    std::cout << str << std::endl;
}

// Yaml的使用
void Test_Yaml() {
    // loadFile必须是绝对路径
    YAML::Node node = YAML::LoadFile("/home/ubuntu/Zero/tests/test_config.yml");
    // ZERO_LOG_INFO(g_logger) << node;

    YAML::Node node1 = YAML::Load("[name: Brewers, city: Milwaukee]");
    std::cout << node1.size() << std::endl;
    std::stringstream ss;
    for (size_t i = 0; i < node1.size(); ++i) {
        ss.str("");
        ss << node1[i];
        std::cout << ss.str() << std::endl;
    }

    if (node1["name"]) {
        std::cout << node1["name"].as<std::string>() << "\n";
    }
    if (node1["mascot"]) {
        std::cout << node1["mascot"].as<std::string>() << "\n";
    }
}

uint64_t Set_Key() {
    static uint64_t key = 0;
    key++;
    return key;
}

void Test_LoadFromYaml() {
    std::string str = "abc";
    // 必须属于限定字符
    // 发现不属于限定字符，返回发现位置，否则返回std::string::npos
    // auto index = str.find_first_not_of("a");
    // if(index == std::string::npos) {
    //     std::cout << index << std::endl;
    // }
    // std::cout << index << std::endl;

    YAML::Node root = YAML::LoadFile("/home/ubuntu/Zero/conf/test_config.yml");
    std::cout << root.size() << std::endl;
    zero::Config::LoadFromYaml(root);
}

void Get_Key() {
    for (int i = 0; i < 10; ++i) {
        std::cout << Set_Key() << " ";
    }
}

// 简单类型配置
void Test_Simple_Config() {

    // Lookup会构造一个ConfigVar对象，在该类构造函数里会初始化配置值，同时会将name与value做上映射
    zero::ConfigVar<int>::ptr g_int = zero::Config::Lookup("global.int", ( int )8888, "global int");
    ZERO_LOG_INFO(g_logger) << g_int->getValue();

    // 此处设置上回调函数
    g_int->addListener([](const int& old_value, const int& new_value) {
        ZERO_LOG_INFO(g_logger) << "g_int value changed, old_value: " << old_value << ", new_value: " << new_value;
    });

    // 加载过程中发生配置变更
    YAML::Node node = YAML::LoadFile("/home/ubuntu/Zero/conf/test_config_cb.yml");
    zero::Config::LoadFromYaml(node);
    /*
    堆栈如下
    (gdb) bt
    #0  zero::ConfigVar<int, zero::LexicalCast<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, int>, zero::LexicalCast<int, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::setValue (this=0x526190, v=@0x7fffffffd70c: 9999) at /home/ubuntu/Zero/tests/../zero/config.h:430
    #1  0x00000000004910af in zero::ConfigVar<int, zero::LexicalCast<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, int>, zero::LexicalCast<int, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::fromString (this=0x526190, val="9999") at /home/ubuntu/Zero/tests/../zero/config.h:404
    #2  0x00007ffff7ea8f32 in zero::Config::LoadFromYaml (root=...) at /home/ubuntu/Zero/zero/config.cc:55
    #3  0x0000000000481350 in Test_Simple_Config () at /home/ubuntu/Zero/tests/test_config.cc:112
    #4  0x0000000000481494 in main () at /home/ubuntu/Zero/tests/test_config.cc:122
    */
}

// 自定义类型配置
class Person {
public:
    Person(){};
    std::string m_name = "default";
    int m_age = 0;
    bool m_sex = 0;

    std::string toString() const {
        std::stringstream ss;
        ss << "[Person name=" << m_name << " age=" << m_age << " sex=" << m_sex << "]";
        return ss.str();
    }
    bool operator==(const Person& oth) const {
        return m_name == oth.m_name && m_age == oth.m_age && m_sex == oth.m_sex;
    }
};

// 自定义配置的序列化和反序列化
using namespace zero;

// 偏特化
template<>
class LexicalCast<std::string, Person> {
public:
    Person operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();
        return p;
    }
};

template<>
class LexicalCast<Person, std::string> {
public:
    std::string operator()(const Person &p) {
        YAML::Node node;
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

void Test_Complex_Config() {
    ConfigVar<Person>::ptr g_person = Config::Lookup("class.person", Person(), "system person");
    g_person->addListener([](const Person &old_value, const Person &new_value){
            ZERO_LOG_INFO(g_logger) << "g_person value change, old value:" << old_value.toString()
                << ", new value:" << new_value.toString();
    });

    ZERO_LOG_INFO(g_logger) << g_person->getValue().toString();

    YAML::Node node = YAML::LoadFile("/home/ubuntu/Zero/conf/test_config_cb.yml");
    zero::Config::LoadFromYaml(node);
    ZERO_LOG_INFO(g_logger) << g_person->getValue().toString();

}


int main() {
    Test_MyFunctor();
    Test_Lexical();
    Test_Yaml();
    Test_Lexical_Vec();
    Get_Key();
    Test_LoadFromYaml();
    Test_Simple_Config();
    Test_Complex_Config();
    return 0;
}