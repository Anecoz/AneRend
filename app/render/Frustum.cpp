#include "Frustum.h"

namespace render {

void Frustum::transform(const glm::mat4& proj, const glm::mat4& view)
{
	double clip[4][4];
	clip[0][0] = view[0][0] * proj[0][0] + view[0][1] * proj[1][0] + view[0][2] * proj[2][0] + view[0][3] * proj[3][0];
	clip[0][1] = view[0][0] * proj[0][1] + view[0][1] * proj[1][1] + view[0][2] * proj[2][1] + view[0][3] * proj[3][1];
	clip[0][2] = view[0][0] * proj[0][2] + view[0][1] * proj[1][2] + view[0][2] * proj[2][2] + view[0][3] * proj[3][2];
	clip[0][3] = view[0][0] * proj[0][3] + view[0][1] * proj[1][3] + view[0][2] * proj[2][3] + view[0][3] * proj[3][3];

	clip[1][0] = view[1][0] * proj[0][0] + view[1][1] * proj[1][0] + view[1][2] * proj[2][0] + view[1][3] * proj[3][0];
	clip[1][1] = view[1][0] * proj[0][1] + view[1][1] * proj[1][1] + view[1][2] * proj[2][1] + view[1][3] * proj[3][1];
	clip[1][2] = view[1][0] * proj[0][2] + view[1][1] * proj[1][2] + view[1][2] * proj[2][2] + view[1][3] * proj[3][2];
	clip[1][3] = view[1][0] * proj[0][3] + view[1][1] * proj[1][3] + view[1][2] * proj[2][3] + view[1][3] * proj[3][3];

	clip[2][0] = view[2][0] * proj[0][0] + view[2][1] * proj[1][0] + view[2][2] * proj[2][0] + view[2][3] * proj[3][0];
	clip[2][1] = view[2][0] * proj[0][1] + view[2][1] * proj[1][1] + view[2][2] * proj[2][1] + view[2][3] * proj[3][1];
	clip[2][2] = view[2][0] * proj[0][2] + view[2][1] * proj[1][2] + view[2][2] * proj[2][2] + view[2][3] * proj[3][2];
	clip[2][3] = view[2][0] * proj[0][3] + view[2][1] * proj[1][3] + view[2][2] * proj[2][3] + view[2][3] * proj[3][3];

	clip[3][0] = view[3][0] * proj[0][0] + view[3][1] * proj[1][0] + view[3][2] * proj[2][0] + view[3][3] * proj[3][0];
	clip[3][1] = view[3][0] * proj[0][1] + view[3][1] * proj[1][1] + view[3][2] * proj[2][1] + view[3][3] * proj[3][1];
	clip[3][2] = view[3][0] * proj[0][2] + view[3][1] * proj[1][2] + view[3][2] * proj[2][2] + view[3][3] * proj[3][2];
	clip[3][3] = view[3][0] * proj[0][3] + view[3][1] * proj[1][3] + view[3][2] * proj[2][3] + view[3][3] * proj[3][3];

	m_data[Right][A] = clip[0][3] - clip[0][0];
	m_data[Right][B] = clip[1][3] - clip[1][0];
	m_data[Right][C] = clip[2][3] - clip[2][0];
	m_data[Right][D] = clip[3][3] - clip[3][0];
	normalize(Right);

	m_data[Left][A] = clip[0][3] + clip[0][0];
	m_data[Left][B] = clip[1][3] + clip[1][0];
	m_data[Left][C] = clip[2][3] + clip[2][0];
	m_data[Left][D] = clip[3][3] + clip[3][0];
	normalize(Left);

	m_data[Bottom][A] = clip[0][3] + clip[0][1];
	m_data[Bottom][B] = clip[1][3] + clip[1][1];
	m_data[Bottom][C] = clip[2][3] + clip[2][1];
	m_data[Bottom][D] = clip[3][3] + clip[3][1];
	normalize(Bottom);

	m_data[Top][A] = clip[0][3] - clip[0][1];
	m_data[Top][B] = clip[1][3] - clip[1][1];
	m_data[Top][C] = clip[2][3] - clip[2][1];
	m_data[Top][D] = clip[3][3] - clip[3][1];
	normalize(Top);

	m_data[Front][A] = clip[0][3] - clip[0][2];
	m_data[Front][B] = clip[1][3] - clip[1][2];
	m_data[Front][C] = clip[2][3] - clip[2][2];
	m_data[Front][D] = clip[3][3] - clip[3][2];
	normalize(Front);

	m_data[Back][A] = clip[0][3] + clip[0][2];
	m_data[Back][B] = clip[1][3] + clip[1][2];
	m_data[Back][C] = clip[2][3] + clip[2][2];
	m_data[Back][D] = clip[3][3] + clip[3][2];
	normalize(Back);
}

void Frustum::normalize(Plane plane)
{
	double magnitude = glm::sqrt(
		m_data[plane][A] * m_data[plane][A] +
		m_data[plane][B] * m_data[plane][B] +
		m_data[plane][C] * m_data[plane][C]
	);

	m_data[plane][A] /= magnitude;
	m_data[plane][B] /= magnitude;
	m_data[plane][C] /= magnitude;
	m_data[plane][D] /= magnitude;
}

Frustum::Visibility Frustum::isInside(const glm::vec3& point) const
{
	for (unsigned int i = 0; i < 6; i++) {
		if (m_data[i][A] * point.x +
			m_data[i][B] * point.y +
			m_data[i][C] * point.z +
			m_data[i][D] <= 0) {
			return Invisible;
		}
	}

	return Completly;
}

Frustum::Visibility Frustum::isInside(const Box3D& box) const
{
	auto GetVisibility = [](const glm::vec4& clip, const Box3D& box)
	{
		double x0 = box.getMin().x * clip.x;
		double x1 = box.getMax().x * clip.x;
		double y0 = box.getMin().y * clip.y;
		double y1 = box.getMax().y * clip.y;
		double z0 = box.getMin().z * clip.z + clip.w;
		double z1 = box.getMax().z * clip.z + clip.w;
		double p1 = x0 + y0 + z0;
		double p2 = x1 + y0 + z0;
		double p3 = x1 + y1 + z0;
		double p4 = x0 + y1 + z0;
		double p5 = x0 + y0 + z1;
		double p6 = x1 + y0 + z1;
		double p7 = x1 + y1 + z1;
		double p8 = x0 + y1 + z1;

		if (p1 <= 0 && p2 <= 0 && p3 <= 0 && p4 <= 0 && p5 <= 0 && p6 <= 0 && p7 <= 0 && p8 <= 0) {
			return Invisible;
		}
		if (p1 > 0 && p2 > 0 && p3 > 0 && p4 > 0 && p5 > 0 && p6 > 0 && p7 > 0 && p8 > 0) {
			return Completly;
		}

		return Partially;
	};

	Visibility v0 = GetVisibility(getPlane(Right), box);
	if (v0 == Invisible) {
		return Invisible;
	}

	Visibility v1 = GetVisibility(getPlane(Left), box);
	if (v1 == Invisible) {
		return Invisible;
	}

	Visibility v2 = GetVisibility(getPlane(Bottom), box);
	if (v2 == Invisible) {
		return Invisible;
	}

	Visibility v3 = GetVisibility(getPlane(Top), box);
	if (v3 == Invisible) {
		return Invisible;
	}

	Visibility v4 = GetVisibility(getPlane(Front), box);
	if (v4 == Invisible) {
		return Invisible;
	}

	if (v0 == Completly && v1 == Completly &&
		v2 == Completly && v3 == Completly &&
		v4 == Completly)
	{
		return Completly;
	}

	return Partially;
}
}