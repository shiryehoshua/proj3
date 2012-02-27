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

int spotGeomGLInit(spotGeom *sgeom) {

  /* Create an uninitialized vertex array object */
  glGenVertexArrays(1, &(sgeom->vaoId));

  /* Initialize vao and bind to it */
  glBindVertexArray(sgeom->vaoId);
   
  glGenBuffers(1, &(sgeom->xyzBuffId));
  glBindBuffer(GL_ARRAY_BUFFER, sgeom->xyzBuffId);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*sgeom->vertNum*3,
               sgeom->xyz, GL_STATIC_DRAW);
  glVertexAttribPointer(spotVertAttrIndx_xyz, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(spotVertAttrIndx_xyz);

  if (sgeom->rgb) {
    glGenBuffers(1, &(sgeom->rgbBuffId));
    glBindBuffer(GL_ARRAY_BUFFER, sgeom->rgbBuffId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*sgeom->vertNum*3,
                 sgeom->rgb, GL_STATIC_DRAW);
    glVertexAttribPointer(spotVertAttrIndx_rgb, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(spotVertAttrIndx_rgb);
  } else {
    sgeom->rgbBuffId = 0;
  }

  glGenBuffers(1, &(sgeom->normBuffId));
  glBindBuffer(GL_ARRAY_BUFFER, sgeom->normBuffId);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*sgeom->vertNum*3,
               sgeom->norm, GL_STATIC_DRAW);
  glVertexAttribPointer(spotVertAttrIndx_norm, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(spotVertAttrIndx_norm);

  if (sgeom->tex2) {
    glGenBuffers(1, &(sgeom->tex2BuffId));
    glBindBuffer(GL_ARRAY_BUFFER, sgeom->tex2BuffId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*sgeom->vertNum*2,
                 sgeom->tex2, GL_STATIC_DRAW);
    glVertexAttribPointer(spotVertAttrIndx_tex2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(spotVertAttrIndx_tex2);
  } else {
    sgeom->tex2BuffId = 0;
  }

  if (sgeom->tang) {
    glGenBuffers(1, &(sgeom->tangBuffId));
    glBindBuffer(GL_ARRAY_BUFFER, sgeom->tangBuffId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*sgeom->vertNum*3,
                 sgeom->tang, GL_STATIC_DRAW);
    glVertexAttribPointer(spotVertAttrIndx_tang, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(spotVertAttrIndx_tang);
  } else {
    sgeom->tangBuffId = 0;
  }

  glGenBuffers(1, &(sgeom->indxBuffId));
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sgeom->indxBuffId);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*sgeom->indxNum,
               sgeom->indx, GL_STATIC_DRAW);
  
  /* Unbind from vao */
  glBindVertexArray(0);
  return 0;
}

int spotGeomDraw(spotGeom *sgeom) {
  /* const char me[]="spotGeomDraw"; */
  unsigned int pi, idx;

  glBindVertexArray(sgeom->vaoId);
  idx = 0;
  for (pi=0; pi<sgeom->primNum; pi++) {
    glDrawElements(sgeom->ptype[pi], sgeom->icnt[pi], GL_UNSIGNED_SHORT,
                   /* this is an address *offset* into the VBO,
                      not an absolute address, and 
                      not a logical index into the VBO */
                   (void*)(idx*sizeof(unsigned short)));
    idx += sgeom->icnt[pi];
  }
  glBindVertexArray(0);
  return 0;
}

int spotGeomGLDone(spotGeom *sgeom) {
  
  glDeleteBuffers(1, &(sgeom->xyzBuffId));
  glDeleteBuffers(1, &(sgeom->rgbBuffId));
  glDeleteBuffers(1, &(sgeom->normBuffId));
  glDeleteBuffers(1, &(sgeom->tex2BuffId));
  glDeleteBuffers(1, &(sgeom->tangBuffId));
  glDeleteBuffers(1, &(sgeom->indxBuffId));
  glDeleteVertexArrays(1, &(sgeom->vaoId));
  return 0;
}

spotGeom *spotGeomNix(spotGeom *sgeom) {

  if (!sgeom) {
    return NULL;
  }
  if (sgeom->xyz) {
    free(sgeom->xyz);
  }
  if (sgeom->rgb) {
    free(sgeom->rgb);
  }
  if (sgeom->norm) {
    free(sgeom->norm);
  }
  if (sgeom->tex2) {
    free(sgeom->tex2);
  }
  if (sgeom->tang) {
    free(sgeom->tang);
  }
  if (sgeom->indx) {
    free(sgeom->indx);
  }
  free(sgeom->ptype);
  free(sgeom->icnt);
  free (sgeom);
  return NULL;
}

