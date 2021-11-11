#version 330 core
in vec4 inputColor;
in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoord;
out vec4 outputColor;
uniform vec3 activeColor;
uniform float isSolid;
uniform vec3 lightPos;
uniform sampler2D textureImg;
void main() {
	float ambientStrength = 0.3;
	vec3 ambient = ambientStrength * vec3(1.0,1.0,1.0);
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(lightPos - FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * vec3(1.0,1.0,1.0);
	if(isSolid < 0.5f) {
		outputColor = vec4(texture(textureImg, TexCoord));
	} else {
		vec3 result = (ambient + diffuse) * activeColor;
		outputColor = vec4(result, 1.0);
	}
	
}