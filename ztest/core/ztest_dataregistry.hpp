/*
 * @Author: starAndHonor 13750616920@163.com
 * @Date: 2025-05-30 16:57:15
 * @LastEditors: starAndHonor 13750616920@163.com
 * @LastEditTime: 2025-05-30 17:10:56
 * @FilePath: /ztest/ztest/core/ztest_dataregistry.hpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置
 * 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once
#include "ztest_parameterized.hpp"
#include <memory>
#include <string>
#include <unordered_map>
class ZTestDataRegistry {
public:
  // 单例模式
  static ZTestDataRegistry &getInstance() {
    static ZTestDataRegistry instance;
    return instance;
  }

  // 注册数据
  template <typename Input, typename Output>
  void registerData(const std::string &key,
                    const ZTestDataManager<Input, Output> &data) {
    dataMap[key] = std::make_shared<ZTestDataManager<Input, Output>>(data);
  }

  // 获取数据
  template <typename Input, typename Output>
  ZTestDataManager<Input, Output> &getData(const std::string &key) {
    auto it = dataMap.find(key);
    if (it == dataMap.end()) {
      throw std::runtime_error("Test data not found: " + key);
    }
    // 使用标准命名 std::static_pointer_cast
    return *std::static_pointer_cast<ZTestDataManager<Input, Output>>(
        it->second);
  }

private:
  ZTestDataRegistry() = default;
  ~ZTestDataRegistry() = default;
  ZTestDataRegistry(const ZTestDataRegistry &) = delete;
  ZTestDataRegistry &operator=(const ZTestDataRegistry &) = delete;

  std::unordered_map<std::string, std::shared_ptr<void>> dataMap;
};