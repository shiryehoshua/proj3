/*
 * callbacks.h
 *
 */
#ifndef CALLBACKS_HAS_BEEN_INCLUDED
#define CALLBACKS_HAS_BEEN_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __APPLE__
#  include <OpenGL/gl3.h>
#else
#  include <GL/gl3.h>
#endif

#define __gl_h_
#define GLFW_NO_GLU /* Also, tell glfw.h not to include GLU header */
#include <GL/glfw.h>

void callbackKeyboard(int key, int action);
void callbackMouseButton(int button, int action);
void callbackMousePos(int xx, int yy);
void callbackResize(int w, int h);
void setScene(int sceneNum);

#ifdef __cplusplus
}
#endif

#endif /* CALLBACKS_HAS_BEEN_INCLUDED */
