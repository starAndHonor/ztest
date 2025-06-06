// ztest_utils.hpp
#pragma once
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
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
using json = nlohmann::json;

// 回调函数处理API响应
static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            std::string *userp) {
  size_t total_size = size * nmemb;
  userp->append((char *)contents, total_size);
  return total_size;
}

// 调用Qwen API的函数
inline std::string call_qwen_api(const std::string &prompt,
                                 const std::string &api_key,
                                 const std::string &model = "qwen-turbo",
                                 double temperature = 0.7) {
  CURL *curl;
  CURLcode res;
  std::string response_string;
  std::string url = "https://dashscope.aliyuncs.com/api/v1/services/aigc/"
                    "text-generation/generation";

  // 准备请求头
  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers =
      curl_slist_append(headers, ("Authorization: Bearer " + api_key).c_str());

  // 构建JSON请求体
  json request_body;
  request_body["model"] = model;
  request_body["input"]["messages"] = json::array();
  request_body["input"]["messages"].push_back(
      {{"role", "user"}, {"content", prompt}});
  request_body["parameters"] = {{"temperature", temperature}};

  std::string json_payload = request_body.dump();

  curl = curl_easy_init();
  if (curl) {
    // 设置CURL选项
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    // 执行请求
    res = curl_easy_perform(curl);

    // 获取HTTP状态码
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    // 清理资源
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
      return "CURL错误: " + std::string(curl_easy_strerror(res));
    }

    // 解析JSON响应
    try {
      json response_json = json::parse(response_string);

      // 调试输出：打印整个响应
      std::cerr << "原始API响应:\n" << response_json.dump(2) << "\n";

      // 检查API返回的错误
      if (http_code != 200) {
        std::string error_msg = "HTTP错误: " + std::to_string(http_code);
        if (response_json.contains("message")) {
          error_msg += " - " + response_json["message"].get<std::string>();
        }
        return error_msg;
      }

      // 检查响应结构是否包含预期字段
      if (response_json.contains("output") &&
          response_json["output"].contains("text") &&
          !response_json["output"]["text"].is_null()) {
        return response_json["output"]["text"].get<std::string>();
      } else {
        return "错误: 无效的API响应结构";
      }
    } catch (const json::parse_error &e) {
      return "JSON解析错误: " + std::string(e.what()) +
             "\n响应内容: " + response_string;
    } catch (const json::type_error &e) {
      return "JSON类型错误: " + std::string(e.what()) +
             "\n响应内容: " + response_string;
    }
  }
  return "无法初始化CURL";
}
inline std::string getApiKey() {
  const char *key = std::getenv("DASHSCOPE_API_KEY");
  if (!key) {
    throw std::runtime_error("API key not found in environment variables");
  }
  return key;
}
