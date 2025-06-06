#pragma once
#include <glad/glad.h>

// #include "./lib/implot/implot.h"
#include "core/ztest_base.hpp"
#include "core/ztest_benchmark.hpp"
#include "core/ztest_context.hpp"
#include "core/ztest_dataregistry.hpp"
#include "core/ztest_error.hpp"
#include "core/ztest_macros.hpp"
#include "core/ztest_parameterized.hpp"
#include "core/ztest_registry.hpp"
#include "core/ztest_result.hpp"
#include "core/ztest_singlecase.hpp"
#include "core/ztest_suite.hpp"
#include "core/ztest_timer.hpp"
#include "core/ztest_types.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "implot.h"
#include "lib/imgui_markdown/imgui_markdown.h"
#include <GLFW/glfw3.h>
#include <map>
// MVC 架构中的模型层，管理测试状态和数据。
class ZTestModel {
public:
  std::atomic<bool> _is_running{false};
  float _progress = 0.0f;
  std::string _selected_test;
  std::mutex _mutex;
  bool tryStartRunning() {
    bool expected = false;
    return _is_running.compare_exchange_strong(expected, true);
  }
  /**
   * @description: 从测试注册表初始化测试模型
   * @param context 测试上下文引用
   */
  void initializeFromRegistry(ZTestContext &context) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto &registry = ZTestRegistry::instance();
    auto tests = registry.takeTests();

    for (auto &test : tests) {
      ZTestResult empty_reslut{test->getName(), ZType::z_unsafe, 0.0,
                               ZState::z_unknown, ""};
      ZTestResultManager::getInstance().addResult(empty_reslut);
    }
    for (auto &&test : tests) {
      context.addTest(std::move(test));
    }
  }
};
// 作为 MVC 架构中的控制器层，处理用户操作并协调模型与视图。
class ZTestController {
public:
  ZTestController(ZTestModel &model, ZTestContext &context)
      : _model(model), _context(context) {}
  ~ZTestController() {
    if (_test_thread.joinable()) {
      _test_thread.join();
    }
  }
  void runAllTests() {
    if (!_model.tryStartRunning()) {
      std::cout << "[INFO] Another test is already running." << std::endl;
      return;
    }

    if (_test_thread.joinable()) {
      _test_thread.join();
    }

    _test_thread = std::thread([this] {
      try {
        _context.runAllTests();
      } catch (...) {
        std::cerr << "[THREAD] Exception caught during test execution."
                  << std::endl;
      }

      {
        std::lock_guard<std::mutex> lock(_model._mutex);
        _model._is_running = false;
        _model._progress = 1.0f;
      }

      std::cout << "[THREAD] All tests completed." << std::endl;
    });

    _test_thread.detach();
  }
  /**
   * @description: 执行指定名称的测试用例
   * @param test_name 要执行的测试用例名称
   */
  void runSelectedTest(const std::string &test_name) {
    if (!_model.tryStartRunning()) {
      std::cout << "[INFO] Another test is already running." << std::endl;
      return;
    }

    if (_test_thread.joinable()) {
      _test_thread.join();
    }

    _test_thread = std::thread([this, test_name] {
      bool found = false;

      try {
        found = _context.runSelectedTest(test_name);
      } catch (const std::exception &e) {
        std::cerr << "[THREAD] Exception caught: " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "[THREAD] Unknown exception caught." << std::endl;
      }

      {
        std::lock_guard<std::mutex> lock(_model._mutex);
        _model._is_running = false;
        _model._progress = found ? 1.0f : 0.0f;
      }

      std::cout << "[THREAD] Thread function completed." << std::endl;
    });

    _test_thread.detach();
  }

private:
  ZTestModel &_model;
  ZTestContext &_context;
  std::thread _test_thread;
  bool isRunning() const { return _model._is_running.load(); }
};
// 作为 MVC 架构中的视图层，负责 UI 渲染和用户交互。
class ZTestView {
public:
  // ZTestView() {

  //   ImGuiIO &io = ImGui::GetIO();
  //   // _markdown_config.linkCallback = [](const char *url, const char *title,
  //   //                                    size_t titleLength) {
  //   //   std::cout << "Link clicked: " << std::string(title, titleLength) <<
  //   "
  //   //   ("
  //   //             << url << ")" << std::endl;
  //   // };
  // }
  void render(ZTestModel &model, ZTestController &controller) {
    renderMainMenu();
    renderTestList(model, controller);
    renderStatusBar(model);
    renderDetailsWindow(model);
    renderResourceMonitor();
    renderAIAnalysisWindow(model);
  }
  /**
   * @description: 设置关联的窗口句柄
   * @param window GLFW窗口指针
   */
  void setWindow(GLFWwindow *window) { _window = window; }
  void applyTheme() {
    ImGuiStyle &style = ImGui::GetStyle();

    switch (_current_theme) {
    case Theme::Dark:
      ImGui::StyleColorsDark();
      break;
    case Theme::Light:
      ImGui::StyleColorsLight();
      break;
    }
  }
  std::chrono::steady_clock::time_point _last_update;
  void updateResourceUsage() {
    if (!_enable_monitoring)
      return;

    auto now = std::chrono::steady_clock::now();

    _cpu_history.push(getCpuUsage());
    _memory_history.push(getMemoryUsage());

    while (_cpu_history.size() > maxHistorySize)
      _cpu_history.pop();
    while (_memory_history.size() > maxHistorySize)
      _memory_history.pop();

    _last_update = now;
  }

private:
  std::queue<float> _cpu_history;
  std::queue<float> _memory_history;
  const size_t maxHistorySize = 60;
  bool _enable_monitoring = true;
  bool show_ai_window = false;
  ImGui::MarkdownConfig _markdown_config;

  std::vector<float> queueToVector(const std::queue<float> &q) {
    std::vector<float> result;
    std::queue<float> temp = q;
    while (!temp.empty()) {
      result.push_back(temp.front());
      temp.pop();
    }
    return result;
  }

  float getCpuUsage() {
    std::ifstream stat("/proc/stat");
    std::string line;
    std::getline(stat, line);
    std::istringstream ss(line);
    std::string cpu;
    long user, nice, system, idle, iowait, irq, softirq;
    ss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq;

    static long prev_total = 0, prev_idle = 0;
    long total = user + nice + system + idle + iowait + irq + softirq;
    long diff_total = total - prev_total;
    long diff_idle = idle - prev_idle;

    float usage =
        (diff_total == 0) ? 0 : 100.0f * (diff_total - diff_idle) / diff_total;

    prev_total = total;
    prev_idle = idle;
    return usage;
  }

  float getMemoryUsage() {
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    long total = 0, free = 0, buffers = 0, cached = 0;

    while (std::getline(meminfo, line)) {
      if (line.find("MemTotal:") == 0) {
        sscanf(line.c_str(), "MemTotal: %ld kB", &total);
      } else if (line.find("MemFree:") == 0) {
        sscanf(line.c_str(), "MemFree: %ld kB", &free);
      } else if (line.find("Buffers:") == 0) {
        sscanf(line.c_str(), "Buffers: %ld kB", &buffers);
      } else if (line.find("Cached:") == 0 || line.find("SReclaimable:") == 0) {
        sscanf(line.c_str(), "Cached: %ld kB", &cached);
      }
    }

    if (total > 0) {
      long used = total - free - buffers - cached;
      return (float)(used * 100) / total;
    }
    return 0.0f;
  }

  enum class Theme { Dark, Light };
  Theme _current_theme = Theme::Dark;
  void renderMainMenu() {
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Exit"))
          glfwSetWindowShouldClose(_window, true);
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Theme")) {
        if (ImGui::MenuItem("Dark", nullptr, _current_theme == Theme::Dark)) {
          _current_theme = Theme::Dark;
          applyTheme();
        }
        if (ImGui::MenuItem("Light", nullptr, _current_theme == Theme::Light)) {
          _current_theme = Theme::Light;
          applyTheme();
        }

        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Options")) {
        ImGui::MenuItem("Enable Resource Monitoring", "", &_enable_monitoring);
        ImGui::MenuItem("Show AI helper", "", &show_ai_window);

        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }
  }
  void renderResourceMonitor() {
    ImGui::Begin("System Resources");

    std::vector<float> cpuData = queueToVector(_cpu_history);
    std::vector<float> memoryData = queueToVector(_memory_history);

    ImVec2 contentSize = ImGui::GetContentRegionAvail();
    const float plotHeight = contentSize.y * 1.0f;

    const float totalWidth = contentSize.x * 0.8f;
    const float plotWidth = totalWidth / 2 - ImGui::GetStyle().ItemSpacing.x;

    ImGui::SetCursorPosX((contentSize.x - totalWidth) * 0.5f);
    ImGui::BeginGroup();

    ImPlot::SetNextAxesLimits(0, maxHistorySize, 0, 100, ImPlotCond_Always);
    if (ImPlot::BeginPlot("##CPU", "Time", "Usage %",
                          ImVec2(plotWidth, plotHeight))) {
      ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), 2.0f);
      ImPlot::PlotLine("CPU", cpuData.data(), cpuData.size());
      ImPlot::EndPlot();
    }

    ImGui::SameLine();

    ImPlot::SetNextAxesLimits(0, maxHistorySize, 0, 100, ImPlotCond_Always);
    if (ImPlot::BeginPlot("##Memory", "Time", "Usage %",
                          ImVec2(plotWidth, plotHeight))) {
      ImPlot::SetNextLineStyle(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), 2.0f);
      ImPlot::PlotLine("Memory", memoryData.data(), memoryData.size());
      ImPlot::EndPlot();
    }

    ImGui::EndGroup();
    ImGui::End();
  }
  float getTotalMemory() {
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    long total = 0;
    while (std::getline(meminfo, line)) {
      if (line.find("MemTotal:") == 0) {
        sscanf(line.c_str(), "MemTotal: %ld kB", &total);
        return total / 1048576.0f;
      }
    }
    return 0.0f;
  }
  /**
   * @description: 渲染AI分析窗口
   * @param model 测试模型引用
   */
  void renderAIAnalysisWindow(ZTestModel &model) {
    static std::string ai_prompt =
        "Please analyze the test result and suggest improvements.";
    static std::string test_file_path;
    static std::string ai_response;
    static std::atomic<bool> is_analyzing{false};
    static std::string error_message;

    if (!show_ai_window)
      return;

    ImGui::Begin("AI Analysis", &show_ai_window);

    if (!model._selected_test.empty()) {
      auto test_result =
          ZTestResultManager::getInstance().getResult(model._selected_test);
      ImGui::Text("Selected Test: %s", test_result.getName().c_str());
      ImGui::TextColored(getStateColor(test_result.getState()), "Status: %s",
                         toString(test_result.getState()));
      ImGui::Text("Used Time: %.2f ms", test_result.getUsedTime());
      ImGui::Separator();
    }

    static char file_path_buffer[1024] = "";
    static char prompt_buffer[4096] = "";

    strncpy(file_path_buffer, test_file_path.c_str(),
            sizeof(file_path_buffer) - 1);
    file_path_buffer[sizeof(file_path_buffer) - 1] = '\0';

    strncpy(prompt_buffer, ai_prompt.c_str(), sizeof(prompt_buffer) - 1);
    prompt_buffer[sizeof(prompt_buffer) - 1] = '\0';

    ImGui::Text("File Path:");
    ImGui::InputTextWithHint("##file_path", "file path", file_path_buffer,
                             sizeof(file_path_buffer),
                             ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::Text("Prompt:");
    ImGui::InputTextMultiline("##prompt", prompt_buffer, sizeof(prompt_buffer),
                              ImVec2(-1, 100));

    test_file_path = file_path_buffer;
    ai_prompt = prompt_buffer;

    if (ImGui::Button("Analyze", ImVec2(-1, 30)) && !is_analyzing) {
      if (!model._selected_test.empty()) {
        is_analyzing = true;
        ai_response.clear();
        error_message.clear();

        std::thread([&, test_name = model._selected_test, prompt = ai_prompt,
                     file_path = test_file_path]() {
          try {

            auto test_result =
                ZTestResultManager::getInstance().getResult(test_name);

            std::ostringstream oss;
            oss << "Test Name: " << test_result.getName() << "\n"
                << "Status: " << toString(test_result.getState()) << "\n"
                << "Used Time: " << test_result.getUsedTime() << " ms\n"
                << "Iterations: " << test_result.getIterations() << "\n";

            if (!file_path.empty()) {
              oss << "\n=== TEST FILE CONTENT ===\n";
              oss << readFileContent(file_path);
              oss << "\n=== END OF FILE ===\n";
            }

            oss << "Prompt: " << prompt;

            ai_response = call_qwen_api(
                oss.str() + "The result should be in commonmark format.",
                getApiKey());

          } catch (const std::exception &e) {
            error_message = std::string("Error: ") + e.what();
          } catch (...) {
            error_message = "Unknown error during AI analysis";
          }
          is_analyzing = false;
        }).detach();

      } else {
        error_message = "No test selected for analysis";
      }
    }

    if (is_analyzing) {
      ImGui::Text("Analyzing...");
      ImGui::ProgressBar(0.0f, ImVec2(-1, 0));
    }

    if (!error_message.empty()) {
      ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", error_message.c_str());
    }

    if (!ai_response.empty()) {
      ImGui::Separator();
      // ImGui::TextWrapped("%s", ai_response.c_str());
      ImGui::PushTextWrapPos();
      ImGui::Markdown(ai_response.c_str(), ai_response.size(),
                      _markdown_config);
      ImGui::PopTextWrapPos();
    }

    ImGui::End();
  }
  std::string readFileContent(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
      throw std::runtime_error("Failed to open file: " + path);
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
  }
  void renderTestList(ZTestModel &model, ZTestController &controller) {
    ImGui::Begin("Test Cases", nullptr);

    static char filterText[128] = "";
    ImGui::InputTextWithHint("##Filter", "Search tests...", filterText,
                             IM_ARRAYSIZE(filterText));
    ImGui::SameLine();

    static int filterState = 0; // 0=All, 1=Passed, 2=Failed, 3=Not Run
    ImGui::Combo("##StateFilter", &filterState,
                 "All\0Passed\0Failed\0Not Run\0");
    ImGui::SameLine();

    enum SortMode { SORT_NAME, SORT_STATUS, SORT_TIME };
    static int sortMode = SORT_NAME;
    static bool sortAscending = true;

    if (ImGui::Button("Name")) {
      sortMode = SORT_NAME;
      sortAscending = !sortAscending;
    }
    ImGui::SameLine();
    if (ImGui::Button("Status")) {
      sortMode = SORT_STATUS;
      sortAscending = !sortAscending;
    }
    ImGui::SameLine();
    if (ImGui::Button("Time")) {
      sortMode = SORT_TIME;
      sortAscending = !sortAscending;
    }

    ImGui::BeginChild("ControlButtons", ImVec2(0, 45), true,
                      ImGuiWindowFlags_NoScrollbar);
    {
      const float totalWidth = ImGui::GetContentRegionAvail().x;
      const float buttonWidth =
          (totalWidth - ImGui::GetStyle().ItemSpacing.x) * 0.48f;
      const float totalButtonsWidth =
          buttonWidth * 2 + ImGui::GetStyle().ItemSpacing.x;
      const float horizontalOffset = (totalWidth - totalButtonsWidth) * 0.5f;

      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + horizontalOffset);

      ImGui::PushStyleColor(
          ImGuiCol_Button, model._is_running
                               ? ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled)
                               : ImVec4(0.25f, 0.65f, 0.25f, 1.0f));
      if (ImGui::Button("Run All", ImVec2(buttonWidth, 30)) &&
          !model._is_running) {
        controller.runAllTests();
      }
      ImGui::PopStyleColor();

      ImGui::SameLine();

      const bool hasSelection = !model._selected_test.empty();
      ImGui::PushStyleColor(
          ImGuiCol_Button, (!hasSelection || model._is_running)
                               ? ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled)
                               : ImVec4(0.25f, 0.45f, 0.75f, 1.0f));
      if (ImGui::Button("Run Selected", ImVec2(buttonWidth, 30)) &&
          hasSelection && !model._is_running) {
        controller.runSelectedTest(model._selected_test);
      }
      ImGui::PopStyleColor();
    }
    ImGui::EndChild();
    ImGui::Separator();

    std::lock_guard<std::mutex> lock(model._mutex);
    auto allResults = ZTestResultManager::getInstance().getResults();

    std::vector<ZTestResult> filteredTests;
    for (const auto &[name, result] : allResults) {

      if (filterText[0] != '\0' && name.find(filterText) == std::string::npos) {
        continue;
      }

      switch (filterState) {
      case 1:
        if (result.getState() != ZState::z_success)
          continue;
        break;
      case 2:
        if (result.getState() != ZState::z_failed)
          continue;
        break;
      case 3:
        if (result.getState() != ZState::z_unknown)
          continue;
        break;
      }

      filteredTests.push_back(result);
    }

    std::sort(filteredTests.begin(), filteredTests.end(),
              [&](const ZTestResult &a, const ZTestResult &b) {
                switch (sortMode) {
                case SORT_NAME:
                  return sortAscending ? a.getName() < b.getName()
                                       : a.getName() > b.getName();
                case SORT_STATUS:
                  return sortAscending ? a.getState() < b.getState()
                                       : a.getState() > b.getState();
                case SORT_TIME:
                  return sortAscending ? a.getUsedTime() < b.getUsedTime()
                                       : a.getUsedTime() > b.getUsedTime();
                }
                return false;
              });

    std::map<std::string, std::vector<ZTestResult>> suiteMap;
    for (const auto &test : filteredTests) {
      size_t dotPos = test.getName().find('.');
      std::string suiteName = (dotPos != std::string::npos)
                                  ? test.getName().substr(0, dotPos)
                                  : "Other";
      suiteMap[suiteName].push_back(test);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {8, 4});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {4, 4});

    ImGuiListClipper clipper;
    clipper.Begin(suiteMap.size());

    while (clipper.Step()) {
      auto it = suiteMap.begin();
      std::advance(it, clipper.DisplayStart);

      for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i, ++it) {
        const auto &[suiteName, tests] = *it;

        bool anyFailed =
            std::any_of(tests.begin(), tests.end(), [](const auto &t) {
              return t.getState() == ZState::z_failed;
            });

        ImGui::PushStyleColor(ImGuiCol_Header,
                              anyFailed ? ImVec4(0.4f, 0.0f, 0.0f, 0.3f)
                                        : ImVec4(0.0f, 0.4f, 0.0f, 0.3f));

        if (ImGui::CollapsingHeader(suiteName.c_str(),
                                    ImGuiTreeNodeFlags_DefaultOpen)) {

          if (ImGui::BeginTable("TestTable", 3,
                                ImGuiTableFlags_BordersInnerV |
                                    ImGuiTableFlags_SizingFixedFit |
                                    ImGuiTableFlags_RowBg)) {

            ImGui::TableSetupColumn("Test Name",
                                    ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed,
                                    100);
            ImGui::TableSetupColumn("Time (ms)",
                                    ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableHeadersRow();

            for (const auto &test : tests) {
              ImGui::TableNextRow();

              ImGui::TableSetColumnIndex(0);
              ImGui::Selectable(test.getName().c_str(),
                                model._selected_test == test.getName(),
                                ImGuiSelectableFlags_SpanAllColumns);

              if (ImGui::IsItemClicked()) {
                model._selected_test = test.getName();
              }

              if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Run Test")) {
                  controller.runSelectedTest(test.getName());
                }
                if (ImGui::MenuItem("Copy Name")) {
                  ImGui::SetClipboardText(test.getName().c_str());
                }
                ImGui::EndPopup();
              }

              ImGui::TableSetColumnIndex(1);
              ImGui::TextColored(getStateColor(test.getState()), "%s",
                                 test.getState() == ZState::z_unknown
                                     ? "Not Run"
                                     : toString(test.getState()));

              ImGui::TableSetColumnIndex(2);
              ImGui::Text("%.2f", test.getUsedTime());
            }
            ImGui::EndTable();
          }
        }
        ImGui::PopStyleColor();
      }
    }

    ImGui::PopStyleVar(2);
    ImGui::End();
  }
  void renderTestCaseRow(const ZTestResult &test, ZTestModel &model) {
    // 添加缩进
    ImGui::Indent(10.0f);

    // 测试名称列
    ImGui::Selectable(test.getName().c_str(),
                      model._selected_test == test.getName());
    if (ImGui::IsItemClicked())
      model._selected_test = test.getName();
    ImGui::Unindent(10.0f);
    ImGui::NextColumn();

    // 状态列
    ImGui::TextColored(getStateColor(test.getState()), "%s",
                       test.getState() == ZState::z_unknown
                           ? "Not Run"
                           : toString(test.getState()));
    ImGui::NextColumn();

    // 时间列
    ImGui::Text("%.2f", test.getUsedTime());
    ImGui::NextColumn();
  }
  ImVec4 getStateColor(ZState state) {
    switch (state) {
    case ZState::z_success:
      return ImVec4(0, 1, 0, 1);
    case ZState::z_failed:
      return ImVec4(1, 0, 0, 1);
    default:
      return ImVec4(1, 1, 1, 1);
    }
  }

  void renderStatusBar(ZTestModel &model) {
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - 40));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 40));

    ImGui::Begin("StatusBar", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_NoNav);

    if (model._is_running) {
      ImGui::ProgressBar(model._progress, ImVec2(-1, 15),
                         model._progress < 1.0f ? "Running..." : "Done...");
    } else {
      int passed = 0, failed = 0;
      const auto &test_cases = ZTestResultManager::getInstance().getResults();
      for (auto &[_, test] : test_cases) {
        if (test.getState() == ZState::z_success)
          passed++;
        else if (test.getState() == ZState::z_failed)
          failed++;
      }
      float cpu_usage = _cpu_history.empty() ? 0.0f : _cpu_history.back();
      float mem_usage = _memory_history.empty() ? 0.0f : _memory_history.back();

      ImVec4 cpuColor = (cpu_usage > 80.0f)   ? ImVec4(1, 0, 0, 1)
                        : (cpu_usage > 50.0f) ? ImVec4(1, 1, 0, 1)
                                              : ImVec4(0, 1, 0, 1);

      ImVec4 memColor = (mem_usage > 80.0f)   ? ImVec4(1, 0, 0, 1)
                        : (mem_usage > 50.0f) ? ImVec4(1, 1, 0, 1)
                                              : ImVec4(0, 1, 0, 1);

      ImGui::Text("Total: %d  Passed: %d  Failed: %d", test_cases.size(),
                  passed, failed);

      ImGui::SameLine();
      ImGui::Text("|");
      ImGui::SameLine();
      ImGui::TextColored(cpuColor, " CPU: %.1f%%", cpu_usage);

      ImGui::SameLine();
      ImGui::TextColored(memColor, " Mem: %.1f%%", mem_usage);
    }

    ImGui::End();
  }

  void renderDetailsWindow(ZTestModel &model) {
    ImGui::Begin("Test Details");

    if (!model._selected_test.empty()) {
      auto it =
          ZTestResultManager::getInstance().getResult(model._selected_test);

      ImGui::Text("Test Name: %s", it.getName().c_str());
      ImGui::TextColored(getStateColor(it.getState()), "Status: %s",
                         toString(it.getState()));
      ImGui::Text("Total Time: %.2f ms", it.getUsedTime());
      ImGui::Text("Average Time: %.6f ms", it.getAverageTime());
      ImGui::Text("Iterations: %d", it.getIterations());

      if (it.getType() == ZType::z_benchmark) {

        auto benchmarkit = &it;
        auto durations = benchmarkit->getIterationTimestamps();
        if (!durations.empty()) {
          if (ImPlot::BeginPlot("##IterationTimes", "Iteration", "Time (ms)",
                                ImVec2(-1, -1))) {
            ImPlot::PlotLine("Duration", durations.data(), durations.size());
            ImPlot::EndPlot();
          }
        }
      }
    }
    ImGui::End();
  }

  GLFWwindow *_window = nullptr;
};

static void glfw_error_callback(int error, const char *description) {
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

inline int showUI() {
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    return 1;

#if defined(IMGUI_IMPL_OPENGL_ES2)
  // GL ES 2.0 + GLSL 100 (WebGL 1.0)
  const char *glsl_version = "#version 100";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
  // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
  const char *glsl_version = "#version 300 es";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
  // GL 3.2 + GLSL 150
  const char *glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
#else
  // GL 3.0 + GLSL 130
  const char *glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+
  // only glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
#endif

  GLFWwindow *window =
      glfwCreateWindow(1280, 720, "ztest gui", nullptr, nullptr);

  if (window == nullptr) {
    logger.error("Failed to create GLFW window");
    glfwTerminate();
    return EXIT_FAILURE;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  ZTestModel model;
  ZTestContext testContext;
  ZTestController controller(model, testContext);
  ZTestView view;
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  view.applyTheme();
  ImFont *font = io.Fonts->AddFontFromFileTTF(
      "Hack-Regular.ttf", 25.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
  // Initialize ImGui backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);
  // variable 初始化测试上下文
  // InitializeTestContext(testContext); // 添加测试用例
  // 初始化MVC组件

  view.setWindow(window);
  model.initializeFromRegistry(testContext);
  bool first_time = true;
  // 主循环

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // 开始ImGui帧
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    if (std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - view._last_update)
            .count() >= 0.2) {
      view.updateResourceUsage();
    }
    ImGuiID dockspace_id = ImGui::DockSpaceOverViewport();

    // if (first_time) {
    //   first_time = false;
    //   ImGui::DockBuilderRemoveNode(dockspace_id);
    //   ImGui::DockBuilderAddNode(dockspace_id);

    //   ImGuiID dock_main = dockspace_id;
    //   ImGuiID dock_left = ImGui::DockBuilderSplitNode(
    //       dock_main, ImGuiDir_Left, 0.3f, nullptr, &dock_main);
    //   ImGuiID dock_right = ImGui::DockBuilderSplitNode(
    //       dock_main, ImGuiDir_Right, 0.5f, nullptr, &dock_main);
    //   ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(
    //       dock_main, ImGuiDir_Down, 0.3f, nullptr, &dock_main);

    //   ImGui::DockBuilderDockWindow("Test Cases", dock_left);
    //   ImGui::DockBuilderDockWindow("Test Details", dock_right);
    //   ImGui::DockBuilderDockWindow("StatusBar", dock_right);
    //   ImGui::DockBuilderDockWindow("System Resources", dock_bottom);

    //   ImGui::DockBuilderFinish(dockspace_id);
    // }
    // 渲染GUI
    view.render(model, controller);

    // 渲染绘制
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }

  // 清理资源
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
inline int runFromCLI(const std::vector<std::string> &args,
                      ZTestContext &context) {
  bool runAll = false;
  std::string selectedTest;

  for (const auto &arg : args) {
    if (arg == "--help") {
      std::cout << "Usage: ztest_cli [OPTIONS]\n"
                << "Options:\n"
                << "  --help           Show this help\n"
                << "  --run-all        Run all tests\n"
                << "  --run-test=name  Run specific test\n"
                << "  --list-tests     List all tests\n"
                << "  --report=formats Generate reports (json,html,junit)\n"
                << "  --no-gui         Run in headless mode\n";
      return 0;
      // TODO: 实现report部分逻辑
    } else if (arg == "--run-all") {
      runAll = true;
    } else if (arg.substr(0, 10) == "--run-test") {
      selectedTest = arg.substr(11);
      // TODO: CLI指定运行指定测试
    } else if (arg == "--list-tests") {
      for (const auto &test : ZTestRegistry::instance().takeTests()) {
        std::cout << test->getName() << "\n";
      }
      return 0;
    }
  }

  ZTestModel model;
  model.initializeFromRegistry(context);

  if (runAll) {
    context.runAllTests();
  } else if (!selectedTest.empty()) {
    if (!context.runSelectedTest(selectedTest)) {
      std::cerr << "Test not found: " << selectedTest << "\n";
      return 1;
    }
  } else {
    std::cerr << "No test specified to run.\n";
    return 1;
  }

  return 0;
}