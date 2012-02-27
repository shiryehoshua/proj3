/*
 * matrixFunctions.h: general transformations, as well as code to update the projection and uvn
 *                    matrices.
 */
#ifndef MATRIXFUNCTIONS_HAS_BEEN_INCLUDED
#define MATRIXFUNCTIONS_HAS_BEEN_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __APPLE__
#  include <OpenGL/gl3.h>
#else
#  include <GL/gl3.h>
#endif

#include "types.h"

void printM3(GLfloat m[3*3]);
void printM4(GLfloat m[4*4]);

void rotate_V3(GLfloat x[3], GLfloat a[3], GLfloat *s, size_t i);
void rotate_1st_V3(GLfloat x[3], GLfloat *uvn, GLfloat *s, size_t i);
void rotate_2nd_V3(GLfloat x[3], GLfloat *uvn, GLfloat *s, size_t i);
void rotate_3rd_V3(GLfloat x[3], GLfloat *uvn, GLfloat *s, size_t i);
void rotate_1st_2nd_V3(GLfloat x[3], GLfloat *uvn, GLfloat *s, size_t i);

void rotate_view(GLfloat v, int i);
void rotate_view_U(GLfloat z);
void rotate_view_V(GLfloat z);
void rotate_view_N(GLfloat x);

void m_rotate_1st_V3(GLfloat *t, GLfloat *s, size_t i);
void m_rotate_2nd_V3(GLfloat *t, GLfloat *s, size_t i);
void m_rotate_3rd_V3(GLfloat *t, GLfloat *s, size_t i);
void m_rotate_1st_2nd_V3(GLfloat *t, GLfloat *s, size_t i);
void m_rotate_view_U(GLfloat *t, GLfloat *s, size_t i);
void m_rotate_view_V(GLfloat *t, GLfloat *s, size_t i);
void m_rotate_view_N(GLfloat *t, GLfloat *s, size_t i);
void m_rotate_view_UV(GLfloat *t, GLfloat *s, size_t i);

void translate(GLfloat xyzw[4*4], GLfloat v[3]);

void translate_model_UV(GLfloat *t, GLfloat *s, size_t i);
void translate_model_N(GLfloat *t, GLfloat *s, size_t i);
void translate_view_UV(GLfloat *t, GLfloat *s, size_t i);
void translate_view_N(GLfloat *t, GLfloat *s, size_t i);

void translate_1st_3D(GLfloat m[4*4], GLfloat *s, size_t i);
void translate_2nd_3D(GLfloat m[4*4], GLfloat *s, size_t i);
void translate_3rd_3D(GLfloat m[4*4], GLfloat *s, size_t i);
void translate_1st_2nd_3D(GLfloat m[4*4], GLfloat *s, size_t i);
void translate_1st_3rd_3D(GLfloat m[4*4], GLfloat *s, size_t i);
void translate_2nd_3rd_3D(GLfloat m[4*4], GLfloat *s, size_t i);

void scale_near_far(GLfloat *f, GLfloat *s, size_t i);

void scale(GLfloat xyzw[4*4], GLfloat *s);

void scale_1D(GLfloat t[1], GLfloat *s, size_t i);

void translateGeomU(spotGeom *g, GLfloat s);
void translateGeomV(spotGeom *g, GLfloat s);
void translateGeomN(spotGeom *g, GLfloat s);

void scaleGeom(spotGeom *g, GLfloat s);
void scaleGeomX(spotGeom *g, GLfloat s);
void scaleGeomY(spotGeom *g, GLfloat s);
void scaleGeomZ(spotGeom *g, GLfloat s);

void identity(GLfloat *t, GLfloat *s, size_t i);

void copy_V3(GLfloat u[3], GLfloat uvn[4*4], int i);
void copy_1st_V3(GLfloat u[3], GLfloat uvn[4*4]);
void copy_2nd_V3(GLfloat v[3], GLfloat uvn[4*4]);
void copy_3rd_V3(GLfloat n[3], GLfloat uvn[4*4]);

void updateUVN(GLfloat uvn[4*4], GLfloat at[3], GLfloat from[3], GLfloat up[3]);
void updateProj(GLfloat m[4*4], GLfloat w, GLfloat h, GLfloat n, GLfloat f, int ortho);

void updateNormals(GLfloat n[4*4], GLfloat m[3*3]);

void norm_M4(GLfloat m[4*4]);

#ifdef __cplusplus
}
#endif

#endif /* MATRIXFUNCTIONS_HAS_BEEN_INCLUDED */
