#include <cmath>
#define PI 3.14159265358979323846f
#define DEG2RAD (PI / 180.0f)
#define RAD2DEG (180.0f / PI)

struct float2 {
  union {
    float data[2]{};
    struct {
      float x, y;
    };
  };

  float2() {}
  // float2(const float2& v) :x(v.x), y(v.y) {}
  template <typename X>
  explicit float2(X x) : x(static_cast<float>(x)), y(static_cast<float>(x)) {}
  template <typename X, typename Y>
  explicit float2(X x, Y y)
      : x(static_cast<float>(x)), y(static_cast<float>(y)) {}
  float& operator[](int order) { return data[order]; }
  float operator[](int order) const { return data[order]; }
  // float2& operator=(const float2& v) { x = v.x, y = v.y; return *this; }
  void operator+=(const float2& v) { x += v.x, y += v.y; }
  void operator-=(const float2& v) { x -= v.x, y -= v.y; }
  void operator*=(float s) { x *= s, y *= s; }
  void operator/=(float s) { *this *= 1.f / s; }
};

struct float3 {
  union {
    float data[3]{};
    struct {
      union {
        struct {
          float x, y;
        };
        float2 xy;
      };
      float z;
    };
  };

  float3() {}
  template <typename X>
  explicit float3(X x)
      : x(static_cast<float>(x)),
        y(static_cast<float>(x)),
        z(static_cast<float>(x)) {}
  template <typename X, typename Y, typename Z>
  explicit float3(X x, Y y, Z z)
      : x(static_cast<float>(x)),
        y(static_cast<float>(y)),
        z(static_cast<float>(z)) {}
  explicit float3(const float2& xy, float z) : xy(xy), z(z) {}
  float& operator[](int order) { return data[order]; }
  float operator[](int order) const { return data[order]; }
  void operator+=(const float3& v) { x += v.x, y += v.y, z += v.z; }
  void operator-=(const float3& v) { x -= v.x, y -= v.y, z -= v.z; }
  void operator*=(float s) { x *= s, y *= s, z *= s; }
  void operator/=(float s) { *this *= 1.f / s; }
};

struct float4 {
  union {
    float data[4]{};
    struct {
      union {
        struct {
          float x, y, z;
        };
        float3 xyz;
      };
      float w;
    };
  };

  float4() {}
  template <typename X>
  explicit float4(X x)
      : x(static_cast<float>(x)),
        y(static_cast<float>(x)),
        z(static_cast<float>(x)),
        w(static_cast<float>(x)) {}
  template <typename X, typename Y, typename Z, typename W>
  explicit float4(X x, Y y, Z z, W w)
      : x(static_cast<float>(x)),
        y(static_cast<float>(y)),
        z(static_cast<float>(z)),
        w(static_cast<float>(w)) {}
  explicit float4(const float3& xyz, float w) : xyz(xyz), w(w) {}
  float& operator[](int order) { return data[order]; }
  float operator[](int order) const { return data[order]; }
  void operator+=(const float4& v) { x += v.x, y += v.y, z += v.z, w += v.w; }
  void operator-=(const float4& v) { x -= v.x, y -= v.y, z -= v.z, w -= v.w; }
  void operator*=(float s) { x *= s, y *= s, z *= s, w *= s; }
  void operator/=(float s) { *this *= 1.f / s; }
};

struct uint2;
struct int2 {
  union {
    int data[2]{};
    struct {
      int x, y;
    };
    struct {
      int i, j;
    };
  };

  int2() {}
  // int2(const int2& v) :x(v.x), y(v.y) {}
  template <typename I, typename J>
  int2(I i, J j) : i((int)i), j((int)j) {}
  int& operator[](int order) { return data[order]; }
  int operator[](int order) const { return data[order]; }
  explicit operator uint2();
  // uint2& operator=(const uint2& v) { x = v.x, y = v.y; return *this; }
};

struct uint2 {
  union {
    UINT data[2]{};
    struct {
      UINT x, y;
    };
    struct {
      UINT i, j;
    };
  };

  uint2() {}
  // uint2(const uint2& v) :x(v.x), y(v.y) {}
  template <typename I, typename J>
  uint2(I i, J j) : i((UINT)i), j((UINT)j) {}
  UINT& operator[](int order) { return data[order]; }
  UINT operator[](int order) const { return data[order]; }
  explicit operator int2();
  // uint2& operator=(const uint2& v) { x = v.x, y = v.y; return *this; }
  void operator+=(const uint2& v) { x += v.x, y += v.y; }
  void operator-=(const uint2& v) { x -= v.x, y -= v.y; }
};

inline int2::operator uint2() { return uint2((UINT)i, (UINT)j); }

inline uint2::operator int2() { return int2((int)i, (int)j); }

struct uint3 {
  union {
    UINT data[3]{};
    struct {
      UINT x, y, z;
    };
    struct {
      UINT i, j, k;
    };
  };

  uint3() {}
  // uint3(const uint3& v) :x(v.x), y(v.y), z(v.z) {}
  template <typename I, typename J, typename K>
  uint3(I i, J j, K k) : i((UINT)i), j((UINT)j), k((UINT)k) {}
  UINT& operator[](int order) { return data[order]; }
  UINT operator[](int order) const { return data[order]; }
  // uint3& operator=(const uint3& v) { x = v.x, y = v.y, z = v.z; return *this;
  // }
  void operator+=(const uint3& v) { x += v.x, y += v.y, z += v.z; }
  void operator-=(const uint3& v) { x -= v.x, y -= v.y, z -= v.z; }
};


struct Transform {
  float mat[4][4]{};
  Transform() {}
  explicit constexpr Transform(float scala) : mat{} {
    mat[0][0] = mat[1][1] = mat[2][2] = scala;
    mat[3][3] = 1.0f;
  }
  static constexpr Transform identity() { return Transform(1.0f); }
};

template <typename T>
inline constexpr T _min(T x, T y) {
  return x < y ? x : y;
}

template <typename T>
inline constexpr T _max(T x, T y) {
  return x > y ? x : y;
}

template <typename T>
inline constexpr T _clamp(T value, T lowerBound, T upperBound) {
  return _min(_max(lowerBound, value), upperBound);
}

inline float3 operator+(const float3& v, const float3& w) {
  return float3(v.x + w.x, v.y + w.y, v.z + w.z);
}
inline float3 operator-(const float3& v, const float3& w) {
  return float3(v.x - w.x, v.y - w.y, v.z - w.z);
}
inline float3 operator-(const float3& v) { return float3(-v.x, -v.y, -v.z); }
template <typename T>
inline float3 operator*(const float3& v, T s) {
  float ss = static_cast<float>(s);
  return float3(v.x * ss, v.y * ss, v.z * ss);
}
template <typename T>
inline float3 operator*(T s, const float3& v) {
  float ss = static_cast<float>(s);
  return float3(v.x * ss, v.y * ss, v.z * ss);
}
template <typename T>
inline float3 operator/(const float3& v, T s) {
  float ss = 1.0f / static_cast<float>(s);
  return float3(v.x * ss, v.y * ss, v.z * ss);
}
inline float dot(const float3& v, const float3& w) {
  return v.x * w.x + v.y * w.y + v.z * w.z;
}
inline float squaredLength(const float3& v) {
  return v.x * v.x + v.y * v.y + v.z * v.z;
}
inline float length(const float3& v) { return sqrtf(squaredLength(v)); }
inline float3 normalize(const float3& v) { return v / length(v); }
inline float3 cross(const float3& v, const float3& w) {
  return float3(v.y * w.z - v.z * w.y, v.z * w.x - v.x * w.z,
                v.x * w.y - v.y * w.x);
}

inline Transform composeMatrix(const float3& translation,
                               const float4& rotation, float scale) {
  Transform ret;
  ret.mat[0][0] =
      scale * (1.f - 2.f * (rotation.y * rotation.y + rotation.z * rotation.z));
  ret.mat[0][1] =
      scale * (2.f * (rotation.x * rotation.y - rotation.z * rotation.w));
  ret.mat[0][2] =
      scale * (2.f * (rotation.x * rotation.z + rotation.y * rotation.w));
  ret.mat[0][3] = translation.x;
  ret.mat[1][0] =
      scale * (2.f * (rotation.x * rotation.y + rotation.z * rotation.w));
  ret.mat[1][1] =
      scale * (1.f - 2.f * (rotation.x * rotation.x + rotation.z * rotation.z));
  ret.mat[1][2] =
      scale * (2.f * (rotation.y * rotation.z - rotation.x * rotation.w));
  ret.mat[1][3] = translation.y;
  ret.mat[2][0] =
      scale * (2.f * (rotation.x * rotation.z - rotation.y * rotation.w));
  ret.mat[2][1] =
      scale * (2.f * (rotation.y * rotation.z + rotation.x * rotation.w));
  ret.mat[2][2] =
      scale * (1.f - 2.f * (rotation.x * rotation.x + rotation.y * rotation.y));
  ret.mat[2][3] = translation.z;
  ret.mat[3][0] = 0.f;
  ret.mat[3][1] = 0.f;
  ret.mat[3][2] = 0.f;
  ret.mat[3][3] = 1.f;
  return ret;
}

inline void composeMatrix(float matrix[16], const float3& translation,
                          const float4& rotation, float scale) {
  matrix[0] =
      scale * (1.f - 2.f * (rotation.y * rotation.y + rotation.z * rotation.z));
  matrix[1] =
      scale * (2.f * (rotation.x * rotation.y - rotation.z * rotation.w));
  matrix[2] =
      scale * (2.f * (rotation.x * rotation.z + rotation.y * rotation.w));
  matrix[3] = translation.x;
  matrix[4] =
      scale * (2.f * (rotation.x * rotation.y + rotation.z * rotation.w));
  matrix[5] =
      scale * (1.f - 2.f * (rotation.x * rotation.x + rotation.z * rotation.z));
  matrix[6] =
      scale * (2.f * (rotation.y * rotation.z - rotation.x * rotation.w));
  matrix[7] = translation.y;
  matrix[8] =
      scale * (2.f * (rotation.x * rotation.z - rotation.y * rotation.w));
  matrix[9] =
      scale * (2.f * (rotation.y * rotation.z + rotation.x * rotation.w));
  matrix[10] =
      scale * (1.f - 2.f * (rotation.x * rotation.x + rotation.y * rotation.y));
  matrix[11] = translation.z;
  matrix[12] = 0.f;
  matrix[13] = 0.f;
  matrix[14] = 0.f;
  matrix[15] = 1.f;
}

inline float4 getRotationAsQuternion(const float3& axis, float degree) {
  float angle = degree * DEG2RAD;

  return float4(sinf(0.5f * angle) * axis, cosf(0.5f * angle));
}

inline int2 operator-(const int2& v, const int2& w) {
  return int2(v.x - w.x, v.y - w.y);
}
