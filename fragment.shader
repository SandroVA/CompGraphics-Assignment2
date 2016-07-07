#version 330 core

uniform vec4 color;
uniform vec3 startPos;
uniform vec3 endPos;
uniform vec3 viewPos;
uniform bool useLighting;

in vec3 outNormal;
in vec3 fragPosition;

out vec4 outputF;
 
void main()
{
	if (useLighting)
	{
		vec3 lightColour[2];
		lightColour[0] = vec3(1.0f, 0.1f, 0.1f);
		lightColour[1] = vec3(0.1f, 1.0f, 0.1f);

		vec3 light_position[2];
		light_position[0] = startPos;
		light_position[1] = endPos;

		vec3 resultantColour;

		for (int i = 0; i < 2; i++)
		{
			//ambient lighting
			float ambientStrength = 0.1f;
			vec3 ambient_contribution = ambientStrength * lightColour[i];

			//diffuse lighting
			vec3 norm = normalize(outNormal);
			vec3 light_direction = normalize(light_position[i] - fragPosition);
			float incident_degree = max(dot(norm, light_direction), 0.0f);
			vec3 diffuse_contribution = incident_degree * lightColour[i];

			//specular lighting
			float specularStrength = 0.1f;
			vec3 viewDir = normalize(viewPos - fragPosition);
			vec3 reflectDir = reflect(-light_direction, norm);
			float spec = pow(max(dot(viewDir, reflectDir), 0.0), 512);
			vec3 specular_contribution = specularStrength * spec * lightColour[i];
			
			float distance = length(light_position[i] - fragPosition)/10;

			resultantColour = resultantColour + ((ambient_contribution + diffuse_contribution + specular_contribution) * vec3(color))/distance;
		}
		
		//ambient lighting
		float ambientStrength = 0.1f;
		vec3 ambient_contribution = ambientStrength * vec3(1.0f, 1.0f, 1.0f);

		//diffuse lighting
		vec3 norm = normalize(outNormal);
		vec3 light_direction = normalize(viewPos);
		float incident_degree = max(dot(norm, light_direction), 0.0f);
		vec3 diffuse_contribution = incident_degree * vec3(1.0f, 1.0f, 1.0f);

		//specular lighting
		float specularStrength = 0.1f;
		vec3 viewDir = normalize(viewPos - fragPosition);
		vec3 reflectDir = reflect(-light_direction, norm);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), 512);
		vec3 specular_contribution = specularStrength * spec * vec3(1.0f, 1.0f, 1.0f);
		
		float distance = length(viewPos - fragPosition)/10;
		resultantColour = resultantColour + ((ambient_contribution + diffuse_contribution + specular_contribution) * vec3(color))/distance;
		
		outputF = vec4(resultantColour, 1.0f);
	}
	else
	{
		outputF = color;
	}
}