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

#include "spot.h"


/* m2 = adjugate matrix of m1 = the transpose of the matrix of cofactors of m1
      = det(m1)*inv(m1) (when det(m1) is non-zero) */
#define SPOT_M3_ADJUGATE(m2, m1)        \
  ((m2)[0] =  _SPOT_M2_DET(m1,4,5,7,8), \
   (m2)[1] = -_SPOT_M2_DET(m1,1,2,7,8), \
   (m2)[2] =  _SPOT_M2_DET(m1,1,2,4,5), \
   (m2)[3] = -_SPOT_M2_DET(m1,3,5,6,8), \
   (m2)[4] =  _SPOT_M2_DET(m1,0,2,6,8), \
   (m2)[5] = -_SPOT_M2_DET(m1,0,2,3,5), \
   (m2)[6] =  _SPOT_M2_DET(m1,3,4,6,7), \
   (m2)[7] = -_SPOT_M2_DET(m1,0,1,6,7), \
   (m2)[8] =  _SPOT_M2_DET(m1,0,1,3,4))


static int isPowerOfTwo(unsigned int x) {
  return ((x != 0) && !(x & (x - 1)));
}

int spotImageCubeMapGLInit(spotImage *img) {
  const char me[]="spotImageCubeMapGLInit";
  unsigned int sizeY, sizeImage;
  GLenum type;

  if (3 != img->sizeP) {
    spotErrorAdd("%s: can only handle RGB cube map images (not sizeP %u)", 
                 me, img->sizeP);
    return 1;
  }
  if (!isPowerOfTwo(img->sizeX)) {
    spotErrorAdd("%s: image X dimension %u not power of two", me, img->sizeX);
    return 1;
  }
  sizeY = img->sizeY/6; /* actual size along Y */
  if (!( img->sizeY == 6*sizeY && isPowerOfTwo(sizeY) )) {
    spotErrorAdd("%s: image Y dimension %u not multiple of 6, "
                 "or %u/6=%u not power of two", me, img->sizeY, 
                 img->sizeY, sizeY);
    return 1;
  }

  type = (1 == img->sizeC
          ? GL_UNSIGNED_BYTE
          : GL_UNSIGNED_SHORT);
  glGenTextures(1, &(img->textureId));
  glBindTexture(GL_TEXTURE_CUBE_MAP, img->textureId);
  /* courtesy http://www.opengl.org/wiki/Common_Mistakes */
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 

  sizeImage = (img->sizeC)*(img->sizeP)*(img->sizeX)*sizeY;
  glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA8, img->sizeX, sizeY,
               0, GL_RGB, type, img->data.uc + 0*sizeImage);
  glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA8, img->sizeX, sizeY,
               0, GL_RGB, type, img->data.uc + 1*sizeImage);
  glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA8, img->sizeX, sizeY,
               0, GL_RGB, type, img->data.uc + 2*sizeImage);
  glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA8, img->sizeX, sizeY,
               0, GL_RGB, type, img->data.uc + 3*sizeImage);
  glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA8, img->sizeX, sizeY,
               0, GL_RGB, type, img->data.uc + 4*sizeImage);
  glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA8, img->sizeX, sizeY,
               0, GL_RGB, type, img->data.uc + 5*sizeImage);

  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

  return 0;
}

int spotGeomTransform(spotGeom *sgeom, const GLfloat vertXform[16]) {
  const char me[]="spotGeomTransform";
  GLfloat tangXform[9], normXform[9];
  GLfloat vec[3], vertA[4], vertB[4], vlen;
  unsigned int vi;
  
  if (!( sgeom && vertXform )) {
    spotErrorAdd("%s: got NULL pointer", me);
    return 1;
  }
  SPOT_M3M4_EXTRACT(tangXform, vertXform);
  SPOT_M3_ADJUGATE(normXform, tangXform);
  vertA[3] = 1.0f;
  for (vi=0; vi<sgeom->vertNum; vi++) {
    /* transform verts */
    SPOT_V3_COPY(vertA, sgeom->xyz + 3*vi);
    SPOT_M4V4_MUL(vertB, vertXform, vertA);
    SPOT_V3_SCALE(sgeom->xyz + 3*vi, 1.0f/vertB[3], vertB);
    /* transform normals */
    SPOT_M3V3_MUL(vec, normXform, sgeom->norm + 3*vi);
    SPOT_V3_NORM(sgeom->norm + 3*vi, vec, vlen);
    /* transform tangents */
    if (sgeom->tang) {
      SPOT_M3V3_MUL(vec, tangXform, sgeom->tang + 3*vi);
      SPOT_V3_NORM(sgeom->tang + 3*vi, vec, vlen);
    }
  }
  
  return 0;
}

int spotGeomColorRGB(spotGeom *sgeom, const GLfloat RGB[3]) {
  const char me[]="spotGeomColorRGB";
  unsigned int vi;

  if (!( sgeom && RGB )) {
    spotErrorAdd("%s: got NULL pointer", me);
    return 1;
  }
  for (vi=0; vi<sgeom->vertNum; vi++) {
    SPOT_V3_COPY(sgeom->rgb + 3*vi, RGB);
  }
  
  return 0;
}
