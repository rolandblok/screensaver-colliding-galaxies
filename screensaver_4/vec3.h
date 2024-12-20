#pragma once


#include <cmath>


template <class T>
class vec3 {
public:
	T x, y, z;

	vec3() : x(0), y(0), z(0) {}
	vec3(T x, T y, T z) : x(x), y(y), z(z) {}

	vec3 operator+(const vec3& v) const {
		return vec3(x + v.x, y + v.y, z + v.z);
	}

	vec3 operator-(const vec3& v) const {
		return vec3(x - v.x, y - v.y, z - v.z);
	}

	vec3 operator*(T s) const {
		return vec3(x * s, y * s, z * s);
	}

	vec3 operator/(T s) const {
		return vec3(x / s, y / s, z / s);
	}

	// operator +=
	vec3& operator+=(const vec3& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	T dot(const vec3& v) const {
		return x * v.x + y * v.y + z * v.z;
	}

	vec3 cross(const vec3& v) const {
		return vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
	}

	T length() const {
		return std::sqrt(x * x + y * y + z * z);
	}

	T length2() const {
		return x * x + y * y + z * z;
	}

	vec3 normalize() const {
		return *this / length();
	}

	// rotate x
	vec3 rotateX(T angle) const {
		T c = std::cos(angle);
		T s = std::sin(angle);
		return vec3(x, y * c - z * s, y * s + z * c);
	}

	// rotate y
	vec3 rotateY(T angle) const {
		T c = std::cos(angle);
		T s = std::sin(angle);
		return vec3(x * c + z * s, y, -x * s + z * c);
	}

	// rotate z
	vec3 rotateZ(T angle) const {
		T c = std::cos(angle);
		T s = std::sin(angle);
		return vec3(x * c - y * s, x * s + y * c, z);
	}

	// rotate around an arbitrary axis
	vec3 rotate(const vec3& axis, T angle) const {
		T c = std::cos(angle);
		T s = std::sin(angle);
		vec3 u = axis.normalize();
		vec3 v = u.cross(*this);
		return *this * c + v * s + u * u.dot(*this) * (1 - c);
	}

	// return int version of the vector
	vec3<int> toInt() const {
		return vec3<int>(static_cast<int>(x), static_cast<int>(y), static_cast<int>(z));
	}

};