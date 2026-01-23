#pragma once
#include <cmath>
#include "iostream"
namespace RBX {

    struct Vector3 final {
        float x, y, z;

        Vector3() noexcept : x(0), y(0), z(0) {}

        Vector3(float x, float y, float z) noexcept : x(x), y(y), z(z) {}

        inline const float& operator[](int i) const noexcept {
            return reinterpret_cast<const float*>(this)[i];
        }

        inline float& operator[](int i) noexcept {
            return reinterpret_cast<float*>(this)[i];
        }

        Vector3 operator/(float s) const noexcept {
            return *this * (1.0f / s);
        }

        float dot(const Vector3& vec) const noexcept {
            return x * vec.x + y * vec.y + z * vec.z;
        }

        float distance(const Vector3& vector) const noexcept {
            return std::sqrt((vector.x - x) * (vector.x - x) +
                (vector.y - y) * (vector.y - y) +
                (vector.z - z) * (vector.z - z));
        }

        Vector3 operator*(float value) const noexcept {
            return { x * value, y * value, z * value };
        }

        bool operator!=(const Vector3& other) const noexcept {
            return x != other.x || y != other.y || z != other.z;
        }

        float squared() const noexcept {
            return x * x + y * y + z * z;
        }

        Vector3 normalize() const noexcept {
            float mag = magnitude();
            if (mag == 0) return { 0, 0, 0 };
            return { x / mag, y / mag, z / mag };
        }

        Vector3 direction() const noexcept {
            return normalize();
        }

        static const Vector3& one() noexcept {
            static const Vector3 v(1, 1, 1);
            return v;
        }

        static const Vector3& unitX() noexcept {
            static const Vector3 v(1, 0, 0);
            return v;
        }

        static const Vector3& unitY() noexcept {
            static const Vector3 v(0, 1, 0);
            return v;
        }

        static const Vector3& unitZ() noexcept {
            static const Vector3 v(0, 0, 1);
            return v;
        }

        Vector3 cross(const Vector3& b) const noexcept {
            return { y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x };
        }

        Vector3 operator+(const Vector3& vec) const noexcept {
            return { x + vec.x, y + vec.y, z + vec.z };
        }

        Vector3 operator-(const Vector3& vec) const noexcept {
            return { x - vec.x, y - vec.y, z - vec.z };
        }

        Vector3 operator*(const Vector3& vec) const noexcept {
            return { x * vec.x, y * vec.y, z * vec.z };
        }

        Vector3 operator/(const Vector3& vec) const noexcept {
            return { x / vec.x, y / vec.y, z / vec.z };
        }

        Vector3& operator+=(const Vector3& vec) noexcept {
            x += vec.x;
            y += vec.y;
            z += vec.z;
            return *this;
        }

        Vector3& operator-=(const Vector3& vec) noexcept {
            x -= vec.x;
            y -= vec.y;
            z -= vec.z;
            return *this;
        }

        Vector3& operator*=(float fScalar) noexcept {
            x *= fScalar;
            y *= fScalar;
            z *= fScalar;
            return *this;
        }

        Vector3& operator/=(const Vector3& other) noexcept {
            x /= other.x;
            y /= other.y;
            z /= other.z;
            return *this;
        }

        bool operator==(const Vector3& other) const noexcept {
            return x == other.x && y == other.y && z == other.z;
        }

        float magnitude() const noexcept {
            return std::sqrt(x * x + y * y + z * z);
        }
    };

}

namespace RBX {
    struct Vector4 final { float x, y, z, w; };
}
#include <algorithm>
namespace RBX {

    struct Vector2 final {
        float x, y;

        Vector2 operator-(const Vector2& other) const noexcept {
            return { x - other.x, y - other.y };
        }

        Vector2 operator+(const Vector2& other) const noexcept {
            return { x + other.x, y + other.y };
        }

        Vector2 operator/(float factor) const noexcept {
            return { x / factor, y / factor };
        }

        Vector2 operator/(const Vector2& factor) const noexcept {
            return { x / factor.x, y / factor.y };
        }

        Vector2 operator*(float factor) const noexcept {
            return { x * factor, y * factor };
        }

        Vector2 operator*(const Vector2& factor) const noexcept {
            return { x * factor.x, y * factor.y };
        }

        float getMagnitude() const noexcept {
            return std::sqrt(x * x + y * y);
        }
    };

}

namespace RBX {
    struct Matrix4x4 final { float data[16]; };
}

#define M_PI 3.14159265358979323846
#include <string>
#include <vector>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <queue>
namespace RBX {

    struct Matrix3x3 final {
        float data[9];

        Vector3 MatrixToEulerAngles() const {
            Vector3 angles;

            angles.y = std::asin(std::clamp(data[6], -1.0f, 1.0f));

            if (std::abs(data[6]) < 0.9999f) {
                angles.x = std::atan2(-data[7], data[8]);
                angles.z = std::atan2(-data[3], data[0]);
            }
            else {
                angles.x = 0.0f;
                angles.z = std::atan2(data[1], data[4]);
            }

            angles.x = angles.x * (180.0f / M_PI);
            angles.y = angles.y * (180.0f / M_PI);
            angles.z = angles.z * (180.0f / M_PI);

            return angles;
        }

        Matrix3x3 EulerAnglesToMatrix(const Vector3& angles) const {
            Matrix3x3 rotationMatrix;

            float pitch = angles.x * (M_PI / 180.0f);
            float yaw = angles.y * (M_PI / 180.0f);
            float roll = angles.z * (M_PI / 180.0f);

            float cosPitch = std::cos(pitch), sinPitch = std::sin(pitch);
            float cosYaw = std::cos(yaw), sinYaw = std::sin(yaw);
            float cosRoll = std::cos(roll), sinRoll = std::sin(roll);

            rotationMatrix.data[0] = cosYaw * cosRoll;
            rotationMatrix.data[1] = cosYaw * sinRoll;
            rotationMatrix.data[2] = -sinYaw;

            rotationMatrix.data[3] = sinPitch * sinYaw * cosRoll - cosPitch * sinRoll;
            rotationMatrix.data[4] = sinPitch * sinYaw * sinRoll + cosPitch * cosRoll;
            rotationMatrix.data[5] = sinPitch * cosYaw;

            rotationMatrix.data[6] = cosPitch * sinYaw * cosRoll + sinPitch * sinRoll;
            rotationMatrix.data[7] = cosPitch * sinYaw * sinRoll - sinPitch * cosRoll;
            rotationMatrix.data[8] = cosPitch * cosYaw;

            return rotationMatrix;
        }

        Vector3 GetForwardVector() const { return { data[2], data[5], data[8] }; }
        Vector3 GetRightVector() const { return { data[0], data[3], data[6] }; }
        Vector3 GetUpVector() const { return { data[1], data[4], data[7] }; }

        Matrix3x3 Transpose() const {
            Matrix3x3 result;
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    result.data[i + j * 3] = data[j + i * 3];
            return result;
        }

        Vector3 multiplyVector(const Vector3& vec) const {
            return {
                data[0] * vec.x + data[1] * vec.y + data[2] * vec.z,
                data[3] * vec.x + data[4] * vec.y + data[5] * vec.z,
                data[6] * vec.x + data[7] * vec.y + data[8] * vec.z
            };
        }

        Matrix3x3 operator*(const Matrix3x3& other) const {
            Matrix3x3 result;
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    result.data[i + j * 3] = data[i * 3 + 0] * other.data[0 + j] +
                    data[i * 3 + 1] * other.data[1 + j] +
                    data[i * 3 + 2] * other.data[2 + j];
            return result;
        }

        Matrix3x3 operator+(const Matrix3x3& other) const {
            Matrix3x3 result;
            for (int i = 0; i < 9; ++i)
                result.data[i] = data[i] + other.data[i];
            return result;
        }

        Matrix3x3 operator-(const Matrix3x3& other) const {
            Matrix3x3 result;
            for (int i = 0; i < 9; ++i)
                result.data[i] = data[i] - other.data[i];
            return result;
        }

        Matrix3x3 operator/(float scalar) const {
            Matrix3x3 result;
            for (int i = 0; i < 9; ++i)
                result.data[i] = data[i] / scalar;
            return result;
        }

        Vector3 getColumn(int index) const { return { data[index], data[index + 3], data[index + 6] }; }
    };

    static Vector3 lookvec(const Matrix3x3& rotationMatrix) { return rotationMatrix.getColumn(2); }
    static Vector3 rightvec(const Matrix3x3& rotationMatrix) { return rotationMatrix.getColumn(0); }

}


namespace RBX {

    struct CFrame {
        Vector3 right_vector = { 1, 0, 0 };
        Vector3 up_vector = { 0, 1, 0 };
        Vector3 back_vector = { 0, 0, 1 };
        Vector3 position = { 0, 0, 0 };

        CFrame() = default;

        CFrame(Vector3 position) : position{ position } {}

        CFrame(Vector3 right_vector, Vector3 up_vector, Vector3 back_vector, Vector3 position)
            : right_vector{ right_vector }, up_vector{ up_vector }, back_vector{ back_vector }, position{ position } {
        }

        void look_at_locked(Vector3 point) noexcept {

            Vector3 look_vector = (position - point).normalize();

            right_vector = Vector3{ 0, 1, 0 }.cross(look_vector).normalize();

            up_vector = look_vector.cross(right_vector).normalize();

            back_vector = look_vector * Vector3{ -1, -1, -1 };
        }

        CFrame look_at(Vector3 point) noexcept {

            Vector3 look_vector = (position - point).normalize();

            Vector3 right_vector = Vector3{ 0, 1, 0 }.cross(look_vector).normalize();
            Vector3 up_vector = look_vector.cross(right_vector).normalize();

            return CFrame{ right_vector, up_vector, look_vector, position };
        }

        CFrame operator*(const CFrame& cframe) const noexcept {

            CFrame result;

            result.right_vector = right_vector * cframe.right_vector;
            result.up_vector = up_vector * cframe.up_vector;
            result.back_vector = back_vector * cframe.back_vector;

            result.position = right_vector * cframe.position + position;

            return result;
        }

        Vector3 operator*(const Vector3& vec) const noexcept {

            return {
                right_vector.x * vec.x + right_vector.y * vec.y + right_vector.z * vec.z + position.x,
                up_vector.x * vec.x + up_vector.y * vec.y + up_vector.z * vec.z + position.y,
                back_vector.x * vec.x + back_vector.y * vec.y + back_vector.z * vec.z + position.z
            };
        }
    };

}

// math