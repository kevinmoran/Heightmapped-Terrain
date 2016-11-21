#version 140

in vec3 vp, vn;
uniform mat4 M,V,P;

out float height;
out vec3 norm;

void main() {
	height = vp.y;
	norm = vn;
	gl_Position = P*V*M*vec4(vp, 1.0);
}