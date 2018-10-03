#version 450
#extension GL_ARB_separate_shader_objects : enable

// Note that the order of the uniform, in and out declarations doesn't matter. 
// The binding directive is similar to the location directive for attributes. 

layout(binding = 0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 proj;
  vec4 lightPos;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragViewVec;
layout(location = 4) out vec3 fragLightVec;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    fragColor = inColor.xyz;
	fragTexCoord = inTexCoord;
	//mat4 modelview = ubo.view * ubo.model;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition.xyz, 1.0);

	vec4 pos = ubo.view * ubo.model * vec4(inPosition.xyz, 1.0);
	fragNormal = mat3(ubo.view * ubo.model) * inNormal;
	vec3 lPos = mat3(ubo.view * ubo.model) * ubo.lightPos.xyz;
	fragLightVec = lPos - pos.xyz;
	fragViewVec = -pos.xyz;
}
