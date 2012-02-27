/*
 * types.h: modularizing the code (proj1.c got too big).
 *
 */
#ifndef TYPES_HAS_BEEN_INCLUDED
#define TYPES_HAS_BEEN_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __APPLE__
#  include <OpenGL/gl3.h>
#else
#  include <GL/gl3.h>
#endif

#include <AntTweakBar.h>

#include "spot.h"

/*
** A string to use as title bar name for the "tweak bar"
*/
#define TBAR_NAME "Project2-Params"

// Shaders are populated in our main
#define NUM_PROGRAMS 4
// Easy program lookup--refer to programIds[ID_${shader}] for the id to use with
//    `glLinkProgram'
#define ID_SIMPLE 0
#define ID_PHONG 1
#define ID_TEXTURE 2
#define ID_BUMP 3

/*
** The camera_t is a suggested storage place for all the parameters associated
** with determining how you see the scene, which is used to determine one of
** the transforms applied in the vertex shader.  Right now there are no helper
** functions to initialize or compute with the camera_t; that is up to you.
**
*/
typedef struct {
  GLfloat from[3],    /* location (in world-space) of eyepoint */
    at[3],            /* what point (in world-space) we are looking at */
    up[3],            /* what is up direction for eye (this is not updated to
                         the "true" up) */
    aspect,           /* the ratio of horizontal to vertical size of the view
                         window */
    fov,              /* The angle, in degrees, vertically subtended by the
                         near clipping plane */
    near, far;        /* near and far clipping plane distances.  Whether you
                         interpret these as relative to the eye "from" point
                         (the convention in graphics) or relative to the
                         "at" point (arguably more convenient) is up to you */
  int ortho,          /* (a boolean) no perspective projection: just
                         orthographic */
      fixed;
  GLfloat uvn[4*4];
  GLfloat proj[4*4];

  GLfloat wf, hf;

} camera_t;

typedef struct{
  GLfloat xyzw[4*4];
  GLfloat custom[4*4];
} model_t;

typedef struct {
  GLfloat *m;
  void (*f)(GLfloat*, GLfloat*, size_t);
  GLfloat offset, multiplier;
  int i;
} mouseFun_t;

/*
** The uniloc_t is a possible place to store "locations" of shader
** uniform variables, so they can be learned once and re-used once per
** render.  Modify as you see fit! 
*/
typedef struct {
  GLint modelMatrix;  /* same name as field in spotGeom */
  GLint normalMatrix; /* same name as field in spotGeom */
  GLint objColor;     /* same name as field in spotGeom */
  GLint Ka;           /* same name as field in spotGeom */
  GLint Kd;           /* same name as field in spotGeom */
  GLint Ks;           /* same name as field in spotGeom */
  GLint shexp;        /* same name as field in spotGeom */
  GLint gi;           /* index of spotGeom object */
  /* vvvvvvvvvvvvvvvvvvvvv YOUR CODE HERE vvvvvvvvvvvvvvvvvvvvvvvv */
  GLint viewMatrix;   /* possible name of view matrix in vertex shader */
  GLint projMatrix;   /* possible name of projection matrix in vertex shader */
  /* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
  GLint lightDir;     /* same name as field in context_t */
  GLint lightColor;   /* same name as field in context_t */
  GLint gouraudMode;  /* same name as field in context_t */
  GLint seamFix;
  GLint samplerA;     /* possible name of texture sampler in fragment shader */
  GLint samplerB;     /* possible name of texture sampler in fragment shader */
  GLint samplerC;     /* possible name of texture sampler in fragment shader */
} uniloc_t;

/*
** The context_t is a suggested storage place for what might otherwise be
** separate global variables (globals obscure the flow of information and are
** hence bad style).  Modify as you see fit.  Don't forget to respect the
** order of operations on spotGeom:
**     initialization: sgeom = spotGeomNewSphere(); (for example)
**                     spotGeomGLInit(sgeom);
**     rendering loop: ... spotGeomDraw(sgeom); ...
**     cleaning up:    spotGeomGLDone(sgeom);
**                     sgeom = spotGeomNix(sgeom); (sets sgeom to NULL)
*/
typedef struct {
  const char *vertFname,  /* file name of vertex shader */
    *fragFname;           /* file name of fragment shader */
  spotGeom **geom;        /* array of spotGeom's to render */
  unsigned int geomNum;   /* length of geom */
  spotImage **image;      /* array of texture images to use */
  unsigned int imageNum;  /* length of image */
  GLfloat bgColor[3];     /* background color */
  GLfloat lightDir[3];    /* direction pointing to light (at infinity) */
  GLfloat lightColor[3];  /* color of light */
  int running;            /* we exit when this is zero */
  GLint program;          /* the linked shader program */
  int winSizeX, winSizeY; /* size of rendering window */

  int tbarSizeX,          /* initial width of tweak bar */
    tbarSizeY,            /* initial height of tweak bar */
    tbarMargin;           /* margin between tweak bar and window */

  camera_t camera;        /* a camera */
  uniloc_t uniloc;       /* store of uniform locations */
  model_t model;

  int lastX, lastY;       /* coordinates of last known mouse position */
  int buttonDown,         /* mouse button is being held down */
    shiftDown;            /* shift was down at time of mouse click */
  int viewMode,           /* 1 when in view mode, 0 otherwise */
    modelMode,            /* 1 when in model mode, 0 otherwise */
    lightMode,            /* 1 when in light mode, 0 otherwise */
    gouraudMode,          /* 1 when in gouraud mode, 0 otherwise */
    perVertexTexturingMode,
    seamFix;
  TwBar *tbar;            /* pointer to the parameter "tweak bar" */
  /* vvvvvvvvvvvvvvvvvvvvv YOUR CODE HERE vvvvvvvvvvvvvvvvvvvvvvvv */
  /* (any other information about the state of mouse or keyboard
     input, geometry, camera, transforms, or anything else that may
     need to be accessed from anywhere */
  mouseFun_t mouseFun;
  /* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
  GLfloat vspNear, vspFar,  U[3], V[3], N[3];
} context_t;

#ifdef __cplusplus
}
#endif

#endif /* TYPES_HAS_BEEN_INCLUDED */
