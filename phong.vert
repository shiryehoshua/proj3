#version 150

// Vertex shader for phong/gouraud shading 

uniform mat4 modelMatrix;
uniform mat3 normalMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform vec3 lightDir;  // assumed to be unit-length (already normalized)
uniform vec3 lightColor;
uniform vec3 objColor;
uniform float Ka;
uniform float Kd;
uniform float Ks;
uniform float shexp;

in vec4 vertPos;
in vec2 vertTex2;
in vec3 vertNorm;

out vec2 fragTex;
out vec3 vnrm;

void main() {

  // transform vertices 
  gl_Position = projMatrix * viewMatrix * modelMatrix * vertPos;
  
  // calculate surface normal in view coords
  vnrm = normalize(normalMatrix * vertNorm);

  // pass fragment shader the vertTex
  fragTex = vertTex2;
}
