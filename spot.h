/*
  spot: Utilities for UChicago CMSC 23700 Intro to Computer Graphics
  Copyright (C) 2012  University of Chicago; Author: Gordon Kindlmann

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software, to deal in the software without
  restriction, including without limitation the rights to use, copy,
  modify, merge, publish, distribute, sublicense, and/or sell copies
  of the software, and to permit persons to whom the software is
  furnished to do so, subject to the following condition: the above
  copyright notice and this permission notice shall be included in all
  copies or substantial portions of the software.
*/

/*
** "spot" is a set of functions to make it less annoying for students to start
** getting geometry rendered. The name "spot" is not significant or
** meaningful; its just short and pronounceable. Students are free to use as
** much or as little of this as they see fit; or to write their own wrapper
** functions around what is provided here.
**
** Students can also directly modify the spot files, although doing so is not
** necessary to complete the project, and modifications should be justified.
** 
*/

#ifndef SPOT_HAS_BEEN_INCLUDED
#define SPOT_HAS_BEEN_INCLUDED

#ifdef __APPLE__
#  include <OpenGL/gl3.h>
#else
#  include <GL/gl3.h>
#endif

#include <stdio.h>
#include <stdlib.h> /* for malloc, free */
#include <string.h> /* for memcpy */
#include <stdarg.h> /* for var-args */
#include <math.h>
#include <png.h>    /* for PNG image IO */

#define SPOT_TRUE 1
#define SPOT_FALSE 0

/* 
** The spot macros are a clumsy way of doing vector and matrix operations.
** You are expected to read through spotMacros.h once to see what
** functionality is available and to understand how it works; but feel free to
** implement your own functions instead of or on top of these
**
*/
#include "spotMacros.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
** This enum establishes our convention for how to index into the OpenGL
** generic vertex attributes.  Code for spotGeomGLInit, for example, uses
** these values.
*/
enum {
  spotVertAttrIndx_xyz,
  spotVertAttrIndx_rgb,
  spotVertAttrIndx_norm,
  spotVertAttrIndx_tex2,
  spotVertAttrIndx_tang,
};

/*
** The spotGeom struct contains geometric and OpenGL information needed to
** draw an object: The geometric information includes the per-vertex
** world-space positions (xyz) and normals (norm), the type of primitive they
** represent (ptype).
**
** We also use this to store the vertex buffer object IDs (or "buffer object
** names" in the parlance of OpenGL documentation) for these arrays
** (xyzBuffId, normBuffId, indxBuffId), as well for the vertex array object or
** VAO around all these buffers (vaoId).
**
** Finally, the object color (color), model transform (modelMatrix) and normal
** transform (normalMatrix) can be stored here, although you are responsible
** for writing the code to set and use them.
**
*/
typedef struct {
  /* ---------------------- Information independent of GPU representation */
  GLfloat *xyz;          /* vertNum position 3-vectors */
  GLfloat *rgb;          /* vertNum RGB colors */
  GLfloat *norm;         /* vertNum unit normal 3-vectors */
  GLfloat *tex2;         /* vertNum (s,t) texture coordinate 2-vectors */
  GLfloat *tang;         /* vertNum unit surface tangent 3-vectors */
  unsigned int vertNum;  /* logical size of arrays above (if non-NULL);
                            e.g. for 4 vertices,
                            ==> vertNum = 4
                            ==> norm allocated for 3*4 GLfloats
                            ==> tex2 allocated for 2*4 GLfloats */
  
  GLushort *indx;        /* all indices (into arrays above) for all primitives,
                            concatenated together into one array */
  unsigned int indxNum;  /* length of indx */

  unsigned int primNum;  /* there are primNum primitives */
  GLenum *ptype;         /* primitive ii has type ptype[ii], possible value:
                            GL_TRIANGLES
                            GL_TRIANGLE_STRIP
                            GL_TRIANGLE_FAN */
  unsigned int *icnt;    /* primitive ii has icnt[ii] vertex indices */
  GLfloat objColor[3],   /* uniform object color, may be overridden */
    Ka, Kd, Ks, shexp,   /* scalar coefficients for Phong lighting:
                            Ka: amount by which to reflect a white ambient
                            light (a light that is not itself adjusted).
                            Kd: amount of diffuse reflection.
                            Ks: amount of specular reflection.
                            shexp: shininess exponent.
                            Ka, Kd, and Ks are normally in range [0.0,1.0],
                            shexp are anything > 0.0, about 5 to 5000 */
    quaternion[4],       /* rotation of model coords */
    modelMatrix[16],     /* transformation of model coords */
    normalMatrix[9];     /* transformation of normals */
  GLint program;         /* if non-zero, specific shader program to use */
  /* ---------------------- Information reflecting current GPU state */
  GLuint vaoId,          /* for storing return of glGenVertexArrays */
    xyzBuffId,           /* for storing return of glGenBuffers */
    rgbBuffId,
    normBuffId,
    tex2BuffId,
    tangBuffId,
    indxBuffId;
} spotGeom;

/*
** The spotImage struct contains all the information about the image data
** and meta-data that may come from a PNG image file
*/
typedef struct {
  /* ---------------------- Information independent of GPU representation */
  unsigned int sizeC,    /* size of channel (# bytes per channel):
                            1 for uchar, 2 for ushort */
    sizeP,               /* size of pixel (# channels per pixel), meaning:
                            1: gray
                            2: gray, alpha
                            3: red, green, blue
                            4: red, green, blue, alpha */
    sizeX,               /* size along X (image width, # columns) */
    sizeY;               /* size along Y (image height, # rows) */
  union {                /* union of data pointers */
    void *v;             /* as void, from calloc() */
    unsigned char *uc;   /* as uchar, for 8-bit images */
    unsigned short *us;  /* as ushort, for 16-bit images */
  } data;
  /* ---------------------- Information reflecting current GPU state */
  GLuint textureId;      /* for storing return of glGenTextures */
} spotImage;

/* . . . descriptions of spot functions organized by file . . . */


/* --------------------- spotUtils.c --------------------- */
/* current time, as seconds since the start of the '70s */
extern double spotTime(void);
/* quaternion-related functions, see also SPOT_Q_* in spotMacros.h
** spotQuatToM3, spotQuatToM4: first normalize then convert to matrix
** spotQuatToAA, spotAAToQuat: convert between quaternion and (angle,axis);
**      rotate by angle around axis
** spotQuatExp, spotQuatLog: exponentiation and logarithm of quaternions
**      (the logarithm is a 3-vector not a quaternion), which for
**      clarity are actually implemented in terms of (angle,axis)
*/
extern void spotQuatToM3(GLfloat mm[9], const GLfloat qq[4]);
extern void spotQuatToM4(GLfloat mm[16], const GLfloat qq[4]);
extern GLfloat spotQuatToAA(GLfloat axis[3], const GLfloat qq[4]);
extern void spotAAToQuat(GLfloat qq[4], GLfloat angle, const GLfloat axis[3]);
extern void spotQuatExp(GLfloat qq[4], const GLfloat qlog[3]);
extern void spotQuatLog(GLfloat qlog[3], const GLfloat qq[4]);
/* print 3x3 and 4x4 matrices to stdout */
extern void spotM3print_f(const GLfloat m[9]);
extern void spotM4print_f(const GLfloat m[16]);
/* The spotError functions are used internally by other spot functions to
** record error messages:
**     * spotErrorAdd() to add one-line error message, using printf syntax.
**       The message should not end with '\n'.
**     * spotErrorPrint() to print to stderr the accumulated messages
**     * spotErrorClear() to free them
** If you have any functions that might generate errors, which are called by
** other functions that want to notice those errors, please feel fee to use
** these to simplify the work of describing those errors!  
*/
extern void spotErrorAdd(const char *fmt, ...)
#ifdef __GNUC__
__attribute__ ((format(printf,1,2)))
#endif
;
extern void spotErrorPrint(void);
extern void spotErrorClear(void);
/* spotStrdup is same as strdup(), but strdup() isn't ANSI C */
char *spotStrdup(const char *s);
/* spotGLErrorString returns a (const) string version of GL error values.
   You can spotStrdup() or print this string, but don't try to free() it. */
extern const char *spotGLErrorString(GLenum error);

/* spotReadFile(fname) returns contents of file fname as newly allocated
   string */
extern char *spotReadFile(const char *fname);
/* spotShaderNew(shtype, fname) creates and compiles a shader of type shtype
   (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER) from the source in file fname */
extern GLint spotShaderNew(GLint shtype, const char *fname);
/* spotProgramNew(vertFname, fragName, ...) creates a complete shader program
   by compiling and linking the vertex and fragment shaders from vertFname and
   fragFname.  Additionally, here is where you specify the generic vertex
   attribute indices that you OpenGL to associate with the variable names in
   those shaders.  You do this by supplying additional pairs of arguments; the
   first is the variable name (type char *), and the second is the variable's
   attribute index (type GLuint).  You MUST terminate the argument list with
   NULL (because we are using the fragile "var-args" mechanism of C, which
   requires some explicit indication of how many args have been passed) */
extern GLint spotProgramNew(const char *vertFname,
                            const char *fragFname,
                            ...);

/* --------------------- spotImage.c --------------------- */
extern spotImage *spotImageNew();
extern int spotImageLoadPNG(spotImage *img, char *fname);
extern int spotImageSavePNG(char *fname, spotImage *img);
extern int spotImageScreenshot(spotImage *img, int withAlpha);
extern int spotImageGLInit(spotImage *img);
extern int spotImageGLDone(spotImage *img);
extern spotImage *spotImageNix(spotImage *img);

/* --------------------- spotGeomShapes.c --------------------- */
/* All of these spotGeomNew* functions allocate and initialize a spotGeom
   struct to contain some object.  All the objects fit inside the
   [-1,1]x[-1,1]x[-1,1] cube in world space, and come with correct
   normals. There are three kinds of cubes to play with: Cube0 has a single
   normal at each corner; Cube1 has three normals at each corner (one for each
   face); and Softcube is a puffy cube with rounded edges and corners. */
extern spotGeom *spotGeomNewCube0(void);
extern spotGeom *spotGeomNewCube1(void);
extern spotGeom *spotGeomNewCone(void);
extern spotGeom *spotGeomNewSoftcylinder(void); /* formerly "Cylinder" */
extern spotGeom *spotGeomNewCylinder(void); /* hard circular edges */
extern spotGeom *spotGeomNewSphere(void);
extern spotGeom *spotGeomNewSoftcube(void);
/* The Ellipsoid is the result of scaling the Sphere by: 1.0 along X (first
  coordinate of world space), 0.5 along Y, and 0.2 along Z.  The normals have
  been correctly updated accordingly.  This might come in handy! */
extern spotGeom *spotGeomNewEllipsoid(void);
/* Just a single square (made of two triangles), which may be useful for
   testing texture mapping effects */
extern spotGeom *spotGeomNewSquare(void);


/* --------------------- spotGeomMethods.c --------------------- */
/* These functions provide the basic functionality required to work with and
   draw spotGeom structs.  The order of operations is:
       initialization: sgeom = spotGeomNew___();  (see above)
                       spotGeomGLInit(sgeom); 
       rendering loop: ... spotGeomDraw(sgeom); ...
       cleaning up:    spotGeomGLDone(sgeom);
                       sgeom = spotGeomNix(sgeom); (sets sgeom to NULL)
   Note that spotGeomDraw does NOT do anything to pass values to the 
   uniform variables of the shaders; you will have to handle this.
*/
extern int spotGeomGLInit(spotGeom *sgeom);
extern int spotGeomDraw(spotGeom *sgeom);
extern int spotGeomGLDone(spotGeom *sgeom);
extern spotGeom *spotGeomNix(spotGeom *sgeom);

/* ------------------------ spotProj3A.c ------------------------ */
/* New functions for Project 3 functionality */
/* spotImageCubeMapGLInit: initialize spotImage as a cube map */
extern int spotImageCubeMapGLInit(spotImage *img);
/* spotGeomTransform: apply given transform to the spotGeom,
 *   including appropriate transform of normal and tangents */
extern int spotGeomTransform(spotGeom *sgeom, const GLfloat xform[16]);
/* spotGeomColorRGB: apply color to all per-vertex colors in spotGeom */
extern int spotGeomColorRGB(spotGeom *sgem, const GLfloat RGB[3]);

#ifdef __cplusplus
}
#endif

#endif /* SPOT_HAS_BEEN_INCLUDED */
