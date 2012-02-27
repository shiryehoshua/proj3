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

static void _spotImageInit(spotImage *img) {
  
  if (img) {
    img->sizeC = 0;
    img->sizeP = 0;
    img->sizeX = 0;
    img->sizeY = 0;
    if (img->data.v) {
      free(img->data.v);
      img->data.v = NULL;
    }
    img->textureId = 0;
  }
  return;
}

spotImage *spotImageNew() {
  const char me[]="spotImageNew";

  spotImage *img;
  img = (spotImage *)calloc(1, sizeof(spotImage));
  if (!img) {
    spotErrorAdd("%s: allocation failure", me);
    return NULL;
  }
  /* else struct allocation succeeded */
  _spotImageInit(img);
  return img;
}

/*
** spotImageAlloc: allocate the data array within the given spotImage
** to hold the data size sizeC*sizeP*sizeX*sizeY
*/
int spotImageAlloc(spotImage *img, unsigned int sizeC, unsigned int sizeP,
                   unsigned int sizeX, unsigned int sizeY) {
  const char me[]="spotImageAlloc";
  
  if (!img) {
    spotErrorAdd("%s: got NULL pointer", me);
    return 1;
  }
  if (!( 1 == sizeC || 2 == sizeC )) {
    spotErrorAdd("%s: sizeC (%u) not 1 (for 8-bit) or 2 (for 16-bit)", me,
                 sizeC);
    return 1;
  }
  if (!( 1 <= sizeP && sizeP <= 4 )) {
    spotErrorAdd("%s: sizeP (%u) not 1, 2, 3, or 4", me, sizeP);
    return 1;
  }
  if (!( sizeX > 0 && sizeY > 0 )) {
    spotErrorAdd("%s: image X,Y size (%u,%u) not both > 0", me,
                 sizeX, sizeY);
    return 1;
  }
  
  _spotImageInit(img);
  if (!( img->data.v = calloc(sizeX*sizeY, sizeC*sizeP) )) {
    spotErrorAdd("%s: couln't alloc %u*%u=%u pixels of size %u*%u=%u", me,
                 sizeX, sizeY, sizeX*sizeY,
                 sizeC, sizeP, sizeC*sizeP);
    return 1;
  }
  img->sizeC = sizeC;
  img->sizeP = sizeP;
  img->sizeX = sizeX;
  img->sizeY = sizeY;
  img->textureId = 0;
  return 0;
}

void _spotImageHandleErrorPNG(png_structp png, png_const_charp message) {
  static const char me[]="_spotImageHandleErrorPNG";
  /* add PNG error message to spot */
  spotErrorAdd("%s: PNG error: %s", me, message);
  /* longjmp back to the setjmp, return 1 */
  longjmp(png_jmpbuf(png), 1);
}

void _spotImageHandleWarningPNG(png_structp png, png_const_charp message) {
  static const char me[]="_spotImageHandleWarningPNG";
  SPOT_UNUSED(png);
  /* fprintf warning to stderr and move on */
  fprintf(stderr, "%s: PNG warning: %s", me, message);
  return;
}

static int _spotLittleEndian(void) {
  int tmpI;
  
  tmpI = 1;
  return (*((char*)(&tmpI)));
}

/*
** spotImageLoadPNG: read PNG image from fname into img
**
** Note: this borrows from the _nrrdFormatPNG_read() function 
** from Teem <http://teem.sf.net>, written by Milan Ikits
*/
int spotImageLoadPNG(spotImage *img, char *fname) {
  const char me[]="spotImageLoadPNG";
  unsigned char header[8];
  png_structp png_ptr;
  png_infop info_ptr;
  FILE *file;
  int itype, idepth;
  png_uint_32 rowsize;
  png_bytep *row;
  unsigned int rowIdx;

  if (!( img && fname )) {
    spotErrorAdd("%s: got NULL pointer (%p %p)", me, img, fname);
    return 1;
  }
  _spotImageInit(img);
  if (!(file = fopen(fname, "rb"))) {
    spotErrorAdd("%s: couldn't open \"%s\" for reading", me, fname);
    return 1;
  }
#define BYE1 fclose(file)
  if (8 != fread(header, 1, 8, file)) {
    spotErrorAdd("%s: didn't get 8 chars from \"%s\"", me, fname);
    BYE1; return 1;
  }
  if (png_sig_cmp(header, 0, 8)) {
    spotErrorAdd("%s: file %s not recognized as PNG file", me, fname);
    BYE1; return 1;
  }
  if (!(png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
                                         _spotImageHandleErrorPNG,
                                         _spotImageHandleWarningPNG))) {
    spotErrorAdd("%s: failed to create PNG read struct", me);
    BYE1; return 1;
  }
#define BYE2 BYE1; png_destroy_read_struct(&png_ptr, NULL, NULL);
  if (!(info_ptr = png_create_info_struct(png_ptr))) {
    spotErrorAdd("%s: failed to create PNG image info struct", me);
    BYE2; return 1;
  }
#define BYE3 BYE2; png_destroy_read_struct(NULL, &info_ptr, NULL);
  if (setjmp(png_jmpbuf(png_ptr))) {
    spotErrorAdd("%s: error during PNG IO", me);
    BYE3; return 1;
  }
  png_init_io(png_ptr, file);
  png_set_sig_bytes(png_ptr, 8); /* have read 8 bytes already */
  png_read_info(png_ptr, info_ptr);
  png_get_IHDR(png_ptr, info_ptr,
               &(img->sizeX), &(img->sizeY), &idepth, &itype,
               NULL, NULL, NULL);
  /* expand paletted colors into rgb triplets */
  if (itype == PNG_COLOR_TYPE_PALETTE) {
    png_set_palette_to_rgb(png_ptr);
  }
  /* expand grayscale images to 8 bits from 1, 2, or 4 bits */
  if (itype == PNG_COLOR_TYPE_GRAY && idepth < 8) {
    png_set_expand_gray_1_2_4_to_8(png_ptr);
  }
  /* expand paletted or rgb images with transparency to full alpha
     channels so the data will be available as rgba quartets */
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
    png_set_tRNS_to_alpha(png_ptr);
  }
  if (idepth > 8) {
    /* fix endianness for 16 bit formats */
    if (_spotLittleEndian()) {
      png_set_swap(png_ptr);
    }
    img->sizeC = 2;
  } else {
    img->sizeC = 1;
  }
  /* update reader */
  png_read_update_info(png_ptr, info_ptr);  
  /* allocate memory for the image data */
  switch (itype) {
  case PNG_COLOR_TYPE_GRAY:
    img->sizeP = 1;
    break;
  case PNG_COLOR_TYPE_GRAY_ALPHA:
    img->sizeP = 2;
    break;
  case PNG_COLOR_TYPE_RGB:
    img->sizeP = 3;
    break;
  case PNG_COLOR_TYPE_RGB_ALPHA:
    img->sizeP = 4;
    break;
  default:
    spotErrorAdd("%s: unknown or unsupported PNG type %d", me, itype);
    BYE3; return 1;
    break;
  }
  if (!(img->data.v = calloc(img->sizeP*img->sizeX*img->sizeY, 
                             img->sizeC))) {
    spotErrorAdd("%s: couldn't allocate %d x %d x %d %s", me,
                 img->sizeC, img->sizeX, img->sizeY, 
                 1 == img->sizeC ? "uchars" : "ushorts");
    BYE3; return 1;
  }
  rowsize = png_get_rowbytes(png_ptr, info_ptr);
  /* check byte size of row */
  if (img->sizeC*img->sizeP*img->sizeX != rowsize) {
    spotErrorAdd("%s: row byte size mismatch (%u != %u)", me,
                 (unsigned int)img->sizeC*img->sizeP*img->sizeX, rowsize);
    BYE3; return 1;
  }
  /* set up row pointers */
  row = (png_bytep*)calloc(img->sizeY, sizeof(png_bytep));
  for (rowIdx=0; rowIdx<img->sizeY; rowIdx++) {
    row[rowIdx] = img->data.uc + rowIdx*rowsize;
  }
  /* read the entire image in one pass */
  png_read_image(png_ptr, row);
  /* finish reading */
  png_read_end(png_ptr, info_ptr);
  /* clean up */
  free(row);
  
  BYE3;
  return 0;
}
#undef BYE1
#undef BYE2
#undef BYE3

int spotImageSavePNG(char *fname, spotImage *img) {
  const char me[]="spotImageSavePNG";
  FILE *file;
  int type;
  png_structp png_ptr;
  png_infop info_ptr;
  png_bytep *row;
  png_uint_32 rowsize, hi;

  if (!(file = fopen(fname, "wb"))) {
    spotErrorAdd("%s: couldn't open \"%s\" for writing", me, fname);
    return 1;
  }
#define BYE1 fclose(file)
  /* create png struct with the error handlers above */
  if (!(png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
                                          _spotImageHandleErrorPNG,
                                          _spotImageHandleWarningPNG))) {
    spotErrorAdd("%s: failed to create PNG struct", me);
    BYE1; return 1;
  }
#define BYE2 BYE1; png_destroy_write_struct(&png_ptr, NULL)
  /* create image info struct */
  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    png_destroy_write_struct(&png_ptr, NULL);
    spotErrorAdd("%s: failed to create PNG image info struct", me);
    BYE2; return 1;
  }
#define BYE3 BYE2; png_destroy_write_struct(NULL, &info_ptr)
  /* set up error png style error handling */
  if (setjmp(png_jmpbuf(png_ptr))) {
    /* the error is reported inside the error handler, 
       but we still need to clean up an return with an error */
    BYE3; return 1;
  }
  /* initialize png I/O */
  png_init_io(png_ptr, file);        
  /* calculate row size */
  rowsize = img->sizeX*img->sizeP*img->sizeC;
  switch (img->sizeP) {
  case 1:
    type = PNG_COLOR_TYPE_GRAY;
    break;
  case 2:
    type = PNG_COLOR_TYPE_GRAY_ALPHA;
    break;
  case 3:
    type = PNG_COLOR_TYPE_RGB;
    break;
  case 4:
    type = PNG_COLOR_TYPE_RGB_ALPHA;
    break;
  default:
    spotErrorAdd("%s: unknown or unimplemented sizeP %u", me, img->sizeP);
    BYE3; return 1;
    break;
  }
  /* set image header info */
  png_set_IHDR(png_ptr, info_ptr, img->sizeX, img->sizeY,
               (1 == img->sizeC ? 8 : 16), type,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
               PNG_FILTER_TYPE_BASE);
  /* write header */
  png_write_info(png_ptr, info_ptr);
  /* fix endianness for 16 bit formats */
  if (img->sizeC > 1 && _spotLittleEndian()) {
    png_set_swap(png_ptr);
  }
  /* set up row pointers */
  row = (png_bytep*)malloc(sizeof(png_bytep)*img->sizeY);
  for (hi=0; hi<img->sizeY; hi++) {
    row[hi] = img->data.uc + hi*rowsize;
  }
  png_set_rows(png_ptr, info_ptr, row);
  /* write the entire image in one pass */
  png_write_image(png_ptr, row);
  /* finish writing */
  png_write_end(png_ptr, info_ptr);
  free(row);

  BYE3;
  return 0;
}
#undef BYE1
#undef BYE2
#undef BYE3

int spotImageScreenshot(spotImage *img, int withAlpha) {
  const char me[]="spotImageScreenshot";
  GLint vport[4], lastBuffer;
  void *rowB;
  unsigned int rowsize, yi, yj;

  glGetIntegerv(GL_VIEWPORT, vport);
  if (spotImageAlloc(img, 1, withAlpha ? 4 : 3,
                     vport[2], vport[3])) {
    spotErrorAdd("%s: couldn't allocate %u x %u x %u output", me,
                 withAlpha ? 4 : 3, vport[2], vport[3]);
    return 1;
  }
  glGetIntegerv(GL_READ_BUFFER, &lastBuffer);
  glReadBuffer(GL_FRONT);
  if (withAlpha) {
    glReadPixels(vport[0], vport[1], vport[2], vport[3],
                 GL_RGBA, GL_UNSIGNED_BYTE, img->data.v);
  } else {
    glReadPixels(vport[0], vport[1], vport[2], vport[3],
                 GL_RGB, GL_UNSIGNED_BYTE, img->data.v);
  }
  glReadBuffer(lastBuffer);
  if (!( rowB = calloc(img->sizeP*img->sizeC, img->sizeX) )) {
    spotErrorAdd("%s: couldn't allocate row buffer", me);
    return 1;
  }
  /* flip ordering of rows */
  rowsize = img->sizeX*img->sizeP*img->sizeC;
  for (yi=0, yj=img->sizeY - 1;
       yi < yj;
       yi++, yj--) {
    unsigned char *rowI, *rowJ;
    rowI = img->data.uc + yi*rowsize;
    rowJ = img->data.uc + yj*rowsize;
    memcpy(rowB, rowI, rowsize);
    memcpy(rowI, rowJ, rowsize);
    memcpy(rowJ, rowB, rowsize);
  }
  free(rowB);
  return 0;
}

/* method "9. Decrement and Compare" from:
** http://www.exploringbinary.com/ten-ways-to-check-if-an-integer-is-a-power-of-two-in-c/
*/
static int isPowerOfTwo(unsigned int x) {
  return ((x != 0) && !(x & (x - 1)));
}

int spotImageGLInit(spotImage *img) {
  const char me[]="spotImageGLInit";
  GLint dataFormat;
  GLenum type;

  switch (img->sizeP) {
  case 1:
    dataFormat = GL_RED; /* not tested! */
    break;
  case 2:
    dataFormat = GL_RG; /* not tested! */
    break;
  case 3:
    dataFormat = GL_RGB;
    break;
  case 4:
    dataFormat = GL_RGBA;
    break;
  default:
    spotErrorAdd("%s: can't handle sizeP %d", me, img->sizeP);
    return 1;
    break;
  }
  if (!( isPowerOfTwo(img->sizeX)
         && isPowerOfTwo(img->sizeY) )) {
    spotErrorAdd("%s: image dimensions (%u,%u) not both powers of two",
                 me, img->sizeX, img->sizeY);
    return 1;
  }

  type = (1 == img->sizeC
          ? GL_UNSIGNED_BYTE
          : GL_UNSIGNED_SHORT);
  glGenTextures(1, &(img->textureId));
  glBindTexture(GL_TEXTURE_2D, img->textureId);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img->sizeX, img->sizeY, 
               0, dataFormat, type, img->data.v);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  return 0;
}

int spotImageGLDone(spotImage *img) {

  glDeleteTextures(1, &(img->textureId));
  return 0;
}

spotImage *spotImageNix(spotImage *img) {

  if (img) {
    _spotImageInit(img);
    free(img);
  }
  return NULL;
}
