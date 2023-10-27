// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules#page_Compiling-the-shaders

#version 450

layout(location=0)out vec3 fragColor;

vec2 positions[3]=vec2[](
    vec2(0.,-.5),
    vec2(.5,.5),
    vec2(-.5,.5)
);

vec3 colors[3]=vec3[](
    vec3(1.,0.,0.),
    vec3(0.,1.,0.),
    vec3(0.,0.,1.)
);

void main(){
    gl_Position=vec4(positions[gl_VertexIndex],0.,1.);
    // ! DebugDrawManager::setupPipelines：当i==0时，会检查着色器中是否包含point size，因此只需要加上即可
    gl_PointSize=2;
    fragColor=colors[gl_VertexIndex];
}