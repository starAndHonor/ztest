#include <glad/glad.h>

#include "gui.hpp"
ZTestModel model;
ZTestContext testContext;
ZTestController controller(model, testContext);
ZTestView view;
int add(int a, int b) {
  sleep(1);
  return a + b;
}
int subtract(int a, int b) { return a - b; }

ZTEST_F(BasicMath, AdditionTest) {
  EXPECT_EQ(5, add(2, 3));
  EXPECT_EQ(0, add(0, 0));
  return ZState::z_success;
}

ZTEST_F(BasicMath, NegativeTest) {
  EXPECT_EQ(-1, add(2, -3));
  EXPECT_EQ(-5, subtract(-2, 3));
  return ZState::z_success;
}
ZTEST_F(BasicMath, FailedAdditionTest) {
  EXPECT_EQ(6, add(2, 3));
  return ZState::z_success;
}
ZTEST_F(BasicMath, FailedSubtractionTest) { EXPECT_EQ(3, subtract(5, 3)); }
ZTEST_F(Performance, MemoryAlloc) {
  const size_t MB100 = 100 * 1024 * 1024;
  auto ptr = std::make_unique<char[]>(MB100);
  ASSERT_TRUE(ptr != nullptr);
  return ZState::z_success;
}
void InitializeTestContext(ZTestContext &context) {
  ZTestSuiteFactory::createSuite("BasicMath", ZType::z_safe,
                                 "基础数学运算测试套件")
      .addTest(TestFactory::createTest("Addition", ZType::z_safe, "", add, 2, 3)
                   .setExpectedOutput(5)
                   .beforeAll([]() { logger.info("准备加法测试环境...\n"); })
                   .build())
      .addTest(TestFactory::createTest("Subtraction", ZType::z_safe, "",
                                       subtract, 5, 3)
                   .setExpectedOutput(2)
                   .afterEach([]() { logger.info("清理减法测试数据...\n"); })
                   .build())
      .addToRegistry();
}
static void glfw_error_callback(int error, const char *description) {
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
int main() {
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    return 1;
    // glewInit(); // 这一步是为了stb_image能用

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
  if (window == nullptr)
    return 1;
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }
  // In main.cpp after creating window:
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  ImGui::StyleColorsDark();
  ImFont *font = io.Fonts->AddFontFromFileTTF(
      "Hack-Regular.ttf", 25.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
  // Initialize ImGui backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version); // Use the actual glsl_version variable
  // 初始化测试上下文
  InitializeTestContext(testContext); // 添加测试用例
  // 初始化MVC组件
  model.initializeFromRegistry(testContext);

  // 主循环
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // 开始ImGui帧
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 更新模型数据
    model.updateFromContext(testContext);

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
