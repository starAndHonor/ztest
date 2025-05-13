#pragma once
#include "ztest_context.hpp"
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
  ofstream _log_file;

public:
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

  void info(const string &s) { log("INFO", s); }
  void error(const string &s) { log("ERROR", s); }
  void debug(const string &s) { log("DEBUG", s); }

  //   void generateHtmlReport(const ZTestContext &context,
  //                           const string &filename = "test_report.html") {
  //     ofstream report(filename);
  //     if (!report) {
  //       error("Failed to create report file: " + filename);
  //       return;
  //     }

  //     const auto &results = context.getAllResults();

  //     report << "<html><head><title>Test Report</title>"
  //            << "<style>table {border-collapse: collapse;} td, th {border:
  //            1px "
  //               "solid #ddd; padding: 8px;}</style>"
  //            << "</head><body>\n"
  //            << "<h1>Test Report</h1>\n"
  //            << "<table>\n"
  //            << "<tr><th>Test Name</th><th>Status</th><th>Duration "
  //               "(ms)</th><th>Error</th></tr>\n";

  //     for (const auto &[name, result] : results) {
  //       report << "<tr style='background-color:"
  //              << (result.getState() == ZState::z_success ? "#dfffdf" :
  //              "#ffdfdf")
  //              << "'>" << "<td>" << name << "</td>" << "<td>"
  //              << toString(result.getState()) << "</td>" << "<td>"
  //              << result.getUsedTime() << "</td>" << "<td>"
  //              << (result.getErrorMsg().empty() ? "-" : result.getErrorMsg())
  //              << "</td></tr>\n";
  //     }

  //     report << "</table></body></html>";
  //     info("Generated HTML report: " + filename);
  //   }
};
ZLogger logger;
