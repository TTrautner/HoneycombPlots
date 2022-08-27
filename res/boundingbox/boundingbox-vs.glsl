#version 400

in vec3 position;
out vec3 vPosition;

void main()
{
    vPosition = position.xyz;
}