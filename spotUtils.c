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

#include <sys/time.h>  /* for time functions */

#define PLENTY_BIG_WE_HOPE 2048

static char **_spotError = NULL;
static unsigned int _spotErrorNum = 0;

/*
** returns current time in seconds (with millisecond resolution) as a double
** From "man gettimeofday": The time is expressed in seconds and microseconds
** since midnight (0 hour), January 1, 1970.  The date doesn't matter, since
** we only use this to measure intervals (differences in time)
*/
double
spotTime(void) {
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return((double)(tv.tv_sec + tv.tv_usec/1000000.0));
}

/* from quaternion to 3x3 matrix; quaternion is normalized 
   prior to conversion */
void spotQuatToM3(GLfloat mm[9], const GLfloat _qq[4]) {
  GLfloat len, qq[4];

  SPOT_Q_NORM(qq, _qq, len);
  SPOT_Q_TO_M3(mm, qq);
}

/* from quaternion to 4x4 matrix; quaternion is normalized 
   prior to conversion */
void spotQuatToM4(GLfloat mm[16], const GLfloat _qq[4]) {
  GLfloat len, qq[4];

  SPOT_Q_NORM(qq, _qq, len);
  SPOT_Q_TO_M4(mm, qq);
}

/* conversion from quaternion to angle,axis representation
   from a quaternion that isn't necessarily unit-length */
GLfloat spotQuatToAA(GLfloat axis[3], const GLfloat qq[4]) {
  GLfloat len, halfAngle;

  len = (GLfloat)SPOT_V3_LEN(qq+1);
  halfAngle = (GLfloat)atan2(len, qq[0]);
  if (len) {
    SPOT_V3_SCALE(axis, 1.0f/len, qq+1);
  } else {
    SPOT_V3_SET(axis, 1.0f, 0.0f, 0.0f);
  }
  return 2.0f*halfAngle;
}

/* conversion from angle,axis to unit quaternion */
void spotAAToQuat(GLfloat qq[4], GLfloat angle, const GLfloat axis[3]) {
  GLfloat sinha, cosha;

  sinha = (GLfloat)sin(angle/2); /* sin of half the angle */
  cosha = (GLfloat)cos(angle/2);
  SPOT_Q_SET(qq, cosha, sinha*axis[0], sinha*axis[1], sinha*axis[2]);
  return;
}

/* Note that the unit quaternion log(qq) returns a *3*-vector ql */
void spotQuatLog(GLfloat qlog[3], const GLfloat qq[4]) {
  GLfloat angle, axis[3];

  angle = spotQuatToAA(axis, qq);
  SPOT_V3_SCALE(qlog, angle/2.0f, axis);
  return;
}

/* Note that we take the unit quaternion exp(qlog) of a *3*-vector qlog */
void spotQuatExp(GLfloat qq[4], const GLfloat qlog[3]) {
  GLfloat halfAngle, axis[3];

  halfAngle = (GLfloat)SPOT_V3_LEN(qlog);
  if (halfAngle) {
    SPOT_V3_SCALE(axis, 1.0f/halfAngle, qlog);
  } else {
    SPOT_V3_SET(axis, 1.0f, 0.0f, 0.0f);
  }
  spotAAToQuat(qq, 2.0f*halfAngle, axis);
  return;
}

void spotM3print_f(const GLfloat m[9]) {
  printf("% 15.7f % 15.7f % 15.7f\n", 
         m[0], m[3], m[6]);
  printf("% 15.7f % 15.7f % 15.7f\n", 
         m[1], m[4], m[7]);
  printf("% 15.7f % 15.7f % 15.7f\n", 
         m[2], m[5], m[8]);
  return;
}

void spotM4print_f(const GLfloat m[16]) {
  printf("% 15.7f % 15.7f % 15.7f % 15.7f\n", 
          m[ 0], m[ 4], m[ 8], m[12]);
  printf("% 15.7f % 15.7f % 15.7f % 15.7f\n", 
          m[ 1], m[ 5], m[ 9], m[13]);
  printf("% 15.7f % 15.7f % 15.7f % 15.7f\n", 
          m[ 2], m[ 6], m[10], m[14]);
  printf("% 15.7f % 15.7f % 15.7f % 15.7f\n", 
          m[ 3], m[ 7], m[11], m[15]);
  return;
}

/* unfortunately strdup() isn't reliably in C */
char *spotStrdup(const char *s) {
  char *ret;

  if (!s) {
    ret = NULL;
  }
  else {
    ret = (char *)malloc(strlen(s)+1);
    if (ret) {
      strcpy(ret, s);
    }
  }
  return ret;
}

void spotErrorAdd(const char *fmt, ...) {
  char errstr[PLENTY_BIG_WE_HOPE];
  char **newerr;
  va_list args;
  unsigned int eidx;

  va_start(args, fmt);
  vsprintf(errstr, fmt, args);
  va_end(args);
  
  newerr = (char**)calloc(_spotErrorNum+1, sizeof(char*));
  for (eidx=0; eidx<_spotErrorNum; eidx++) {
    newerr[eidx] = _spotError[eidx];
  }
  newerr[eidx] = spotStrdup(errstr);
  free(_spotError);
  _spotError = newerr;
  _spotErrorNum++;
  return;
}

void spotErrorPrint(void) {
  unsigned int eidx;

  if (_spotErrorNum) {
    fprintf(stderr, "ERROR ***\n");
    for (eidx=_spotErrorNum; eidx>=1; eidx--) {
      fprintf(stderr, "ERROR *** %s\n", _spotError[eidx-1]);
    }
    fprintf(stderr, "ERROR ***\n");
  }
  return;
}

void spotErrorClear(void) {
  unsigned int eidx;
  
  if (_spotErrorNum) {
    for (eidx=0; eidx<_spotErrorNum; eidx++) {
      free(_spotError[eidx]);
    }
    free(_spotError);
    _spotError = NULL;
    _spotErrorNum = 0;
  }
  return;
}

/*
** returns NULL in case of error
*/
char *spotReadFile(const char *fname) {
  const char me[]="spotReadFile";
  char *ret;
  FILE *file;
  long len;

  if (!fname) {
    spotErrorAdd("%s: got NULL fname", me);
    return NULL;
  }
  if (!(file = fopen(fname, "r"))) {
    spotErrorAdd("%s: couldn't open \"%s\" for reading", me, fname);
    return NULL;
  }
  /* learn length of file contents */
  fseek(file, 0L, SEEK_END);
  len = ftell(file);
  fseek(file, 0L, SEEK_SET);
  ret = (char*)malloc(len+1);
  if (!ret) {
    spotErrorAdd("%s: allocation failure", me);
    fclose(file);
    return NULL;
  }
  /* copy file into string */
  fread(ret, 1, len, file);
  ret[len] = '\0';
  fclose(file);
  return ret;
}

GLint spotShaderNew(GLint shtype, const char *filename) {
  const char me[]="spotShaderNew";
  GLuint shaderId;
  GLint status;
  char *shaderTxt;

  if (!( GL_VERTEX_SHADER == shtype
         || GL_FRAGMENT_SHADER == shtype )) {
    spotErrorAdd("%s: given shtype %d not GL_VERTEX_SHADER (%d) "
                 "or GL_FRAGMENT_SHADER (%d)", me, shtype,
                 GL_VERTEX_SHADER, GL_FRAGMENT_SHADER);
    return 0;
  }
  if (!(shaderTxt = spotReadFile(filename))) {
    spotErrorAdd("%s: trouble reading from \"%s\"", me, filename);
    return 0;
  }
  shaderId = glCreateShader(shtype);
  glShaderSource(shaderId, 1, (const GLchar **)(&shaderTxt), NULL);
  glCompileShader(shaderId);
  glGetShaderiv(shaderId, GL_COMPILE_STATUS, &status);
  if (GL_FALSE == status) {
    GLint logSize;
    char *logMsg;
    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logSize);
    if (logSize) {
      logMsg = (char*)malloc(logSize);
      glGetShaderInfoLog(shaderId, logSize, NULL, logMsg);
      spotErrorAdd("%s: shader compiler error:\n%s", me, logMsg);
      glDeleteShader(shaderId);
      free(logMsg);
    }
    return 0;
  }
  return shaderId;
}

GLint spotProgramNew(const char *vertFileName,
                     const char *fragFileName,
                     ...) {
  const char me[]="spotProgramBuild";
  va_list vargs;
  GLuint program, vertId=0, fragId=0, varIndx;
  GLint status;
  const char *varName;
  
  if (!(vertId = spotShaderNew(GL_VERTEX_SHADER, vertFileName))) {
    spotErrorAdd("%s: vertex shader error", me);
    return 0;
  }
  if (!(fragId = spotShaderNew(GL_FRAGMENT_SHADER, fragFileName))) {
    spotErrorAdd("%s: fragment shader error", me);
    glDeleteShader(vertId); 
    return 0;
  }
  program = glCreateProgram();
  glAttachShader(program, vertId);
  glAttachShader(program, fragId);

  /* process var-args, consisting of pairs of
     1) const char *"variableName"
     2)  GLuint attrIndx
     arguments, until "variableName" is NULL
  */
  va_start(vargs, fragFileName);
  for (varName = va_arg(vargs, const char *);
       varName; 
       varName = va_arg(vargs, const char *)) {
    varIndx = va_arg(vargs, GLuint);
    glBindAttribLocation(program, varIndx, varName);
  }
  va_end(vargs);

  glLinkProgram(program);

  /* Make sure link worked too */
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (GL_FALSE == status) {
    GLint logSize;
    char *logMsg;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);
    if (logSize) {
      logMsg = (char*)malloc(logSize);
      glGetProgramInfoLog(program, logSize, NULL, logMsg);
      spotErrorAdd("%s: linking error:\n%s", me, logMsg);
      glDeleteShader(vertId); 
      glDeleteShader(fragId); 
      free(logMsg);
      glDeleteProgram(program);
    }
    return 0;
  }

  /* shaders no longer needed post-linking */
  glDeleteShader(vertId);
  glDeleteShader(fragId);
    
  return program;
}

/* based these strings on "man glGetError */
static const char str_GL_NO_ERROR[] = "GL_NO_ERROR: No error has been recorded.";
static const char str_GL_INVALID_ENUM[] = "GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument.";
static const char str_GL_INVALID_VALUE[] = "GL_INVALID_VALUE: A numeric argument is out of range.";
static const char str_GL_INVALID_OPERATION[] = "GL_INVALID_OPERATION: The specified operation is not allowed in the current state.";
static const char str_GL_OUT_OF_MEMORY[] = "GL_OUT_OF_MEMORY: There is not enough memory left to execute the command.";
/* these correspond to earlier OpenGL versions
static const char str_GL_STACK_OVERFLOW[] = "GL_STACK_OVERFLOW: This command would cause a stack overflow.";
static const char str_GL_STACK_UNDERFLOW[] = "GL_STACK_UNDERFLOW: This command would cause a stack underflow.";
static const char str_GL_TABLE_TOO_LARGE[] = "GL_TABLE_TOO_LARGE: The specified table exceeds the implementation's maximum supported table size.";
*/
static const char str_unknown[] = "(value is not known GL error code)";

const char *spotGLErrorString(GLenum error) {
  const char *ret;

  switch(error) {
  case GL_NO_ERROR:
    ret = str_GL_NO_ERROR;
    break;
  case GL_INVALID_ENUM:
    ret = str_GL_INVALID_ENUM;
    break;
  case GL_INVALID_VALUE:
    ret = str_GL_INVALID_VALUE;
    break;
  case GL_INVALID_OPERATION:
    ret = str_GL_INVALID_OPERATION;
    break;
  case GL_OUT_OF_MEMORY:
    ret = str_GL_OUT_OF_MEMORY;
    break;
    /*
  case GL_STACK_OVERFLOW:
    ret = str_GL_STACK_OVERFLOW;
    break;
  case GL_STACK_UNDERFLOW:
    ret = str_GL_STACK_UNDERFLOW;
    break;
  case GL_TABLE_TOO_LARGE:
    ret = str_GL_TABLE_TOO_LARGE;
    break;
    */
  default:
    ret = str_unknown;
  }
  return ret;
}
