#version 450 core
#extension GL_ARB_seperate_shader_objects : enable

layout(location=0) in vec4 out_color;

layout(location=0) out vec4 FragColor;

void main(void)
{
	//code
	FragColor = out_color;
}

