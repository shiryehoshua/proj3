#version 150

// Sample vertex shader for Project 2.  Hack away!

uniform int gouraudMode;
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
in vec3 vertRgb;
in vec3 vertNorm;
in vec2 vertTex2;

out vec4 fragColor;

void main() {
  // transform vertices 
  gl_Position = projMatrix * viewMatrix * modelMatrix * vertPos;

  // transform normals
  vec3 nrm = normalize(normalMatrix * vertNorm);

  // Surface brightest when normal and lightDir are aligned, but this
  // is not Phong lighting, and may not be a good starting point for it.
  float dt = pow((dot(nrm, lightDir)+1.0)/2.0, 4);
  fragColor.rgb = vertRgb * objColor.rgb * (Ka + Kd*dt);
  fragColor.a = 1.0;
}
