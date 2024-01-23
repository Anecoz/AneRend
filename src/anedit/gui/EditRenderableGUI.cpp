#include "EditRenderableGUI.h"

#include "../logic/AneditContext.h"
#include <render/scene/Scene.h>

#include <glm/glm.hpp>
#include <imgui.h>
#include <ImGuizmo.h>

namespace gui {

EditRenderableGUI::EditRenderableGUI()
  : IGUI()
  , _bsGuizmoOp(ImGuizmo::OPERATION::UNIVERSAL + 1)
{}

EditRenderableGUI::~EditRenderableGUI()
{}

void EditRenderableGUI::immediateDraw(logic::AneditContext* c)
{
  auto id = c->getFirstSelection();
  bool changed = false;

  glm::vec3 tint{ 0.0f };

  glm::vec3 boundingSphereCenter{ 0.0f };
  float boundingSphereRadius = 0.0f;

  bool visible = false;

  char name[100];
  name[0] = '\0';

  char model[100];
  model[0] = '\0';

  /*char skeleton[100];
  skeleton[0] = '\0';*/

  std::vector<util::Uuid> materials;

  {
    if (!id) {
      ImGui::BeginDisabled();
    }
    else {
      auto& rendComp = c->scene().registry().getComponent<component::Renderable>(id);

      strcpy_s(name, rendComp._name.c_str());
      tint = rendComp._tint;
      boundingSphereCenter = glm::vec3(rendComp._boundingSphere.x, rendComp._boundingSphere.y, rendComp._boundingSphere.z);
      boundingSphereRadius = rendComp._boundingSphere.w;
      visible = rendComp._visible;
      materials = rendComp._materials;

      auto modelName = c->scene().getModel(rendComp._model)->_name;
      //auto skeletonName = rendComp._skeleton ? c->scene().getSkeleton(rendComp._skeleton)->_name : "(None)";

      strcpy_s(model, modelName.c_str());
      //strcpy_s(skeleton, skeletonName.c_str());
    }

    // Name
    ImGui::Separator();
    ImGui::Text("Name");
    if (ImGui::InputText("##name", name, 100) && id) {
      changed = true;
    }

    // Model name
    ImGui::Separator();
    ImGui::Text("Model");
    ImGui::Text(model);

    // Skeleton name
    /*ImGui::Separator();
    ImGui::Text("Skeleton");
    ImGui::Text(skeleton);*/

    // Materials (potentially many, so put in a collapsible header)
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Material(s)")) {
      for (auto& mat : materials) {
        auto p = c->scene().getMaterial(mat);
        ImGui::Selectable(p->_name.c_str(), false);
      }
    }

    // Tint
    ImGui::Separator();
    ImGui::Text("Tint");
    if (ImGui::ColorEdit3("##tint", &tint[0]) && id) {
      changed = true;
    }

    // Bounding sphere
    ImGui::Separator();
    ImGui::Text("Bounding Sphere");
    if (ImGui::InputFloat3("##boundingSphereCenter", &boundingSphereCenter[0], "%.3f", ImGuiInputTextFlags_EnterReturnsTrue) && id) {
      changed = true;
    }

    if (ImGui::InputFloat("##boundingSphereRadius", &boundingSphereRadius) && id) {
      changed = true;
    }

    // Visible flag
    ImGui::Separator();
    ImGui::Text("Visibility");
    if (ImGui::Checkbox("##visible", &visible)) {
      changed = true;
    }

    if (!id) {
      ImGui::EndDisabled();
    }
    else if (changed) {
      auto& rendComp = c->scene().registry().getComponent<component::Renderable>(id);
      rendComp._boundingSphere = glm::vec4(glm::vec4(boundingSphereCenter, boundingSphereRadius));
      rendComp._name = name;
      rendComp._tint = tint;
      rendComp._visible = visible;

      c->scene().registry().patchComponent<component::Renderable>(id);
    }
  }
}

}