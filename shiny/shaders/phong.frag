#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D samplerColorMap;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragViewVec;
layout(location = 4) in vec3 fragLightVec;

layout(location = 0) out vec4 outColor;

void main() {
	vec4 color = texture(samplerColorMap, fragTexCoord) * vec4(fragColor, 1.0);
	vec3 N = normalize(fragNormal);
	vec3 L = normalize(fragLightVec);
	vec3 V = normalize(fragViewVec);
	vec3 R = reflect(-L, N);
	vec3 diffuse = fragColor * max(dot(N,L), 0.0);
	vec3 specular = pow(max(dot(R,V), 0.0), 4.0) * vec3(0.5) * color.r;
	outColor = vec4(diffuse * color.rgb + specular, 1.0);
	//outColor = color;
	//outColor = vec4(diffuse, 1.0);
	
	//Just the texture image
	//outColor = texture(samplerColorMap, fragTexCoord);
}
