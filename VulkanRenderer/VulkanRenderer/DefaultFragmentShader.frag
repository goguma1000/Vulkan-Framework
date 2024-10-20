#version 450
layout(location = 0) out vec4 outColor;
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 texCoord;
layout(binding = 1) uniform sampler2D diff; 
void main(){
	outColor = texture(diff,texCoord);
}