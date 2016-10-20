#version 410

in vec3 vp;
uniform mat4 M,V,P;

out float height;

void main () {
	height = vp.y;
	gl_Position = P*V*M*vec4(vp, 1.0);
}