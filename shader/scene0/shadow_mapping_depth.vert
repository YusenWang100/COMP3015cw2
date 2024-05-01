#version 410 core
layout (location = 0) in vec3 aPos;

uniform LightMatrixUBO
{
    uniform mat4 lightSpaceMatrix;
};

uniform TransformUBO
{
    uniform mat4 model;
};

void main()
{
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}