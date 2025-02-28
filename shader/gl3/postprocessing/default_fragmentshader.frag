#version 330 core

// uniforms
uniform sampler2D colorBufferTextureUnit;
uniform sampler2D depthBufferTextureUnit;
uniform float bufferTexturePixelWidth;
uniform float bufferTexturePixelHeight;

// passed from vertex shader
in vec2 vsFragTextureUV;

// passed out
out vec4 outColor;

// main
void main(void) {
	outColor = texture(colorBufferTextureUnit, vsFragTextureUV);
	gl_FragDepth = texture(depthBufferTextureUnit, vsFragTextureUV).r;
}
