// ztest_utils.hpp
#pragma once
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant> // 使用 variant 需要包含该头文件
#include <vector>

// 定义 CSV 单元格支持的类型
using CSVCell = std::variant<int, double, std::string>;

class CSVStream {
public:
  // 构造函数（增加 strictMode 参数用于严格类型检查）
  CSVStream(const std::string &filename, const std::string &delimiter = ",",
            bool strictMode = false)
      : filename(filename), delimiter(delimiter), mode("overwrite"),
        strictMode(strictMode) {}

  // 设置模式：覆写（overwrite）或追加（append）
  CSVStream &setMode(const std::string &mode) {
    this->mode = mode;
    return *this;
  }

  // -------------------- 原有字符串版本接口保持兼容 --------------------
  // 重载 >> 运算符，用于读取CSV文件（字符串版本）
  CSVStream &operator>>(std::vector<std::vector<std::string>> &data) {
    std::ifstream file(filename);
    if (!file.is_open()) {
      std::cerr << "Error opening file: " << filename << std::endl;
      return *this;
    }

    std::string line;
    while (std::getline(file, line)) {
      std::vector<std::string> row;
      std::stringstream ss(line);
      std::string cell;
      while (std::getline(ss, cell, delimiter[0])) {
        row.push_back(cell);
      }
      data.push_back(row);
    }

    file.close();
    return *this;
  }

  // 重载 << 运算符，用于写入CSV文件（字符串版本）
  CSVStream &operator<<(const std::vector<std::vector<std::string>> &data) {
    std::ofstream file;
    if (mode == "append") {
      file.open(filename, std::ios::app); // 追加模式
    } else {
      file.open(filename, std::ios::out | std::ios::trunc); // 覆写模式
    }

    if (!file.is_open()) {
      std::cerr << "Error opening file: " << filename << std::endl;
      return *this;
    }

    for (const auto &row : data) {
      for (size_t i = 0; i < row.size(); ++i) {
        file << row[i];
        if (i < row.size() - 1) {
          file << delimiter;
        }
      }
      file << "\n";
    }

    file.close();
    return *this;
  }

  // -------------------- 新增多类型版本接口 --------------------
  // 重载 >> 运算符，用于读取CSV文件（多类型版本）
  CSVStream &operator>>(std::vector<std::vector<CSVCell>> &data) {
    std::ifstream file(filename);
    if (!file.is_open()) {
      std::cerr << "Error opening file: " << filename << std::endl;
      return *this;
    }

    std::string line;
    while (std::getline(file, line)) {
      std::vector<CSVCell> row;
      std::stringstream ss(line);
      std::string cell;
      while (std::getline(ss, cell, delimiter[0])) {
        // 自动类型推断
        if (isInteger(cell)) {
          row.push_back(std::stoi(cell));
        } else if (isDouble(cell)) {
          row.push_back(std::stod(cell));
        } else {
          row.push_back(cell);
        }
      }
      data.push_back(row);
    }

    file.close();
    return *this;
  }

  // 重载 << 运算符，用于写入CSV文件（多类型版本）
  CSVStream &operator<<(const std::vector<std::vector<CSVCell>> &data) {
    std::ofstream file;
    if (mode == "append") {
      file.open(filename, std::ios::app);
    } else {
      file.open(filename, std::ios::out | std::ios::trunc);
    }

    if (!file.is_open()) {
      std::cerr << "Error opening file: " << filename << std::endl;
      return *this;
    }

    for (const auto &row : data) {
      for (size_t i = 0; i < row.size(); ++i) {
        // 使用 std::visit 处理 variant 类型
        std::visit([&file](const auto &value) { file << value; }, row[i]);

        if (i < row.size() - 1) {
          file << delimiter;
        }
      }
      file << "\n";
    }

    file.close();
    return *this;
  }

private:
  // 判断字符串是否为整数
  static bool isInteger(const std::string &str) {
    if (str.empty())
      return false;
    size_t pos = 0;
    // 支持负数
    if (str[0] == '-')
      pos = 1;
    // 检查所有字符是否为数字
    while (pos < str.size()) {
      if (!std::isdigit(str[pos]))
        return false;
      pos++;
    }
    return true;
  }

  // 判断字符串是否为浮点数
  static bool isDouble(const std::string &str) {
    try {
      // 检查是否能成功转换为浮点数
      size_t pos;
      std::stod(str, &pos);
      // 确保整个字符串都被转换
      return pos == str.size();
    } catch (...) {
      return false;
    }
  }

  std::string filename;  // 文件名
  std::string delimiter; // 分隔符，默认为逗号
  std::string mode;      // 模式：overwrite 或 append，默认为 overwrite
  bool strictMode; // 严格模式标志位（用于控制类型转换错误处理）
};