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
    }

#if 0
    // Radio buttons for choosing guizmo mode
    ImGui::SameLine();
    if (ImGui::RadioButton("BS", _currentGuizmoOp == _bsGuizmoOp)) {
      _currentGuizmoOp = _bsGuizmoOp;
    }
#endif

    // Name
    ImGui::Separator();
    ImGui::Text("Name");
    if (ImGui::InputText("##name", name, 32) && id) {
      changed = true;
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

  // Do gizmo (transform) editing with ImGuizmo
  if (id) {
#if 0
    glm::vec3 modelTrans = c->scene().getRenderable(id)->_localTransform[3];
    glm::mat4 globalTransform = c->scene().getRenderable(id)->_globalTransform;

    glm::vec3 globalTranslationBefore = globalTransform[3];
    glm::vec3 parent = globalTranslationBefore - modelTrans;
    glm::vec3 parentLocalDiff = modelTrans - parent;
    glm::mat4 m{ 1.0f };

    if (_currentGuizmoOp == _bsGuizmoOp) {
      m = glm::translate(m, boundingSphereCenter + modelTrans);
    }
    else {
      m = c->scene().getRenderable(id)->_localTransform;

      // Set translation to global trans, just for visual purposes
      m[3] = globalTransform[3];
    }
    
    auto& viewMatrix = c->camera().getCamMatrix();
    auto& projMatrix = c->camera().getProjection();

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    bool manipulated = ImGuizmo::Manipulate(
      &viewMatrix[0][0],
      &projMatrix[0][0],
      _currentGuizmoOp == _bsGuizmoOp ? ImGuizmo::OPERATION::TRANSLATE : (ImGuizmo::OPERATION)_currentGuizmoOp,
      ImGuizmo::MODE::WORLD,
      (float*)&m[0][0]);

    if (manipulated) {

      if (_currentGuizmoOp == _bsGuizmoOp) {
        glm::vec3 t = m[3];
        t = t - modelTrans;
        c->scene().setRenderableBoundingSphere(id, glm::vec4(t, boundingSphereRadius));
      }
      else {
        // Calc new local translation from new global and old global translation
        glm::vec3 newGlobalTrans = m[3];
        glm::vec3 newLocalTrans = newGlobalTrans - globalTranslationBefore + parentLocalDiff + parent;
        glm::mat4 temp = glm::translate(glm::mat4(1.0f), newLocalTrans);

        m[3] = temp[3];
        c->scene().setRenderableTransform(id, m);
      }
    }
#endif
  }

}

}