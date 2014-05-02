#version 150

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

in vec3 in_pos;
in vec3 in_norm;
out vec3 frag_pos_eye;
out vec3 frag_norm_eye;

void main() {
	frag_pos_eye  = vec3(view * model * vec4(in_pos,  1.));
	frag_norm_eye = vec3(view * model * vec4(in_norm, 0.));

	gl_Position = proj * vec4(frag_pos_eye, 1.);
}
