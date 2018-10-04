#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D samplerColorMap;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragViewVec;
layout(location = 4) in vec3 fragLightVecs[3];

layout(location = 0) out vec4 outColor;

void main() {
	vec4 color = texture(samplerColorMap, fragTexCoord) * vec4(fragColor, 1.0);
	vec3 lightColor = vec3(0.0);
	vec3 N = normalize(fragNormal);
	vec3 V = normalize(fragViewVec);
	for(int i = 0; i < 2; ++i)
	{
		float dist = length(fragLightVecs[i]);
		if(dist > 0.0)
		{
			vec3 L = normalize(fragLightVecs[i]);
			float falloff = smoothstep(1.0, 5.0, dist);
			vec3 R = reflect(-L, N);
			vec3 diffuse = falloff * fragColor * max(dot(N,L), 0.0);
			vec3 specular = falloff * pow(max(dot(R,V), 0.0), 4.0) * vec3(0.5) * color.rgb;
			lightColor += diffuse;
			color.rgb += specular;
		}
	}

	//outColor = vec4(diffuse * color.rgb + specular, 1.0);
	//outColor = color * vec4(lightColor, 1.0);
	outColor = vec4(lightColor * color.rgb, 1.0);
	
	//Just the texture image
	//outColor = texture(samplerColorMap, fragTexCoord);
}
