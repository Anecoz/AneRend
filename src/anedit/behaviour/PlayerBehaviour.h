#pragma once

#include <util/Uuid.h>

#include <behaviour/IBehaviour.h>

namespace behaviour
{

class PlayerBehaviour : public IBehaviour
{
public:
  PlayerBehaviour();

  void start() final;
  void update(double delta) final;
};

}