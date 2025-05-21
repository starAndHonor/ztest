#pragma once
#include "ztest_context.hpp"
#include "ztest_result.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <ostream>
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

  void warning(const string &s) { // 新增warning级别
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

    // Manual time formatting for YYYY-MM-DD HH:MM:SS
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%F %T", &time_info);

    string entry = "[";
    entry += time_str; // append formatted time
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
  generateHtmlReport(const std::string &reportFilename = "test_report.html") {
    std::ofstream reportFile(reportFilename);
    debug("generating html log");
    if (!reportFile) {
      std::cerr << "Failed to open report file: " << reportFilename
                << std::endl;
      return;
    }

    std::ostringstream html;

    // Start HTML document
    html << "<!DOCTYPE html>\n"
         << "<html lang=\"en\">\n"
         << "<head>\n"
         << "    <meta charset=\"UTF-8\">\n"
         << "    <title>ZTest Report</title>\n"
         << "    <style>\n"
         << "        body { font-family: Arial, sans-serif; padding: 20px; }\n"
         << "        table { border-collapse: collapse; width: 100%; "
            "margin-top: 20px; }\n"
         << "        th, td { border: 1px solid #ddd; padding: 8px; "
            "text-align: left; }\n"
         << "        tr:nth-child(even) { background-color: #f2f2f2; }\n"
         << "        .success { color: green; font-weight: bold; }\n"
         << "        .failed { color: red; font-weight: bold; }\n"
         << "        .header { font-size: 24px; margin-bottom: 20px; }\n"
         << "    </style>\n"
         << "</head>\n"
         << "<body>\n"
         << "    <div class=\"header\">ZTest Execution Report</div>\n"
         << "    <table>\n"
         << "        <thead>\n"
         << "            <tr>\n"
         << "                <th>Test Name</th>\n"
         << "                <th>Status</th>\n"
         << "                <th>Duration (ms)</th>\n"
         << "                <th>Error Message</th>\n"
         << "            </tr>\n"
         << "        </thead>\n"
         << "        <tbody>\n";
    html << "<style>\n"
         << "    :root {\n"
         << "        --primary-color: #2c3e50;\n"
         << "        --success-color: #27ae60;\n"
         << "        --fail-color: #c0392b;\n"
         << "        --hover-color: #f8f9fa;\n"
         << "    }\n"
         << "    body {\n"
         << "        font-family: 'Segoe UI', system-ui, sans-serif;\n"
         << "        line-height: 1.6;\n"
         << "        color: #34495e;\n"
         << "        margin: 0;\n"
         << "        padding: 20px;\n"
         << "    }\n"
         << "    .header {\n"
         << "        background: var(--primary-color);\n"
         << "        color: white;\n"
         << "        padding: 2rem;\n"
         << "        border-radius: 8px;\n"
         << "        margin: 2rem auto;\n"
         << "        width: 80%;\n"
         << "        box-sizing: border-box;\n"
         << "        box-shadow: 0 2px 4px rgba(0,0,0,0.1);\n"
         << "        text-align: center;\n"
         << "    }\n"
         << "    table {\n"
         << "        width: 80%;\n"        // Changed from 100% to 80%
         << "        margin: 2rem auto;\n" // Add auto margin for centering
         << "        border-collapse: collapse;\n"
         << "        background: white;\n"
         << "        box-shadow: 0 1px 3px rgba(0,0,0,0.1);\n"
         << "        border-radius: 8px;\n"
         << "        overflow: hidden;\n"
         << "        box-sizing: border-box;\n" // Add box-sizing
         << "    }\n"
         << "    th, td {\n"
         << "        padding: 12px 15px;\n"
         << "        text-align: left;\n"
         << "        border-bottom: 1px solid #ecf0f1;\n"
         << "    }\n"
         << "    th {\n"
         << "        background-color: var(--primary-color);\n"
         << "        color: white;\n"
         << "        font-weight: 600;\n"
         << "    }\n"
         << "    tr:hover { background-color: var(--hover-color); }\n"
         << "    .status-badge {\n"
         << "        display: inline-block;\n"
         << "        padding: 4px 12px;\n"
         << "        border-radius: 20px;\n"
         << "        font-size: 0.9em;\n"
         << "        font-weight: 500;\n"
         << "    }\n"
         << "    .success { background-color: var(--success-color); color: "
            "white; }\n"
         << "    .failed { background-color: var(--fail-color); color: white; "
            "}\n"
         << "    .summary-card {\n"
         << "        display: flex;\n"
         << "        justify-content: center;\n"
         << "        gap: 2rem;\n"
         << "        margin: 2rem auto;\n"
         << "        padding: 1.5rem;\n"
         << "        width: 80%;\n"
         << "        box-sizing: border-box;\n"
         << "        background: white;\n"
         << "        border-radius: 8px;\n"
         << "        box-shadow: 0 1px 3px rgba(0,0,0,0.1);\n"
         << "    }\n"
         << "    .summary-item {\n"
         << "        text-align: center;\n"
         << "        padding: 0 1.5rem;\n"
         << "    }\n"
         << "    .summary-item {\n"
         << "        text-align: center;\n"
         << "        padding: 0 1.5rem;\n"
         << "    }\n"
         << "    .summary-item strong {\n"
         << "        display: block;\n"
         << "        font-size: 1.5rem;\n"
         << "        color: var(--primary-color);\n"
         << "        margin-bottom: 0.5rem;\n"
         << "    }\n"
         << "    @media (max-width: 768px) {\n"
         << "        table, .summary-card { display: block; }\n"
         << "        td { position: relative; padding-left: 50%; }\n"
         << "        td:before {\n"
         << "            position: absolute;\n"
         << "            left: 6px;\n"
         << "            content: attr(data-label);\n"
         << "            font-weight: bold;\n"
         << "        }\n"
         << "    }\n"
         << "</style>\n";
    // Add test results
    auto results = ZTestResultManager::getInstance().getResults();
    int passed = 0, failed = 0;

    for (const auto &[name, result] : results) {
      const char *statusClass =
          (result.getState() == ZState::z_success) ? "success" : "failed";
      const char *statusText =
          (result.getState() == ZState::z_success) ? "Passed" : "Failed";

      html << "            <tr>\n"
           << "                <td>" << name << "</td>\n"
           << "                <td><span class=\"status-badge " << statusClass
           << "\">" << statusText << "</span></td>\n"
           << "                <td>" << std::fixed << std::setprecision(2)
           << result.getUsedTime() << " ms</td>\n"
           << "                <td>"
           << (result.getErrorMsg().empty() ? "-" : result.getErrorMsg())
           << "</td>\n"
           << "            </tr>\n";

      if (result.getState() == ZState::z_success)
        passed++;
      else
        failed++;
    }

    html << "    <div class=\"summary-card\">\n"
         << "        <div class=\"summary-item\">\n"
         << "            <strong>" << (passed + failed) << "</strong>\n"
         << "            Total Tests\n"
         << "        </div>\n"
         << "        <div class=\"summary-item\">\n"
         << "            <strong style=\"color: var(--success-color)\">"
         << passed << "</strong>\n"
         << "            Passed\n"
         << "        </div>\n"
         << "        <div class=\"summary-item\">\n"
         << "            <strong style=\"color: var(--fail-color)\">" << failed
         << "</strong>\n"
         << "            Failed\n"
         << "        </div>\n"
         << "    </div>\n";

    // Write to file
    reportFile << html.str();
    reportFile.close();
  }
};
ZLogger logger;
