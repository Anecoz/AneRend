#include "PlayerBehaviour.h"

#include <render/scene/Scene.h>
#include <component/Registry.h>
#include <component/Components.h>

#include "../../common/input/KeyInput.h"

namespace behaviour {

PlayerBehaviour::PlayerBehaviour()
  : IBehaviour()
{}

void PlayerBehaviour::start()
{

}

void PlayerBehaviour::update(double delta)
{
  auto& charComp = _scene->registry().getComponent<component::CharacterController>(_me);

  bool moving = true;

  if (KeyInput::isKeyDown(GLFW_KEY_W)) {
    charComp._desiredLinearVelocity.x = 1.0f;
  }
  else if (KeyInput::isKeyDown(GLFW_KEY_S)) {
    charComp._desiredLinearVelocity.x = -1.0f;
  }
  else {
    charComp._desiredLinearVelocity.x = 0.0f;
    moving = false;
  }
}

}