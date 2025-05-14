#include <stdint.h>
#include <stdbool.h>

#define PI 3.14159265358979323846f
#define E 2.71828182845904523536f
#define SQRT2 1.41421356237309504880f
#define SQRT3 1.73205080756887729352f
#define DEG_TO_RAD (PI / 180.0f)
#define RAD_TO_DEG (180.0f / PI)

typedef struct {
    float x;
    float y;
} vec2_t;

typedef struct {
    float x;
    float y;
    float z;
} vec3_t;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} vec4_t;

typedef struct {
    float m[4][4];
} mat4_t;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} quat_t;

float absf(float x) {
    return x < 0 ? -x : x;
}

float minf(float a, float b) {
    return a < b ? a : b;
}

float maxf(float a, float b) {
    return a > b ? a : b;
}

float clampf(float x, float min, float max) {
    return x < min ? min : x > max ? max : x;
}

float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

float smoothstepf(float edge0, float edge1, float x) {
    float t = clampf((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float sinf(float x) {
    x = fmodf(x, 2.0f * PI);
    if (x < 0) x += 2.0f * PI;
    
    float x2 = x * x;
    float x3 = x2 * x;
    float x5 = x3 * x2;
    float x7 = x5 * x2;
    
    return x - x3/6.0f + x5/120.0f - x7/5040.0f;
}

float cosf(float x) {
    return sinf(x + PI/2.0f);
}

float tanf(float x) {
    return sinf(x) / cosf(x);
}

float asinf(float x) {
    if (x < -1.0f) x = -1.0f;
    if (x > 1.0f) x = 1.0f;
    
    float x2 = x * x;
    float x3 = x2 * x;
    float x5 = x3 * x2;
    float x7 = x5 * x2;
    
    return x + x3/6.0f + 3.0f*x5/40.0f + 5.0f*x7/112.0f;
}

float acosf(float x) {
    return PI/2.0f - asinf(x);
}

float atanf(float x) {
    float x2 = x * x;
    float x3 = x2 * x;
    float x5 = x3 * x2;
    float x7 = x5 * x2;
    
    return x - x3/3.0f + x5/5.0f - x7/7.0f;
}

float atan2f(float y, float x) {
    if (x > 0) return atanf(y/x);
    if (x < 0 && y >= 0) return atanf(y/x) + PI;
    if (x < 0 && y < 0) return atanf(y/x) - PI;
    if (x == 0 && y > 0) return PI/2.0f;
    if (x == 0 && y < 0) return -PI/2.0f;
    return 0.0f;
}

float sqrtf(float x) {
    if (x < 0) return 0.0f;
    
    float y = x;
    float z = (y + x/y) / 2.0f;
    
    while (absf(y - z) > 0.000001f) {
        y = z;
        z = (y + x/y) / 2.0f;
    }
    
    return z;
}

float powf(float x, float y) {
    return expf(y * logf(x));
}

float expf(float x) {
    float sum = 1.0f;
    float term = 1.0f;
    
    for (int i = 1; i < 10; i++) {
        term *= x / i;
        sum += term;
    }
    
    return sum;
}

float logf(float x) {
    if (x <= 0) return 0.0f;
    
    float y = 0.0f;
    float z = (x - 1.0f) / (x + 1.0f);
    float z2 = z * z;
    float z3 = z2 * z;
    float z5 = z3 * z2;
    float z7 = z5 * z2;
    
    return 2.0f * (z + z3/3.0f + z5/5.0f + z7/7.0f);
}

vec2_t vec2_add(vec2_t a, vec2_t b) {
    return (vec2_t){a.x + b.x, a.y + b.y};
}

vec2_t vec2_sub(vec2_t a, vec2_t b) {
    return (vec2_t){a.x - b.x, a.y - b.y};
}

vec2_t vec2_mul(vec2_t a, float s) {
    return (vec2_t){a.x * s, a.y * s};
}

float vec2_dot(vec2_t a, vec2_t b) {
    return a.x * b.x + a.y * b.y;
}

float vec2_length(vec2_t v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

vec2_t vec2_normalize(vec2_t v) {
    float len = vec2_length(v);
    if (len == 0.0f) return (vec2_t){0.0f, 0.0f};
    return vec2_mul(v, 1.0f / len);
}

vec3_t vec3_add(vec3_t a, vec3_t b) {
    return (vec3_t){a.x + b.x, a.y + b.y, a.z + b.z};
}

vec3_t vec3_sub(vec3_t a, vec3_t b) {
    return (vec3_t){a.x - b.x, a.y - b.y, a.z - b.z};
}

vec3_t vec3_mul(vec3_t a, float s) {
    return (vec3_t){a.x * s, a.y * s, a.z * s};
}

float vec3_dot(vec3_t a, vec3_t b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3_t vec3_cross(vec3_t a, vec3_t b) {
    return (vec3_t){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

float vec3_length(vec3_t v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

vec3_t vec3_normalize(vec3_t v) {
    float len = vec3_length(v);
    if (len == 0.0f) return (vec3_t){0.0f, 0.0f, 0.0f};
    return vec3_mul(v, 1.0f / len);
}

vec4_t vec4_add(vec4_t a, vec4_t b) {
    return (vec4_t){a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

vec4_t vec4_sub(vec4_t a, vec4_t b) {
    return (vec4_t){a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

vec4_t vec4_mul(vec4_t a, float s) {
    return (vec4_t){a.x * s, a.y * s, a.z * s, a.w * s};
}

float vec4_dot(vec4_t a, vec4_t b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

float vec4_length(vec4_t v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

vec4_t vec4_normalize(vec4_t v) {
    float len = vec4_length(v);
    if (len == 0.0f) return (vec4_t){0.0f, 0.0f, 0.0f, 0.0f};
    return vec4_mul(v, 1.0f / len);
}

mat4_t mat4_identity() {
    mat4_t m = {0};
    m.m[0][0] = 1.0f;
    m.m[1][1] = 1.0f;
    m.m[2][2] = 1.0f;
    m.m[3][3] = 1.0f;
    return m;
}

mat4_t mat4_mul(mat4_t a, mat4_t b) {
    mat4_t m = {0};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                m.m[i][j] += a.m[i][k] * b.m[k][j];
            }
        }
    }
    return m;
}

mat4_t mat4_translate(vec3_t v) {
    mat4_t m = mat4_identity();
    m.m[3][0] = v.x;
    m.m[3][1] = v.y;
    m.m[3][2] = v.z;
    return m;
}

mat4_t mat4_scale(vec3_t v) {
    mat4_t m = mat4_identity();
    m.m[0][0] = v.x;
    m.m[1][1] = v.y;
    m.m[2][2] = v.z;
    return m;
}

mat4_t mat4_rotate_x(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    mat4_t m = mat4_identity();
    m.m[1][1] = c;
    m.m[1][2] = -s;
    m.m[2][1] = s;
    m.m[2][2] = c;
    return m;
}

mat4_t mat4_rotate_y(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    mat4_t m = mat4_identity();
    m.m[0][0] = c;
    m.m[0][2] = s;
    m.m[2][0] = -s;
    m.m[2][2] = c;
    return m;
}

mat4_t mat4_rotate_z(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    mat4_t m = mat4_identity();
    m.m[0][0] = c;
    m.m[0][1] = -s;
    m.m[1][0] = s;
    m.m[1][1] = c;
    return m;
}

mat4_t mat4_perspective(float fovy, float aspect, float near, float far) {
    float f = 1.0f / tanf(fovy / 2.0f);
    mat4_t m = {0};
    m.m[0][0] = f / aspect;
    m.m[1][1] = f;
    m.m[2][2] = (far + near) / (near - far);
    m.m[2][3] = -1.0f;
    m.m[3][2] = (2.0f * far * near) / (near - far);
    return m;
}

mat4_t mat4_ortho(float left, float right, float bottom, float top, float near, float far) {
    mat4_t m = mat4_identity();
    m.m[0][0] = 2.0f / (right - left);
    m.m[1][1] = 2.0f / (top - bottom);
    m.m[2][2] = -2.0f / (far - near);
    m.m[3][0] = -(right + left) / (right - left);
    m.m[3][1] = -(top + bottom) / (top - bottom);
    m.m[3][2] = -(far + near) / (far - near);
    return m;
}

mat4_t mat4_look_at(vec3_t eye, vec3_t center, vec3_t up) {
    vec3_t f = vec3_normalize(vec3_sub(center, eye));
    vec3_t s = vec3_normalize(vec3_cross(f, up));
    vec3_t u = vec3_cross(s, f);

    mat4_t m = mat4_identity();
    m.m[0][0] = s.x;
    m.m[0][1] = s.y;
    m.m[0][2] = s.z;
    m.m[1][0] = u.x;
    m.m[1][1] = u.y;
    m.m[1][2] = u.z;
    m.m[2][0] = -f.x;
    m.m[2][1] = -f.y;
    m.m[2][2] = -f.z;
    m.m[3][0] = -vec3_dot(s, eye);
    m.m[3][1] = -vec3_dot(u, eye);
    m.m[3][2] = vec3_dot(f, eye);

    return m;
}

quat_t quat_identity() {
    return (quat_t){0.0f, 0.0f, 0.0f, 1.0f};
}

quat_t quat_from_axis_angle(vec3_t axis, float angle) {
    float s = sinf(angle / 2.0f);
    return (quat_t){
        axis.x * s,
        axis.y * s,
        axis.z * s,
        cosf(angle / 2.0f)
    };
}

quat_t quat_mul(quat_t a, quat_t b) {
    return (quat_t){
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
    };
}

quat_t quat_normalize(quat_t q) {
    float len = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (len == 0.0f) return quat_identity();
    return (quat_t){q.x / len, q.y / len, q.z / len, q.w / len};
}

mat4_t quat_to_mat4(quat_t q) {
    float x2 = q.x * q.x;
    float y2 = q.y * q.y;
    float z2 = q.z * q.z;
    float xy = q.x * q.y;
    float xz = q.x * q.z;
    float yz = q.y * q.z;
    float wx = q.w * q.x;
    float wy = q.w * q.y;
    float wz = q.w * q.z;

    mat4_t m = mat4_identity();
    m.m[0][0] = 1.0f - 2.0f * (y2 + z2);
    m.m[0][1] = 2.0f * (xy - wz);
    m.m[0][2] = 2.0f * (xz + wy);
    m.m[1][0] = 2.0f * (xy + wz);
    m.m[1][1] = 1.0f - 2.0f * (x2 + z2);
    m.m[1][2] = 2.0f * (yz - wx);
    m.m[2][0] = 2.0f * (xz - wy);
    m.m[2][1] = 2.0f * (yz + wx);
    m.m[2][2] = 1.0f - 2.0f * (x2 + y2);

    return m;
} 