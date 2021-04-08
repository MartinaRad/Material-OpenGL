#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D diffuse; //only diffuse map, the ambient usually has the same color as the diffuse 
    sampler2D specular;    
	float ka; //ambient coefficient
	float kd; //diffuse coefficient
	float ks; //specular coefficient
    float shininess;
}; 

struct Light {
    vec3 position;
    vec3 diffuse; //the color of the light
};

in vec3 FragPos;  
in vec3 Normal;  
in vec2 TexCoords;
  
uniform vec3 viewPos;
uniform Material material;
uniform Light light;

void main()
{
    // ambient
    vec3 ambient = material.ka * texture(material.diffuse, TexCoords).rgb;
  	
    // diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0); //cos to light direction
    vec3 diffuse = light.diffuse * material.kd * diff * texture(material.diffuse, TexCoords).rgb;
    
    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
	//cos to the power of shininess of angle between light reflected ray and view direction
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess); 
    vec3 specular = material.ks * spec * texture(material.specular, TexCoords).rgb;  
        
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1);
} 


