#version 330
# define PI 3.14159265358979323846
# define Directional 0
# define Point 1
# define Spot 2

in vec2 texCoord;
in vec3 vertex_color;
in vec3 vertex_normal;
in vec3 vertex_pos;

out vec4 fragColor;

uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform vec3 light_dir;
uniform vec3 light_pos;
uniform vec3 diffuse;
uniform vec3 specular;
uniform vec3 ambient;
uniform int cur_light_type;
uniform int shininess;
uniform int angle;
uniform int shade_mode;

// [TODO] passing texture from main.cpp
// Hint: sampler2D
uniform sampler2D texmap;
uniform vec2 eye_offset;

void main() {

	// [TODO] sampleing from texture
	// Hint: texture

	vec3 result;

	//---Ambient Light---
	vec3 Ia = ambient;
	
	//---Diffuse Reflection---
	vec3 Id = diffuse;
	vec3 N = normalize(vertex_normal); //normalize normal vector
	vec3 V = -normalize(vertex_pos); //normalize viewpoint vector
	vec3 L; //normalize light vector

	if (cur_light_type == Directional)
		L = normalize(light_pos);
	else if (cur_light_type == Point || cur_light_type == Spot)
		L = normalize(light_pos - vertex_pos);
	
	float diffuse_cos = max(dot(N, L), 0.0); // ppt p.20

	//---Specular Highlight---
	vec3 Is = specular;

	//half way vector
	vec3 H = normalize( L + V);
	float spec = pow(max(dot(H, N), 0.0), shininess); //specular reflection ppt p.27

	//---Attenuation---
	float attenuation_p;
	float attenuation_s;
	if (cur_light_type == Point){
		float distance = length(light_pos - vertex_pos);
		attenuation_p = 1.0 / (0.01 + 0.8 * distance + 0.1 * distance * distance);
	}
	else if (cur_light_type == Spot){
		float distance = length(light_pos - vertex_pos);
		attenuation_s = 1.0 / (0.05 + 0.3 * distance + 0.6 * distance * distance);
	}

	//---result---
	if (cur_light_type == Directional){
		result = Ia * ka + Id * kd * diffuse_cos  + Is * ks * spec;
	}
	else if (cur_light_type == Point){
		result = attenuation_p * (Ia * ka + Id * kd * diffuse_cos +  Is * ks * spec);
	}
	else if (cur_light_type == Spot){
		float theta = dot(normalize(vertex_pos - light_pos), normalize(light_dir));
		if (theta <= cos(angle * PI / 180)){
			result = Ia * ka;
		}
		else if (theta > cos(angle * PI / 180)){
			float spot_effect = pow(max(theta, 0), 50); //ppt p.49
			result = Ia * ka + spot_effect * (Id * kd * diffuse_cos + Is * ks * spec);
		}
	}
	

	if (shade_mode == 0) fragColor = vec4(result, 1.0f) * texture2D(texmap,texCoord.xy + eye_offset);
	if (shade_mode == 1) fragColor = vec4(vertex_color, 1.0f) * texture2D(texmap,texCoord.xy + eye_offset);

}
