#pragma once
#include "ztest_base.hpp"
#include "ztest_logger.hpp"
#include "ztest_result.hpp"
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
class TestView;

// TestContext维护要管理的ZTestBase的队列，并管理测试结果。
class ZTestContext {
private:
  queue<shared_ptr<ZTestBase>> _test_queue;
  vector<shared_ptr<ZTestBase>> _test_list;
  // vector<ZTestResult> _test_result;
  mutex _result_mutex;
  mutex _queue_mutex;
  mutable mutex _list_mutex;
  TestView *_visualizer;
  std::unordered_map<std::string, ZTestResult> _test_result_map;

public:
  vector<weak_ptr<ZTestBase>> getTests() {
    lock_guard<mutex> lock(_queue_mutex);
    vector<weak_ptr<ZTestBase>> tests;
    while (!_test_queue.empty()) {
      auto shared_test =
          _test_list.emplace_back(std::move(_test_queue.front()));
      _test_queue.pop();
      tests.push_back(shared_test);
    }
    return tests;
  }
  // 运行所有 z_unsafe 测试（单线程）
  void runUnsafeOnly() {
    for (auto &test : _test_list) {
      if (test->getType() == ZType::z_unsafe) {
        try {
          ZTimer timer;
          timer.start();
          test->run();
          timer.stop();

          ZTestResult result;
          result.setResult(test->getName(), ZState::z_success, "",
                           timer.getStartTime(), timer.getEndTime(),
                           timer.getElapsedMilliseconds());

          std::lock_guard<std::mutex> lock(_result_mutex);
          _test_result_map[test->getName()] = std::move(result);
        } catch (const std::exception &e) {
          ZTestResult result;
          result.setResult(test->getName(), ZState::z_failed, e.what(), {}, {},
                           0);

          std::lock_guard<std::mutex> lock(_result_mutex);
          _test_result_map[test->getName()] = std::move(result);
        }
      }
    }
  }

  // 并行运行 z_safe 测试
  void runSafeInParallel() {
    std::queue<shared_ptr<ZTestBase>> safe_queue;

    {
      std::lock_guard<std::mutex> lock(_list_mutex);
      for (auto &test : _test_list) {
        if (test->getType() == ZType::z_safe) {
          safe_queue.push(test);
        }
      }
    }

    std::vector<std::thread> workers;
    const unsigned num_workers = std::thread::hardware_concurrency();

    auto worker_task = [this, &safe_queue] {
      while (!safe_queue.empty()) {
        shared_ptr<ZTestBase> test;
        {
          std::lock_guard<std::mutex> lock(_list_mutex);
          if (!safe_queue.empty()) {
            test = safe_queue.front();
            safe_queue.pop();
          }
        }
        if (test) {
          runTest(test); // 已有锁保护
        }
      }
    };

    for (unsigned i = 0; i < num_workers; ++i)
      workers.emplace_back(worker_task);

    for (auto &t : workers)
      if (t.joinable())
        t.join();
  }
  void clearTestQueue() {
    std::lock_guard<std::mutex> q_lock(_queue_mutex);
    std::queue<shared_ptr<ZTestBase>> empty;
    std::swap(_test_queue, empty);
  }

  void setVisualizer(TestView *visualizer) { _visualizer = visualizer; }

  void addTest(unique_ptr<ZTestBase> test_case) {
    lock_guard<mutex> q_lock(_queue_mutex);
    lock_guard<mutex> l_lock(_list_mutex);

    auto shared_test = shared_ptr<ZTestBase>(std::move(test_case));
    _test_list.push_back(shared_test);
    _test_queue.push(std::move(shared_test));
  }
  vector<weak_ptr<ZTestBase>> getTestList() const {
    lock_guard<mutex> lock(_list_mutex); // Now works with mutable
    vector<weak_ptr<ZTestBase>> result;
    for (const auto &test : _test_list) {
      result.emplace_back(test);
    }
    return result;
  }
  void runTest(shared_ptr<ZTestBase> test_case) {
    ZTestResult result;
    ZTimer local_timer;
    auto *test_ptr = test_case.get();
    const string test_name = test_ptr->getName();

    try {
      local_timer.start();
      auto test_state = test_ptr->run();
      local_timer.stop();

      lock_guard<mutex> lock(_result_mutex);
      result.setResult(test_ptr->getName(), test_state, "",
                       local_timer.getStartTime(), local_timer.getEndTime(),
                       local_timer.getElapsedMilliseconds());

      logger.info(result.getResultString(test_name) + "\n");
    } catch (const exception &e) {
      lock_guard<mutex> lock(_result_mutex);
      result.setResult(test_ptr->getName(), ZState::z_failed, e.what(),
                       local_timer.getStartTime(), local_timer.getEndTime(),
                       local_timer.getElapsedMilliseconds());
      logger.error(result.getResultString(test_name) + "\n");
    }
    lock_guard<mutex> lock(_result_mutex);
    _test_result_map[test_name] = std::move(result);
  }
  const ZTestResult &getTestResult(const string &test_name) const {
    return _test_result_map.at(test_name);
  }

  const std::unordered_map<std::string, ZTestResult> &getAllResults() const {
    return _test_result_map;
  }
  void runAll() {
    vector<thread> workers;
    const unsigned num_workers = thread::hardware_concurrency();

    auto worker_task = [this] {
      while (true) {
        shared_ptr<ZTestBase> test;
        {
          lock_guard<mutex> lock(_queue_mutex);
          if (_test_queue.empty())
            return;
          test = _test_queue.front();
          _test_queue.pop();
        }
        runTest(move(test));
      }
    };
    for (unsigned i = 0; i < num_workers; ++i)
      workers.emplace_back(worker_task);
    for (auto &t : workers) {
      if (t.joinable())
        t.join();
    }
  }
  void resetQueue() {
    std::lock_guard<std::mutex> q_lock(_queue_mutex);
    std::lock_guard<std::mutex> l_lock(_list_mutex);

    _test_queue = {};
    for (auto &test : _test_list) {
      _test_queue.push(test);
    }
  }
  ZTestContext() {
    while (!_test_queue.empty())
      _test_queue.pop();
    _test_result_map.clear();
  }

  ~ZTestContext() = default;
};
