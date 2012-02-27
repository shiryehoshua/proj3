#version 150 

// Fragment shader for filtering 

#define PI_INV 0.31830988618379067153776752674 

uniform int gouraudMode;
uniform int gi;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 objColor;
uniform sampler2D samplerA;
uniform sampler2D samplerB;
uniform float Ka;
uniform float Kd;
uniform float Ks;
uniform float shexp;

in vec4 fragColor;
in vec3 texCoord;
in vec3 vnrm;

out vec4 color;

void main() {

  vec4 c; // to store the color given by either texture or object color
  vec2 tc; // to store texture coordinates

  if (false) { // without seam
    tc.x = 0.5 * PI_INV * atan(texCoord.z, texCoord.x) ;
    tc.y = texCoord.y;
  }
  else {
    tc = texCoord.xy;
  }

  tc *= 5;

  switch (gi)
  {
    case 0:
      c = texture(samplerA, tc);
      break;
    case 1:
      c = texture(samplerB, tc);
      break;
    default:
      c.rgb = objColor;
  }

  if (gouraudMode != 0) { // in Gouraud mode
    color = fragColor;
  }
  else { // in Phong mode
    vec3 diff = Kd * max(0.0, dot(vnrm, lightDir)) * objColor;
    vec3 amb = Ka * c.rgb;

    vec3 r = normalize(reflect(-normalize(lightDir), normalize(vnrm)));
    float vnrmdotr = max(0.0, dot(normalize(vnrm), r));
    vec3 spec = Ks * pow(vnrmdotr, shexp) * lightColor;

    color.rgb = diff + amb + spec;
    color.a = 1.0;
  }
}
