#ifndef MATH_HELPER_H_INCLUDED
#define MATH_HELPER_H_INCLUDED

#include <algorithm>
#include <memory.h>

#define TORAD 0.01745329251994329576923690768489f

static void createViewMatrix(const float position[3], float angleX, float angleZ, float out[4][4])
{
    // Normalized direction
    float R2[3] = {
        -std::sinf(angleZ * TORAD) * std::cosf(angleX * TORAD),
        -std::cosf(angleZ * TORAD) * std::cosf(angleX * TORAD),
        -std::sinf(angleX * TORAD)
    };

    float R0[3] = {
        -1 * R2[1],
        1 * R2[0],
        0
    };
    float len = std::sqrtf(R0[0] * R0[0] + R0[1] * R0[1]);
    R0[0] /= len;
    R0[1] /= len;

    float R1[3] = {
        R2[1] * R0[2] - R2[2] * R0[1],
        R2[2] * R0[0] - R2[0] * R0[2],
        R2[0] * R0[1] - R2[1] * R0[0]
    };

    auto D0 = R0[0] * -position[0] + R0[1] * -position[1] + R0[2] * -position[2];
    auto D1 = R1[0] * -position[0] + R1[1] * -position[1] + R1[2] * -position[2];
    auto D2 = R2[0] * -position[0] + R2[1] * -position[1] + R2[2] * -position[2];

    out[0][0] = R0[0];
    out[0][1] = R1[0];
    out[0][2] = R2[0];
    out[0][3] = 0.0f;

    out[1][0] = R0[1];
    out[1][1] = R1[1];
    out[1][2] = R2[1];
    out[1][3] = 0.0f;

    out[2][0] = R0[2];
    out[2][1] = R1[2];
    out[2][2] = R2[2];
    out[2][3] = 0.0f;

    out[3][0] = D0;
    out[3][1] = D1;
    out[3][2] = D2;
    out[3][3] = 1.0f;
}

static void createPerspectiveFieldOfView(float fov, float aspectRatio, float nearPlane, float farPlane, float out[4][4])
{
    float CosFov = std::cos(0.5f * fov * TORAD);
    float SinFov = std::sin(0.5f * fov * TORAD);

    float Height = CosFov / SinFov;
    float Width = Height / aspectRatio;
    float fRange = farPlane / (nearPlane - farPlane);

    out[0][0] = Width;
    out[0][1] = 0.0f;
    out[0][2] = 0.0f;
    out[0][3] = 0.0f;

    out[1][0] = 0.0f;
    out[1][1] = Height;
    out[1][2] = 0.0f;
    out[1][3] = 0.0f;

    out[2][0] = 0.0f;
    out[2][1] = 0.0f;
    out[2][2] = fRange;
    out[2][3] = -1.0f;

    out[3][0] = 0.0f;
    out[3][1] = 0.0f;
    out[3][2] = fRange * nearPlane;
    out[3][3] = 0.0f;
}

static void mulMatrix(float a[4][4], float b[4][4], float out[4][4])
{
    memcpy(out, a, sizeof(out) * 16);
    // Cache the invariants in registers
    float x = out[0][0];
    float y = out[0][1];
    float z = out[0][2];
    float w = out[0][3];
    // Perform the operation on the first row
    out[0][0] = (b[0][0] * x) + (b[1][0] * y) + (b[2][0] * z) + (b[3][0] * w);
    out[0][1] = (b[0][1] * x) + (b[1][1] * y) + (b[2][1] * z) + (b[3][1] * w);
    out[0][2] = (b[0][2] * x) + (b[1][2] * y) + (b[2][2] * z) + (b[3][2] * w);
    out[0][3] = (b[0][3] * x) + (b[1][3] * y) + (b[2][3] * z) + (b[3][3] * w);
    // Repeat for all the other rows
    x = out[1][0];
    y = out[1][1];
    z = out[1][2];
    w = out[1][3];
    out[1][0] = (b[0][0] * x) + (b[1][0] * y) + (b[2][0] * z) + (b[3][0] * w);
    out[1][1] = (b[0][1] * x) + (b[1][1] * y) + (b[2][1] * z) + (b[3][1] * w);
    out[1][2] = (b[0][2] * x) + (b[1][2] * y) + (b[2][2] * z) + (b[3][2] * w);
    out[1][3] = (b[0][3] * x) + (b[1][3] * y) + (b[2][3] * z) + (b[3][3] * w);
    x = out[2][0];
    y = out[2][1];
    z = out[2][2];
    w = out[2][3];
    out[2][0] = (b[0][0] * x) + (b[1][0] * y) + (b[2][0] * z) + (b[3][0] * w);
    out[2][1] = (b[0][1] * x) + (b[1][1] * y) + (b[2][1] * z) + (b[3][1] * w);
    out[2][2] = (b[0][2] * x) + (b[1][2] * y) + (b[2][2] * z) + (b[3][2] * w);
    out[2][3] = (b[0][3] * x) + (b[1][3] * y) + (b[2][3] * z) + (b[3][3] * w);
    x = out[3][0];
    y = out[3][1];
    z = out[3][2];
    w = out[3][3];
    out[3][0] = (b[0][0] * x) + (b[1][0] * y) + (b[2][0] * z) + (b[3][0] * w);
    out[3][1] = (b[0][1] * x) + (b[1][1] * y) + (b[2][1] * z) + (b[3][1] * w);
    out[3][2] = (b[0][2] * x) + (b[1][2] * y) + (b[2][2] * z) + (b[3][2] * w);
    out[3][3] = (b[0][3] * x) + (b[1][3] * y) + (b[2][3] * z) + (b[3][3] * w);
}

#endif
