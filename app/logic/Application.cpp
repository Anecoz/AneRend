#include "Application.h"

//#include "../imgui/imgui.h"
//#include "../imgui/imgui_impl_glfw.h"
//#include "../imgui/imgui_impl_opengl3.h"

#include "GLFW/glfw3.h"

#include "../input/KeyInput.h"
#include "../input/MousePosInput.h"
#include "../input/MouseButtonInput.h"

#include <iostream>
#include <string>

static void errorCallback(int error, const char* description)
{
  std::cerr << "GLFW Error code: " << std::to_string(error) << ", description: " << description << std::endl;
}

Application::Application(std::string title)
  : _running(false)
  , _title(std::move(title))
{
  glfwSetErrorCallback(errorCallback);
  if (!glfwInit())
    exit(-1);

  // Create gl window stuff
  initWindowHandle();

  // Dear, imgui
  //initImgui();
}

Application::~Application()
{
  glfwDestroyWindow(_window);
  glfwTerminate();
}

void Application::run()
{
  // Init Main loop stuff
  int frames = 0;
  std::string fps = " fps";

  _running = true;

  // Main loop
  double lastUpdate = glfwGetTime();
  double lastFrameUpdate = glfwGetTime();
  while (_running  && !glfwWindowShouldClose(_window)) {
    // Logic update
    glfwPollEvents();

    // ImGui stuff
    //ImGui_ImplOpenGL3_NewFrame();
    //ImGui_ImplGlfw_NewFrame();
    //ImGui::NewFrame();

    double now = glfwGetTime();
    double delta = (now - lastUpdate);
    lastUpdate = now;
    update(delta);

    // Render
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    render();

    // ImGui render
    //ImGui::Render();
    //ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    //glfwSwapBuffers(_window);
    // Render stop

    frames++;
    double now2 = glfwGetTime();
    if ((now2 - lastFrameUpdate) > 1.0) {
      lastFrameUpdate = now2;
      glfwSetWindowTitle(_window, std::string(_title + " " + std::to_string(frames) + fps).c_str());
      frames = 0;
    }
  }
  // Do cleanup here
  //ImGui_ImplOpenGL3_Shutdown();
  //ImGui_ImplGlfw_Shutdown();
  //ImGui::DestroyContext();
}

void Application::initWindowHandle()
{
  // This should be a config, along with the hardcoded window size.
  //glfwWindowHint(GLFW_SAMPLES, 8);
  //glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  _window = glfwCreateWindow(1920, 1080, _title.c_str(), NULL, NULL);

  if (!_window) {
    std::cerr << "Failed to open GLFW window" << std::endl;
    glfwTerminate();
    exit(-2);
  }

  //glfwMakeContextCurrent(_window);
  //glfwSwapInterval(1); // Vsync or not

  // Set input callbacks
  glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
  glfwSetKeyCallback(_window, KeyInput::invoke);
  //glfwSetCursorPosCallback(_window, MousePosInput::invoke);
  MousePosInput::_window = _window;
  glfwSetMouseButtonCallback(_window, MouseButtonInput::invoke);

  // Check for errors
  /*GLenum err = glewInit();
  if (err != GLEW_OK) {
    std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
    exit(-3);
  }

  glClearColor((GLclampf)(5.0/255.0), (GLclampf)(209.0/255.0), (GLclampf)(255.0/255.0), (GLclampf)0.0);
  glEnable(GL_DEPTH_TEST);
  std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
  std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
  std::cout << "Supported OpenGL version: " << glGetString(GL_VERSION) << std::endl;
  GLint maxPatchVertices = 0;
  glGetIntegerv(GL_MAX_PATCH_VERTICES, &maxPatchVertices);
  std::cout << "Max supported patch vertices: " << std::to_string(maxPatchVertices) << std::endl;
  glPatchParameteri(GL_PATCH_VERTICES, 3);
  glEnable(GL_CULL_FACE);*/
}

/*void Application::initImgui()
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(_window, true);
  ImGui_ImplOpenGL3_Init();
}*/