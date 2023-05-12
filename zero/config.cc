#include "config.h"
#include "zero/log.h"
#include "zero/mutex.h"
#include <algorithm>
#include <bits/stdint-uintn.h>
#include <cctype>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <utility>
#include <vector>
#include <yaml-cpp/node/node.h>

namespace zero {

static zero::Logger::ptr g_logger = ZERO_LOG_NAME("system");

ConfigVarBase::ptr Config::LookupBase(const std::string &name) {
    RWMutexType::ReadLock lock(GetMutex());
    auto it = GetDatas().find(name);
    return it == GetDatas().end() ? nullptr : it->second;
}

static void ListAllMember(const std::string& prefix, const YAML::Node& node,
                          std::list<std::pair<std::string, const YAML::Node>>& output) {
    if (prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") != std::string::npos) {
        ZERO_LOG_ERROR(g_logger) << "Config invalid name: " << prefix << " : " << node;
        return;
    }
    output.push_back(std::make_pair(prefix, node));
    if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            ListAllMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output);
        }
    }
}

void Config::LoadFromYaml(const YAML::Node& root) {
    std::list<std::pair<std::string, const YAML::Node>> all_nodes;
    ListAllMember("", root, all_nodes);
    for (auto& i : all_nodes) {
        std::string key = i.first;
        if (key.empty()) {
            continue;
        }

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        ConfigVarBase::ptr var = LookupBase(key);

        // 加载过程中，如果发现已经存在key，说明需要调用回调函数进行动态变更
        if (var) {
            if (i.second.IsScalar()) {
                var->fromString(i.second.Scalar());
            } else {
                std::stringstream ss;
                ss << i.second;
                var->fromString(ss.str());
            }
        }
    }
}

/// 记录每个文件的修改时间
static std::map<std::string, uint64_t> s_file2modifytime;
/// 是否强制加载配置文件，非强制加载情况下，如果记录的文件修改时间未变化，则跳过该文件的加载
/// 用在LoadFromConfDir()函数当中
static zero::Mutex s_mutex;

/**
 * @brief TODO: 暂未完成
 * 
 * @param path 
 * @param force 
 */
void Config::LoadFromConfDir(const std::string& path, bool force) {
    /// TODO:env
    std::string absoulte_path = "";
    std::vector<std::string> files;
    /// TODO: FSUtil
}

void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
    RWMutexType::ReadLock lock(GetMutex());
    ConfigVarMap& m = GetDatas();
    for (auto it = m.begin(); it != m.end(); ++it) {
        cb(it->second);
    }
}

}  // namespace zero