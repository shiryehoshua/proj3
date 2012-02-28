// Project 2: by Mark (andrus) and Shir (shiryehoshua)
//
// Please see Mark's dir for source (andrus)
//
//
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h> // For UCHAR_MAX and friends...

#define __gl_h_
#define GLFW_NO_GLU // Tell glfw.h not to include GLU header
#include <GL/glfw.h>
#undef GLFW_NO_GLU
#undef __gl_h_

#include <AntTweakBar.h>

// Local includes
#include "callbacks.h"
#include "matrixFunctions.h"
#include "spot.h"
#include "types.h"

// NOTE: this is how we support our stack of shaders; we define each we want to load in
//       `glInitContext()' of our program, load them once, and leave them attached until the app
//       terminates
const char *vertFnames[NUM_PROGRAMS], // Our list of shaders (populated in `contextGLInit()');
           *fragFnames[NUM_PROGRAMS]; // see `types.h' for the definition of NUM_PROGRAMS
int programIds[NUM_PROGRAMS+1];       // List of corresponding program ids (for `glUseProgram()')

// Global context
context_t *gctx = NULL;

// Values for tweak bar
TwType twBumpMappingModes, twFilteringModes;
TwEnumVal twBumpMappingModesEV[]={{Disabled, "Disabled"},
                                  {Bump, "Bump"},
                                  {Parallax, "Parallax"}},
          twFilteringModesEV[]  ={{Nearest, "Nearest"},
                                  {Linear, "Linear"},
                                  {NearestWithMipmap, "NearestWithMipmap"},
                                  {LinearWithMipmap, "LinearWithMipmap"}};

// NOTE: we'd prefer to only draw one shape at a time, while keeping a sphere and
//       square in memory. This variable gets referenced in contextDraw and does just
//       that...
int sceneGeomOffset=0;

// NOTE: the following supports per-vertex texturing. We set the RGB values at each vertex, and
//       our shaders linearly interpolate the values, giving it a (sick) low-res look
int perVertexTexturing() {
  int i, v;
  if (gctx->perVertexTexturingMode) {
    // We are coloring the vertices for each geom
    for (i=0; i<gctx->geomNum; i++) {
      int sizeC=gctx->image[i]->sizeC,            // channel size (e.g., 8- or 16-bit?)
          maxVal=sizeC==1?UCHAR_MAX:USHRT_MAX,    // max value of a channel (e.g. 255)
          sizeX=gctx->image[i]->sizeX,            // width of image, aka number of columns
          sizeY=gctx->image[i]->sizeY,            // height of image, aka number of rows
          sizeP=gctx->image[i]->sizeP,            // sizeP == number of channels (e.g., 3 for RGB)
          sizeOfPixel=sizeP*sizeC,                // sizeOfPixel == num of channels * channel size
          sizeOfRow=sizeX*sizeOfPixel;            // sizeOfRow (for the img_y offset)
      // NOTE: even though we are casting data.us to an array of unsigned chars, we explicitly
      //       handle the memory locations, so this is not a trip-up
      unsigned char *data = sizeC==1 ? gctx->image[i]->data.uc
        : (unsigned char*) gctx->image[i]->data.us;
      // NOTE: now we cycle through the vertices of the i-th geom, converting each vertex's
      //       texture coordinates into pixel coordinates, and finally into in-image memory
      //       locations; then we write the vertex's RGB component, transformed from the range of
      //       (0,maxVal) to (0.0,1.0)
      for (v=0; v<gctx->geom[i]->vertNum; v++) {
        GLfloat s=gctx->geom[i]->tex2[2*v],       // (s,t) texture coordinates of a vertex, v
                t=gctx->geom[i]->tex2[2*v+1];
        int x=s*(sizeX-1),                        // (x,y) location of a pixel in the image
            y=t*(sizeY-1),
            img_x=x*sizeOfPixel,                  // memory location of the (x,y) pixel, given the
            img_y=y*sizeOfRow;                    // size of a pixel
        // NOTE: array indexing in the following way is much clearer than the bracket notation for
        //       this application; we are dealing with images of differing bits, so it's better to
        //       just add up the offsets explicitly defined above
        GLfloat r=(float)(*(data+img_y+img_x+sizeC*0))/maxVal, // scale these from the (0,maxVal)
                g=(float)(*(data+img_y+img_x+sizeC*1))/maxVal, // to (0.0,1.0)
                b=(float)(*(data+img_y+img_x+sizeC*2))/maxVal;
        // Set the vertex-specific RGB values
        gctx->geom[i]->rgb[v*3+0]=r;
        gctx->geom[i]->rgb[v*3+1]=g;
        gctx->geom[i]->rgb[v*3+2]=b;
      }
      // NOTE: we need to update the OpenGL buffer location for this geom's per-vertex RGB values,
      //       otherwise none of this work will be evident in the shaders
      glBindBuffer(GL_ARRAY_BUFFER, gctx->geom[i]->rgbBuffId);
      glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*gctx->geom[i]->vertNum*3, gctx->geom[i]->rgb,
          GL_STATIC_DRAW);
    }
  } else {
    // NOTE: we reset the per-vertex RGB values for each geom to 1
    for (i=0; i<gctx->geomNum; i++) {
      for (v=0; v<gctx->geom[i]->vertNum; v++)
        gctx->geom[i]->rgb[v*3+0]=gctx->geom[i]->rgb[v*3+1]=gctx->geom[i]->rgb[v*3+2]=1;
      // NOTE: we need to update the OpenGL buffer location for this geom's per-vertex RGB values,
      //       otherwise none of this work will be evident in the shaders
      glBindBuffer(GL_ARRAY_BUFFER, gctx->geom[i]->rgbBuffId);
      glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*gctx->geom[i]->vertNum*3, gctx->geom[i]->rgb,
          GL_STATIC_DRAW);
    }
  }
  return gctx->perVertexTexturingMode;
}

/* Creates a context around geomNum spotGeom's and
   imageNum spotImage's */
context_t *contextNew(unsigned int geomNum, unsigned int imageNum) {
  const char me[]="contextNew";
  context_t *ctx;
  unsigned int gi;
  
  ctx = (context_t *)calloc(1, sizeof(context_t));
  if (!ctx) {
    spotErrorAdd("%s: couldn't alloc context?", me);
    return NULL;
  }

  ctx->vertFname = NULL;
  ctx->fragFname = NULL;
  if (geomNum) {
    ctx->geom = (spotGeom **)calloc(geomNum, sizeof(spotGeom*));
    if (!ctx->geom) {
      spotErrorAdd("%s: couldn't alloc %u geoms", me, geomNum);
      free(ctx); return NULL;
    }
    for (gi=0; gi<geomNum; gi++) {
      ctx->geom[gi] = NULL;
    }
  } else {
    ctx->geom = NULL;
  }
  ctx->geomNum = geomNum;
  if (imageNum) {
    ctx->image = (spotImage **)calloc(imageNum, sizeof(spotImage*));
    if (!ctx->image) {
      spotErrorAdd("%s: couldn't alloc %u images", me, imageNum);
      free(ctx); return NULL;
    }
    for (gi=0; gi<imageNum; gi++) {
      ctx->image[gi] = spotImageNew();
    }
  } else {
    ctx->image = NULL;
  }
  ctx->imageNum = imageNum;
  SPOT_V3_SET(ctx->bgColor, 0.2f, 0.25f, 0.3f);
  SPOT_V3_SET(ctx->lightDir, 1.0f, 0.0f, 0.0f);
  SPOT_V3_SET(ctx->lightColor, 1.0f, 1.0f, 1.0f);
  ctx->running = 1;
  ctx->program = 0;
  ctx->winSizeX = 900;
  ctx->winSizeY = 700;
  ctx->tbarSizeX = 200;
  ctx->tbarSizeY = 300;
  ctx->tbarMargin = 20;
  ctx->lastX = ctx->lastY = -1;
  ctx->buttonDown = 0;
  ctx->shiftDown = 0;

  // NOTE: here we make our sphere and square and load our image and bump map
  if (2 == geomNum && 4 == imageNum ) {
    ctx->geom[0] = spotGeomNewSphere();
    ctx->geom[1] = spotGeomNewSquare();
    scaleGeom(ctx->geom[0], 0.25);
    scaleGeom(ctx->geom[1], 0.25);
    spotImageLoadPNG(ctx->image[0], "textimg/uchic-rgb.png");     // texture
    spotImageLoadPNG(ctx->image[1], "textimg/uchic-norm08.png");  // bump map
    spotImageLoadPNG(ctx->image[2], "textimg/uchic-hght08.png");
    spotImageLoadPNG(ctx->image[3], "textimg/check-rgb.png");
    ctx->geom[0]->Kd = 0.3;
    ctx->geom[0]->Ks = 0.3;
    ctx->geom[0]->Ka = 0.3;
  }
  return ctx;
}

// NOTE: it makes sense to let this be its own function, since we need to call it upon changing
//       gctx->program in our shaders
void setUnilocs() {
  /* Learn (once) locations of uniform variables that we will
     frequently set */
#define SET_UNILOC(V) gctx->uniloc.V = glGetUniformLocation(gctx->program, #V)
      SET_UNILOC(lightDir);
      SET_UNILOC(lightColor);
      SET_UNILOC(modelMatrix);
      SET_UNILOC(normalMatrix);
      SET_UNILOC(viewMatrix);
      SET_UNILOC(projMatrix);
      SET_UNILOC(objColor);
      SET_UNILOC(gi);
      SET_UNILOC(Ka);
      SET_UNILOC(Kd);
      SET_UNILOC(Ks);
      SET_UNILOC(gouraudMode);
      SET_UNILOC(seamFix);
      SET_UNILOC(shexp);
      SET_UNILOC(samplerA);
      SET_UNILOC(samplerB);
      SET_UNILOC(samplerC);
      SET_UNILOC(samplerD);
#undef SET_UNILOC;
}

int contextGLInit(context_t *ctx) {
  const char me[]="contextGLInit";
  unsigned int ii, i;

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glDisable(GL_CULL_FACE); // No backface culling for now
  glEnable(GL_DEPTH_TEST); // Yes, do depth testing

  /* Create shader program.  Note that the names of per-vertex attributes
     are specified here.  This includes  vertPos and vertNorm from last project
     as well as new vertTex2 (u,v) per-vertex texture coordinates, and the
     vertTang per-vertex surface tangent 3-vector. */

  // NOTE: here is our shader "stack"; the ID_${shader} definitions allow easy retrieval of the
  //       program id from the programIds array after `glLinkProgram' calls
  vertFnames[ID_SIMPLE]="simple.vert";
  fragFnames[ID_SIMPLE]="simple.frag";
  vertFnames[ID_PHONG]="phong.vert";
  fragFnames[ID_PHONG]="phong.frag";
  vertFnames[ID_TEXTURE]="texture.vert";
  fragFnames[ID_TEXTURE]="texture.frag";
  vertFnames[ID_BUMP]="bump.vert";
  fragFnames[ID_BUMP]="bump.frag";
  vertFnames[ID_PARALLAX]="parallax.vert";
  fragFnames[ID_PARALLAX]="parallax.frag";

  // NOTE: we loop for as many shaders as are in our "stack" (NUM_PROGRAMS), and then once more
  //       to pull in whatever shader was passed in via the terminal (or not, if we have
  //       ctx->vertName==NULL)
  const char *vertFname, *fragFname;
  for (i=0; i<=NUM_PROGRAMS-(ctx->vertFname==NULL?1:0); i++) {
    // NOTE: consider this the "invoked" or default shader paseed via the terminal; it will be
    //       loaded last, and thus the first shader visible
    if (i==NUM_PROGRAMS) {
      vertFname=ctx->vertFname;
      fragFname=ctx->fragFname;
    // otherwise, we want to load our "stack" of shaders
    } else {
      vertFname = vertFnames[i];
      fragFname = fragFnames[i];
    }
    // NOTE: use `spotProgramNew' to handle all the `glLinkProgram' specifics; we also specify the
    //       per-vertex attributes we need
    ctx->program = spotProgramNew(vertFname, fragFname,
                                  "vertPos", spotVertAttrIndx_xyz,
                                  "vertNorm", spotVertAttrIndx_norm,
                                  "vertTex2", spotVertAttrIndx_tex2,
                                  "vertRgb", spotVertAttrIndx_rgb,
                                  "vertTang", spotVertAttrIndx_tang,
                                  /* input name, attribute index pairs
                                     MUST BE TERMINATED with NULL */
                                  NULL);
    // NOTE: we save the program id for easy retrieval from our callbacks; i here corresponds to
    //       one of ID_SIMPLE, ID_PHONG, etc., so we can reset the gctx->program to
    //       programIds[ID_${shader}] to switch shaders
    programIds[i]=ctx->program;
    if (!ctx->program) {
      spotErrorAdd("%s: couldn't create shader program", me);
      return 1;
    } else {
      printf("%d: Program (%s,%s) loaded...\n", ctx->program, vertFname, fragFname);
    }
  }

  // NOTE: the following is equivalent to hitting '1' on the keyboard; i.e. default
  //       scene
  if (ctx->vertFname==NULL) {
    gctx->program=programIds[ID_PHONG];
    gctx->gouraudMode=1;
  }

  // NOTE: this sets the uniform locations for the _invoked_ shader
  setUnilocs();
  
  if (ctx->geom) {
    for (ii=0; ii<ctx->geomNum; ii++) {
      if (spotGeomGLInit(ctx->geom[ii])) {
        spotErrorAdd("%s: trouble with geom[%u]", me, ii);
        return 1;
      }
    }
  }
  if (ctx->image) {
    for (ii=0; ii<ctx->imageNum; ii++) {
      if (ctx->image[ii]->data.v) {
        // Only bother with GL init when image data has been set
        if (spotImageGLInit(ctx->image[ii])) {
          spotErrorAdd("%s: trouble with image[%u]", me, ii);
          return 1;
        }
      }
    }
  }

  // NOTE: set to view mode (default)
  gctx->viewMode = 1;
  gctx->modelMode = 0;
  gctx->lightMode = 0;
  gctx->gouraudMode = 1;
  gctx->seamFix = 0;
  gctx->minFilter = GL_NEAREST;
  gctx->magFilter = GL_NEAREST;
  gctx->perVertexTexturingMode=1; // start in perVertexTexturingMode
  perVertexTexturing();

  // NOTE: model initializations
  SPOT_M4_IDENTITY(gctx->model.xyzw);
  SPOT_M4_IDENTITY(gctx->model.custom);

  // NOTE: camera initializations
  SPOT_M4_IDENTITY(gctx->camera.uvn);
  SPOT_M4_IDENTITY(gctx->camera.proj);
  gctx->camera.ortho = 0; // start in perspective mode
  gctx->camera.fixed = 0;
  gctx->camera.fov = 1.57079633/10; // 90 degrees
  gctx->camera.near = -20;
  gctx->camera.far = 20;
  gctx->camera.up[0] = 0;
  gctx->camera.up[1] = 1;
  gctx->camera.up[2] = 0;
  gctx->camera.from[0] = 0;
  gctx->camera.from[1] = 0;
  gctx->camera.from[2] = -1;
  gctx->camera.at[0] = 0;
  gctx->camera.at[1] = 0;
  gctx->camera.at[2] = 0;

  // NOTE: Mouse function intializations
  gctx->mouseFun.m = NULL;
  gctx->mouseFun.f = identity;
  gctx->mouseFun.offset=gctx->mouseFun.multiplier=gctx->mouseFun.i = 0;

  return 0;
}

int contextGLDone(context_t *ctx) {
  const char me[]="contextGLDone";
  unsigned int ii;

  if (!ctx) {
    spotErrorAdd("%s: got NULL pointer", me);
    return 1;
  }
  if (ctx->geom) {
    for (ii=0; ii<ctx->geomNum; ii++) {
      spotGeomGLDone(ctx->geom[ii]);
    }
  }
  if (ctx->image) {
    for (ii=0; ii<ctx->imageNum; ii++) {
      if (ctx->image[ii]->data.v) {
        spotImageGLDone(ctx->image[ii]);
      }
    }
  }
  return 0;
}

context_t *contextNix(context_t *ctx) {
  unsigned int ii;

  if (!ctx) {
    return NULL;
  }
  if (ctx->geom) {
    for (ii=0; ii<ctx->geomNum; ii++) {
      spotGeomNix(ctx->geom[ii]);
    }
    free(ctx->geom);
  }
  if (ctx->image) {
    for (ii=0; ii<ctx->imageNum; ii++) {
      spotImageNix(ctx->image[ii]);
    }
    free(ctx->image);
  }
  free(ctx);
  return NULL;
}

int contextDraw(context_t *ctx) {
  const char me[]="contextDraw";
  unsigned int gi;

  /* re-assert which program is being used (AntTweakBar uses its own) */
  glUseProgram(ctx->program); 

  /* background color; setting alpha=0 means that we'll see the
     background color in the render window, but upon doing
     "spotImageScreenshot(img, SPOT_TRUE)" (SPOT_TRUE for "withAlpha")
     we'll get a meaningful alpha channel, so that the image can
     recomposited with a different background, or used in programs
     (including web browsers) that respect the alpha channel */
  glClearColor(ctx->bgColor[0], ctx->bgColor[1], ctx->bgColor[2], 0.0f);
  /* Clear the window and the depth buffer */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  /* The following will be useful when you want to use textures,
     especially two textures at once, here sampled in the fragment
     shader with "samplerA" and "samplerB".  There are some
     non-intuitive calls required to specify which texture data will
     be sampled by which sampler.  See OpenGL SuperBible (5th edition)
     pg 279.  Also, http://tinyurl.com/7bvnej3 is amusing and
     informative */

  // NOTE: recall that image[0] is "uchic-rgb.png"
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, ctx->image[0]->textureId);
  glUniform1i(ctx->uniloc.samplerA, 0);

  // NOTE: recall that image[0] is "uchic-norm08.png"
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, ctx->image[1]->textureId);
  glUniform1i(ctx->uniloc.samplerB, 1);

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, ctx->image[2]->textureId);
  glUniform1i(ctx->uniloc.samplerC, 2);

  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, ctx->image[3]->textureId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gctx->minFilter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gctx->magFilter);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ctx->image[3]->sizeX, ctx->image[3]->sizeY, 0,
      GL_RGB, GL_UNSIGNED_BYTE, ctx->image[3]->data.v);
  glGenerateMipmap(GL_TEXTURE_2D);
  glUniform1i(ctx->uniloc.samplerD, 3);

  // NOTE: we must normalize our UVN matrix
  norm_M4(gctx->camera.uvn);

  // NOTE: update our unilocs
  glUniformMatrix4fv(ctx->uniloc.viewMatrix, 1, GL_FALSE, gctx->camera.uvn);
  glUniformMatrix4fv(ctx->uniloc.projMatrix, 1, GL_FALSE, gctx->camera.proj);
  glUniform3fv(ctx->uniloc.lightDir, 1, ctx->lightDir);
  glUniform3fv(ctx->uniloc.lightColor, 1, ctx->lightColor);
  glUniform1i(ctx->uniloc.gouraudMode, ctx->gouraudMode);
  glUniform1i(ctx->uniloc.seamFix, ctx->seamFix);

  // NOTE: update our geom-specific unilocs
  for (gi=sceneGeomOffset; gi<ctx->geomNum-1+sceneGeomOffset; gi++) {
    // NOTE: we normalize the model matrix; while we may not need to, it is cheap to do so
    norm_M4(gctx->geom[gi]->modelMatrix);
    glUniformMatrix4fv(ctx->uniloc.modelMatrix, 1, GL_FALSE, ctx->geom[gi]->modelMatrix);
    // NOTE: we update normals in our `matrixFunctions.c' functions on a case-by-case basis
    // updateNormals(gctx->geom[gi]->normalMatrix, gctx->geom[gi]->normalMatrix);
    glUniformMatrix3fv(ctx->uniloc.normalMatrix, 1, GL_FALSE, ctx->geom[gi]->normalMatrix);
    //
    glUniform3fv(ctx->uniloc.objColor, 1, ctx->geom[gi]->objColor);
    glUniform1f(ctx->uniloc.Ka, ctx->geom[gi]->Ka);
    glUniform1f(ctx->uniloc.Kd, ctx->geom[gi]->Kd);
    glUniform1f(ctx->uniloc.Ks, ctx->geom[gi]->Ks);
    glUniform1i(ctx->uniloc.gi, gi);
    glUniform1f(ctx->uniloc.shexp, ctx->geom[gi]->shexp);
    spotGeomDraw(ctx->geom[gi]);
  }
  
  /* These lines are also related to using textures.  We finish by
     leaving GL_TEXTURE0 as the active unit since AntTweakBar uses
     that, but doesn't seem to explicitly select it */
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, 3);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, 2);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, 1);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);

  /* You are welcome to do error-checking with higher granularity than
     just once per render, in which case this error checking loop
     should be repackaged into its own function. */
  GLenum glerr = glGetError();
  if (glerr) {
    while (glerr) {
      spotErrorAdd("%s: OpenGL error %d (%s)", me, glerr, spotGLErrorString(glerr));
      glerr = glGetError();
    }
    return 1;
  }
  return 0;
}

// NOTE: we use a callback here, since toggling perVertexTexturing requires the loading
//       of different shaders (and thus updating unilocs)
static void TW_CALL setPerVertexTexturingCallback(const void *value, void *clientData) {
  gctx->perVertexTexturingMode = *((const int *) value);
  fprintf(stderr, gctx->perVertexTexturingMode ? "Per-vertex Texturing: ON\n" : "Per-vertex Texturing: OFF\n");
  if (perVertexTexturing()) {
    printf("\tLoading shader 'simple' with id=%d\n", programIds[ID_SIMPLE]);
    gctx->program=programIds[ID_SIMPLE];
  } else {
    printf("\tLoading shader 'texture' with id=%d\n", programIds[ID_TEXTURE]);
    gctx->program=programIds[ID_TEXTURE];
  }
  setUnilocs();
}

static void TW_CALL getPerVertexTexturingCallback(void *value, void *clientData) {
  *((int *) value) = gctx->perVertexTexturingMode;
}

// NOTE: we use a callback here, since toggling bumpMapping requires the loading
//       of different shaders (and thus updating unilocs); additionally, we ensure
//       parallaxMapping is off
static void TW_CALL setBumpMappingCallback(const void *value, void *clientData) {
  gctx->bumpMappingMode = *((const enum BumpMappingModes *) value);
  switch (gctx->bumpMappingMode) {
    case Bump:
      printf("\tLoading shader 'bump' with id=%d\n", programIds[ID_BUMP]);
      gctx->program=programIds[ID_BUMP];
      break;
    case Parallax:
      printf("\tLoading shader 'parallax' with id=%d\n", programIds[ID_PARALLAX]);
      gctx->program=programIds[ID_PARALLAX];
      break;
    default: // Disabled
      printf("\tLoading shader 'texture' with id=%d\n", programIds[ID_TEXTURE]);
      gctx->program=programIds[ID_TEXTURE];
  }
  setUnilocs();
}

static void TW_CALL getBumpMappingCallback(void *value, void *clientData) {
  *((int *) value) = gctx->bumpMappingMode;
}

static void TW_CALL setFilteringCallback(const void *value, void *clientData) {
  gctx->filteringMode = *((const enum FilteringModes *) value);
  switch (gctx->filteringMode) {
    case Nearest:
      gctx->minFilter=GL_NEAREST;
      gctx->magFilter=GL_NEAREST;
      printf("\tGL_NEAREST\n");
      break;
    case Linear:
      gctx->minFilter=GL_LINEAR;
      gctx->magFilter=GL_LINEAR;
      printf("\tGL_LINEAR\n");
      break;
    case NearestWithMipmap:
      gctx->minFilter=GL_NEAREST_MIPMAP_NEAREST;
      gctx->magFilter=GL_NEAREST;
      printf("\tGL_NEAREST & GL_NEAREST_MIPMAP_NEAREST\n");
      break;
    case LinearWithMipmap:
      gctx->minFilter=GL_LINEAR_MIPMAP_LINEAR;
      gctx->magFilter=GL_LINEAR;
      printf("\tGL_LINEAR & GL_LINEAR_MIPMAP_LINEAR\n");
      break;
    default:
      printf("\tDEFAULT\n");
  }
  setUnilocs();
}

static void TW_CALL getFilteringCallback(void *value, void *clientData) {
  *((int *) value) = gctx->filteringMode;
}

// NOTE: here are our tweak bar definitions
int updateTweakBarVars(int scene) {
  int EE=0;
  if (!EE) EE |= !TwRemoveAllVars(gctx->tbar);
  if (!EE) EE |= !TwAddVarRW(gctx->tbar, "Ka",
                             TW_TYPE_FLOAT, &(gctx->geom[0]->Ka),
                             " label='Ka' min=0.0 max=1.0 step=0.005");
  if (!EE) EE |= !TwAddVarRW(gctx->tbar, "Kd",
                             TW_TYPE_FLOAT, &(gctx->geom[0]->Kd),
                             " label='Kd' min=0.0 max=1.0 step=0.005");
  if (!EE) EE |= !TwAddVarRW(gctx->tbar, "Ks",
                             TW_TYPE_FLOAT, &(gctx->geom[0]->Ks),
                             " label='Ks' min=0.0 max=1.0 step=0.005");
  if (!EE) EE |= !TwAddVarRW(gctx->tbar, "shexp",
                             TW_TYPE_FLOAT, &(gctx->geom[0]->shexp),
                             " label='shexp' min=0.0 max=100.0 step=0.05");
  if (!EE) EE |= !TwAddVarRW(gctx->tbar, "bgColor",
                             TW_TYPE_COLOR3F, &(gctx->bgColor),
                             " label='bkgr color' ");
  switch (scene) {
    case 1:
      if (!EE) EE |= !TwAddVarRW(
           gctx->tbar, "shading",
           TW_TYPE_BOOL8, &(gctx->gouraudMode),
           " label='shading' true=Gouraud false=Phong ");
      break;
    case 2:
      if (!EE) EE |= !TwAddVarCB(
           gctx->tbar, "perVertexTexturing",
           TW_TYPE_BOOL8, setPerVertexTexturingCallback,
           getPerVertexTexturingCallback, &(gctx->perVertexTexturingMode),
           " label='per-vertex texturing' true=Enabled false=Disabled ");
      if (!EE) EE |= !TwAddVarRW(
           gctx->tbar, "seamFix",
           TW_TYPE_BOOL8, &(gctx->seamFix),
           " label='seam fix' true=Enabled false=Disabled ");
      break;
    case 3:
      if (!EE) EE |= !TwAddVarCB(
           gctx->tbar, "filteringMode",
           twFilteringModes, setFilteringCallback,
           getFilteringCallback, &(gctx->filteringMode),
           " label='filtering mode' ");
      break;
    case 4:
      if (!EE) EE |= !TwAddVarCB(
           gctx->tbar, "bumpMappingMode",
           twBumpMappingModes, setBumpMappingCallback,
           getBumpMappingCallback, &(gctx->bumpMappingMode),
           " label='bump mapping' ");
      break;
    default:
      break;
  }
  return EE;
}

int createTweakBar(context_t *ctx, int scene) {
  const char me[]="createTweakBar";
  char buff[128];
  int EE;  /* we have an error */

  EE = 0;

  // NOTE: these are nice to have
  twBumpMappingModes=TwDefineEnum("BumpMappingModes", twBumpMappingModesEV, 3);
  twFilteringModes=TwDefineEnum("FilteringModes", twFilteringModesEV, 4);
  
  /* Create a tweak bar for interactive parameter adjustment */
  if (!EE) EE |= !(ctx->tbar = TwNewBar(TBAR_NAME));
  /* documentation for the TwDefine parameter strings here:
     http://www.antisphere.com/Wiki/tools:anttweakbar:twbarparamsyntax */
  /* add a message to be seen in the "help" window */
  if (!EE) EE |= !TwDefine(" GLOBAL help='This description of Project 2 "
                           "has not been changed by anyone but students "
                           "are encouraged to write something descriptive "
                           "here.' ");
  /* change location where bar will be drawn, over to the right some
     to expose more of the left edge of window.  Note that we are
     exploiting the automatic compile-time concatentation of strings
     in C, which connects TBAR_NAME with the rest of the string to
     make one contiguous string */
  sprintf(buff, TBAR_NAME " position='%d %d' ",
          ctx->winSizeX - ctx->tbarSizeX - ctx->tbarMargin,
          ctx->tbarMargin);
  if (!EE) EE |= !TwDefine(buff);
  /* adjust other aspects of the bar */
  sprintf(buff, TBAR_NAME " color='0 0 0' alpha=10 size='%d %d' ",
          ctx->tbarSizeX, ctx->tbarSizeY);
  if (!EE) EE |= !TwDefine(buff);
  
  // NOTE: we broke this section out for easy update of tweak bar vars per-scene
  if (!EE) EE |= updateTweakBarVars(scene);

  /* see also:
     http://www.antisphere.com/Wiki/tools:anttweakbar:twtype
     http://www.antisphere.com/Wiki/tools:anttweakbar:twdefineenum
  */

  if (EE) {
    spotErrorAdd("%s: AntTweakBar initialization failed:\n\t%s", me, TwGetLastError());
    return 1;
  }
  return 0;
}

void usage(const char *me) {
  fprintf(stderr, "usage: %s [<vertshader> <fragshader>]\n", me);
  fprintf(stderr, "\tCall `%s', optionally taking a default pair of vertex and fragment\n", me);
  fprintf(stderr, "\tshaders to render. Otherwise we just load our stack of shaders.\n");
}

int main(int argc, const char* argv[]) {
  const char *me;
  me = argv[0];
  // NOTE: we now allow you to either pass in an "invoked" or default shader to render, or to let
  //       us just set up our stack; hence you either pass 2 additional arguments or none at all
  // NOTE: we aren't explicity defining this functionality, but obviously `proj2 -h' will show the
  //       usage pattern
  if (1!=argc && 3!=argc) {
    usage(me);
    exit(1);
  }

  if (!(gctx = contextNew(2, 4))) { // 4 Images!
    fprintf(stderr, "%s: context set-up problem:\n", me);
    spotErrorPrint();
    spotErrorClear();
    exit(1);
  }

  if (argc==3) {
    gctx->vertFname = argv[1];
    gctx->fragFname = argv[2];
  } else {
    // NOTE: if invoked with no shaders, set these to NULL; `contextGlInit()' will catch these
    gctx->vertFname = NULL;
    gctx->fragFname = NULL;
  }

  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
    exit(1);
  }

  /* Make sure we're using OpenGL 3.2 core.  NOTE: Changing away from
     OpenGL 3.2 core is not needed and not allowed for this project */
  glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
  glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 2);
  glfwOpenWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  if (!glfwOpenWindow(gctx->winSizeX, gctx->winSizeY, 0, 0, 0, 0, 32, 0, GLFW_WINDOW)) {
    fprintf(stderr, "Failed to open GLFW window\n");
    glfwTerminate();
    exit(1);
  }

  glfwSetWindowTitle("Project 2: Shady");
  glfwEnable(GLFW_MOUSE_CURSOR);
  glfwEnable(GLFW_KEY_REPEAT);
  glfwSwapInterval(1);

  /* Initialize AntTweakBar */
  if (!TwInit(TW_OPENGL_CORE, NULL)) {
    fprintf(stderr, "AntTweakBar initialization failed: %s\n",
            TwGetLastError());
    exit(1);
  }

  printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
  printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
  printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
  printf("PNG_LIBPNG_VER_STRING = %s\n", PNG_LIBPNG_VER_STRING);
  
  /* set-up and initialize the global context */
  if (contextGLInit(gctx)) {
    fprintf(stderr, "%s: context OpenGL set-up problem:\n", me);
    spotErrorPrint(); spotErrorClear();
    TwTerminate();
    glfwTerminate();
    exit(1);
  }

  // NOTE: when we create the tweak bar, either load in scene 1 or default, depending
  //       on whether we were passing a pair of shaders
  if (createTweakBar(gctx, (gctx->vertFname==NULL?1:0))) {
    fprintf(stderr, "%s: AntTweakBar problem:\n", me);
    spotErrorPrint(); spotErrorClear();
    TwTerminate();
    glfwTerminate();
    exit(1);
  }

  glfwSetWindowSizeCallback(callbackResize);
  glfwSetKeyCallback(callbackKeyboard);
  glfwSetMousePosCallback(callbackMousePos);
  glfwSetMouseButtonCallback(callbackMouseButton);

  /* Redirect GLFW mouse wheel events directly to AntTweakBar */
  glfwSetMouseWheelCallback((GLFWmousewheelfun)TwEventMouseWheelGLFW);
  /* Redirect GLFW char events directly to AntTweakBar */
  glfwSetCharCallback((GLFWcharfun)TwEventCharGLFW);

  /* Main loop */
  while (gctx->running) {
    // NOTE: we update UVN every step
    updateUVN(gctx->camera.uvn, gctx->camera.at, gctx->camera.from, gctx->camera.up);
    /* render */
    if (contextDraw(gctx)) {
      fprintf(stderr, "%s: trouble drawing:\n", me);
      spotErrorPrint(); spotErrorClear();
      /* Can comment out "break" so that OpenGL bugs are reported but
         do not lead to the termination of the program */
      /* break; */
    }
    /* Draw tweak bar last, just prior to buffer swap */
    if (!TwDraw()) {
      fprintf(stderr, "%s: AntTweakBar error: %s\n", me, TwGetLastError());
      break;
    }
    /* Display rendering results */
    glfwSwapBuffers();
    /* NOTE: don't call glfwWaitEvents() if you want to redraw continuously */
    glfwWaitEvents();
    /* quit if window was closed */
    if (!glfwGetWindowParam(GLFW_OPENED)) {
      gctx->running = 0;
    }
  }
  
  contextGLDone(gctx);
  contextNix(gctx);
  TwTerminate();
  glfwTerminate();

  exit(0);
}
