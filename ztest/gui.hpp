#pragma once
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>  // Windows平台专用头文件
#include <pdh.h>
#pragma comment(lib, "pdh.lib")  // Windows平台需链接PDH库
// 取消定义ERROR宏，以避免与枚举值冲突
#ifdef ERROR
#undef ERROR
#endif
#else
#include <fstream>
#include <sstream>
#endif



#include <glad/glad.h>

#include "./lib/implot/implot.h"
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
#include <GLFW/glfw3.h>
#include <map>
#include <algorithm> // for std::min_element, std::max_element
#include <numeric>   // for std::accumulate
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

class ZTestView {
public:
      ZTestView() {
#ifdef _WIN32
        // Windows平台初始化PDH查询
        if (PdhOpenQuery(nullptr, 0, &_cpuQuery) == ERROR_SUCCESS) {
            PdhAddCounterA(_cpuQuery, "\\Processor(_Total)\\% Processor Time", 0, &_cpuCounter);
            PdhCollectQueryData(_cpuQuery); // 初始收集
        }
#endif
    }

    ~ZTestView() {
#ifdef _WIN32
        // Windows平台清理PDH资源
        if (_cpuQuery) {
            PdhCloseQuery(_cpuQuery);
        }
#endif
    }
  void render(ZTestModel &model, ZTestController &controller) {
    handleShortcuts(model, controller);
    
    // 渲染快捷键配置窗口
    if (_showShortcutConfig) {
        renderShortcutConfig();
    }
    renderMainMenu();
        // 获取屏幕尺寸
    ImVec2 screen_size = ImGui::GetIO().DisplaySize;

    // 测试列表（左侧50%）
    ImGui::SetNextWindowPos(ImVec2(0, 40)); // 保持20px菜单栏偏移
    ImGui::SetNextWindowSize(ImVec2(
        screen_size.x * 0.5f,  // 改为50%宽度
        screen_size.y - 100      // 总高度减去菜单栏和状态栏
    ));
    renderTestList(model, controller);

    // 测试详情（右侧50%的上半部分）
    ImGui::SetNextWindowPos(ImVec2(
        screen_size.x * 0.5f,  // 从50%位置开始
        20                     // 保持20px菜单栏偏移
    ));
    ImGui::SetNextWindowSize(ImVec2(
        screen_size.x * 0.5f,  // 50%宽度
        (screen_size.y - 60) * 0.6f // 高度的60%
    ));
    renderDetailsWindow(model);

    // 资源监控（右侧50%的下半部分）
    ImGui::SetNextWindowPos(ImVec2(
        screen_size.x * 0.5f,  // 从50%位置开始
        20 + (screen_size.y - 60) * 0.6f // 接在详情窗口下方
    ));
    ImGui::SetNextWindowSize(ImVec2(
        screen_size.x * 0.5f,  // 50%宽度
        (screen_size.y - 60) * 0.4f // 高度的40%
    ));
    renderResourceMonitor();

    renderStatusBar(model);

  //   if (_flashRunAllButton) {
  //     _flashTimer += ImGui::GetIO().DeltaTime;
  //     if (_flashTimer > 0.5f) {
  //         _flashRunAllButton = false;
  //     }
  // }
  }
  void handleShortcuts(ZTestModel& model, ZTestController& controller)//新添加6.2-w
  {
    if (!_shortcutsEnabled) return;
    ImGuiIO& io = ImGui::GetIO();
    
    // ctrl-r - 运行所有测试
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_R) 
        && !model._is_running) {
        
        controller.runAllTests();
    }
    
    // Ctrl+Enter - 运行选中测试
      
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Enter) && 
        !model._selected_test.empty() && !model._is_running) {
        controller.runSelectedTest(model._selected_test);
    }
    
    // // Ctrl+F - 聚焦到搜索框
    // if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F)) {
    //     _focusSearchBox = true;
    // }
}
  
  void renderShortcutConfig()//新添加6.2-w
  {
    if (!ImGui::Begin("Shortcut Configuration", &_showShortcutConfig)) {
      ImGui::End();
      return;
    }
  
  ImGui::Text("Configure keyboard shortcuts:");
  ImGui::Separator();
  
  ImGui::Text("Run All Tests:");
  ImGui::SameLine();
  ImGui::Text("Ctrl+R");
  ImGui::Text("Run Selected Test:");
  ImGui::SameLine();
  ImGui::Text("Ctrl+Enter");
  }//finish

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
  // 快捷键相关变量
    //bool _focusSearchBox = false;
    bool _shortcutsEnabled = true;
    bool _showShortcutConfig = false;
    //bool _flashRunAllButton = false;
    float _flashTimer = 0.0f;
    // 添加新方法
    //void handleShortcuts(ZTestModel& model, ZTestController& controller);
    //void renderShortcutConfig();

    #ifdef _WIN32
    PDH_HQUERY _cpuQuery = nullptr;
    PDH_HCOUNTER _cpuCounter = nullptr;
#else
    long _prevTotal = 0, _prevIdle = 0;
#endif

  std::queue<float> _cpu_history;
  std::queue<float> _memory_history;
  const size_t maxHistorySize = 60;
  bool _enable_monitoring = true;
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
  #ifdef _WIN32
  #pragma comment(lib, "pdh.lib")
    static PDH_HQUERY cpuQuery = nullptr;
    static PDH_HCOUNTER cpuTotal = nullptr;
    
    // 初始化查询和计数器
    if (cpuQuery == nullptr) {
        if (PdhOpenQuery(nullptr, 0, &cpuQuery) != ERROR_SUCCESS) {
            return 0.0f;
        }
        
        if (PdhAddCounterA(cpuQuery, "\\Processor(_Total)\\% Processor Time", 0, &cpuTotal) != ERROR_SUCCESS) {
            PdhCloseQuery(cpuQuery);
            cpuQuery = nullptr;
            return 0.0f;
        }
        
        // 第一次调用需要收集数据
        PdhCollectQueryData(cpuQuery);
        Sleep(1000); // 等待1秒收集数据
    }
    
    // 收集当前数据
    if (PdhCollectQueryData(cpuQuery) != ERROR_SUCCESS) {
        return 0.0f;
    }
    
    // 获取计数器值
    PDH_FMT_COUNTERVALUE counterVal;
    if (PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, nullptr, &counterVal) != ERROR_SUCCESS) {
        return 0.0f;
    }
    
    // 返回CPU使用率百分比
    return static_cast<float>(counterVal.doubleValue);
  #else
// 原始的Linux实现
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
#endif
}

  // ��ȡ�ڴ�ʹ����
  float getMemoryUsage() {
    #ifdef _WIN32
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memStatus)) {
        unsigned __int64 totalMemory = memStatus.ullTotalPhys;
        unsigned __int64 availableMemory = memStatus.ullAvailPhys;

        if (totalMemory > 0) {
            unsigned __int64 usedMemory = totalMemory - availableMemory;
            return static_cast<float>(usedMemory * 100) / static_cast<float>(totalMemory);
        }
    }
    return 0.0f;
#else
    // Linux �汾
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
#endif
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

      // Add Theme menu
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
        if (ImGui::BeginMenu("Shortcuts Configuration...", &_showShortcutConfig)) {  // true表示始终保持菜单打开
                // 快捷键配置面板直接作为子菜单内容
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0,1,1,1), "Configure keyboard shortcuts:");
                ImGui::Separator();
                
                // sameline 后面是空格长度-6.4-w
                ImGui::Text("Run All Tests"); 
                ImGui::SameLine(300); ImGui::TextColored(ImVec4(1,1,0,1), "Ctrl+R");
                
                ImGui::Text("Run Selected Test");
                ImGui::SameLine(300); ImGui::TextColored(ImVec4(1,1,0,1), "Ctrl+Enter");
                
                ImGui::EndMenu();
        }
            
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

    // Calculate centered width (80% of available width)
    const float totalWidth = contentSize.x * 0.8f;
    const float plotWidth = totalWidth / 2 - ImGui::GetStyle().ItemSpacing.x;

    // Horizontal centering container
    ImGui::SetCursorPosX((contentSize.x - totalWidth) * 0.5f);
    ImGui::BeginGroup();

    // CPU Plot
    ImPlot::SetNextAxesLimits(0, maxHistorySize, 0, 100, ImPlotCond_Always);
    if (ImPlot::BeginPlot("##CPU", "Time", "Usage %",
                          ImVec2(plotWidth, plotHeight))) {
      ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), 2.0f);
      ImPlot::PlotLine("CPU", cpuData.data(), cpuData.size());
      ImPlot::EndPlot();
    }

    ImGui::SameLine();

    // Memory Plot
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
        return total / 1048576.0f; // Convert to GB
      }
    }
    return 0.0f;
  }
  void renderTestList(ZTestModel &model, ZTestController &controller) {
    
    //ImGui::SetNextWindowSize(ImVec2(640.0f, 720.0f), ImGuiCond_Always);
    ImGui::Begin("Test Cases", nullptr);
    // Filter controls
    static char filterText[128] = "";
    ImGui::InputTextWithHint("##Filter", "Search tests...", filterText,
                             IM_ARRAYSIZE(filterText));
    ImGui::SameLine();

    static int filterState = 0; // 0=All, 1=Passed, 2=Failed, 3=Not Run
    ImGui::Combo("##StateFilter", &filterState,
                 "All\0Passed\0Failed\0Not Run\0");
    ImGui::SameLine();

    // Sorting controls
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

      // Apply horizontal centering
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + horizontalOffset);

      // Run All button
    //   if (_flashRunAllButton) {
    //     ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));
    //     ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.7f));
    //     ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.9f));
    // }

      ImGui::PushStyleColor(
          ImGuiCol_Button, model._is_running
                               ? ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled)
                               : ImVec4(0.25f, 0.65f, 0.25f, 1.0f));
      if (ImGui::Button("Run All", ImVec2(buttonWidth, 30)) &&
          !model._is_running) {
            // _flashRunAllButton = true;
            // _flashTimer = 0.0f;
        controller.runAllTests();
      }
      ImGui::PopStyleColor();

    //   if (_flashRunAllButton) {
    //     ImGui::PopStyleColor(3);
// }

      ImGui::SameLine();

      // Run Selected button
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

    // Filter and sort tests
    std::vector<ZTestResult> filteredTests;
    for (const auto &[name, result] : allResults) {
      // Text filter
      if (filterText[0] != '\0' && name.find(filterText) == std::string::npos) {
        continue;
      }

      // State filter
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

    // Sorting logic
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

    // Group by suite after sorting
    std::map<std::string, std::vector<ZTestResult>> suiteMap;
    for (const auto &test : filteredTests) {
      size_t dotPos = test.getName().find('.');
      std::string suiteName = (dotPos != std::string::npos)
                                  ? test.getName().substr(0, dotPos)
                                  : "Other";
      suiteMap[suiteName].push_back(test);
    }

    // Style improvements
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {8, 4});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {4, 4});

    // Performance optimization
    ImGuiListClipper clipper;
    clipper.Begin(suiteMap.size());

      ImGui::Text("Test Name");
      ImGui::SameLine();
      ImGui::Dummy(ImVec2(285, 0));
      ImGui::SameLine();
      ImGui::Text("Status");
      ImGui::SameLine();
      ImGui::Dummy(ImVec2(20, 0));
      ImGui::SameLine();
      ImGui::Text("Time (ms)");

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

            for (const auto &test : tests) {
              ImGui::TableNextRow();

              // Test name column
              ImGui::TableSetColumnIndex(0);
              ImGui::Selectable(test.getName().c_str(),
                                model._selected_test == test.getName(),
                                ImGuiSelectableFlags_SpanAllColumns);

              if (ImGui::IsItemClicked()) {
                model._selected_test = test.getName();
              }

              // Context menu
              if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Run Test")) {
                  controller.runSelectedTest(test.getName());
                }
                if (ImGui::MenuItem("Copy Name")) {
                  ImGui::SetClipboardText(test.getName().c_str());
                }
                ImGui::EndPopup();
              }

              // Status column
              ImGui::TableSetColumnIndex(1);
              ImGui::TextColored(getStateColor(test.getState()), "%s",
                                 test.getState() == ZState::z_unknown
                                     ? "Not Run"
                                     : toString(test.getState()));

              // Time column
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
    // ��������
    ImGui::Indent(10.0f);

    // ����������
    ImGui::Selectable(test.getName().c_str(),
                      model._selected_test == test.getName());
    if (ImGui::IsItemClicked())
      model._selected_test = test.getName();
    ImGui::Unindent(10.0f);
    ImGui::NextColumn();

    // ״̬��
    ImGui::TextColored(getStateColor(test.getState()), "%s",
                       test.getState() == ZState::z_unknown
                           ? "Not Run"
                           : toString(test.getState()));
    ImGui::NextColumn();

    // ʱ����
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

      // Add color-coded resource indicators
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

        // 基础信息展示
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

// Decide GL+GLSL versions
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
      glfwCreateWindow(2560, 1440, "ztest gui", nullptr, nullptr);

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
  static ImGuiIO& io = ImGui::GetIO();
  // (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  // 由于 ImGuiIO 没有 KeyMap 成员，使用 io.KeyMap 会报错，从 ImGui 1.87 开始使用 io.KeyMap 已被弃用，应使用 io.AddKeyEvent
  io.AddKeyEvent(ImGuiKey_R, GLFW_KEY_R);
  // 由于 ImGuiIO 没有 KeyMap 成员，从 ImGui 1.87 开始使用 io.KeyMap 已被弃用，改用 io.AddKeyEvent
  io.AddKeyEvent(ImGuiKey_Enter, GLFW_KEY_ENTER);
    // io.KeyMap[ImGuiKey_F] = GLFW_KEY_F; // ?疑点。疑似可以去除
  view.applyTheme();
  ImFont *font = io.Fonts->AddFontFromFileTTF(
      "Hack-Regular.ttf", 25.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
  // Initialize ImGui backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version); // Use the actual glsl_version
                                        // variable ��ʼ������������
  // InitializeTestContext(testContext); // ���Ӳ�������
  // ��ʼ��MVC���

  view.setWindow(window);
  model.initializeFromRegistry(testContext);
  bool first_time = true;
  // ��ѭ��

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // ��ʼImGui֡
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
    // ��ȾGUI
    view.render(model, controller);

    // ��Ⱦ����
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }

  // ������Դ
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
      // TODO: ʵ��report�����߼�
    } else if (arg == "--run-all") {
      runAll = true;
    } else if (arg.substr(0, 10) == "--run-test") {
      selectedTest = arg.substr(11);
      // TODO: CLIָ������ָ������
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
