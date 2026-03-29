#version 450 core
#extension GL_ARB_seperate_shader_objects : enable

layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;


layout(location=0) out vec3 fragPosition;
layout(location=1) out vec3 outNormal;


layout(binding=0) uniform mvpMatrix
{
	mat4 modelMatrix;
	mat4 viewMatrix;
	mat4 projectionMatrix;
	vec3 lightPos;
	vec3 viewPos;
}uMVP;

void main(void)
{
	//code
	fragPosition = vec3(uMVP.modelMatrix * vec4(vPosition, 1.0));
	gl_Position = uMVP.projectionMatrix * uMVP.viewMatrix * vec4(fragPosition, 1.0);
	
	outNormal = vNormal;
}
