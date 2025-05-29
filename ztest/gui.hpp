#pragma once
#include <glad/glad.h>

#include "core/ztest_base.hpp"
#include "core/ztest_context.hpp"
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
#include <GLFW/glfw3.h>
#include <map>
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
      ZTestResult empty_reslut{test->getName(), 0.0, ZState::z_unknown, ""};
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
  void render(ZTestModel &model, ZTestController &controller) {
    renderMainMenu();
    renderTestList(model, controller);
    renderStatusBar(model);
    renderDetailsWindow(model);
  }

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

private:
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

      ImGui::EndMainMenuBar();
    }
  }
  void renderTestList(ZTestModel &model, ZTestController &controller) {
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
      ImGui::Text("Total: %d  Passed: %d  Failed: %d", test_cases.size(),
                  passed, failed);
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
      ImGui::Text("Duration: %.2f ms", it.getUsedTime());

      if (!it.getErrorMsg().empty()) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error Message:");
        ImGui::TextWrapped("%s", it.getErrorMsg().c_str());
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
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  view.applyTheme();
  ImFont *font = io.Fonts->AddFontFromFileTTF(
      "Hack-Regular.ttf", 25.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
  // Initialize ImGui backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version); // Use the actual glsl_version variable
                                        // 初始化测试上下文
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
    ImGuiID dockspace_id = ImGui::DockSpaceOverViewport();

    if (first_time) {
      first_time = false;
      ImGui::DockBuilderRemoveNode(dockspace_id);
      ImGui::DockBuilderAddNode(dockspace_id);

      ImGuiID dock_main = dockspace_id;
      ImGuiID dock_left = ImGui::DockBuilderSplitNode(
          dock_main, ImGuiDir_Left, 0.3f, nullptr, &dock_main);
      ImGuiID dock_right = ImGui::DockBuilderSplitNode(
          dock_main, ImGuiDir_Right, 0.5f, nullptr, &dock_main);

      ImGui::DockBuilderDockWindow("Test Cases", dock_left);
      ImGui::DockBuilderDockWindow("Test Details", dock_right);
      ImGui::DockBuilderDockWindow("StatusBar", dock_right);

      ImGui::DockBuilderFinish(dockspace_id);
    }

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