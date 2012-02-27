#version 150

// Vertex shader for phong/gouraud shading 

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

out vec4 fragColor;
out vec3 vnrm;

void main() {

  // transform vertices 
  gl_Position = projMatrix * viewMatrix * modelMatrix * vertPos;
  
  if (gouraudMode != 0) { // in Gouraud mode
    // transform normals
    vec3 nrm = normalize(normalMatrix * vertNorm);
  
    // pre-calculations to simplify
    float ndotl = max(0, dot(nrm, lightDir));
    vec3 reflection = normalize(reflect(-lightDir, nrm));
    float ndotr = max(0, dot(nrm, reflection));
  
    // calculate diffuse, ambient and specular components
    vec3 amb  = Ka * objColor;
    vec3 diff = Kd * objColor * lightColor * ndotl;
    vec3 spec = Ks * lightColor * pow(ndotr, shexp);
  
    // calculate color
    fragColor.rgb = amb + diff + spec;
    fragColor.a = 1.0;

    vnrm = nrm;
  }
  else { // in Phong mode
    // surface normal in view coords
    vnrm = normalMatrix * vertNorm;

    fragColor.rgb = objColor;
    fragColor.a = 1.0;
  }
}
