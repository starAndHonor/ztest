

#pragma once
#include "ztest_base.hpp"
#include "ztest_utils.hpp"
#include <any>
#include <exception>
#include <functional>
template <typename InputType, typename OutputType> class ZTestDataManager {

public:
  using Input = InputType;
  using Output = OutputType;
  ZTestDataManager(initializer_list<pair<Input, Output>> cases)
      : _data(cases) {}

  auto current() const { return _data[_index]; }
  bool has_next() const { return _index < _data.size(); }
  void next() { ++_index; }
  void reset() { _index = 0; }

protected:
  vector<pair<Input, Output>> _data;
  size_t _index = 0;
};

class ZTestCSVDataManager
    : public ZTestDataManager<std::vector<CSVCell>, CSVCell> {
public:
  ZTestCSVDataManager(const string &filename)
      : ZTestDataManager({}), _filename(filename) {
    // Load and parse CSV data
    CSVStream stream(filename);
    vector<vector<CSVCell>> csv_data;
    stream >> csv_data;

    // Populate parent class's _data
    _data.clear();
    for (auto &row : csv_data) {
      auto output = row.back();
      row.pop_back();
      _data.push_back(make_pair(row, output));
    }
    _total_cases = _data.size();
  }

  // Keep all existing methods
  void printSummary() const {
    cout << "=== CSV Test Data Summary ===" << endl;
    cout << "Source File: " << _filename << endl;
    cout << "Total Test Cases: " << _total_cases << endl;
    cout << "Data Format: Each row contains "
         << (_data.empty() ? 0 : _data[0].first.size())
         << " input(s) and 1 output" << endl;
  }

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
class ZTestParameterized : public ZTestBase {
protected:
  ZTestDataManager<Input, Output> &_data;

public:
  ZTestParameterized(const string &name, ZType type, const string &desc,
                     ZTestDataManager<Input, Output> &data)
      : ZTestBase(name, type, desc), _data(data) {}

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
