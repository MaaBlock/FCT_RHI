//
// Created by Administrator on 2025/4/15.
//

#ifndef VEC_H
#define VEC_H
#include "../ThirdParty.h"
namespace FCT {
    struct Vec2
    {
        float x, y;
        Vec2(int x, int y) {
            this->x = static_cast<float>(x);
            this->y = static_cast<float>(y);
        }
        Vec2(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}
        Vec2(uint32_t x, uint32_t y)
        {
            this->x = static_cast<float>(x);
            this->y = static_cast<float>(y);
        }

        Vec2 &operator+=(const Vec2 &rhs)
        {
            x += rhs.x;
            y += rhs.y;
            return *this;
        }
        Vec2 operator+(const Vec2 &rhs) const
        {
            return Vec2(x + rhs.x, y + rhs.y);
        }
        Vec2 operator-(const Vec2 &rhs) const
        {
            return Vec2(x - rhs.x, y - rhs.y);
        }
        Vec2 operator*(const Vec2 &rhs) const
        {
            return Vec2(x * rhs.x, y * rhs.y);
        }
        Vec2 operator*(float scalar) const
        {
            return Vec2(x * scalar, y * scalar);
        }
        Vec2 operator/(float scalar) const
        {
            return Vec2(x / scalar, y / scalar);
        }
        Vec2 operator/(const Vec2 &rhs) const
        {
            return Vec2(x / rhs.x, y / rhs.y);
        }
        bool operator==(const Vec2& other) const
        {
            return (x == other.x) && (y == other.y);
        }
        float length() const
        {
            return std::sqrt(x * x + y * y);
        }
        float distance(const Vec2 &other) const
        {
            return std::sqrt((x - other.x) * (x - other.x) + (y - other.y) * (y - other.y));
        }
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & x;
            ar & y;
        }
    };

    template<typename Real = float>
    struct Vec3
    {
#ifdef FCT_USE_PHYSX
        operator physx::PxExtendedVec3() const {
            return physx::PxExtendedVec3(x, y, z);
        }
        operator physx::PxVec3() const {
            return physx::PxVec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
        }
        Vec3(const physx::PxVec3& other) {
            x = static_cast<Real>(other.x);
            y = static_cast<Real>(other.y);
            z = static_cast<Real>(other.z);
        }
#endif
        Real x, y, z;
        constexpr Vec3(Real x = static_cast<Real>(0), Real y = static_cast<Real>(0), Real z = static_cast<Real>(0))
            : x(x), y(y), z(z) {}

        constexpr Vec3(const Vec3 &other)
            : x(other.x), y(other.y), z(other.z)
        {
        }

        template<typename OtherReal>
        constexpr Vec3(const Vec3<OtherReal> &other)
            : x(static_cast<Real>(other.x)), y(static_cast<Real>(other.y)), z(static_cast<Real>(other.z))
        {
        }

        Vec3 operator-() const
        {
            return {-x, -y, -z};
        }

        bool operator==(const Vec3 &other) const
        {
            return (x == other.x) && (y == other.y) && (z == other.z);
        }

        Vec3 &operator+=(const Vec3 &rhs)
        {
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            return *this;
        }
        bool operator<(const Vec3 &other) const
        {
            return (x < other.x) && (y < other.y) && (z < other.z);
        }

        Real lengthSquared() const
        {
            return x * x + y * y + z * z;
        }
        Vec3 &operator-=(const Vec3 &rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            return *this;
        }
        Vec3 operator+(const Vec3 &rhs) const
        {
            return Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
        }

        Vec3 operator-(const Vec3 &rhs) const
        {
            return Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
        }
        Vec3 operator*(Real scalar) const
        {
            return Vec3(x * scalar, y * scalar, z * scalar);
        }
        Real length() const
        {
            return std::sqrt(x * x + y * y + z * z);
        }
        Vec3 normalize() const
        {
            Real len = std::sqrt(x * x + y * y + z * z);
            return Vec3(x / len, y / len, z / len);
        }
        Vec3 cross(const Vec3 &other) const
        {
            return Vec3(
                y * other.z - z * other.y,
                z * other.x - x * other.z,
                x * other.y - y * other.x);
        }
        Vec3 rotate(const Vec3 &axis, Real angle) const
        {
            Real cosTheta = std::cos(angle);
            Real sinTheta = std::sin(angle);
            Vec3 normalizedAxis = axis.normalize();

            return Vec3(
                (cosTheta + (1 - cosTheta) * normalizedAxis.x * normalizedAxis.x) * x +
                    ((1 - cosTheta) * normalizedAxis.x * normalizedAxis.y - sinTheta * normalizedAxis.z) * y +
                    ((1 - cosTheta) * normalizedAxis.x * normalizedAxis.z + sinTheta * normalizedAxis.y) * z,

                ((1 - cosTheta) * normalizedAxis.x * normalizedAxis.y + sinTheta * normalizedAxis.z) * x +
                    (cosTheta + (1 - cosTheta) * normalizedAxis.y * normalizedAxis.y) * y +
                    ((1 - cosTheta) * normalizedAxis.y * normalizedAxis.z - sinTheta * normalizedAxis.x) * z,

                ((1 - cosTheta) * normalizedAxis.x * normalizedAxis.z - sinTheta * normalizedAxis.y) * x +
                    ((1 - cosTheta) * normalizedAxis.y * normalizedAxis.z + sinTheta * normalizedAxis.x) * y +
                    (cosTheta + (1 - cosTheta) * normalizedAxis.z * normalizedAxis.z) * z);
        }
        Real dot(const Vec3& rhs) const {
            return x * rhs.x + y * rhs.y + z * rhs.z;
        }
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & x;
            ar & y;
            ar & z;
        }
    };

    using Vec3f = Vec3<float>;
    using Vec3d = Vec3<double>;

    template<typename Real>
    inline Vec3<Real> normalize(const Vec3<Real> &v)
    {
        Real length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        return Vec3<Real>(v.x / length, v.y / length, v.z / length);
    }

    template<typename Real>
    inline Vec3<Real> cross(const Vec3<Real> &a, const Vec3<Real> &b)
    {
        return Vec3<Real>(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x);
    }

    template<typename Real>
    inline Real dot(const Vec3<Real> &a, const Vec3<Real> &b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    struct Vec4
    {
        float x, y, z, w;
        Vec4(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 0.0f)
            : x(x), y(y), z(z), w(w) {}
        template<typename Real>
        Vec4(const Vec3<Real>& vec, float w = 0.0f)
        : x(static_cast<float>(vec.x)), y(static_cast<float>(vec.y)), z(static_cast<float>(vec.z)), w(w) {}
        Vec4(Vec2 v, float z = 0.0f, float w = 0.0f)
            : x(v.x), y(v.y), z(z), w(w) {}
        Vec4 operator-(const Vec4 &rhs) const
        {
            return Vec4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
        }

        Vec4 operator-() const
        {
            return Vec4(-x, -y, -z, -w);
        }
        Vec4 normalize() const
        {
            float length = std::sqrt(x * x + y * y + z * z + w * w);
            if (length > 0.0f) {
                return Vec4(x / length, y / length, z / length, w / length);
            }
            return Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        }
        Vec3<> xyz() const
        {
            return Vec3<>(x, y, z);
        }
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & x;
            ar & y;
            ar & z;
            ar & w;
        }
    };
    template<typename T>
    struct Vector4
    {
        T x, y, z, w;
        Vector4(T x = 0, T y = 0, T z = 0, T w = 0) : x(x), y(y), z(z), w(w) {}
    };

    using IVec4 = Vector4<int32_t>;
    using UVec4 = Vector4<uint32_t>;

    BOOST_DESCRIBE_STRUCT(Vec2, (), (x, y))
    BOOST_DESCRIBE_STRUCT(Vec3f, (), (x, y, z))
    BOOST_DESCRIBE_STRUCT(Vec3d, (), (x, y, z))
    BOOST_DESCRIBE_STRUCT(Vec4, (), (x, y, z, w))
}
#endif //VEC_H
