#pragma once
#include "ztest_base.hpp"
#include "ztest_logger.hpp"
#include "ztest_result.hpp"
#include "ztest_thread.hpp"
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
  mutex _result_mutex;
  mutex _queue_mutex;
  mutable mutex _list_mutex;
  queue<shared_ptr<ZTestBase>> _test_queue;
  vector<shared_ptr<ZTestBase>> _test_list;
  TestView *_visualizer;

public:
  /**
   * @description: 设置可视化
   * @param {TestView} *visualizer
   * @return none
   */
  void setVisualizer(TestView *visualizer) { _visualizer = visualizer; }
  /**
   * @description: 运行所有 z_unsafe 测试, 线程不安全/性能测试
   * @return none
   */
  void runUnsafeOnly() {
    logger.debug("[Unsafe] Starting unsafe tests execution");
    size_t total = 0, succeeded = 0, failed = 0;

    for (auto &test : _test_list) {
      if (test->getType() == ZType::z_unsafe) {
        total++;
        const string &test_name = test->getName();
        logger.debug("[Unsafe] Running test: " + test_name);

        try {
          ZTimer timer;
          timer.start();
          test->run();
          timer.stop();

          ZTestResult result;
          result.setResult(test_name, ZState::z_success, "",
                           timer.getStartTime(), timer.getEndTime(),
                           timer.getElapsedMilliseconds());

          {
            std::lock_guard<std::mutex> lock(_result_mutex);
            ZTestResultManager::getInstance().addResult(std::move(result));
          }

          succeeded++;
          logger.info("[Unsafe] Test succeeded: " + test_name + " (" +
                      to_string(timer.getElapsedMilliseconds()) + "ms)");

        } catch (const std::exception &e) {
          ZTestResult result;
          result.setResult(test_name, ZState::z_failed, e.what(), {}, {}, 0);

          {
            std::lock_guard<std::mutex> lock(_result_mutex);
            ZTestResultManager::getInstance().addResult(std::move(result));
          }

          failed++;
          logger.error("[Unsafe] Test failed: " + test_name +
                       " - Reason: " + e.what());
        }
      }
    }

    logger.debug("[Unsafe] Execution completed - Total: " + to_string(total) +
                 " | Succeeded: " + to_string(succeeded) +
                 " | Failed: " + to_string(failed));
  }

  /**
   * @description: 并行运行所有 z_safe 测试, 线程安全
   * @return none
   */
  void runSafeInParallel() {

    std::vector<shared_ptr<ZTestBase>> safe_tests;
    {
      std::lock_guard<std::mutex> lock(_list_mutex);
      std::copy_if(
          _test_list.begin(), _test_list.end(), std::back_inserter(safe_tests),
          [](const auto &test) { return test->getType() == ZType::z_safe; });
    }

    const unsigned num_workers =
        std::max(1u, min(std::thread::hardware_concurrency(), 8u));
    ZThreadPool pool(num_workers);
    std::vector<std::future<void>> futures;
    logger.info("[Safe] Starting parallel execution of " +
                std::to_string(safe_tests.size()) + " safe tests using " +
                std::to_string(num_workers) + " workers");
    // std::thread status_monitor([&pool]() {
    //   while (!pool.is_stopped()) {
    //     pool.log_status();
    //     std::this_thread::sleep_for(std::chrono::seconds(1));
    //   }
    // });

    for (auto &test : safe_tests) {
      futures.emplace_back(pool.enqueue([this, test] { runTest(test); }));
    }
    for (auto &future : futures) {
      future.get();
    }
    // status_monitor.join();
    logger.info("[Safe] Parallel execution completed");
  }
  /**
   * @description: 测试样例入队
   * @param {shared_ptr<ZTestBase>} test_case
   * @return none
   */
  void addTest(shared_ptr<ZTestBase> test_case) {
    lock_guard<mutex> q_lock(_queue_mutex);
    lock_guard<mutex> l_lock(_list_mutex);

    auto shared_test = shared_ptr<ZTestBase>(std::move(test_case));
    _test_list.push_back(shared_test);
    _test_queue.push(std::move(shared_test));
  }
  /**
   * @description: 清空测试队列
   * @return none
   */
  void clearTestQueue() {
    std::lock_guard<std::mutex> q_lock(_queue_mutex);
    std::queue<shared_ptr<ZTestBase>> empty;
    std::swap(_test_queue, empty);
  }

  // TODO: 此处的获取测试列表应该放到注册阶段完成
  vector<weak_ptr<ZTestBase>> getTestList() const {
    lock_guard<mutex> lock(_list_mutex); // Now works with mutable
    vector<weak_ptr<ZTestBase>> result;
    for (const auto &test : _test_list) {
      result.emplace_back(test);
    }
    return result;
  }
  // TODO: 运行每一个测试样例应该放在singlecase和suite中完成
  /**
   * @description: 运行单个测试样例
   * @param {shared_ptr<ZTestBase>} test_case
   * @return {*}
   */
  void runTest(shared_ptr<ZTestBase> test_case) {
    ZTestResult result;
    ZTimer local_timer;
    auto *test_ptr = test_case.get();
    const string test_name = test_ptr->getName();

    try {
      {
        lock_guard<mutex> lock(_result_mutex);
        result.setResult(test_name, ZState::z_success, "",
                         system_clock::time_point{}, system_clock::time_point{},
                         0.0);
      }
      logger.debug(
          "Starting test [" + test_case->getName() + "] on thread: " +
          ZThreadPool::thread_id_to_string(std::this_thread::get_id()));
      local_timer.start();
      auto test_state = test_case->run();
      local_timer.stop();
      logger.debug(
          "Finished test [" + test_case->getName() + "] on thread: " +
          ZThreadPool::thread_id_to_string(std::this_thread::get_id()));
      {
        lock_guard<mutex> lock(_result_mutex);
        result.setResult(test_ptr->getName(), test_state, "",
                         local_timer.getStartTime(), local_timer.getEndTime(),
                         local_timer.getElapsedMilliseconds());
      }
    } catch (const exception &e) {
      lock_guard<mutex> lock(_result_mutex);
      result.setResult(test_ptr->getName(), ZState::z_failed, e.what(),
                       local_timer.getStartTime(), local_timer.getEndTime(),
                       local_timer.getElapsedMilliseconds());
      logger.error(result.getResultString(test_name) + "\n");
      logger.error(
          "Test [" + test_case->getName() + "] failed in thread " +
          ZThreadPool::thread_id_to_string(std::this_thread::get_id()) + ": " +
          e.what());
    }
    lock_guard<mutex> lock(_result_mutex);
    ZTestResultManager::getInstance().addResult(std::move(result));
  }
  /**
   * @description: 运行所有测试
   * @return none
   */
  void runAllTests() {
    runUnsafeOnly();

    runSafeInParallel();

    logger.generateHtmlReport();
    logger.generateJsonReport();
    logger.generateJUnitReport();
  }
  /**
   * @description: 运行选定的测试
   * @param {string&} test_name
   * @return {*}
   */
  bool runSelectedTest(const std::string &test_name) {
    std::lock_guard<std::mutex> lock(_list_mutex);

    for (const auto &test : _test_list) {
      if (test->getName() == test_name) {
        runTest(test);
        return true;
      }
    }

    return false; // Test not found
  }
  /**
   * @description: 重置测试队列
   * @return none
   */
  void resetQueue() {
    std::lock_guard<std::mutex> q_lock(_queue_mutex);
    std::lock_guard<std::mutex> l_lock(_list_mutex);

    _test_queue = {};
    for (auto &test : _test_list) {
      _test_queue.push(test);
    }
  }
  /**
   * @description: 构造函数
   * @return {*}
   */
  ZTestContext() {
    while (!_test_queue.empty())
      _test_queue.pop();
  }

  ~ZTestContext() {
    while (!_test_queue.empty())
      _test_queue.pop();
  }
};
