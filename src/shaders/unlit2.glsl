@vs vs
layout(binding=0) uniform vs_params {
    mat4 mvp;
};

in vec2 position;
in vec2 coords;   
in vec4 vertex_color;

out vec2 uvs;
out vec4 color;

void main() {
    gl_Position = mvp * vec4(position, 0.0, 1.0);
		uvs = coords;
    color = vertex_color;
} 
@end

@fs fs
layout(binding=0) uniform texture2D tex;
layout(binding=0) uniform sampler smp;

in vec2 uvs;
in vec4 color;

out vec4 frag_color;

void main() {
    frag_color = texture(sampler2D(tex, smp), uvs) * color;
}
@end

@program unlit2 vs fs
