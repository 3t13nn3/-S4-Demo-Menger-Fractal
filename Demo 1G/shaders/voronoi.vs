#version 330

layout (location = 0) in vec3 vsiPosition;
layout (location = 1) in vec3 vsiNormal;
layout (location = 2) in vec2 vsiTexCoord; /* ne sont pas utilisées dans cet exemple */

out vec3 vsoNormal;
out vec2 vsoTexCoord;

void main(void) {
  
	vsoTexCoord = vsiTexCoord;
	gl_Position = vec4(vsiPosition, 1.0);
	
}
