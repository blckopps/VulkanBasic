#version 450 core
#extension GL_ARB_seperate_shader_objects : enable
layout(location=0) in vec4 vPosition;
void main(void)
{
	//code
	gl_Position = vPosition;
}
