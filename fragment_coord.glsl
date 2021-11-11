#version 330 core

out vec4 outputColor;
uniform vec3 activeColor;
void main() {
	outputColor = vec4(activeColor, 1.0);
}