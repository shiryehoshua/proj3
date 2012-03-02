/*
 * matrixFunctions.c: general transformations, as well as code to update the projection and uvn
 *                    matrices.
 */

#ifdef __APPLE__
#  include <OpenGL/gl3.h>
#else
#  include <GL/gl3.h>
#endif
#include "spot.h"

#include "callbacks.h"
#include "matrixFunctions.h"
#include "types.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327
#endif

extern context_t *gctx;

/* Printing */

void printM3(GLfloat m[3*3]) {
  fprintf(stderr, "%f\t%f\t%f\n", m[0], m[3], m[6]);
  fprintf(stderr, "%f\t%f\t%f\n", m[1], m[4], m[7]);
  fprintf(stderr, "%f\t%f\t%f\n", m[2], m[5], m[8]);
}

void printM4(GLfloat m[4*4]) {
  fprintf(stderr, "%f\t%f\t%f\t%f\n", m[ 0], m[ 4], m[ 8], m[12]);
  fprintf(stderr, "%f\t%f\t%f\t%f\n", m[ 1], m[ 5], m[ 9], m[13]);
  fprintf(stderr, "%f\t%f\t%f\t%f\n", m[ 2], m[ 6], m[10], m[14]);
  fprintf(stderr, "%f\t%f\t%f\t%f\n", m[ 3], m[ 7], m[11], m[15]);
}

/* Rotation */

void rotate_V3(GLfloat x[3], GLfloat a[3], GLfloat *s, size_t i) 
{
  GLfloat r[4*4], t[3];
  // set r, the rotation transformation around axis a
  GLfloat si=sinf(s[i]), co=cosf(s[i]);
  SPOT_M4_ROTATE_AROUND(r, a, co, si);
  // set x = r * x
  SPOT_M4V3_MUL(t, r, x);
  SPOT_V3_COPY(x, t);
}

void rotate_1st_V3(GLfloat x[3], GLfloat *uvn, GLfloat *s, size_t i)
{
  GLfloat u[3];
  copy_1st_V3(u, uvn);
  rotate_V3(x, u, s, i);
}

void rotate_2nd_V3(GLfloat x[3], GLfloat *uvn, GLfloat *s, size_t i)
{
  GLfloat v[3];
  copy_2nd_V3(v, uvn);
  rotate_V3(x, v, s, i);
}

void rotate_3rd_V3(GLfloat x[3], GLfloat *uvn, GLfloat *s, size_t i)
{
  GLfloat n[3];
  copy_3rd_V3(n, uvn);
  rotate_V3(x, n, s, i);
}

void rotate_1st_2nd_V3(GLfloat x[3], GLfloat *uvn, GLfloat *s, size_t i)
{
  GLfloat u[3], v[3];
  copy_1st_V3(u, uvn);
  copy_2nd_V3(v, uvn);
  s[i+1] *= -1;
  rotate_V3(x, u, s, i+1);
  rotate_V3(x, v, s, i);
}

void rotate_spotlight(GLfloat v, int i)
{
  GLfloat temp[3], w[3];

  // temp = from - at
  SPOT_V3_SUB(temp, gctx->spotlight.from, gctx->spotlight.at);

  // rotate from - at around the proper axis
  copy_V3(w, gctx->spotlight.uvn, i);
  rotate_V3(temp, w, &v, 0);
  SPOT_V3_ADD(gctx->spotlight.from, temp, gctx->spotlight.at);

  // rotate the light around the proper axis
  rotate_V3(gctx->lightDir, w, &v, 0);

  // rotate spotlight around the proper axis
  rotate_V3(gctx->spotlight.from, w, &v, 0);

  if (!gctx->spotlight.fixed)
    rotate_V3(gctx->spotlight.up, w, &v, 0);
}

void rotate_spotlight_U(GLfloat z)
{
  rotate_spotlight(z, 0);
}

void rotate_spotlight_V(GLfloat z) 
{
  rotate_spotlight(z, 1);
}

void rotate_spotlight_N(GLfloat x) 
{
  GLfloat temp[3], n[3], l;
  copy_3rd_V3(n, gctx->spotlight.uvn);
  // rotate the up vector
  SPOT_V3_COPY(temp, gctx->spotlight.up);
  rotate_V3(temp, n, &x, 0);
  SPOT_V3_NORM(gctx->spotlight.up, temp, l);
};

void rotate_view(GLfloat v, int i)
{
  GLfloat temp[3], w[3];

  // temp = from - at
  SPOT_V3_SUB(temp, gctx->camera.from, gctx->camera.at);

  // rotate from - at around the proper axis
  copy_V3(w, gctx->camera.uvn, i);
  rotate_V3(temp, w, &v, 0);
  SPOT_V3_ADD(gctx->camera.from, temp, gctx->camera.at);

  // rotate the light around the proper axis
  rotate_V3(gctx->lightDir, w, &v, 0);

  // rotate spotlight around the proper axis
  rotate_V3(gctx->spotlight.from, w, &v, 0);

  if (!gctx->camera.fixed)
    rotate_V3(gctx->camera.up, w, &v, 0);
}

void rotate_view_U(GLfloat z)
{
  rotate_view(z, 0);
}

void rotate_view_V(GLfloat z) 
{
  rotate_view(z, 1);
}

void rotate_view_N(GLfloat x) 
{
  GLfloat temp[3], n[3], l;
  copy_3rd_V3(n, gctx->camera.uvn);
  // rotate the up vector
  SPOT_V3_COPY(temp, gctx->camera.up);
  rotate_V3(temp, n, &x, 0);
  SPOT_V3_NORM(gctx->camera.up, temp, l);
};

void rotate_model(spotGeom *obj, GLfloat t, size_t i) 
{
  GLfloat angle, axis[3], quat[4], newquat[4];

  // calculate angle of rotation
  angle = M_PI * 2.0f * t;

  // extract axis of rotation
  copy_V3(axis, gctx->camera.uvn, i);
  spotAAToQuat(quat, angle, axis);

  // apply rotation
  SPOT_Q_MUL(newquat, quat, obj->quaternion);
  SPOT_V4_COPY(obj->quaternion, newquat);
}

void rotate_model_N(GLfloat t)
{
  rotate_model(gctx->geom[gctx->gi], -t, 2);
}

void rotate_model_V(GLfloat t)
{
  rotate_model(gctx->geom[gctx->gi], -t, 1);
}

void rotate_model_U(GLfloat t)
{
  rotate_model(gctx->geom[gctx->gi], -t, 0);
}

void rotate_model_UV(GLfloat x, GLfloat y)
{
  rotate_model_U(y);
  rotate_model_V(x);
}

/* Wrapped functions to be passed to mouseFun.f */

void m_rotate_1st_V3(GLfloat *t, GLfloat *s, size_t i)
{
  rotate_1st_V3(t, gctx->camera.uvn, s, i);
} 

void m_rotate_2nd_V3(GLfloat *t, GLfloat *s, size_t i)
{
  rotate_2nd_V3(t, gctx->camera.uvn, s, i);
}

void m_rotate_3rd_V3(GLfloat *t, GLfloat *s, size_t i)
{
  rotate_3rd_V3(t, gctx->camera.uvn, s, i);
} 

void m_rotate_1st_2nd_V3(GLfloat *t, GLfloat *s, size_t i)
{
  rotate_1st_2nd_V3(t, gctx->camera.uvn, s, i);
}

void m_rotate_spotlight_U(GLfloat *t, GLfloat *s, size_t i)
{
  rotate_spotlight_U(s[i]);
}

void m_rotate_spotlight_V(GLfloat *t, GLfloat *s, size_t i)
{
  rotate_spotlight_V(s[i]);
}

void m_rotate_spotlight_N(GLfloat *t, GLfloat *s, size_t i)
{
  rotate_spotlight_N(s[i]);
}

void m_rotate_spotlight_UV(GLfloat *t, GLfloat *s, size_t i)
{
  rotate_spotlight_U(s[i+1]);
  rotate_spotlight_V(-s[i]);
}

void m_rotate_view_U(GLfloat *t, GLfloat *s, size_t i)
{
  rotate_view_U(s[i]);
}

void m_rotate_view_V(GLfloat *t, GLfloat *s, size_t i)
{
  rotate_view_V(s[i]);
}

void m_rotate_view_N(GLfloat *t, GLfloat *s, size_t i)
{
  rotate_view_N(s[i]);
}

void m_rotate_view_UV(GLfloat *t, GLfloat *s, size_t i)
{
  rotate_view_U(s[i+1]);
  rotate_view_V(-s[i]);
}

void m_rotate_model_N(GLfloat *t, GLfloat *s, size_t i)
{
  rotate_model_N(s[i]);
}

void m_rotate_model_UV(GLfloat *t, GLfloat *s, size_t i)
{
  rotate_model_UV(-s[i], s[i+1]);
}

/* Translation */

void translate(GLfloat xyzw[4*4], GLfloat v[3])
{
  GLfloat translation[4*4], t[4*4], v4[4];
  SPOT_M4_IDENTITY(translation);
  SPOT_V4_SET(v4, v[0], v[1], v[2], 1);
  SPOT_M4_COL3_SET(translation, v4);
  SPOT_M4_MUL(t, xyzw, translation);
  SPOT_M4_SET_2(xyzw, t);
}

void translate_model_UV(GLfloat *t, GLfloat *s, size_t i)
{
  t=gctx->geom[gctx->gi]->modelMatrix;
  GLfloat u[3], v[3], m[3], l;
  copy_1st_V3(u, gctx->camera.uvn);
  SPOT_V3_NORM(m, u, l);
  m[0] *= s[i];
  m[1] *= s[i];
  m[2] *= s[i];
  translate(t, m);
  copy_2nd_V3(v, gctx->camera.uvn);
  SPOT_V3_NORM(m, v, l);
  m[0] *= s[i+1];
  m[1] *= s[i+1];
  m[2] *= s[i+1];
  translate(t, m);
}

void translate_model_N(GLfloat *t, GLfloat *s, size_t i)
{
  t=gctx->geom[gctx->gi]->modelMatrix;
  GLfloat n[3], m[3], l;
  SPOT_V3_SUB(n, gctx->camera.from, gctx->camera.at);
  SPOT_V3_NORM(m, n, l);
  m[0] *= s[i];
  m[1] *= s[i];
  m[2] *= s[i];
  translate(t, m);
}

void translate_view_UV(GLfloat *t, GLfloat *s, size_t i)
{
  GLfloat u[3], v[3], m[3], l;
  copy_1st_V3(u, gctx->camera.uvn);
  SPOT_V3_NORM(m, u, l);
  gctx->camera.from[0] += s[i]*m[0];
  gctx->camera.at[0] += s[i]*m[0];
  gctx->camera.from[1] += s[i]*m[1];
  gctx->camera.at[1] += s[i]*m[1];
  gctx->camera.from[2] += s[i]*m[2];
  gctx->camera.at[2] += s[i]*m[2];
  copy_2nd_V3(v, gctx->camera.uvn);
  SPOT_V3_NORM(m, v, l);
  gctx->camera.from[0] += s[i+1]*m[0];
  gctx->camera.at[0] += s[i+1]*m[0];
  gctx->camera.from[1] += s[i+1]*m[1];
  gctx->camera.at[1] += s[i+1]*m[1];
  gctx->camera.from[2] += s[i+1]*m[2];
  gctx->camera.at[2] += s[i+1]*m[2];
}

void translate_view_N(GLfloat *t, GLfloat *s, size_t i)
{
  GLfloat n[3], m[3], l;
  SPOT_V3_SUB(n, gctx->camera.from, gctx->camera.at);
  SPOT_V3_NORM(m, n, l);
  gctx->camera.from[0] += s[i]*m[0];
  gctx->camera.at[0] += s[i]*m[0];
  gctx->camera.from[1] += s[i]*m[1];
  gctx->camera.at[1] += s[i]*m[1];
  gctx->camera.from[2] += s[i]*m[2];
  gctx->camera.at[2] += s[i]*m[2];
}

// NOTE: These are helper functions for wrapping in mouseFun_t structs; for example, these could
//       be used to translate the u, v or n components of gctx->camera.uvn.
void translate_1st_3D(GLfloat m[4*4], GLfloat *s, size_t i)
{
  GLfloat v3[3] = {s[i], 0, 0};
  translate(m, v3);
}

void translate_2nd_3D(GLfloat m[4*4], GLfloat *s, size_t i)
{
  GLfloat v3[3] = {0, s[i], 0};
  translate(m, v3);
}

void translate_3rd_3D(GLfloat m[4*4], GLfloat *s, size_t i)
{
  GLfloat v3[3] = {0, 0, s[i]};
  translate(m, v3);
}

void translate_1st_2nd_3D(GLfloat m[4*4], GLfloat *s, size_t i)
{
  GLfloat v3[3] = {s[i], s[i+1], 0};
  translate(m, v3);
}

void translate_1st_3rd_3D(GLfloat m[4*4], GLfloat *s, size_t i)
{
  GLfloat v3[3] = {s[i], 0, s[i+1]};
  translate(m, v3);
}

void translate_2nd_3rd_3D(GLfloat m[4*4], GLfloat *s, size_t i)
{
  GLfloat v3[3] = {0, s[i], s[i+1]};
  translate(m, v3);
}

/* Scaling */

void scale_near_far2(GLfloat *t, GLfloat *s, size_t i)
{
  gctx->spotlight.near += s[i];
  gctx->spotlight.far  -= s[i];
  gctx->spotlight.fov  -= 0.5*s[i];
//  gctx->camera.fov = atanf((tanf(gctx->camera.fov / 2)*(gctx->camera.near - s[i]))/gctx->camera.near);
  fprintf(stderr, "SPOTLIGHT: far - near = %f - %f = %f\nfov = %f\n",
    gctx->spotlight.far, gctx->spotlight.near,
    gctx->spotlight.far - gctx->spotlight.near,
    gctx->spotlight.fov);

//  callbackResize(gctx->winSizeX, gctx->winSizeY);

}

void scale_near_far(GLfloat *t, GLfloat *s, size_t i)
{
  gctx->camera.near += s[i];
  gctx->camera.far  -= s[i];
  gctx->camera.fov  -= 0.5*s[i];
//  gctx->camera.fov = atanf((tanf(gctx->camera.fov / 2)*(gctx->camera.near - s[i]))/gctx->camera.near);
  fprintf(stderr, "far - near = %f - %f = %f\nfov = %f\n",
    gctx->camera.far, gctx->camera.near,
    gctx->camera.far - gctx->camera.near,
    gctx->camera.fov);

  callbackResize(gctx->winSizeX, gctx->winSizeY);

}

void scale(GLfloat xyzw[4*4], GLfloat *s)
{
  GLfloat scale[4*4], t[4*4];
  SPOT_M4_IDENTITY(scale);
  scale[ 0] = s[0];
  scale[ 5] = s[0];
  scale[10] = s[0];
  SPOT_M4_MUL(t, xyzw, scale);
  SPOT_M4_SET_2(xyzw, t);
}

// NOTE: The following is a helper function that may be wrapped in a mouseFun_t struct; for
//       example, this could be used to update the fov of gctx->camera.
void scale_1D(GLfloat t[1], GLfloat *s, size_t i)
{
  t[0] *= s[i];
  if (t[0] >= 3.14) t[0] = 3.14;
  if (t[0] <= 0) t[0] = 0.1;
  callbackResize(gctx->winSizeX, gctx->winSizeY);
}

void translateGeomU(spotGeom *g, GLfloat s)
{
  GLfloat t[2];
  t[0]=s;t[1]=0;
  translate_1st_3D(g->modelMatrix, t, 0);
}

void translateGeomV(spotGeom *g, GLfloat s)
{
  GLfloat t[2];
  t[0]=s;t[1]=0;
  translate_2nd_3D(g->modelMatrix, t, 0);
}

void translateGeomN(spotGeom *g, GLfloat s)
{
  GLfloat t[2];
  t[0]=s;t[1]=0;
  translate_3rd_3D(g->modelMatrix, t, 0);
}

void scaleGeom(spotGeom *g, GLfloat s)
{
  GLfloat t[2];
  t[0]=s;t[1]=0;
  scale(g->modelMatrix, t);
}

void scaleGeomX(spotGeom *g, GLfloat s)
{
  GLfloat scale[4*4], t[4*4];
  SPOT_M4_IDENTITY(scale);
  scale[0] = s;
  SPOT_M4_MUL(t, g->modelMatrix, scale);
  SPOT_M4_SET_2(g->modelMatrix, t);
}

void scaleGeomY(spotGeom *g, GLfloat s)
{
  GLfloat scale[4*4], t[4*4];
  SPOT_M4_IDENTITY(scale);
  scale[5] = s;
  SPOT_M4_MUL(t, g->modelMatrix, scale);
  SPOT_M4_SET_2(g->modelMatrix, t);
}

void scaleGeomZ(spotGeom *g, GLfloat s)
{
  GLfloat scale[4*4], t[4*4];
  SPOT_M4_IDENTITY(scale);
  scale[10] = s;
  SPOT_M4_MUL(t, g->modelMatrix, scale);
  SPOT_M4_SET_2(g->modelMatrix, t);
}

/* Identity (AKA do nothing) */

void identity(GLfloat *t, GLfloat *s, size_t i)
{
  (void)(t);
  (void)(s);
  (void)(i);
}

/* Copy V3 functions */

void copy_V3(GLfloat u[3], GLfloat uvn[4*4], int i)
{
  SPOT_V3_SET(u, uvn[i], uvn[4+i], uvn[8+i]);
}

void copy_1st_V3(GLfloat u[3], GLfloat uvn[4*4])
{
  copy_V3(u, uvn, 0);
}

void copy_2nd_V3(GLfloat v[3], GLfloat uvn[4*4])
{
  copy_V3(v, uvn, 1);
}

void copy_3rd_V3(GLfloat n[3], GLfloat uvn[4*4])
{
  copy_V3(n, uvn, 2);
}

/*
 * The following are more specific transformations...
 *
 */

/* updateUVN: for use with gctx->camera.uvn */

void updateUVN(GLfloat uvn[4*4], GLfloat at[3], GLfloat from[3], GLfloat up[3])
{
  // Temporary vector and scalar
  GLfloat t[4], u[4], v[4], n[4], l;

  // Set n = at - fr / |at - fr|
  SPOT_V3_SUB(t, at, from);
  SPOT_V3_NORM(n, t, l); 
  n[3] = 0;
  SPOT_M4_ROW2_SET(uvn, n); // first column of uvn is n

  // Set u = n x up / | n x up |
  SPOT_V3_CROSS(t, n, up);
  SPOT_V3_NORM(u, t, l); 
  SPOT_M4_ROW0_SET(uvn, u); // uvn[0] == u

  // Set v = u x n
  SPOT_V3_CROSS(t, u, n);
  SPOT_V3_NORM(v, t, l); 
  SPOT_M4_ROW1_SET(uvn, v);

  // Set the fourth column
  SPOT_V4_SET(t, 
    -((u[0]*from[0]) + (u[1]*from[1]) + (u[2]*from[2])),
    -((v[0]*from[0]) + (v[1]*from[1]) + (v[2]*from[2])),
    -((n[0]*from[0]) + (n[1]*from[1]) + (n[2]*from[2])),
    1);
  SPOT_M4_COL3_SET(uvn, t);
}

void inverseUVN(GLfloat inverse_uvn[4*4], GLfloat uvn[4*4])
{
	GLfloat temp[3*3], inverse_temp[3*3];
	SPOT_M3M4_EXTRACT(temp, uvn);
	GLfloat det = SPOT_M3_DET(temp);
	if (det != 0) {
		SPOT_M3_INVERSE(inverse_temp, temp, det);
		SPOT_M4_SET(inverse_uvn,
			inverse_temp[0], inverse_temp[3], inverse_temp[6], 0,
			inverse_temp[1], inverse_temp[4], inverse_temp[7], 0,
			inverse_temp[2], inverse_temp[5], inverse_temp[8], 0,
			0, 0, 0, 1);
	}
}

/* updateProj: for use with gctx->camera.proj */

void updateProj(GLfloat m[4*4], GLfloat w, GLfloat h, GLfloat n, GLfloat f, int ortho)
{
  // NOTE: These variables are just to make the matrix below more readable. Compiler optimizations
  //       likely minimize or remove any performance cost associated with this.
  GLfloat i = f + n;
  GLfloat j = f - n;
  if (ortho) {
    m[ 0]=-2/w; m[ 4]=   0; m[ 8]=   0; m[12]=        0;
    m[ 1]=   0; m[ 5]= 2/h; m[ 9]=   0; m[13]=        0;
    m[ 2]=   0; m[ 6]=   0; m[10]= 2/j; m[14]= (-1)*i/j;
    m[ 3]=   0; m[ 7]=   0; m[11]=   0; m[15]=       1;
  } else {
    m[ 0]=-2*n/w; m[ 4]=     0; m[ 8]=   0; m[12]=          0;
    m[ 1]=     0; m[ 5]= 2*n/h; m[ 9]=   0; m[13]=          0;
    m[ 2]=     0; m[ 6]=     0; m[10]= i/j; m[11]= (-2)*f*n/j;
    m[ 3]=     0; m[ 7]=     0; m[14]=  -1; m[15]=          0;
  }
}

/* updateNormals: to be called on any transformed modelMatrix */

void updateNormals(GLfloat n[4*4], GLfloat m[3*3])
{
  GLfloat mat2[3*3], mat1[3*3], tmp;

  // mat1 = 3x3 model mat
  SPOT_M3M4_EXTRACT(mat1, m);

  // mat2 = mat1^-1
  SPOT_M3_INVERSE(mat2, mat1, tmp);

  // n = mat2^T
  SPOT_M3_TRANSPOSE(n, mat2);
}

/* Normalize homogenous coordinates */

void norm_M4(GLfloat m[4*4])
{
  if (m[15]!=1) {
    m[ 0]/=m[15]; m[ 1]/=m[15]; m[ 2]/=m[15]; m[ 3]/=m[15];
    m[ 4]/=m[15]; m[ 5]/=m[15]; m[ 6]/=m[15]; m[ 7]/=m[15];
    m[ 8]/=m[15]; m[ 9]/=m[15]; m[10]/=m[15]; m[11]/=m[15];
    m[12]/=m[15]; m[13]/=m[15]; m[14]/=m[15]; m[15]/=m[15];
  }
}

void set_model_transform(GLfloat m[4*4], spotGeom *obj)
{
  GLfloat temp[16];

  // extract matrix quaternion
  spotQuatToM4(temp, obj->quaternion);

  // apply rotation quaternion, then apply model transform
  SPOT_M4_MUL(m, obj->modelMatrix, temp);
}
