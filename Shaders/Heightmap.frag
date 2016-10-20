#version 410

in float height;
out vec4 frag_colour;

void main () {
	frag_colour = vec4(height/5.0, 0.4, 0.1, 1.0);
}