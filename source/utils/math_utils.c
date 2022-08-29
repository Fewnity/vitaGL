/*
 * This file is part of vitaGL
 * Copyright 2017, 2018, 2019, 2020 Rinnegatamante
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* 
 * math_utils.c:
 * Utilities for math operations
 */

#include "../shared.h"
#include <math.h>
#include <math_neon.h>

// NOTE: matrices are row-major.

void matrix4x4_identity(matrix4x4 m) {
	sceClibMemset(m, 0, sizeof(matrix4x4));
	m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0f;
}

void matrix4x4_copy(matrix4x4 dst, const matrix4x4 src) {
	vgl_fast_memcpy(dst, src, sizeof(matrix4x4));
}

void matrix4x4_multiply(matrix4x4 dst, const matrix4x4 src1, const matrix4x4 src2) {
	matmul4_neon((float *)src2, (float *)src1, (float *)dst);
}

void matrix4x4_rotate(matrix4x4 src, float rad, float x, float y, float z) {
	float cs[2];
	sincosf_c(rad, cs);
	
	matrix4x4 m;
	matrix4x4_identity(m);
	const float c = 1 - cs[1];
	float axis[3] = {x, y, z};
	normalize3_neon(axis, axis);
	const float xc = axis[0] * c, yc = axis[1] * c, zc = axis[2] * c;
	m[0][0] = axis[0] * xc + cs[1];
	m[0][1] = axis[1] * xc + axis[2] * cs[0];
	m[0][2] = axis[2] * xc - axis[1] * cs[0];
	
	m[1][0] = axis[0] * yc - axis[2] * cs[0];
	m[1][1] = axis[1] * yc + cs[1];
	m[1][2] = axis[2] * yc + axis[0] * cs[0];
	
	m[2][0] = axis[0] * zc + axis[1] * cs[0];
	m[2][1] = axis[1] * zc - axis[0] * cs[0];
	m[2][2] = axis[2] * zc + cs[1];
	
	matrix4x4 res;
	matrix4x4_multiply(res, src, m);
	matrix4x4_copy(src, res);
}

void matrix4x4_init_translation(matrix4x4 m, float x, float y, float z) {
	matrix4x4_identity(m);

	m[3][0] = x;
	m[3][1] = y;
	m[3][2] = z;
}

void matrix4x4_translate(matrix4x4 m, float x, float y, float z) {
	matrix4x4 m1, m2;

	matrix4x4_init_translation(m1, x, y, z);
	matrix4x4_multiply(m2, m, m1);
	matrix4x4_copy(m, m2);
}

void matrix4x4_init_scaling(matrix4x4 m, float scale_x, float scale_y, float scale_z) {
	matrix4x4_identity(m);

	m[0][0] = scale_x;
	m[1][1] = scale_y;
	m[2][2] = scale_z;
}

void matrix4x4_scale(matrix4x4 m, float scale_x, float scale_y, float scale_z) {
	matrix4x4 m1, m2;

	matrix4x4_init_scaling(m1, scale_x, scale_y, scale_z);
	matrix4x4_multiply(m2, m, m1);
	matrix4x4_copy(m, m2);
}

void matrix2x2_transpose(matrix2x2 out, const matrix2x2 m) {
	int i, j;

	for (i = 0; i < 2; i++) {
		for (j = 0; j < 2; j++)
			out[i][j] = m[j][i];
	}
}

void matrix3x3_transpose(matrix3x3 out, const matrix3x3 m) {
	int i, j;

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++)
			out[i][j] = m[j][i];
	}
}

void matrix4x4_transpose(matrix4x4 out, const matrix4x4 m) {
	int i, j;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++)
			out[i][j] = m[j][i];
	}
}

void matrix4x4_init_orthographic(matrix4x4 m, float left, float right, float bottom, float top, float near, float far) {
	sceClibMemset(m, 0, sizeof(matrix4x4));

	m[0][0] = 2.0f / (right - left);
	m[1][1] = 2.0f / (top - bottom);
	m[2][2] = -2.0f / (far - near);
	m[3][0] = -(right + left) / (right - left);
	m[3][1] = -(top + bottom) / (top - bottom);
	m[3][2] = -(far + near) / (far - near);
	m[3][3] = 1.0f;
}

void matrix4x4_init_frustum(matrix4x4 m, float left, float right, float bottom, float top, float near, float far) {
	sceClibMemset(m, 0, sizeof(matrix4x4));
	
	m[0][0] = (2.0f * near) / (right - left);
	m[1][1] = (2.0f * near) / (top - bottom);
	m[2][0] = (right + left) / (right - left);
	m[2][1] = (top + bottom) / (top - bottom);
	m[2][2] = -(far + near) / (far - near);
	m[2][3] = -1.0f;
	m[3][2] = (-2.0f * far * near) / (far - near);
}

void matrix4x4_init_perspective(matrix4x4 m, float fov, float aspect, float near, float far) {
	float half_height = near * tanf_neon(DEG_TO_RAD(fov) * 0.5f);
	float half_width = half_height * aspect;

	matrix4x4_init_frustum(m, -half_width, half_width, -half_height, half_height, near, far);
}

int matrix4x4_invert(matrix4x4 out, const matrix4x4 m) {
	int i, j;

	const float a0 = m[0][0] * m[1][1] - m[0][1] * m[1][0];
	const float a1 = m[0][0] * m[1][2] - m[0][2] * m[1][0];
	const float a2 = m[0][0] * m[1][3] - m[0][3] * m[1][0];
	const float a3 = m[0][1] * m[1][2] - m[0][2] * m[1][1];
	const float a4 = m[0][1] * m[1][3] - m[0][3] * m[1][1];
	const float a5 = m[0][2] * m[1][3] - m[0][3] * m[1][2];
	const float b0 = m[2][0] * m[3][1] - m[2][1] * m[3][0];
	const float b1 = m[2][0] * m[3][2] - m[2][2] * m[3][0];
	const float b2 = m[2][0] * m[3][3] - m[2][3] * m[3][0];
	const float b3 = m[2][1] * m[3][2] - m[2][2] * m[3][1];
	const float b4 = m[2][1] * m[3][3] - m[2][3] * m[3][1];
	const float b5 = m[2][2] * m[3][3] - m[2][3] * m[3][2];

	float det = a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0;

	if (fabsf(det) > 0.0001f) {
		out[0][0] = m[0][1] * b5 - m[1][2] * b4 + m[1][3] * b3;
		out[1][0] = -m[1][0] * b5 + m[1][2] * b2 - m[1][3] * b1;
		out[2][0] = m[1][0] * b4 - m[1][1] * b2 + m[1][3] * b0;
		out[3][0] = -m[1][0] * b3 + m[1][1] * b1 - m[1][2] * b0;
		out[0][1] = -m[0][1] * b5 + m[0][2] * b4 - m[0][3] * b3;
		out[1][1] = m[0][0] * b5 - m[0][2] * b2 + m[0][3] * b1;
		out[2][1] = -m[0][0] * b4 + m[0][1] * b2 - m[0][3] * b0;
		out[3][1] = m[0][0] * b3 - m[0][1] * b1 + m[0][2] * b0;
		out[0][2] = m[3][1] * a5 - m[3][2] * a4 + m[3][3] * a3;
		out[1][2] = -m[3][0] * a5 + m[3][2] * a2 - m[3][3] * a1;
		out[2][2] = m[3][0] * a4 - m[3][1] * a2 + m[3][3] * a0;
		out[3][2] = -m[3][0] * a3 + m[3][1] * a1 - m[3][2] * a0;
		out[0][3] = -m[2][1] * a5 + m[2][2] * a4 - m[2][3] * a3;
		out[1][3] = m[2][0] * a5 - m[2][2] * a2 + m[2][3] * a1;
		out[2][3] = -m[2][0] * a4 + m[2][1] * a2 - m[2][3] * a0;
		out[3][3] = m[2][0] * a3 - m[2][1] * a1 + m[2][2] * a0;

		det = 1.0f / det;

		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++)
				out[i][j] *= det;
		}

		return 1;
	}

	return 0;
}

void vector4f_matrix4x4_mult(vector4f *u, const matrix4x4 m, const vector4f *v) {
	u->x = m[0][0] * v->x + m[1][0] * v->y + m[2][0] * v->z + m[3][0] * v->w;
	u->y = m[0][1] * v->x + m[1][1] * v->y + m[2][1] * v->z + m[3][1] * v->w;
	u->z = m[0][2] * v->x + m[1][2] * v->y + m[2][2] * v->z + m[3][2] * v->w;
	u->w = m[0][3] * v->x + m[1][3] * v->y + m[2][3] * v->z + m[3][3] * v->w;
}

void vector3f_cross_product(vector3f *r, const vector3f *v1, const vector3f *v2) {
    r->x = v1->y * v2->z - v1->z * v2->y;
    r->y = -v1->x * v2->z + v1->z * v2->x;
    r->z = v1->x * v2->y - v1->y * v2->x;
}

void vector4f_normalize(vector4f *v) {
	normalize4_neon((float *)v, (float *)v);
}
