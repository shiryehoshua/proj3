#version 150 

// Fragment shader for bump mapping 

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform sampler2D samplerA;
uniform sampler2D samplerB;
uniform float Ka;
uniform float Kd;
uniform float Ks;
uniform float shexp;

in vec4 fragColor;
in vec2 texCoord;
in vec3 vnrm;

out vec4 color;

void main() {

  vec4 a, b, c;
  a = texture(samplerA, texCoord);
  b = texture(samplerB, texCoord);

  // calculate new normal
  vec3 n;
  n.x = vnrm.x + b.x;
  n.y = vnrm.y + b.y;
  n.z = vnrm.z;

  // Phong shading
  vec3 diff = Kd * max(0.0, dot(n, lightDir)) * a.rgb;
  vec3 amb = Ka * a.rgb;

  vec3 r = normalize(reflect(-normalize(lightDir), normalize(n)));
  float vnrmdotr = max(0.0, dot(normalize(n), r));
  vec3 spec = Ks * pow(vnrmdotr, shexp) * lightColor;

  color.rgb = diff + amb + spec;
  color.a = 1.0;
}
