/**
 * @file config.h
 * @author ggbond (you@domain.com)
 * @brief 配置模块学习
 * @version 0.1
 * @date 2023-05-06
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __ZERO_CONFIG_H___
#define __ZERO_CONFIG_H___

#include <algorithm>
#include <bits/stdint-uintn.h>
#include <boost/lexical_cast.hpp>
#include <cstddef>
#include <exception>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/node/type.h>
#include <yaml-cpp/yaml.h>

#include "log.h"
#include "util.h"
// TODO:mutex.h

namespace zero {

/**
 * @brief 配置变量基类
 * 
 */
class ConfigVarBase {
public:
    using ptr = std::shared_ptr<ConfigVarBase>;
    /**
     * @brief 构造函数
     * 
     */
    ConfigVarBase(const std::string& name, const std::string& description = "") : m_name(name), m_description(description) {
        // 直接转成小写字母
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }

    /**
     * @brief 析构函数
     * 
     */
    virtual ~ConfigVarBase() {}

    /**
      * @brief 返回配置参数名称
      * 
     */
    const std::string& getName() const {
        return m_name;
    }

    /**
     * @brief 返回配置参数描述
     * 
     */
    const std::string& getDescription() const {
        return m_description;
    }

    /**
      * @brief 转成字符串
      * 
     */
    virtual std::string toString() = 0;

    /**
     * @brief 由字符串进行初始化
     * 
     */
    virtual bool fromString(const std::string& val) = 0;

    /**
      * @brief 返回配置类型名称
      * 
      */
    virtual std::string getTypeName() const = 0;

protected:
    /// 配置参数名称
    std::string m_name;
    /// 配置参数描述
    std::string m_description;
};

/**
 * @brief 简单类型转换模板
 * 
 * @tparam F 源类型
 * @tparam T 目标类型
 */
template <class F, class T>
class LexicalCast {
public:
    /**
   * @brief 类型转换
   * 
   * @param v 源类型
   * @return T 返回目标类型
   */
    T operator()(const F& v) {
        return boost::lexical_cast<T>(v);
    }
};

/**
 * @brief 具体化/偏特化，由YAML String 转换为 std::vector<T>
 * 
 */
template <class T>
class LexicalCast<std::string, std::vector<T>> {
public:
    std::vector<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        // 限定名，防止vector内部二义性
        typename std::vector<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            // 每次清空一下ss,再放入vector
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

/**
 * @brief 偏特化 std::vector<T>转 std::string
 * 
 * @tparam T 
 */
template <class T>
class LexicalCast<std::vector<T>, std::string> {
public:
    std::string operator()(const std::vector<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 偏特化 std::string 转 std::list<T>
 * 
 * @tparam T 
 */
template <class T>
class LexicalCast<std::string, std::list<T>> {
public:
    std::list<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::list<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

/**
 * @brief 偏特化 std::list<T> 转 std::string
 * 
 * @tparam T 
 */
template <class T>
class LexicalCast<std::list<T>, std::string> {
public:
    std::string operator()(const std::list<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 偏特化 std::string 转 std::set<T>
 * 
 * @tparam T 
 */
template <class T>
class LexicalCast<std::string, std::set<T>> {
public:
    std::set<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::set<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

/**
 * @brief 偏特化 std::set<T> 转 std::string
 * 
 * @tparam T 
 */
template <class T>
class LexicalCast<std::set<T>, std::string> {
public:
    std::string operator()(const std::set<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 偏特化 std::string 转 std::unordered_set<T>
 * 
 * @tparam T 
 */
template <class T>
class LexicalCast<std::string, std::unordered_set<T>> {
public:
    std::unordered_set<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_set<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

/**
 * @brief 偏特化 std::unordered_set<T> 转 std::string
 * 
 * @tparam T 
 */
template <class T>
class LexicalCast<std::unordered_set<T>, std::string> {
public:
    std::string operator()(const std::unordered_set<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 偏特化 std::string 转 std::map<std::string, T>
 * 
 * @tparam T 
 */
template <class T>
class LexicalCast<std::string, std::map<std::string, T>> {
public:
    std::map<std::string, T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::map<std::string, T> vec;
        std::stringstream ss;
        for (auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

/**
 * @brief 偏特化 std::map<std::string, T> 转 std::string
 * 
 * @tparam T 
 */
template <class T>
class LexicalCast<std::map<std::string, T>, std::string> {
public:
    std::string operator()(const std::map<std::string, T>& v) {
        YAML::Node node(YAML::NodeType::Map);
        for (auto& i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 偏特化 std::string 转 std::unordered_map<std::string, T>
 * 
 * @tparam T 
 */
template <class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>> {
public:
    std::unordered_map<std::string, T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_map<std::string, T> vec;
        std::stringstream ss;
        for (auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

/**
 * @brief 偏特化 std::unordered_map<std::string, T> 转 std::string>
 * 
 * @tparam T 
 */
template <class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
    std::string operator()(const std::unordered_map<std::string, T>& v) {
        YAML::Node node(YAML::NodeType::Map);
        for (auto& i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template <class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase {
public:
    /// TODO: 读写锁
    typedef std::shared_ptr<ConfigVar> ptr;
    /// 动态配置，当配置更改时，程序运行过程中改变配置
    typedef std::function<void(const T& old_value, const T& new_value)> on_change_cb;

    ConfigVar(const std::string& name, const T& default_value, const std::string& description = "")
        : ConfigVarBase(name, description), m_val(default_value) {}

    /**
     * @brief 将参数值转换为YAML String
     * 
     * @return std::string 
     */
    std::string toString() override {
        try {
            /// TODO:锁
            return ToStr()(m_val);
        } catch (std::exception& e) {
            /// TODO:Util::TypeToName()
            ZERO_LOG_ERROR(ZERO_LOG_ROOT()) << "ConfigVar::toString exception " << e.what()
                                            << " convert: " << std::string("TypeToName<T>()") << " to string"
                                            << " name=" << m_name;
        }
        return "";
    }

    /**
     * @brief 从YAML String转成参数的值，同时如果val发生了变化会通知回调函数
     *          
     * @param val 
     * @return true 
     * @return false 
     */
    bool fromString(const std::string& val) override {
        try {
            setValue(FromStr()(val));
        } catch (std::exception& e) {
            /// TODO:Util::TypeToName()
            ZERO_LOG_ERROR(ZERO_LOG_ROOT()) << "ConfigVar::fromString exception " << e.what() << " convert: string to "
                                            << std::string("TypeToName<T>()") << " name=" << m_name << " - " << val;
        }
        return false;
    }

    /**
     * @brief Get the Value object获取当前参数的值
     * 
     * @return const T 
     */
    const T getValue() {
        /// TODO:锁
        return m_val;
    }

    /**
     * @brief 可以设置参数的值，如果参数值发生变化，将通知注册的回调函数
     * 
     */
    void setValue(const T& v) {
        {
            /// TODO:锁
            if (v == m_val) {
                return;
            }
            for (auto& i : m_cbs) {
                i.second(m_val, v);
            }
        }
        /// TODO:锁
        m_val = v;
    }

    std::string getTypeName() const override {
        /// TODO:Util:TypeToName
        return "";
    }

    /**
     * @brief 注册回调函数
     * 
     * @param cb 
     * @return uint64_t 
     */
    uint64_t addListener(on_change_cb cb) {
        static uint64_t s_fun_id = 0;
        /// TODO:锁
        ++s_fun_id;
        m_cbs[s_fun_id] = cb;
        return s_fun_id;
    }

    /**
     * @brief 删除回调
     * 
     * @param key 
     */
    void delListener(uint64_t key) {
        /// TODO:锁
        m_cbs.erase(key);
    }

    /**
     * @brief 获取回调通过key
     * 
     * @param key 
     * @return on_change_cb 
     */
    on_change_cb getListener(uint64_t key) {
        /// TODO:锁
        auto it = m_cbs.find(key);
        return it == m_cbs.end() ? nullptr : it->second;
    }

    /**
     * @brief 删除所有回调
     * 
     */
    void clearListener() {
        /// TODO:锁
        m_cbs.clear();
    }

private:
    /// TODO:锁
    T m_val;
    /// 变更配置的回调函数集 uint64_t key要求唯一 可用hash
    std::map<uint64_t, on_change_cb> m_cbs;
};

/**
 * @brief ConfigVar的管理类，提供方法访问ConfigVar
 * 
 */
class Config {
public:
    typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;
    /// TODO:锁
    
    /**
     * @brief 根据name获取/创建对应参数名的配置,如果name不存在，则创建配置并使用默认参数
              
     * 
     * @tparam T 
     * @param name 
     * @param default_value 默认参数
     * @param description 
     * @return ConfigVar<T>::ptr 
     */
    template <class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name, const T& default_value, const std::string& description = "") {
        /// TODO:锁
        auto it = GetDatas().find(name);
        if (it != GetDatas().end()) {
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
            if (tmp) {
                ZERO_LOG_INFO(ZERO_LOG_ROOT()) << "Lookup name=" << name << " exists";
                return tmp;
            } else {
                ZERO_LOG_ERROR(ZERO_LOG_ROOT()) << "Lookup name=" << name << " exists but type not " << std::string("TypeToName<T>()")
                                                << " real_type=" << it->second->getTypeName() << " " << it->second->toString();
                return nullptr;
            }
        }

        if (name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") != std::string::npos) {
            ZERO_LOG_ERROR(ZERO_LOG_ROOT()) << "Lookup name invalid" << name;
            throw std::invalid_argument(name);
        }

        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
        GetDatas()[name] = v;
        return v;
    }

    template <class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name) {
        /// TODO:锁
        auto it = GetDatas().find(name);
        if (it == GetDatas().end()) {
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }

    /**
     * @brief 使用YAML::Node 初始化配置模块
     * 
     * @param root 
     */
    static void LoadFromYaml(const YAML::Node &root);

    /**
     * @brief 加载path文件夹里面的配置文件
     * 
     * @param path 
     * @param force 
     */
    static void LoadFromConfDir(const std::string &path, bool force = false);

    /**
     * @brief 查找配置参数，返回配置基类
     * 
     * @param name 
     * @return ConfigVarBase::ptr 
     */
    static ConfigVarBase::ptr LookupBase(const std::string &name);

    /**
     * @brief 遍历配置模块里面的配置项
     * 
     * @param cb 
     */
    static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

private:
    /**
     * @brief 返回所有的配置
     * 
     * @return ConfigVarMap& 
     */
    static ConfigVarMap& GetDatas() {
        static ConfigVarMap s_datas;
        return s_datas;
    }

    /// TODO:锁
};

}  // namespace zero

#endif