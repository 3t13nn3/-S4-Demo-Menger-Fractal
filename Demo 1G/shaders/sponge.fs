#version 330

uniform vec4 couleur;

in  vec3 vsoNormal;
out vec4 fragColor;

void main(void) {
	vec3 N = normalize(vsoNormal);
	vec3 L = normalize(vec3(-0.2, -0.5, -0.6));
	float diffuse = dot(N, -L);
	fragColor = vec4((couleur.rgb * diffuse), 0.0);  
}