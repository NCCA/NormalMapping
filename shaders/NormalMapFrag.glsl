#version 410 core
out vec4 Fragcolour;

in VS_OUT {
    vec3 fragPos;
    vec2 uv;
    vec3 tangentLightPos[3];
    vec3 tangentViewPos[3];
    vec3 tangentFragPos[3];
} fs_in;

//layout(binding=0) uniform sampler2D diffuseMap;
// this is set to the spec map (texture unit 1)
//layout(binding=1) uniform sampler2D spec;
// normal map set a texture unit 2
//layout(binding=2) uniform sampler2D normalMap;

uniform sampler2D diffuseMap;
// this is set to the spec map (texture unit 1)
uniform sampler2D spec;
// normal map set a texture unit 2
uniform sampler2D normalMap;


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
  // lookup normal from normal map, move from [0,1] to  [-1, 1] range, normalize
  vec3 normal=normalize( texture(normalMap, fs_in.uv).xyz * 2.0 - 1.0);
  // we need to flip the z as this is done in zBrush
  normal.z = -normal.z;

  // compute specular lighting
  vec3 specularMaterial=vec3(0.01);//texture(spec, fs_in.uv).rgb;

  // get diffuse colour
  vec3 colour = texture(diffuseMap, fs_in.uv).rgb;
  // ambient
  vec3 ambient=vec3(0);
  vec3 diffuse=vec3(0);
  vec3 specular=vec3(0);
  for(int i=0; i<3; ++i)
  {
    ambient += light[i].ambient * colour;
    // diffuse
    vec3 lightDir = normalize(fs_in.tangentLightPos[i] - fs_in.tangentFragPos[i]);
    float LdotN = max(dot(lightDir, normal), 0.0);
    diffuse+= LdotN * colour *light[i].diffuse;
    // specular
    if(LdotN >0)
    {
      vec3 viewDir = normalize(fs_in.tangentViewPos[i] - fs_in.tangentFragPos[i]);
      vec3 reflectDir = reflect(-lightDir, normal);
      vec3 halfwayDir = normalize(lightDir + viewDir);
      float spec = pow(max(dot(normal, halfwayDir), 0.0), 500);
      specular += specularMaterial * spec * light[0].specular;
    }
  }
  Fragcolour = vec4(ambient + diffuse + specular, 1.0);
  //Fragcolour.rgb=normal;
}
