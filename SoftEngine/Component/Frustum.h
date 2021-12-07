#pragma once

#include <IObject.h>

#include <Buffer.h>

class Frustum : public IRenderableObject
{
private:
	VertexBuffer* m_vb = 0;

public:
	Frustum(const Mat4x4& projection);
	~Frustum();

public:
	static void GetFrustumCorners(std::vector<Vec3>& corners, const Mat4x4& projection);

public:


};

inline Frustum::Frustum(const Mat4x4& projection)
{
    std::vector<Vec3> corners;
    GetFrustumCorners(corners, projection);
    m_vb = new VertexBuffer(&corners[0], corners.size(), sizeof(Vec3));
}

inline Frustum::~Frustum()
{
	delete m_vb;
}

inline void Frustum::GetFrustumCorners(std::vector<Vec3>& corners, const Mat4x4& projection)
{
    corners.clear();

    Vec4 hcorners[8];

    hcorners[0] = Vec4(-1, 1, -1, 1);
    hcorners[1] = Vec4(1, 1, -1, 1);
    hcorners[2] = Vec4(1, -1, -1, 1);
    hcorners[3] = Vec4(-1, -1, -1, 1);

    hcorners[4] = Vec4(-1, 1, 1, 1);
    hcorners[5] = Vec4(1, 1, 1, 1);
    hcorners[6] = Vec4(1, -1, 1, 1);
    hcorners[7] = Vec4(-1, -1, 1, 1);

    Mat4x4 inverseProj = GetInverse(projection);

    for (int i = 0; i < 8; i++) {
        hcorners[i] = hcorners[i] * inverseProj;
        hcorners[i] = hcorners[i] * (1 / hcorners[i].w);

        corners.push_back(Vec3(hcorners[i].x, hcorners[i].y, hcorners[i].z));
    }
}
