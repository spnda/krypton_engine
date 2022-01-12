#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT bool hitPayload;
hitAttributeEXT vec2 attribs;

layout(binding = 1, set = 0) uniform accelerationStructureEXT tlas;

void main() {
	hitPayload = true;
}
