#version 150 

// Fragment shader for phong/gouraud shading 

uniform int gi;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 objColor;
uniform float Ka;
uniform float Kd;
uniform float Ks;
uniform float shexp;
uniform float Zu;
uniform float Zv;
uniform float Zspread;

in vec2 fragTex;
in vec3 vnrm;

out vec4 color;

void main() {

  // implement Phong shading
  vec3 diff = Kd * max(0.0, dot(vnrm, lightDir)) * objColor;
  vec3 amb = Ka * objColor;

  vec3 r = normalize(reflect(-normalize(lightDir), normalize(vnrm)));
  float vnrmdotr = max(0.0, dot(normalize(vnrm), r));
  vec3 spec = Ks * pow(vnrmdotr, shexp) * lightColor;

  color.rgb = diff + amb + spec;
  color.a = 1.0;
}
