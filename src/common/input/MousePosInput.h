#pragma once

#include <stdio.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class MousePosInput {

public:
  static glm::vec2 getPosition()
  { 
    //return glm::vec2(_x, _y);
    double xPos, yPos;
    glfwGetCursorPos(_window, &xPos, &yPos);
    //printf("Cursor pos: %lf %lf\n", xPos, yPos);
    return glm::vec2(xPos, yPos);
  }

  static void setDisabledMode()
  {
    //glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }

  static void setNormalMode()
  {
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  }

  static void invoke(GLFWwindow* window, double xPos, double yPos);

  static GLFWwindow* _window;

private:  
  static double _x;
  static double _y;
};