
#pragma once
#include "ztest_types.hpp"
#include <bits/unique_ptr.h>
#include <functional>
#include <string>

using namespace std;
// ZtestInterface接口,定义了必须实现的方法。
class ZTestInterface {
public:
  explicit ZTestInterface() = default;
  virtual ~ZTestInterface() = default;
  virtual const string &getName() const = 0;
  virtual const ZType getType() const = 0;
  virtual ZState run() = 0;
};

// ZtestBase测试基类,定义了一些属性
class ZTestBase : public ZTestInterface {
private:
  string _name;
  ZType _type;
  string _description;
  ZState _state;
  vector<function<void()>> _before_all_hooks;
  vector<function<void()>> _after_each_hooks;
  vector<function<void()>> _after_all_hooks;

public:
  ZTestBase(string name, ZType type, string description)
      : _name(name), _type(type), _description(description) {}
  ~ZTestBase() override = default;
  const string &getName() const override { return _name; }
  const ZType getType() const override { return _type; }

  virtual unique_ptr<ZTestBase> clone() const = 0;

  virtual const ZState getState() const { return _state; }

  virtual void setState(ZState state) { _state = state; }

  virtual const string getDescription() const { return _description; }
  /**
 * @description: 设置测试用例的描述信息
 * @param description 要设置的描述
 */
  virtual void setDescription(string description) {
    _description = description;
  }
  /**
  * @description: 添加测试前的钩子函数
  * @param hook 要添加的钩子函数
  * @return 当前测试用例对象的引用
  */
  virtual ZTestBase &addBeforeAll(function<void()> hook) {
    _before_all_hooks.push_back(move(hook));
    return *this;
  }
  /**
   * @description: 执行所有测试前的钩子函数
   */
  virtual void runBeforeAll() {
    for (auto &hook : _before_all_hooks) {
      hook();
    }
  }
  /**
 * @description: 添加每个测试后的钩子函数
 * @param hook 要添加的钩子函数
 * @return 当前测试用例对象的引用
 */
  virtual ZTestBase &addAfterEach(function<void()> hook) {
    _after_each_hooks.push_back(move(hook));
    return *this;
  }
  /**
 * @description: 执行所有每个测试后的钩子函数
 */
  virtual void runAfterEach() {
    for (auto &hook : _after_each_hooks) {
      hook();
    }
  }
  /**
 * @description: 执行所有测试后的钩子函数
 */
  virtual void runAfterAll() {
    for (auto &hook : _after_all_hooks) {
      hook();
    }
  }
  /**
 * @description: 添加测试后的钩子函数
 * @param hook 要添加的钩子函数
 * @return 当前测试用例对象的引用
 */
  virtual ZTestBase &addAfterAll(function<void()> hook) {
    _after_all_hooks.push_back(move(hook));
    return *this;
  }
};
