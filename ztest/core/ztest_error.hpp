#pragma once
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;
class ZTestFailureException : public std::exception {
private:
  string _msg;

public:
  ZTestFailureException(const string &test_name, const string &expected,
                        const string &actual) {
    ostringstream oss;
    oss << "Test Failure in " << test_name << ":\n"
        << "  Expected: " << expected << "\n"
        << "  Actual  : " << actual;
    _msg = oss.str();
  }

  const char *what() const noexcept override { return _msg.c_str(); }
};
