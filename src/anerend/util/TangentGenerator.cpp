#include "TangentGenerator.h"

#include "../render/asset/Mesh.h"

#include <mikktspace.h>

namespace util {

namespace {

int getNumFaces(const SMikkTSpaceContext* context)
{
  auto* mesh = (render::asset::Mesh*)context->m_pUserData;
  return (int)mesh->_indices.size() / 3;
}

int getNumVerticesOfFace(const SMikkTSpaceContext* context, int iFace)
{
  return 3;
}

int getVertexIndex(const SMikkTSpaceContext* context, int iFace, int iVert)
{
  auto* mesh = (render::asset::Mesh*)context->m_pUserData;

  auto faceSize = getNumVerticesOfFace(context, iFace);

  auto indicesIndex = (iFace * faceSize) + iVert;

  int index = mesh->_indices[indicesIndex];
  return index;
}

void getPosition(const SMikkTSpaceContext* context, float outpos[], int iFace, int iVert)
{
  auto* mesh = (render::asset::Mesh*)context->m_pUserData;

  auto index = getVertexIndex(context, iFace, iVert);
  auto& vertex = mesh->_vertices[index];

  outpos[0] = vertex.pos.x;
  outpos[1] = vertex.pos.y;
  outpos[2] = vertex.pos.z;
}

void getNormal(const SMikkTSpaceContext* context, float outnormal[], int iFace, int iVert)
{
  auto* mesh = (render::asset::Mesh*)context->m_pUserData;

  auto index = getVertexIndex(context, iFace, iVert);
  auto& vertex = mesh->_vertices[index];

  outnormal[0] = vertex.normal.x;
  outnormal[1] = vertex.normal.y;
  outnormal[2] = vertex.normal.z;
}

void getTexCoords(const SMikkTSpaceContext* context, float outuv[], int iFace, int iVert)
{
  auto* mesh = (render::asset::Mesh*)context->m_pUserData;

  auto index = getVertexIndex(context, iFace, iVert);
  auto& vertex = mesh->_vertices[index];

  outuv[0] = vertex.uv.x;
  outuv[1] = vertex.uv.y;
}

void setTspaceBasic(const SMikkTSpaceContext* context, const float tangentu[], float fSign, int iFace, int iVert)
{
  auto* mesh = (render::asset::Mesh*)context->m_pUserData;

  auto index = getVertexIndex(context, iFace, iVert);
  auto& vertex = mesh->_vertices[index];

  vertex.tangent.x = tangentu[0];
  vertex.tangent.y = tangentu[1];
  vertex.tangent.z = tangentu[2];
  vertex.tangent.w = fSign;
}

}

void TangentGenerator::generateTangents(render::asset::Mesh& mesh)
{
  SMikkTSpaceInterface iface{};
  iface.m_getNormal = getNormal;
  iface.m_getNumFaces = getNumFaces;
  iface.m_getNumVerticesOfFace = getNumVerticesOfFace;
  iface.m_getPosition = getPosition;
  iface.m_getTexCoord = getTexCoords;
  iface.m_setTSpaceBasic = setTspaceBasic;

  SMikkTSpaceContext context{};
  context.m_pInterface = &iface;
  context.m_pUserData = (void*)&mesh;

  genTangSpaceDefault(&context);
}

}