#pragma once

#include <string>

struct GLFWwindow;

class Application {
public:
  Application(std::string title);
  virtual ~Application();

  Application() = delete;
  Application(const Application&) = delete;
  Application(Application&&) = delete;
  Application& operator=(const Application&) = delete;
  Application& operator=(Application&&) = delete;

  void run();

  virtual void render() = 0;
  virtual void update(double delta) = 0;

protected:
  GLFWwindow* _window;

private:
  void initWindowHandle();
  //void initImgui();

  bool _running;
  std::string _title;
};

