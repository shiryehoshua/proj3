#version 150 

// Fragment shader for texture mapping 

#define PI_INV 0.31830988618379067153776752674 

uniform int gouraudMode;
uniform int seamFix;
uniform int gi;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 objColor;
uniform sampler2D samplerA;
uniform sampler2D samplerB;
uniform samplerCube cubeMap;
uniform float Ka;
uniform float Kd;
uniform float Ks;
uniform float shexp;

in vec4 fragColor;
in vec3 texCoord;
in vec3 vnrm;

out vec4 color;

in vec3 fromEye;

uniform mat4 inverseViewMatrix;

void main() {

//	color = texture(cubeMap, texCoord);
	color.rgb = texture(cubeMap, (inverseViewMatrix * vec4(reflect(fromEye, vnrm),1.0)).xyz).rgb;

}


