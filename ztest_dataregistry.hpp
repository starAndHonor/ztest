#pragma once
#include "ztest_parameterized.hpp"
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>
//ZDataRegistry 类是一个数据注册与缓存管理类，采用单例模式实现，用于管理测试数据的加载和缓存
class ZDataRegistry {
public:
  static ZDataRegistry &instance() {
    static ZDataRegistry reg;
    return reg;
  }
  void setMaxSize(size_t max) {
    std::lock_guard<std::mutex> lock(_mutex);
    _max_size = max;
    logger.info("Set LRU cache max size to: " + std::to_string(max));
  }

  template <typename T> std::shared_ptr<T> load(const std::string &filePath) {
    std::lock_guard<std::mutex> lock(_mutex);

    logger.debug("Checking cache for: " + filePath);
   /**
   * @description: LRU访问更新
   */
    if (auto it = _cache_map.find(filePath); it != _cache_map.end()) {
      logger.debug("Cache hit for: " + filePath);
      _lru_list.splice(_lru_list.begin(), _lru_list, it->second.second);
      return std::static_pointer_cast<T>(it->second.first);
    }
    /**
   * @description: 创建新加载器
   */
    logger.info("Loading new file: " + filePath);
    auto loader = std::make_shared<T>(filePath);
    _lru_list.push_front(filePath);
    /**
  * @description: 插入新条目
  */
    
    _cache_map[filePath] = {loader, _lru_list.begin()};
    logger.debug("Cached file: " + filePath +
                 " | Size: " + std::to_string(loader->size()));
    
  /**
  * @description: LRU淘汰
  */
    if (_max_size > 0 && _cache_map.size() > _max_size) {
      auto last = _lru_list.back();
      logger.debug("Evicting LRU cache item: " + last);
      _cache_map.erase(last);
      _lru_list.pop_back();
    }

    return loader;
  }
  /**
  * @description:获取缓存数据
  */
  
  template <typename T> std::shared_ptr<T> get(const std::string &filePath) {
    std::lock_guard<std::mutex> lock(_mutex);
    logger.debug("Getting cache for: " + filePath);
    if (auto it = _cache_map.find(filePath); it != _cache_map.end()) {
      return std::static_pointer_cast<T>(it->second);
    }
    return nullptr;
  }
  /**
* @description: 清空指定缓存
*/
 
  void clear(const std::string &filePath) {
    std::lock_guard<std::mutex> lock(_mutex);
    _cache_map.erase(filePath);
  }
  void printCacheStats() const {
    std::lock_guard<std::mutex> lock(_mutex);
    size_t total_size = 0;
    for (const auto &[path, entry] : _cache_map) {
      total_size += entry.first->size() * sizeof(CSVCell);
    }

    logger.info("LRU Cache Status:");
    logger.info("- Max Size: " + std::to_string(_max_size));
    logger.info("- Current Items: " + std::to_string(_cache_map.size()));
    logger.info("- Total Memory: " + std::to_string(total_size / 1024) + " KB");
    logger.info("- Hit Rate: " +
                std::to_string(_hit_count * 100 / (_hit_count + _miss_count)) +
                "%");
  }

private:
  ZDataRegistry() = default;

  mutable std::mutex _mutex;
  size_t _max_size = 0; // 等于0的时候不启用LRU,大于0的时候启用LRU
  std::list<std::string> _lru_list;
  std::unordered_map<std::string, std::pair<std::shared_ptr<ZDataManager>,
                                            std::list<std::string>::iterator>>
      _cache_map;

  mutable size_t _hit_count = 0;
  mutable size_t _miss_count = 0;
};
