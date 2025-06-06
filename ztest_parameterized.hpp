

#pragma once
#include "ztest_base.hpp"
#include "ztest_utils.hpp"
#include <any>
#include <exception>
#include <functional>
//定义数据管理器的抽象接口，用于管理测试数据。
class ZDataManager {
public:
  virtual ~ZDataManager() = default;
 /**
 * @description: 获取数据管理器的名称
 * @return 数据管理器名称的常量引用
 */
  virtual const std::string &getName() const = 0;
  /**
 * @description: 获取数据集中的测试用例数量
 * @return 测试用例的数量
 */
  virtual size_t size() const = 0;
};
template <typename InputType, typename OutputType> class ZTestDataManager {

public:
  using Input = InputType;
  using Output = OutputType;
  /**
 * @description: 构造函数，使用初始化列表初始化测试数据集
 * @param cases 包含输入-输出对的初始化列表
 */
  ZTestDataManager(initializer_list<pair<Input, Output>> cases)
      : _data(cases) {}
  /**
 * @description: 获取当前索引对应的测试用例
 * @return 当前测试用例的输入-输出对
 */
  auto current() const { return _data[_index]; }
  /**
 * @description: 判断是否存在下一个测试用例
 * @return 存在下一个测试用例返回true，否则返回false
 */
  bool has_next() const { return _index < _data.size(); }
  /**
 * @description: 移动到下一个测试用例
 */
  void next() { ++_index; }
  /**
 * @description: 重置索引到第一个测试用例
 */
  void reset() { _index = 0; }

protected:
  vector<pair<Input, Output>> _data;
  size_t _index = 0;
};
//从 CSV 文件加载参数化测试数据的具体实现类。
class ZTestCSVDataManager
    : public ZTestDataManager<std::vector<CSVCell>, CSVCell>,
      public ZDataManager {
public:
    /**
 * @description: 获取CSV数据管理器的名称
 * @return 文件名的常量引用
 */
  const std::string &getName() const override { return _filename; }
  /**
 * @description: 获取CSV数据集中的测试用例数量
 * @return 测试用例的数量
 */
  size_t size() const override { return _total_cases; }
  ZTestCSVDataManager(const string &filename)
      : ZTestDataManager({}), _filename(filename) {
    logger.debug("Initializing CSV data manager for: " + filename);

    CSVStream stream(filename);
    vector<vector<CSVCell>> csv_data;
    stream >> csv_data;

    logger.info("Loaded " + std::to_string(csv_data.size()) +
                " rows from CSV file");

    _data.clear();
    for (auto &row : csv_data) {
      auto output = row.back();
      row.pop_back();
      _data.push_back(make_pair(row, output));
    }
    _total_cases = _data.size();

    logger.debug("Processed " + std::to_string(_total_cases) + " test cases");
  }
  /**
 * @description: 打印CSV数据集的摘要信息
 */
  void printSummary() const {
    cout << "=== CSV Test Data Summary ===" << endl;
    cout << "Source File: " << _filename << endl;
    cout << "Total Test Cases: " << _total_cases << endl;
    cout << "Data Format: Each row contains "
         << (_data.empty() ? 0 : _data[0].first.size())
         << " input(s) and 1 output" << endl;
  }
  /**
 * @description: 打印CSV数据集的详细信息
 */
  void dumpData() const {
    cout << "\n=== Detailed Test Data ===" << endl;
    for (size_t i = 0; i < _data.size(); ++i) {
      cout << "Case " << i + 1 << ":" << endl;
      cout << "  Inputs: [";
      for (const auto &input : _data[i].first) {
        std::visit([](const auto &value) { cout << value << " "; }, input);
      }
      cout << "]" << endl;
      cout << "  Output: ";
      std::visit([](const auto &value) { cout << value; }, _data[i].second);
      cout << endl;
    }
  }

private:
  string _filename;
  size_t _total_cases = 0;
};
template <typename Input, typename Output>
//参数化测试的基类，实现测试用例的批量执行逻辑。
class ZTestParameterized : public ZTestBase {
protected:
  ZTestDataManager<Input, Output> &_data;

public:
  ZTestParameterized(const string &name, ZType type, const string &desc,
                     ZTestDataManager<Input, Output> &data)
      : ZTestBase(name, type, desc), _data(data) {}
  /**
 * @description: 执行参数化测试，遍历所有测试用例
 * @return 测试执行状态（成功或失败）
 */
  ZState run() override {
    _data.reset();
    while (_data.has_next()) {
      if (run_single_case() != ZState::z_success)
        return ZState::z_failed;
      _data.next();
    }
    return ZState::z_success;
  }

  virtual ZState run_single_case() = 0;
};
