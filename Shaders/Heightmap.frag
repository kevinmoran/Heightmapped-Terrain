#version 140

in float height;
in vec3 norm;
out vec4 frag_colour;

vec3 sun_dir = normalize(vec3(-1.0, -1.0, 0.3));

void main() {
	float x = dot(-sun_dir, norm);
	frag_colour = (x+0.2)*vec4((0.8-norm.y), 0.1*height+0.5, 0.1, 1.0);
}