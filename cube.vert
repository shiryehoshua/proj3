#version 150

// Vertex shader for texture mapping 

#define PI 3.14159265358979323846264338327

uniform int gouraudMode;
uniform int seamFix;
uniform mat4 modelMatrix;
uniform mat3 normalMatrix;
uniform mat4 viewMatrix;
uniform mat4 inverseViewMatrix;
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
out vec3 texCoord;
out vec3 vnrm;

out vec3 fromEye;

void main() {

	vnrm = normalMatrix * vertNorm;
	vec4 vVert4 = viewMatrix * modelMatrix * vertPos;
	vec3 vEyeVertex = normalize(vVert4.xyz / vVert4.w);
	vec4 vCoords = vec4(reflect(vEyeVertex, vnrm), 1.0);
	vCoords = inverseViewMatrix * vCoords;
	texCoord = normalize(vCoords.xyz);

	fromEye = vEyeVertex;

  // transform vertices 
  gl_Position = projMatrix * viewMatrix * modelMatrix * vertPos;

//	texCoord = normalize(vertPos.xyz);

}
