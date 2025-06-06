#pragma once
#include "ztest_context.hpp"
#include "ztest_result.hpp"
#include "ztest_utils.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <ostream>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
using namespace std::chrono;
using namespace std;
constexpr const char *green = "\033[1;32m";
constexpr const char *red = "\033[1;31m";
constexpr const char *reset = "\033[0m";

class ZLogger {
private:
  mutex _log_mutex;
  ofstream _log_file; // log输出流
  ZLogLevel _log_level = ZLogLevel::DEBUG;

public:
  void set_level(ZLogLevel level) { _log_level = level; }
  /**
   * @description: 输出测试信息
   * @param {string} &s
   * @return {*}
   */
  void debug(const string &s) {
    if (_log_level <= ZLogLevel::DEBUG)
      log("DEBUG", s);
  }

  void warning(const string &s) 
  { // 新增warning级别
    if (_log_level <= ZLogLevel::WARNING)
      log("WARNING", s);
  }
  ZLogger(const string &filename = "test_log.txt") {
    _log_file.open(filename, ios::out | ios::app);
    if (!_log_file) {
      cerr << "Failed to open log file: " << filename << endl;
    }
  }

  ~ZLogger() {
    if (_log_file.is_open()) {
      _log_file.close();
    }
  }
  /**
   * @description: log 辅助函数
   * @param {string} &level 日志等级
   * @param {string} &s 日志内容
   * @return none
   */
  void log(const string &level, const string &s) {
    auto now = chrono::system_clock::now();
    auto time = chrono::system_clock::to_time_t(now);
    lock_guard<mutex> lock(_log_mutex);

    struct tm time_info;
    localtime_r(&time, &time_info);

    //时间格式为YYYY-MM-DD HH:MM:SS
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%F %T", &time_info);

    string entry = "[";
    entry += time_str;//附加格式化时间
    entry += "] [" + level + "] " + s + "\n";

    cout << entry;
    if (_log_file) {
      _log_file << entry;
    }
  }
  /**
   * @description: 输出信息
   * @param {string} &s
   * @return {*}
   */
  void info(const string &s) { log("INFO", s); }
  /**
   * @description: 输出错误
   * @param {string} &s
   * @return {*}
   */
  void error(const string &s) { log("ERROR", s); }

  /**
   * @description: 生成html报告信息
   * @return {*}
   */
  void
  generateHtmlReport(const std::string &reportFilename = "test_report.html",
                     const std::string &test_file = "",
                     bool generateAI = true) {
    auto results = ZTestResultManager::getInstance().getResults();
    int passed = 0, failed = 0;

    std::ostringstream statsStream;
    for (const auto &[name, result] : results) {
      if (result.getState() == ZState::z_success)
        passed++;
      else
        failed++;
    }

    std::string prompt = "请根据以下单元测试结果生成中文分析报告：\n\n"
                         "### 测试统计\n"
                         "- 总测试用例: " +
                         std::to_string(passed + failed) +
                         "\n"
                         "- 通过: " +
                         std::to_string(passed) +
                         "\n"
                         "- 失败: " +
                         std::to_string(failed) +
                         "\n\n"
                         "### 失败用例详情\n";

    for (const auto &[name, result] : results) {
      if (result.getState() == ZState::z_failed) {
        prompt += "- " + name + ": " + result.getErrorMsg() + "\n";
      }
    }
    if (test_file != "") {

      std::cout << "test_file: " << test_file << "\n";
      std::ifstream test_file_stream(test_file);
      if (test_file_stream) {
        std::string test_file_content(
            (std::istreambuf_iterator<char>(test_file_stream)),
            std::istreambuf_iterator<char>());
        prompt += "### 测试文件内容\n" + test_file_content + "\n\n";
      }
    }
    prompt += "\n请提供以下内容(不多于100字)：\n"
              "1. 识别失败的根本原因\n"
              "2. 提供修复建议\n"
              "3. 指出高风险测试用例\n"
              "4. 评估整体测试覆盖率\n"
              "5. 提出系统稳定性改进建议\n";
    std::string jsonReport = generateJsonReport(); 
    std::cout << "prompt: " << prompt << "\n";
    std::string api_key = getApiKey();
    std::cout << "api_key: " << api_key << std::endl;
    std::string ai_advice;
    if (generateAI)
      ai_advice = call_qwen_api(prompt, api_key);
    else
      ai_advice = "No AI advice";
    std::cout << "ai_advice: " << ai_advice << "\n";

    ai_advice = std::regex_replace(ai_advice, std::regex("`"), "\\`");
    ai_advice = std::regex_replace(ai_advice, std::regex("\""), "\\\"");

    std::ofstream reportFile(reportFilename);
    if (!reportFile) {
      std::cerr << "Failed to open report file: " << reportFilename
                << std::endl;
      return;
    }

    std::ostringstream html;

    //HTML标头
    html << "<!DOCTYPE html>\n"
         << "<html lang=\"zh-CN\">\n"
         << "<head>\n"
         << "    <meta charset=\"UTF-8\">\n"
         << "    <title>ZTest 测试报告</title>\n"
         << "    <script "
            "src='https://cdn.jsdelivr.net/npm/markdown-it@14.0.0/dist/"
            "markdown-it.min.js'></script>\n"
         << "    <style>\n"
         << "        :root {\n"
         << "            --primary-color: #2c3e50;\n"
         << "            --success-color: #27ae60;\n"
         << "            --fail-color: #c0392b;\n"
         << "            --hover-color: #f8f9fa;\n"
         << "        }\n"
         << "        body {\n"
         << "            font-family: 'Segoe UI', system-ui, sans-serif;\n"
         << "            line-height: 1.6;\n"
         << "            color: #34495e;\n"
         << "            margin: 0;\n"
         << "            padding: 20px;\n"
         << "        }\n"
         << "        .header {\n"
         << "            background: var(--primary-color);\n"
         << "            color: white;\n"
         << "            padding: 2rem;\n"
         << "            border-radius: 8px;\n"
         << "            margin: 2rem auto;\n"
         << "            width: 80%;\n"
         << "            box-sizing: border-box;\n"
         << "            box-shadow: 0 2px 4px rgba(0,0,0,0.1);\n"
         << "            text-align: center;\n"
         << "        }\n"
         << "        table {\n"
         << "            width: 80%;\n"
         << "            margin: 2rem auto;\n"
         << "            border-collapse: collapse;\n"
         << "            background: white;\n"
         << "            box-shadow: 0 1px 3px rgba(0,0,0,0.1);\n"
         << "            border-radius: 8px;\n"
         << "            overflow: hidden;\n"
         << "        }\n"
         << "        th, td {\n"
         << "            padding: 12px 15px;\n"
         << "            text-align: left;\n"
         << "            border-bottom: 1px solid #ecf0f1;\n"
         << "        }\n"
         << "        th {\n"
         << "            background-color: var(--primary-color);\n"
         << "            color: white;\n"
         << "        }\n"
         << "        tr:hover { background-color: var(--hover-color); }\n"
         << "        .status-badge {\n"
         << "            display: inline-block;\n"
         << "            padding: 4px 12px;\n"
         << "            border-radius: 20px;\n"
         << "            font-size: 0.9em;\n"
         << "            font-weight: 500;\n"
         << "        }\n"
         << "        .success { background-color: var(--success-color); color: "
            "white; }\n"
         << "        .failed { background-color: var(--fail-color); color: "
            "white; }\n"
         << "        .summary-card {\n"
         << "            display: flex;\n"
         << "            justify-content: center;\n"
         << "            gap: 2rem;\n"
         << "            margin: 2rem auto;\n"
         << "            padding: 1.5rem;\n"
         << "            width: 80%;\n"
         << "            background: white;\n"
         << "            border-radius: 8px;\n"
         << "            box-shadow: 0 1px 3px rgba(0,0,0,0.1);\n"
         << "        }\n"
         << "        .ai-analysis {\n"
         << "            margin: 2rem auto;\n"
         << "            width: 80%;\n"
         << "            background: white;\n"
         << "            padding: 1.5rem;\n"
         << "            border-radius: 8px;\n"
         << "            box-shadow: 0 1px 3px rgba(0,0,0,0.1);\n"
         << "        }\n"
         << "    </style>\n"
         << "</head>\n"
         << "<body>\n";

    html << "    <div class=\"header\">ZTest 测试报告</div>\n";
    html << "        </tbody>\n"
         << "    </table>\n"
         << "    <div class=\"summary-card\">\n"
         << "        <div class=\"summary-item\">\n"
         << "            <strong style='font-size: 2rem;'>" << (passed + failed)
         << "</strong>\n"
         << "            <div>总测试用例</div>\n"
         << "        </div>\n"
         << "        <div class=\"summary-item\">\n"
         << "            <strong style='color: var(--success-color); "
            "font-size: 2rem;'>"
         << passed << "</strong>\n"
         << "            <div>通过</div>\n"
         << "        </div>\n"
         << "        <div class=\"summary-item\">\n"
         << "            <strong style='color: var(--fail-color); font-size: "
            "2rem;'>"
         << failed << "</strong>\n"
         << "            <div>失败</div>\n"
         << "        </div>\n"
         << "    </div>\n";
    html << "    <table>\n"
         << "        <thead>\n"
         << "            <tr>\n"
         << "                <th>测试用例</th>\n"
         << "                <th>状态</th>\n"
         << "                <th>耗时 (ms)</th>\n"
         << "                <th>类型</th>\n"
         << "                <th>错误信息</th>\n"
         << "            </tr>\n"
         << "        </thead>\n"
         << "        <tbody>\n";

    // 测试结果
    for (const auto &[name, result] : results) {
      const char *statusClass =
          (result.getState() == ZState::z_success) ? "success" : "failed";
      const char *statusText =
          (result.getState() == ZState::z_success) ? "通过" : "失败";

      html << "            <tr>\n"
           << "                <td>" << name << "</td>\n"
           << "                <td>" << toString(result.getType()) << "</td>\n"
           << "                <td><span class=\"status-badge " << statusClass
           << "\">" << statusText << "</span></td>\n"
           << "                <td>" << std::fixed << std::setprecision(2)
           << result.getUsedTime() << " ms</td>\n"
           << "                <td>"
           << (result.getErrorMsg().empty() ? "-" : result.getErrorMsg())
           << "</td>\n"
           << "            </tr>\n";
    }

    // AI分析报告
    html << "    <div class=\"ai-analysis\">\n"
         << "        <h3 style='color: var(--primary-color); margin-bottom: "
            "1rem;'>AI分析报告</h3>\n"
         << "        <div id=\"ai-content\" style='padding: 1rem; background: "
            "#f8f9fa; border-radius: 4px;'>"
         << "正在加载AI分析结果...</div>\n"
         << "    </div>\n"
         << "    <script>\n"
         << "        document.addEventListener('DOMContentLoaded', () => {\n"
         << "            const md = window.markdownit({html: true, linkify: "
            "true});\n"
         << "            const aiContent = `" << ai_advice << "`;\n"
         << "            const container = "
            "document.getElementById('ai-content');\n"
         << "            if (container && aiContent.trim()) {\n"
         << "                container.innerHTML = md.render(aiContent);\n"
         << "            } else {\n"
         << "                container.innerHTML = "
            "'<em>未能获取AI分析结果</em>';\n"
         << "            }\n"
         << "        });\n"
         << "    </script>\n"
         << "</body>\n"
         << "</html>";

    // 文件写入
    reportFile << html.str();
    reportFile.close();
  }
  /**
   * @description: 生成json报告
   * @param {const std::string&} reportFilename
   * @return {void}
   */
  std::string
  generateJsonReport(const std::string &reportFilename = "test_report.json") {
    std::ofstream reportFile(reportFilename);
    debug("generating json log");

    if (!reportFile) {
      std::cerr << "Failed to open JSON report file: " << reportFilename
                << std::endl;
      return "error";
    }

    auto results = ZTestResultManager::getInstance().getResults();
    int passed = 0, failed = 0;

    std::ostringstream json;
    json << "{\n";
    json << "  \"summary\": {\n";


    for (const auto &[name, result] : results) {
      result.getState() == ZState::z_success ? passed++ : failed++;
    }

    json << "    \"total\": " << (passed + failed) << ",\n";
    json << "    \"passed\": " << passed << ",\n";
    json << "    \"failed\": " << failed << "\n";
    json << "  },\n";
    json << "  \"tests\": [\n";

    bool first = true;
    for (const auto &[name, result] : results) {
      if (!first)
        json << ",\n";
      first = false;

      json << "    {\n";
      json << "      \"name\": \"" << name << "\",\n";
      json << "      \"status\": \""
           << (result.getState() == ZState::z_success ? "Passed" : "Failed")
           << "\",\n";
      json << "      \"duration\": " << std::fixed << std::setprecision(2)
           << result.getUsedTime() << ",\n";
      json << "      \"error\": \""
           << (result.getErrorMsg().empty() ? "" : result.getErrorMsg())
           << "\"\n";
      json << "    }";
    }

    json << "\n  ]\n}";

    reportFile << json.str();
    reportFile.close();
    return json.str();
  }
  /**
   * @description:生成junit格式报告，可以用于CI集成
   * @return {*}
   */
  void
  generateJUnitReport(const std::string &reportFilename = "test_report.xml") {
    std::ofstream reportFile(reportFilename);
    debug("generating JUnit XML report");

    if (!reportFile) {
      std::cerr << "Failed to open JUnit report file: " << reportFilename
                << std::endl;
      return;
    }

    auto results = ZTestResultManager::getInstance().getResults();

    std::map<std::string, std::vector<ZTestResult>> suiteMap;
    for (const auto &[name, result] : results) {
      size_t dotPos = name.find('.');
      std::string suiteName =
          (dotPos != std::string::npos) ? name.substr(0, dotPos) : "Other";
      suiteMap[suiteName].push_back(result);
    }

    std::ostringstream xml;
    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    xml << "<testsuites>\n";

    for (const auto &[suiteName, tests] : suiteMap) {
      int total = tests.size();
      int failed = 0;
      double duration = 0.0;

      for (const auto &test : tests) {
        duration += test.getUsedTime();
        if (test.getState() == ZState::z_failed)
          failed++;
      }

      xml << "  <testsuite name=\"" << suiteName << "\" tests=\"" << total
          << "\" failures=\"" << failed << "\" errors=\"0\" time=\""
          << duration / 1000.0 << "\">\n";

      for (const auto &test : tests) {
        const std::string &name = test.getName();
        double used_time = test.getUsedTime() / 1000.0; // seconds
        std::string error_msg = test.getErrorMsg();

        xml << "    <testcase name=\"" << name << "\" classname=\"" << suiteName
            << "\" time=\"" << std::fixed << std::setprecision(3) << used_time
            << "\">";

        if (test.getState() == ZState::z_failed) {
          xml << "\n      <failure message=\"" << error_msg << "\"/>";
        }

        xml << "</testcase>\n";
      }

      xml << "  </testsuite>\n";
    }

    xml << "</testsuites>\n";
    reportFile << xml.str();
    reportFile.close();
  }
};
static ZLogger logger;