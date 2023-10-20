#ifndef LINMATH_H
#define LINMATH_H

#include <string.h>
#include <math.h>
#include <string.h>

typedef float vec3[3];
typedef float vec4[4];
typedef vec4 mat4x4[4];

static void vec3_add(vec3 r, vec3 const a, vec3 const b)
{
    for (int i = 0; i < 3; ++i)
        r[i] = a[i] + b[i];
}

static void vec4_add(vec4 r, vec4 const a, vec4 const b)
{
    for (int i = 0; i < 4; ++i)
        r[i] = a[i] + b[i];
}

static void vec3_sub(vec3 r, vec3 const a, vec3 const b)
{
    for (int i = 0; i < 3; ++i)
        r[i] = a[i] - b[i];
}

static void vec4_sub(vec4 r, vec4 const a, vec4 const b)
{
    for (int i = 0; i < 4; ++i)
        r[i] = a[i] - b[i];
}

static void vec3_scale(vec3 r, vec3 const v, float const s)
{
    for (int i = 0; i < 3; ++i)
        r[i] = v[i] * s;
}

static void vec4_scale(vec4 r, vec4 const v, float const s)
{
    for (int i = 0; i < 4; ++i)
        r[i] = v[i] * s;
}

static float vec3_mul_inner(vec3 const a, vec3 const b)
{
    float p = 0.f;
    for (int i = 0; i < 3; ++i)
        p += b[i] * a[i];
    return p;
}

static float vec4_mul_inner(vec4 const a, vec4 const b)
{
    float p = 0.f;
    for (int i = 0; i < 4; ++i)
        p += b[i] * a[i];
    return p;
}

static float vec3_len(vec3 const v)
{
    return sqrtf(vec3_mul_inner(v, v));
}

static float vec4_len(vec4 const v)
{
    return sqrtf(vec4_mul_inner(v, v));
}

static void vec3_norm(vec3 r, vec3 const v)
{
    float k = 1.f / vec3_len(v);
    vec3_scale(r, v, k);
}

static void vec4_norm(vec4 r, vec4 const v)
{
    float k = 1.f / vec4_len(v);
    vec4_scale(r, v, k);
}

static void vec3_dup(vec3 r, vec3 const src)
{
    for (int i = 0; i < 3; ++i) r[i] = src[i];
}

static void vec4_dup(vec4 r, vec4 const src)
{
    for (int i = 0; i < 4; ++i) r[i] = src[i];
}

static void vec3_mul_cross(vec3 r, vec3 const a, vec3 const b)
{
	r[0] = a[1]*b[2] - a[2]*b[1];
	r[1] = a[2]*b[0] - a[0]*b[2];
	r[2] = a[0]*b[1] - a[1]*b[0];
}

static void vec4_mul_cross(vec4 r, vec4 const a, vec4 const b)
{
	r[0] = a[1]*b[2] - a[2]*b[1];
	r[1] = a[2]*b[0] - a[0]*b[2];
	r[2] = a[0]*b[1] - a[1]*b[0];
	r[3] = 1.f;
}

static void vec4_reflect(vec4 r, vec4 const v, vec4 const n)
{
	float p  = 2.f*vec4_mul_inner(v, n);
	for(int i=0;i<4;++i)
		r[i] = v[i] - p*n[i];
}

static void mat4x4_identity(mat4x4 M)
{
	for(int i=0; i<4; ++i)
		for(int j=0; j<4; ++j)
			M[i][j] = i==j ? 1.f : 0.f;
}

static void mat4x4_dup(mat4x4 M, mat4x4 const N)
{
	for (int i=0; i<4; ++i)
		vec4_dup(M[i], N[i]);
}

static  void mat4x4_row(vec4 r, mat4x4 const M, int i)
{
	for (int k=0; k<4; ++k)
		r[k] = M[k][i];
}

static void mat4x4_transpose(mat4x4 M, mat4x4 const N)
{
	for (int j=0; j<4; ++j)
		for (int i=0; i<4; ++i)
			M[i][j] = N[j][i];
}

static void mat4x4_add(mat4x4 M, mat4x4 const a, mat4x4 const b)
{
	for (int i=0; i<4; ++i)
		vec4_add(M[i], a[i], b[i]);
}

static void mat4x4_sub(mat4x4 M, mat4x4 const a, mat4x4 const b)
{
	for (int i=0; i<4; ++i)
		vec4_sub(M[i], a[i], b[i]);
}

static void mat4x4_scale(mat4x4 M, mat4x4 const a, float k)
{
	for (int i=0; i<4; ++i)
		vec4_scale(M[i], a[i], k);
}

static void mat4x4_scale_aniso(mat4x4 M, mat4x4 const a, float x, float y, float z)
{
	vec4_scale(M[0], a[0], x);
	vec4_scale(M[1], a[1], y);
	vec4_scale(M[2], a[2], z);
	vec4_dup(M[3], a[3]);
}

static void mat4x4_translate_in_place(mat4x4 M, float x, float y, float z)
{
	vec4 t = {x, y, z, 0};
	vec4 r;
	for (int i = 0; i < 4; ++i) {
		mat4x4_row(r, M, i);
		M[3][i] += vec4_mul_inner(r, t);
	}
}

static void mat4x4_perspective(mat4x4 m, float y_fov, float aspect, float n, float f)
{
	/* NOTE: Degrees are an unhandy unit to work with.
	 * linmath.h uses radians for everything! */
	float const a = 1.f / tanf(y_fov / 2.f);

	m[0][0] = a / aspect;
	m[0][1] = 0.f;
	m[0][2] = 0.f;
	m[0][3] = 0.f;

	m[1][0] = 0.f;
	m[1][1] = a;
	m[1][2] = 0.f;
	m[1][3] = 0.f;

	m[2][0] = 0.f;
	m[2][1] = 0.f;
	m[2][2] = -((f + n) / (f - n));
	m[2][3] = -1.f;

	m[3][0] = 0.f;
	m[3][1] = 0.f;
	m[3][2] = -((2.f * f * n) / (f - n));
	m[3][3] = 0.f;
}

static void mat4x4_look_at(mat4x4 m, vec3 const eye, vec3 const center, vec3 const up)
{
	vec3 f, s, t;
	vec3_sub(f, center, eye);	
	vec3_norm(f, f);	
	vec3_mul_cross(s, f, up);
	vec3_norm(s, s);
	vec3_mul_cross(t, s, f);

    for (int i = 0; i <= 2; ++i)
    {
        m[i][0] =  s[i];
        m[i][1] =  t[i];
        m[i][2] = -f[i];
        m[i][3] =   0.f;
    }

	m[3][0] =  0.f;
	m[3][1] =  0.f;
	m[3][2] =  0.f;
	m[3][3] =  1.f;

	mat4x4_translate_in_place(m, -eye[0], -eye[1], -eye[2]);
}
#endif
