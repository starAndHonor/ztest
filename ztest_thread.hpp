#pragma once
#include "ztest_logger.hpp"
#include <atomic>
#include <condition_variable>
#include <future>
#include <queue>
#include <sstream>
#include <thread>

class ZThreadPool {
private:
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> tasks;

  std::atomic<uint64_t> _total_tasks{0};
  std::atomic<uint64_t> _completed_tasks{0};
  std::vector<std::thread::id> _worker_ids;

  std::mutex queue_mutex;
  std::condition_variable condition;
  std::atomic<bool> stop{false};

public:
  explicit ZThreadPool(size_t threads) {
    for (size_t i = 0; i < threads; ++i) {
      workers.emplace_back([this, i] {
        {
          std::lock_guard<std::mutex> lock(queue_mutex);
          _worker_ids.push_back(std::this_thread::get_id());
        }

        logger.debug("Worker " + std::to_string(i) + " started (TID: " +
                     thread_id_to_string(std::this_thread::get_id()) + ")");

        while (true) {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock,
                           [this] { return stop.load() || !tasks.empty(); });

            if (stop && tasks.empty())
              return;

            task = std::move(tasks.front());
            tasks.pop();
          }

          auto start = std::chrono::steady_clock::now();
          task();
          auto end = std::chrono::steady_clock::now();

          _completed_tasks++;
          logger.debug(
              "Task completed in " +
              std::to_string(
                  std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                        start)
                      .count()) +
              "ms (Remaining: " + std::to_string(tasks.size()) + ")");
        }
      });
    }
  }
  /**
 * @description: 向线程池添加任务
 * @tparam F 任务函数类型
 * @param f 任务函数
 * @return 任务的未来对象，用于获取任务执行结果
 */
  template <class F> auto enqueue(F &&f) -> std::future<decltype(f())> {
    using ReturnType = decltype(f());

    auto task =
        std::make_shared<std::packaged_task<ReturnType()>>(std::forward<F>(f));

    std::future<ReturnType> res = task->get_future();
    {
      std::lock_guard<std::mutex> lock(queue_mutex);
      if (stop)
        throw std::runtime_error("Enqueue on stopped ThreadPool");

      tasks.emplace([task, this]() { (*task)(); });

      _total_tasks++;
      logger.debug("New task enqueued (Total pending: " +
                   std::to_string(tasks.size()) + ")");
    }
    condition.notify_one();
    return res;
  }
  ~ZThreadPool() {
    stop.store(true);
    condition.notify_all();
    logger.debug("Destroying pool with " + std::to_string(workers.size()) +
                 " workers");
    for (auto &w : workers) {
      if (w.joinable()) {
        w.join();
        logger.debug("Worker joined: " + thread_id_to_string(w.get_id()));
      }
    }
  }

  /**
 * @description: 将线程ID转换为字符串
 * @param id 线程ID
 * @return 线程ID的字符串表示
 */
  static std::string thread_id_to_string(const std::thread::id &id) {
    std::stringstream ss;
    ss << id;
    return ss.str();
  }
  /**
 * @description: 打印线程池状态信息
 */
  void log_status() const {
    logger.debug("[ThreadPool Status]"
                 "\n- Active workers: " +
                 std::to_string(workers.size()) + "\n- Total tasks: " +
                 std::to_string(_total_tasks.load()) + "\n- Completed tasks: " +
                 std::to_string(_completed_tasks.load()) +
                 "\n- Pending tasks: " + std::to_string(tasks.size()));
  }

  /**
   * @description: 判断线程池是否已停止
   * @return 已停止返回true，否则返回false
   */
  bool is_stopped() const { return stop.load(); }
};