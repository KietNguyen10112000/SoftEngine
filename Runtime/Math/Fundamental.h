#pragma once
#pragma warning(disable:26495)

#define GLM_FORCE_RADIANS
//#define GLM_FORCE_LEFT_HANDED
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/matrix_decompose.hpp"
//#include <glm/gtx/quaternion.hpp>
#include "glm/gtx/quaternion.hpp"

#include "glm/gtx/matrix_transform_2d.hpp"

namespace math
{

constexpr float PI = 3.14159265359f;

class Vec2 : glm::vec2
{
private:
    friend class Mat3;
    friend class Vec3;
    friend class Quaternion;
    using Base = glm::vec2;

public:
    const static Vec2 ZERO;

    // Vec2(1, 0)
    const static Vec2 X_AXIS;
    // Vec2(0, 1)
    const static Vec2 Y_AXIS;

    const static Vec2 UP;
    const static Vec2 DOWN;

    const static Vec2 LEFT;
    const static Vec2 RIGHT;

private:
    inline Vec2(const glm::vec2& v)
    {
        GLMVec() = v;
    }

    inline glm::vec2& GLMVec()
    {
        return reinterpret_cast<glm::vec2&>(*this);
    }

    inline const glm::vec2& GLMVecConst() const
    {
        return reinterpret_cast<const glm::vec2&>(*this);
    }

public:
    inline Vec2(const Vec2& v)
    {
        x = v.x;
        y = v.y;
    }

    using Base::Base;
    using Base::operator[];
    using Base::x;
    using Base::y;

#define Vec2ScalarOperator(opt)                             \
    template <typename T>                                   \
    inline Vec2& operator##opt##=(T scalar)                 \
    {                                                       \
        x opt##= static_cast<float>(scalar);                \
        y opt##= static_cast<float>(scalar);                \
        return *this;                                       \
    }                                                       \
    template <typename T>                                   \
    inline Vec2 operator##opt##(T scalar) const             \
    {                                                       \
        Vec2 ret;                                           \
        ret.x = x opt static_cast<float>(scalar);           \
        ret.y = y opt static_cast<float>(scalar);           \
        return ret;                                         \
    }                                                       \
    template<typename T>                                    \
    friend Vec2 operator##opt##(T scalar, const Vec2& vec)  \
    {                                                       \
        Vec2 ret;                                           \
        ret.x = static_cast<float>(scalar) opt vec.x;       \
        ret.y = static_cast<float>(scalar) opt vec.y;       \
        return ret;                                         \
    }

#define Vec2Vec2Operator(opt)                           \
    inline Vec2& operator##opt##=(const Vec2& v)        \
    {                                                   \
        x opt##= v.x;                                   \
        y opt##= v.y;                                   \
        return *this;                                   \
    }                                                   \
    inline Vec2 operator##opt##(const Vec2& v) const    \
    {                                                   \
        Vec2 ret;                                       \
        ret.x = x opt v.x;                              \
        ret.y = y opt v.y;                              \
        return ret;                                     \
    }

    Vec2ScalarOperator(+);
    Vec2ScalarOperator(-);
    Vec2ScalarOperator(*);
    Vec2ScalarOperator(/);

    Vec2Vec2Operator(+);
    Vec2Vec2Operator(-);
    Vec2Vec2Operator(*);
    Vec2Vec2Operator(/);

#undef Vec2ScalarOperator
#undef Vec2Vec2Operator

    inline friend Vec2 operator-(const Vec2& v)
    {
        return { -v.x, -v.y };
    }

    inline friend bool operator==(const Vec2& v1, const Vec2& v2)
    {
        return v1.x == v2.x && v1.y == v2.y;
    }

    inline friend bool operator!=(const Vec2& v1, const Vec2& v2)
    {
        return v1.x != v2.x || v1.y != v2.y;
    }

public:
    inline float Length() const
    {
        return glm::length(GLMVecConst());
    }

    inline float Length2() const
    {
        return x * x + y * y;
    }

    inline Vec2& Normalize()
    {
        GLMVec() = glm::normalize(GLMVecConst());
        return *this;
    }

    inline Vec2 Normal() const
    {
        Vec2 ret;
        ret.GLMVec() = glm::normalize(GLMVecConst());
        return ret;
    }

    // dot product
    inline float Dot(const Vec2& v) const
    {
        return glm::dot(GLMVecConst(), v);
    }

    inline Vec2& Integerize()
    {
        x = (int)x;
        y = (int)y;
        return *this;
    }

    inline Vec2& Floor()
    {
        x = std::floor(x);
        y = std::floor(y);
        return *this;
    }

    inline Vec2& Round()
    {
        x = std::round(x);
        y = std::round(y);
        return *this;
    }

    inline Vec2& Ceil()
    {
        x = std::ceil(x);
        y = std::ceil(y);
        return *this;
    }
};

inline constexpr const Vec2 Vec2::ZERO      = Vec2(0, 0);

inline constexpr const Vec2 Vec2::X_AXIS    = Vec2(1, 0);
inline constexpr const Vec2 Vec2::Y_AXIS    = Vec2(0, 1);

inline constexpr const Vec2 Vec2::UP        = Vec2(0, 1);
inline constexpr const Vec2 Vec2::DOWN      = Vec2(0, -1);

inline constexpr const Vec2 Vec2::LEFT      = Vec2(-1, 0);
inline constexpr const Vec2 Vec2::RIGHT     = Vec2(1, 0);


// check p insides p0-p1-p2
inline bool Point2DInTriangle(const Vec2& p, const Vec2& p0, const Vec2& p1, const Vec2& p2)
{
    auto s = (p0.x - p2.x) * (p.y - p2.y) - (p0.y - p2.y) * (p.x - p2.x);
    auto t = (p1.x - p0.x) * (p.y - p0.y) - (p1.y - p0.y) * (p.x - p0.x);

    if ((s < 0) != (t < 0) && s != 0 && t != 0)
        return false;

    auto d = (p2.x - p1.x) * (p.y - p1.y) - (p2.y - p1.y) * (p.x - p1.x);

    return d == 0 || (d < 0) == (s + t <= 0);
}


class Vec3 : glm::vec3
{
private:
    friend class Mat3;
    friend class Vec2;
    friend class Mat4;
    friend class Vec4;
    friend class Quaternion;
    using Base = glm::vec3;

public:
    const static Vec3 ZERO;

    // Vec3(1, 0, 0)
    const static Vec3 X_AXIS;
    // Vec3(0, 1, 0)
    const static Vec3 Y_AXIS;
    // Vec3(0, 0, 1)
    const static Vec3 Z_AXIS;

    const static Vec3 UP;
    const static Vec3 DOWN;

    const static Vec3 FORWARD;
    const static Vec3 BACK;

    const static Vec3 LEFT;
    const static Vec3 RIGHT;

private:
    inline Vec3(const glm::vec3& v)
    {
        GLMVec() = v;
    }

    inline glm::vec3& GLMVec()
    {
        return reinterpret_cast<glm::vec3&>(*this);
    }

    inline const glm::vec3& GLMVecConst() const
    {
        return reinterpret_cast<const glm::vec3&>(*this);
    }

public:
    using Base::Base;

    Vec3(const Vec2& v, float w) : Base(v, w) {}

    using Base::operator[];
    using Base::x;
    using Base::y;
    using Base::z;

#define Vec3ScalarOperator(opt)                             \
    template <typename T>                                   \
    inline Vec3& operator##opt##=(T scalar)                 \
    {                                                       \
        x opt##= static_cast<float>(scalar);                \
        y opt##= static_cast<float>(scalar);                \
        z opt##= static_cast<float>(scalar);                \
        return *this;                                       \
    }                                                       \
    template <typename T>                                   \
    inline Vec3 operator##opt##(T scalar) const             \
    {                                                       \
        Vec3 ret;                                           \
        ret.x = x opt static_cast<float>(scalar);           \
        ret.y = y opt static_cast<float>(scalar);           \
        ret.z = z opt static_cast<float>(scalar);           \
        return ret;                                         \
    }                                                       \
    template<typename T>                                    \
    friend Vec3 operator##opt##(T scalar, const Vec3& vec)  \
    {                                                       \
        Vec3 ret;                                           \
        ret.x = static_cast<float>(scalar) opt vec.x;       \
        ret.y = static_cast<float>(scalar) opt vec.y;       \
        ret.z = static_cast<float>(scalar) opt vec.z;       \
        return ret;                                         \
    }

#define Vec3Vec3Operator(opt)                           \
    inline Vec3& operator##opt##=(const Vec3& v)        \
    {                                                   \
        x opt##= v.x;                                   \
        y opt##= v.y;                                   \
        z opt##= v.z;                                   \
        return *this;                                   \
    }                                                   \
    inline Vec3 operator##opt##(const Vec3& v) const    \
    {                                                   \
        Vec3 ret;                                       \
        ret.x = x opt v.x;                              \
        ret.y = y opt v.y;                              \
        ret.z = z opt v.z;                              \
        return ret;                                     \
    }

    Vec3ScalarOperator(+);
    Vec3ScalarOperator(-);
    Vec3ScalarOperator(*);
    Vec3ScalarOperator(/);

    Vec3Vec3Operator(+);
    Vec3Vec3Operator(-);
    Vec3Vec3Operator(*);
    Vec3Vec3Operator(/);

#undef Vec3ScalarOperator
#undef Vec3Vec3Operator

    inline friend Vec3 operator-(const Vec3& v)
    {
        return { -v.x, -v.y, -v.z };
    }

    inline friend bool operator==(const Vec3& v1, const Vec3& v2)
    {
        return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z;
    }

    inline friend bool operator!=(const Vec3& v1, const Vec3& v2)
    {
        return v1.x != v2.x || v1.y != v2.y || v1.z != v2.z;
    }

public:
    inline float Length() const
    {
        return glm::length(GLMVecConst());
    }

    inline float Length2() const
    {
        return x * x + y * y + z * z;
    }

    inline Vec3& Normalize()
    {
        GLMVec() = glm::normalize(GLMVecConst());
        return *this;
    }

    inline Vec3 Normal() const
    {
        Vec3 ret;
        ret.GLMVec() = glm::normalize(GLMVecConst());
        return ret;
    }

    // dot product
    inline float Dot(const Vec3& v) const
    {
        return glm::dot(GLMVecConst(), v);
    }

    // cross product
    inline Vec3 Cross(const Vec3& v) const
    {
        Vec3 ret;
        ret.GLMVec() = glm::cross(GLMVecConst(), v);
        return ret;
    }

    // take xy component as Vec2
    inline Vec2& xy() const
    {
        return *((Vec2*)this);
    }

};

inline constexpr const Vec3 Vec3::ZERO     = Vec3(0, 0, 0);

// Vec3(1, 0, 0)
inline constexpr const Vec3 Vec3::X_AXIS   = Vec3(1, 0, 0);
// Vec3(0, 1, 0)
inline constexpr const Vec3 Vec3::Y_AXIS   = Vec3(0, 1, 0);
// Vec3(0, 0, 1)
inline constexpr const Vec3 Vec3::Z_AXIS   = Vec3(0, 0, 1);

inline constexpr const Vec3 Vec3::UP       = Vec3(0, 1, 0);
inline constexpr const Vec3 Vec3::DOWN     = Vec3(0, -1, 0);

inline constexpr const Vec3 Vec3::LEFT     = Vec3(-1, 0, 0);
inline constexpr const Vec3 Vec3::RIGHT    = Vec3(1, 0, 0);

inline constexpr const Vec3 Vec3::FORWARD  = Vec3(0, 0, 1);
inline constexpr const Vec3 Vec3::BACK     = Vec3(0, 0, -1);



class Vec4 : glm::vec4
{
private:
    friend class Mat4;
    friend Vec4 operator*(const Vec4& vec, const Mat4& mat);

    using Base = glm::vec4;

    inline glm::vec4& GLMVec()
    {
        return reinterpret_cast<glm::vec4&>(*this);
    }

    inline const glm::vec4& GLMVecConst() const
    {
        return reinterpret_cast<const glm::vec4&>(*this);
    }

public:
    Vec4() : Base() {};
    Vec4(float x, float y, float z, float w) : Base(x, y, z, w) {};
    Vec4(const Vec3& v, float w) : Base(v, w) {}

public:
#define Vec4ScalarOperator(opt)                             \
    template <typename T>                                   \
    inline Vec4& operator##opt##=(T scalar)                 \
    {                                                       \
        x opt##= static_cast<float>(scalar);                \
        y opt##= static_cast<float>(scalar);                \
        z opt##= static_cast<float>(scalar);                \
        w opt##= static_cast<float>(scalar);                \
        return *this;                                       \
    }                                                       \
    template <typename T>                                   \
    inline Vec4 operator##opt##(T scalar) const             \
    {                                                       \
        Vec4 ret;                                           \
        ret.x = x opt static_cast<float>(scalar);           \
        ret.y = y opt static_cast<float>(scalar);           \
        ret.z = z opt static_cast<float>(scalar);           \
        ret.w = w opt static_cast<float>(scalar);           \
        return ret;                                         \
    }                                                       \
    template<typename T>                                    \
    friend Vec4 operator##opt##(T scalar, const Vec4& vec)  \
    {                                                       \
        Vec4 ret;                                           \
        ret.x = static_cast<float>(scalar) opt vec.x;       \
        ret.y = static_cast<float>(scalar) opt vec.y;       \
        ret.z = static_cast<float>(scalar) opt vec.z;       \
        ret.w = static_cast<float>(scalar) opt vec.w;       \
        return ret;                                         \
    }

#define Vec4Vec4Operator(opt)                           \
    inline Vec4& operator##opt##=(const Vec4& v)        \
    {                                                   \
        x opt##= v.x;                                   \
        y opt##= v.y;                                   \
        z opt##= v.z;                                   \
        w opt##= v.w;                                   \
        return *this;                                   \
    }                                                   \
    inline Vec4 operator##opt##(const Vec4& v) const    \
    {                                                   \
        Vec4 ret;                                       \
        ret.x = x opt v.x;                              \
        ret.y = y opt v.y;                              \
        ret.z = z opt v.z;                              \
        ret.w = w opt v.w;                              \
        return ret;                                     \
    }

    Vec4ScalarOperator(+);
    Vec4ScalarOperator(-);
    Vec4ScalarOperator(*);
    Vec4ScalarOperator(/);

    Vec4Vec4Operator(+);
    Vec4Vec4Operator(-);
    Vec4Vec4Operator(*);
    Vec4Vec4Operator(/);

#undef Vec4ScalarOperator
#undef Vec4Vec4Operator

    inline friend Vec4 operator-(const Vec4& v)
    {
        return { -v.x, -v.y, -v.z, -v.w };
    }

    inline friend bool operator==(const Vec4& v1, const Vec4& v2)
    {
        return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z && v1.w == v2.w;
    }

    inline friend bool operator!=(const Vec4& v1, const Vec4& v2)
    {
        return v1.x != v2.x || v1.y != v2.y || v1.z != v2.z || v1.w != v2.w;
    }

public:
    //using Base::Base;
    using Base::operator[];
    using Base::x;
    using Base::y;
    using Base::z;
    using Base::w;

    // take xyz component as Vec3
    inline Vec3& xyz() const
    {
        return *((Vec3*)this);
    }

    // take xyz component as Vec2
    inline Vec2& xy() const
    {
        return *((Vec2*)this);
    }

public:
    inline float Length() const
    {
        return glm::length(GLMVecConst());
    }

    inline float Length2() const
    {
        return x * x + y * y + z * z + w * w;
    }

    inline Vec4& Normalize()
    {
        GLMVec() = glm::normalize(GLMVecConst());
        return *this;
    }

    inline Vec4 Normal() const
    {
        Vec4 ret;
        ret.GLMVec() = glm::normalize(GLMVecConst());
        return ret;
    }

    // dot product
    inline float Dot(const Vec4& v) const
    {
        return glm::dot(GLMVecConst(), v);
    }
};

class Mat4;

class Quaternion : glm::quat
{
private:
    friend class Mat4;
    using Base = glm::quat;

    inline glm::quat& GLMQuat()
    {
        return reinterpret_cast<glm::quat&>(*this);
    }

    inline const glm::quat& GLMQuatConst() const
    {
        return reinterpret_cast<const glm::quat&>(*this);
    }

    Quaternion(const glm::quat& glmQuat)
    {
        GLMQuat() = glmQuat;
    };

public:
    using Base::operator[];
    using Base::x;
    using Base::y;
    using Base::z;
    using Base::w;

    using Base::Base;

    Quaternion() 
    {
        x = 0, y = 0, z = 0, w = 1;
    };

    Quaternion(const Vec3& eulerAngles) 
    {
        GLMQuat() = glm::quat(eulerAngles.GLMVecConst());
    };

    Quaternion(const Mat4& mat4);

    inline Mat4 ToMat4() const;

    inline Vec3 ToEulerAngles() const
    {
        return glm::eulerAngles(GLMQuatConst());
    }

    inline auto& Normalize()
    {
        GLMQuat() = glm::normalize(GLMQuat());
        return *this;
    }

    inline auto Normal()
    {
        Quaternion ret;
        ret.GLMQuat() = glm::normalize(GLMQuat());
        return ret;
    }

    inline friend bool operator==(const Quaternion& v1, const Quaternion& v2)
    {
        return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z && v1.w == v2.w;
    }

    inline void SetFromMat4(const Mat4& mat);

    //return Quaternion that is shortest rotation {from->to}
    //source from https://github.com/toji/gl-matrix/blob/f0583ef53e94bc7e78b78c8a24f09ed5e2f7a20c/src/gl-matrix/quat.js#L54 line 54
    inline static Quaternion RotationFromTo(const Vec3& from, const Vec3& to);

    /*inline static Quaternion RotationFromTo(const Quaternion& from, const Quaternion& to)
    {

    }*/

    friend inline Quaternion SLerp(const Quaternion& q1, const Quaternion& q2, float t)
    {
        return glm::slerp(q1.GLMQuatConst(), q2.GLMQuatConst(), t);
    }
};

class Mat3 : glm::mat3
{
private:
    friend class Quaternion;
    friend class Vec3;

    using Base = glm::mat3;

    /*constexpr operator glm::mat4&()
    {
        return reinterpret_cast<glm::mat4&>(*this);
    }*/

    inline glm::mat3& GLMMat()
    {
        return reinterpret_cast<glm::mat3&>(*this);
    }

    inline const glm::mat3& GLMMatConst() const
    {
        return reinterpret_cast<const glm::mat3&>(*this);
    }

    inline void operator=(const glm::mat3& mat)
    {
        reinterpret_cast<glm::mat3&>(*this) = mat;
    }

    Mat3(const glm::mat3& mat)
    {
        *this = mat;
    };

public:
    using Base::Base;

    inline Vec3& operator[](size_t rowId) const
    {
        return *((Vec3*)this + rowId);
    }

    // row-major *=
    inline Mat3& operator*=(const Mat3& mat)
    {
        GLMMat() = mat.GLMMatConst() * GLMMat();
        return *this;
    }

    inline Mat3 operator*(const Mat3& mat) const
    {
        Mat3 ret;
        ret.GLMMat() = mat.GLMMatConst() * GLMMatConst();
        return ret;
    }

    /*inline Vec4 operator*(const Vec4& vec)
    {
        Vec4 ret;
        ret.GLMVec() = GLMMatConst() * vec;
        return ret;
    }*/

    inline friend Vec3 operator*(const Vec3& vec, const Mat3& mat)
    {
        auto& m = mat.GLMMatConst();
        auto& v = vec;

        Vec3 ret = {
            m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2],
            m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2],
            m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2],
        };

        return ret;
    }

    inline Mat3& operator+=(const Mat3& mat)
    {
        GLMMat() += mat.GLMMatConst();
        return *this;
    }

    inline Mat3 operator+(const Mat3& mat)
    {
        auto ret = GLMMat() + mat.GLMMatConst();
        return ret;
    }

    inline Mat3 operator-(const Mat3& mat)
    {
        auto ret = GLMMat() - mat.GLMMatConst();
        return ret;
    }

    inline Mat3& operator-=(const Mat3& mat)
    {
        GLMMat() -= mat.GLMMatConst();
        return *this;
    }

    inline Mat3 operator*(const float& scala)
    {
        auto ret = GLMMat() * scala;
        return ret;
    }

    inline Mat3 operator*=(const float& scala)
    {
        GLMMat() *= scala;
        return *this;
    }

    inline Mat3 operator/(const float& scala)
    {
        auto ret = GLMMat() / scala;
        return ret;
    }

    inline Mat3 operator/=(const float& scala)
    {
        GLMMat() /= scala;
        return *this;
    }

public:
    inline Mat3& SetIdentity()
    {
        GLMMat() = glm::identity<glm::mat3>();
        return *this;
    }

    inline Mat3& SetTranslation(const Vec2& vec)
    {
        SetIdentity();
        (*this)[2] = Vec3(vec, 1.0f);
        return *this;
    }

    inline Mat3& SetTranslation(float x, float y)
    {
        SetTranslation({ x,y });
        return *this;
    }

    inline Mat3& SetPosition(const Vec2& vec)
    {
        auto& self = GLMMat();
        self[2][0] = vec.x;
        self[2][1] = vec.y;
        return *this;
    }

    inline Mat3& SetPosition(float x, float y)
    {
        auto& self = GLMMat();
        self[2][0] = x;
        self[2][1] = y;
        return *this;
    }

    inline Mat3& SetRotation(float angle)
    {
        GLMMat() = glm::rotate(glm::identity<glm::mat3>(), angle);
        return *this;
    }

    inline Mat3& SetScale(const Vec2& vec)
    {
        GLMMat() = glm::scale(glm::identity<glm::mat3>(), vec);
        return *this;
    }

    inline Mat3& SetScale(float x, float y)
    {
        GLMMat() = glm::scale(glm::identity<glm::mat3>(), { x, y });
        return *this;
    }

    inline Mat3& Transpose()
    {
        GLMMat() = glm::transpose(GLMMat());
        return *this;
    }

    inline Mat3 GetTranspose() const
    {
        Mat3 ret = *this;
        return ret.Transpose();
    }

    inline Mat3& Inverse()
    {
        GLMMat() = glm::inverse(GLMMat());
        return *this;
    }

    inline Mat3 GetInverse() const
    {
        Mat3 ret = *this;
        return ret.Inverse();
    }

public:
    inline static Mat3 Identity()
    {
        return Mat3().SetIdentity();
    }

    inline static Mat3 Transpose(const Mat3& mat)
    {
        return mat.GetTranspose();
    }

    inline static Mat3 Inverse(const Mat3& mat)
    {
        return mat.GetInverse();
    }

    inline static Mat3 Scaling(const Vec2& v)
    {
        return Mat3().SetScale(v);
    }

    inline static Mat3 Scaling(float x, float y)
    {
        return Mat3().SetScale(x, y);
    }

    inline static Mat3 Rotation(float angle)
    {
        return Mat3().SetRotation(angle);
    }

    inline static Mat3 Translation(const Vec2& v)
    {
        return Mat3().SetTranslation(v);
    }

    inline static Mat3 Translation(float x, float y, float z)
    {
        return Mat3().SetTranslation(x, y);
    }

};

class Mat4 : glm::mat4
{
private:
    friend class Quaternion;

    using Base = glm::mat4;

    template <typename T, typename _Scala>
    friend T Lerp(const T& v1, const T& v2, _Scala t);

    /*constexpr operator glm::mat4&()
    {
        return reinterpret_cast<glm::mat4&>(*this);
    }*/

    inline glm::mat4& GLMMat()
    {
        return reinterpret_cast<glm::mat4&>(*this);
    }

    inline const glm::mat4& GLMMatConst() const
    {
        return reinterpret_cast<const glm::mat4&>(*this);
    }

    inline void operator=(const glm::mat4& mat)
    {
        reinterpret_cast<glm::mat4&>(*this) = mat;
    }

    Mat4(const glm::mat4& mat)
    {
        *this = mat;
    };

public:
    using Base::Base;

    Mat4()
    {
        SetIdentity();
    }

    inline Vec4& operator[](size_t rowId) const
    {
        return *((Vec4*)this + rowId);
    }

    // row-major *=
    inline Mat4& operator*=(const Mat4& mat)
    {
        GLMMat() = mat.GLMMatConst() * GLMMat();
        return *this;
    }

    inline Mat4 operator*(const Mat4& mat) const
    {
        Mat4 ret;
        ret.GLMMat() = mat.GLMMatConst() * GLMMatConst();
        return ret;
    }

    /*inline Vec4 operator*(const Vec4& vec)
    {
        Vec4 ret;
        ret.GLMVec() = GLMMatConst() * vec;
        return ret;
    }*/

    inline friend Vec4 operator*(const Vec4& vec, const Mat4& mat)
    {
        auto& m = mat.GLMMatConst();
        auto& v = vec.GLMVecConst();

        Vec4 ret = { 
            m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2] + m[3][0] * v[3],
            m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2] + m[3][1] * v[3],
            m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2] + m[3][2] * v[3],
            m[0][3] * v[0] + m[1][3] * v[1] + m[2][3] * v[2] + m[3][3] * v[3] 
        };

        return ret;
    }

    inline Mat4& operator+=(const Mat4& mat)
    {
        GLMMat() += mat.GLMMatConst();
        return *this;
    }

    inline Mat4 operator+(const Mat4& mat)
    {
        auto ret = GLMMat() + mat.GLMMatConst();
        return ret;
    }

    inline Mat4 operator-(const Mat4& mat)
    {
        auto ret = GLMMat() - mat.GLMMatConst();
        return ret;
    }

    inline Mat4& operator-=(const Mat4& mat)
    {
        GLMMat() -= mat.GLMMatConst();
        return *this;
    }

    inline Mat4 operator*(const float& scala)
    {
        auto ret = GLMMat() * scala;
        return ret;
    }

    inline Mat4 operator*=(const float& scala)
    {
        GLMMat() *= scala;
        return *this;
    }

public:
    inline Mat4& SetIdentity()
    {
        GLMMat() = glm::identity<glm::mat4>();
        return *this;
    }

    inline Mat4& SetTranslation(const Vec3& vec)
    {
        GLMMat() = glm::translate(glm::identity<glm::mat4>(), vec);
        return *this;
    }

    inline Mat4& SetTranslation(float x, float y, float z)
    {
        GLMMat() = glm::translate(glm::identity<glm::mat4>(), { x, y, z });
        return *this;
    }

    inline Mat4& SetPosition(const Vec3& vec)
    {
        auto& self = GLMMat();
        self[3][0] = vec.x;
        self[3][1] = vec.y;
        self[3][2] = vec.z;
        return *this;
    }

    inline Mat4& SetPosition(float x, float y, float z)
    {
        auto& self = GLMMat();
        self[3][0] = x;
        self[3][1] = y;
        self[3][2] = z;
        return *this;
    }

    inline Mat4& SetRotation(const Quaternion& quaternion)
    {
        GLMMat() = glm::toMat4(quaternion.GLMQuatConst());
        return *this;
    }

    inline Mat4& SetRotation(const Vec3& axis, float angle)
    {
        GLMMat() = glm::rotate(glm::identity<glm::mat4>(), angle, axis);
        return *this;
    }

    inline Mat4& SetScale(const Vec3& vec)
    {
        GLMMat() = glm::scale(glm::identity<glm::mat4>(), vec);
        return *this;
    }

    inline Mat4& SetScale(float x, float y, float z)
    {
        GLMMat() = glm::scale(glm::identity<glm::mat4>(), { x, y, z });
        return *this;
    }

    inline Mat4& Transpose()
    {
        GLMMat() = glm::transpose(GLMMat());
        return *this;
    }

    inline Mat4 GetTranspose() const
    {
        Mat4 ret = *this;
        return ret.Transpose();
    }

    inline Mat4& Inverse()
    {
        GLMMat() = glm::inverse(GLMMat());
        return *this;
    }

    inline Mat4 GetInverse() const
    {
        Mat4 ret = *this;
        return ret.Inverse();
    }

    inline Mat4& SetLookAtLH(const Vec3& position, const Vec3& focusPos, const Vec3& up)
    {
        GLMMat() = glm::lookAtLH(position, focusPos, up);
        return *this;
    }

    inline Mat4& SetPerspectiveFovLH(float fovAngleY, float aspectRatio, float dNear, float dFar)
    {
        /*float sinFov = std::sin(0.5f * fovAngleY);
        float cosFov = std::cos(0.5f * fovAngleY);

        float height = cosFov / sinFov;
        float width = height / aspectRatio;
        float fRange = dFar / (dFar - dNear);

        float (*m)[4] = (float (*)[4])this;
        m[0][0] = width;
        m[0][1] = 0.0f;
        m[0][2] = 0.0f;
        m[0][3] = 0.0f;

        m[1][0] = 0.0f;
        m[1][1] = height;
        m[1][2] = 0.0f;
        m[1][3] = 0.0f;

        m[2][0] = 0.0f;
        m[2][1] = 0.0f;
        m[2][2] = fRange;
        m[2][3] = 1.0f;

        m[3][0] = 0.0f;
        m[3][1] = 0.0f;
        m[3][2] = -fRange * dNear;
        m[3][3] = 0.0f;*/

        reinterpret_cast<glm::mat4&>(*this) = glm::perspectiveLH_ZO(fovAngleY, aspectRatio, dNear, dFar);
        return *this;
    }

    inline Mat4& SetOrthographicLH(float viewWidth, float viewHeight, float nearZ, float farZ)
    {
        auto& m = GLMMat();
        float fRange = 1.0f / (farZ - nearZ);

        m[0][0] = 2.0f / viewWidth;
        m[0][1] = 0.0f;
        m[0][2] = 0.0f;
        m[0][3] = 0.0f;

        m[1][0] = 0.0f;
        m[1][1] = 2.0f / viewHeight;
        m[1][2] = 0.0f;
        m[1][3] = 0.0f;

        m[2][0] = 0.0f;
        m[2][1] = 0.0f;
        m[2][2] = fRange;
        m[2][3] = 0.0f;

        m[3][0] = 0.0f;
        m[3][1] = 0.0f;
        m[3][2] = -fRange * nearZ;
        m[3][3] = 1.0f;

        return *this;
    }

    inline bool Decompose(Vec3& scaling, Quaternion& rotation, Vec3& translation) const
    {
        glm::vec3 skew;
        glm::vec4 perspective;
        if (glm::decompose(GLMMatConst(), scaling, rotation.GLMQuat(), translation, skew, perspective))
        {
            //rotation.GLMQuat() = glm::conjugate(rotation.GLMQuat());
            return true;
        }
        return false;
    }

public:
    // get forward direction of this transform matrix
    inline const Vec3& Forward() const
    {
        return (*this)[2].xyz();
    }

    // get up direction of this transform matrix
    inline const Vec3& Up() const
    {
        return (*this)[1].xyz();
    }

    // get right direction of this transform matrix
    inline const Vec3& Right() const
    {
        return (*this)[0].xyz();
    }

    // accesser for position component
    inline Vec3& Position() const
    {
        return (*this)[3].xyz();
    }

public:
    inline static Mat4 Identity()
    {
        return Mat4().SetIdentity();
    }

    inline static Mat4 Transpose(const Mat4& mat)
    {
        return mat.GetTranspose();
    }

    inline static Mat4 Inverse(const Mat4& mat)
    {
        return mat.GetInverse();
    }

    inline static Mat4 Scaling(const Vec3& v)
    {
        return Mat4().SetScale(v);
    }

    inline static Mat4 Scaling(float x, float y, float z)
    {
        return Mat4().SetScale(x, y, z);
    }

    inline static Mat4 Rotation(const Quaternion& quaternion)
    {
        return Mat4().SetRotation(quaternion);
    }

    inline static Mat4 Rotation(const Vec3& axis, float angle)
    {
        return Mat4().SetRotation(axis, angle);
    }

    inline static Mat4 Translation(const Vec3& v)
    {
        return Mat4().SetTranslation(v);
    }

    inline static Mat4 Translation(float x, float y, float z)
    {
        return Mat4().SetTranslation(x, y, z);
    }

};

inline Mat4 Quaternion::ToMat4() const
{
    return Mat4().SetRotation(*this);
}

inline Quaternion::Quaternion(const Mat4& mat4)
{
    GLMQuat() = glm::quat(mat4.GLMMatConst());
    Normalize();
}

inline void Quaternion::SetFromMat4(const Mat4& mat4)
{
    GLMQuat() = glm::quat(mat4.GLMMatConst());
    Normalize();
}

inline Quaternion Quaternion::RotationFromTo(const Vec3& from, const Vec3& to)
{
    Vec3 xUnitVec3 = { 1, 0, 0 };
    Vec3 yUnitVec3 = { 0, 1, 0 };
    float dot = from.Dot(to);

    if (dot < -0.999999) {
        Vec3 temp = xUnitVec3.Cross(from);
        if (temp.Length() < 0.000001)
            temp = yUnitVec3.Cross(from);

        temp.Length();
        //quat.setAxisAngle(out, tmpvec3, Math.PI);
        return Mat4::Rotation(temp, PI);
    }
    else if (dot > 0.999999) {
        return { 1,0,0,0 };
    }
    else {
        Quaternion re;
        Vec3 temp = from.Cross(to);
        re.x = temp.x;
        re.y = temp.y;
        re.z = temp.z;
        re.w = 1 + dot;
        return re.Normalize();
    }
}

using Vector3 = Vec3;
using Matrix4x4 = Mat4;

inline float ToRadians(float degrees)
{
    return glm::radians(degrees);
}

inline float ToDegrees(float radians)
{
    return glm::degrees(radians);
}


template <typename T, typename _Scala>
inline T Lerp(const T& v1, const T& v2, _Scala t)
{
    return v1 + (v2 - v1) * t;
}

}