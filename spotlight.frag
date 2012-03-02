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

uniform sampler2D samplerA;

in vec2 fragTex;
in vec3 vnrm;

out vec4 color;

in vec3 l;
in vec3 s;
uniform float penumbra;
uniform vec3 spotPoint;

in vec3 vertPos2;
uniform vec3 spotUp;

uniform float rStart, rEnd;

void main() {

	float Il=1.0;
	float r_0 = dot(s-l,s);

	float theta = acos(dot(l,s));

	if (theta < penumbra*0.1) {
		if (theta < penumbra*0.1*0.75) {
			Il=1.0;
		} else {
			if (r_0 < -1 * rEnd) {
				Il = (rEnd - r_0)/(rEnd - rStart);
			} else {
				Il = 0;
			}
		}
	} else {
		Il = 0.0;
	}

	/// FUUUUUUHHH


  //vec3 diff = Il * Kd * max(0.0, dot(vnrm, lightDir)) * objColor;
  vec3 diff = Il * Kd * max(0.0, dot(vnrm, lightDir)) * objColor;
  //vec3 amb = Ka * objColor;
	vec3 amb; amb.x=amb.y=amb.z=0;

  vec3 r = normalize(reflect(-normalize(lightDir), normalize(vnrm)));
  float vnrmdotr = max(0.0, dot(normalize(vnrm), r));
  //vec3 spec = Ks * pow(vnrmdotr, shexp) * lightColor;
	vec3 spec; spec.x=spec.y=spec.z=0;

  color.rgb = diff + amb + spec;
	color.r=color.g=color.b=0;
//	color.rgb = spotPoint;
	vec2 tc; // tc.x=tc.y=theta-(penumbra*0.1);
	// tc = tc/(penumbra*0.1);
	float rad = 0.5*((penumbra*0.1)-theta)/(penumbra*0.1);
	vec3 m = spotUp * dot(spotUp, normalize(l-s));
	tc.x = rad * sin(m.y);
	tc.y = rad * cos(m.x);
	color.rgb = Il * texture(samplerA, tc).rgb;
//	color.r=color.g=color.b=Il;
	//color.rgb = Il * color.rgb;
}
