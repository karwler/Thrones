#version 330 core

flat in uvec2 fragAddr;

out uvec2 fragColor;

void main() {
	fragColor = fragAddr;
}
