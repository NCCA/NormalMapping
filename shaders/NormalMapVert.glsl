#version 330 core
layout (location = 0) in vec3 inVert;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBiTangent;

out VS_OUT {
    vec3 fragPos;
    vec2 uv;
    vec3 tangentLightPos[3];
    vec3 tangentViewPos[3];
    vec3 tangentFragPos[3];
} vs_out;


uniform mat4 M;
uniform mat4 MVP;
uniform mat3 normalMatrix;
uniform vec3 viewPos;


struct Light
{
  vec3 position;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};
uniform Light light[3];


void main()
{
    vs_out.fragPos = vec3(M * vec4(inVert, 1.0));
    vs_out.uv = inUV;
    vec3 T = normalize(normalMatrix * inTangent);
    vec3 N = normalize(normalMatrix * inNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    mat3 TBN = transpose(mat3(T, B, N));
    for(int i=0; i<3; ++i)
    {
      vs_out.tangentLightPos[i] = (TBN * light[i].position);
      vs_out.tangentViewPos[i]  = (TBN * viewPos);
      vs_out.tangentFragPos[i]  = (TBN * vs_out.fragPos);
    }
    gl_Position = MVP * vec4(inVert, 1.0);
}
