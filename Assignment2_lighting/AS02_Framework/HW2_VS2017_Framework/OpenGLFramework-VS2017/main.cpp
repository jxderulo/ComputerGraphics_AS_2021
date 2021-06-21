#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "textfile.h"

#include "Vectors.h"
#include "Matrices.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

using namespace std;

// Default window size
const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 800;

// <SELF ADD> Record window size
GLfloat current_window_width = WINDOW_WIDTH;
GLfloat current_window_height = WINDOW_HEIGHT;

bool mouse_pressed = false;
int starting_press_x = -1;
int starting_press_y = -1;

// <SELF ADD>
enum ModeType
{
	None = -1,
	Transformation = 0,
	Lighting = 1,
};

enum TransMode
{
	GeoTranslation = 0,
	GeoRotation = 1,
	GeoScaling = 2,
};

// <SELF ADD>
GLint iLocMVP;
GLint iLocMV;
GLint iLocKa;
GLint iLocKd;
GLint iLocKs;
GLint iLocDir;
GLint iLocPos;
GLint iLocMode;
GLint iLocDiffuse;
GLint iLocSpecular;
GLint iLocAmbient;
GLint iLocShininess;
GLint iLocAngle;
GLint iLocshademode;

vector<string> filenames; // .obj filename list

struct PhongMaterial
{
	Vector3 Ka;
	Vector3 Kd;
	Vector3 Ks;

};

typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	GLuint p_color;
	int vertex_count;
	GLuint p_normal;
	PhongMaterial material;
	int indexCount;
	GLuint m_texture;
} Shape;

struct model
{
	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 rotation = Vector3(0, 0, 0);	// Euler form

	vector<Shape> shapes;
};
vector<model> models;

struct camera
{
	Vector3 position;
	Vector3 center;
	Vector3 up_vector;
};
camera main_camera;

struct project_setting
{
	GLfloat nearClip, farClip;
	GLfloat fovy;
	GLfloat aspect;
	GLfloat left, right, top, bottom;
};
project_setting proj;

enum ProjMode
{
	Orthogonal = 0,
	Perspective = 1,
};

// <SELF ADD>
struct light {
	Vector3 position;
	Vector3 direction;
	Vector3 diffuse;
	Vector3 specular;
	GLint angle;
};

// <SELF ADD>
struct light_types {
	light directional;
	light point;
	light spot;
};
light_types Light;

// <SELF ADD>
enum LightMode
{
	LightSwitch = 0,
	LightEdit = 1,
	ShininessEdit = 2
};

// <SELF ADD>
enum LightType
{
	Directional = 0,
	Point = 1,
	Spot = 2,
};

// <SELF ADD>
ModeType cur_mode_type = None;
ProjMode cur_proj_mode = Orthogonal;
TransMode cur_trans_mode = GeoTranslation;
LightMode cur_light_mode = LightSwitch;
LightType cur_light_type = Directional;

Matrix4 view_matrix;
Matrix4 project_matrix;

Shape quad;
Shape m_shpae;
int cur_idx = 0; // represent which model should be rendered now

// <SELF ADD>
GLint shininess;
Vector3 ambient;

static GLvoid Normalize(GLfloat v[3])
{
	GLfloat l;

	l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] /= l;
	v[1] /= l;
	v[2] /= l;
}

static GLvoid Cross(GLfloat u[3], GLfloat v[3], GLfloat n[3])
{

	n[0] = u[1] * v[2] - u[2] * v[1];
	n[1] = u[2] * v[0] - u[0] * v[2];
	n[2] = u[0] * v[1] - u[1] * v[0];
}


// [TODO] given a translation vector then output a Matrix4 (Translation Matrix)
Matrix4 translate(Vector3 vec)
{
	Matrix4 mat;

	mat = Matrix4(
		1, 0, 0, vec[0],
		0, 1, 0, vec[1],
		0, 0, 1, vec[2],
		0, 0, 0, 1
	);

	return mat;
}

// [TODO] given a scaling vector then output a Matrix4 (Scaling Matrix)
Matrix4 scaling(Vector3 vec)
{
	Matrix4 mat;

	mat = Matrix4(
		vec[0], 0, 0, 0,
		0, vec[1], 0, 0,
		0, 0, vec[2], 0,
		0, 0, 0, 1
	);

	return mat;
}


// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
Matrix4 rotateX(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(
		1, 0, 0, 0,
		0, cos(val), -sin(val), 0,
		0, sin(val), cos(val), 0,
		0, 0, 0, 1
	);

	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Y (rotate alone axis-Y)
Matrix4 rotateY(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(
		cos(val), 0, sin(val), 0,
		0, 1, 0, 0,
		-sin(val), 0, cos(val), 0,
		0, 0, 0, 1
	);

	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Z (rotate alone axis-Z)
Matrix4 rotateZ(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(
		cos(val), -sin(val), 0, 0,
		sin(val), cos(val), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);

	return mat;
}

Matrix4 rotate(Vector3 vec)
{
	return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}

// [TODO] compute viewing matrix accroding to the setting of main_camera
void setViewingMatrix()
{
	Vector3 p1_p2, p1_p3;
	Vector3 Rx, Ry, Rz;
	Matrix4 R, T;

	p1_p2 = main_camera.center - main_camera.position;
	p1_p3 = main_camera.up_vector - main_camera.position;

	Rz = -p1_p2 / p1_p2.length();
	Rx = p1_p2.cross(p1_p3) / p1_p2.cross(p1_p3).length();
	Ry = Rz.cross(Rx);

	R = Matrix4(
		Rx[0], Rx[1], Rx[2], 0,
		Ry[0], Ry[1], Ry[2], 0,
		Rz[0], Rz[1], Rz[2], 0,
		0, 0, 0, 1
	);

	T = Matrix4(
		1, 0, 0, -main_camera.position.x,
		0, 1, 0, -main_camera.position.y,
		0, 0, 1, -main_camera.position.z,
		0, 0, 0, 1
	);
	view_matrix = R * T;
}

// [TODO] compute orthogonal projection matrix
void setOrthogonal()
{
	cur_proj_mode = Orthogonal;

	project_matrix[0] = 2 / (proj.right - proj.left);
	project_matrix[1] = project_matrix[2] = 0;
	project_matrix[3] = -(proj.right + proj.left) / (proj.right - proj.left);

	project_matrix[4] = 0;
	project_matrix[5] = 2 / (proj.top - proj.bottom);
	project_matrix[6] = 0;
	project_matrix[7] = -(proj.top + proj.bottom) / (proj.top - proj.bottom);

	project_matrix[8] = project_matrix[9] = 0;
	project_matrix[10] = -2 / (proj.farClip - proj.nearClip);
	project_matrix[11] = -(proj.farClip + proj.nearClip) / (proj.farClip - proj.nearClip);

	project_matrix[12] = project_matrix[13] = project_matrix[14] = 0;
	project_matrix[15] = 1;
}

// [TODO] compute persepective projection matrix
void setPerspective()
{
	cur_proj_mode = Perspective;

	GLfloat f;
	f = -1 / tan(proj.fovy / 2);

	project_matrix = Matrix4(
		f / proj.aspect, 0, 0, 0,
		0, f, 0, 0,
		0, 0, (proj.farClip + proj.nearClip) / (proj.nearClip - proj.farClip), 2 * proj.farClip * proj.nearClip / (proj.nearClip - proj.farClip),
		0, 0, -1, 0
	);
}


// Vertex buffers
GLuint VAO, VBO;

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	current_window_width = (GLfloat)width;
	current_window_height = (GLfloat)height;

	// [TODO] change your aspect ratio
	GLfloat ratio = GLfloat(width) / height;
	GLfloat span_x, span_y;

	span_x = (ratio > 1.0f) ? ratio : 1.0f;
	span_y = (ratio > 1.0f) ? 1.0f : 1.0f / ratio;

	proj.aspect = ratio / 2;
	proj.left = -span_x;
	proj.right = span_x;
	proj.bottom = -span_y;
	proj.top = span_y;

	cur_proj_mode == Perspective ? setPerspective() : setOrthogonal();
}

// <SELF ADD> compute phong model
void phong_model(int material_index, Matrix4 V, int shade_mode)
{
	PhongMaterial phong_material = models[cur_idx].shapes[material_index].material;
	Vector3 ka = phong_material.Ka;
	Vector3 kd = phong_material.Kd;
	Vector3 ks = phong_material.Ks;

	GLfloat Ka[3] = { ka[0], ka[1], ka[2] };
	GLfloat Kd[3] = { kd[0], kd[1], kd[2] };
	GLfloat Ks[3] = { ks[0], ks[1], ks[2] };

	light light_temp;
	switch(cur_light_type) {
		case Directional:
			light_temp = Light.directional;
			break;
		case Point:
			light_temp = Light.point;
			break;
		case Spot:
			light_temp = Light.spot;
			break;
	}
	
	GLfloat light_diffuse[3] = { light_temp.diffuse.x, light_temp.diffuse.y, light_temp.diffuse.z };
	GLfloat light_specular[3] = { light_temp.specular.x, light_temp.specular.y, light_temp.specular.z };
	GLfloat light_ambient[3] = { ambient.x, ambient.y, ambient.z };
	GLint light_angle = light_temp.angle;

	//change light position to view space
	Vector4 light_pos4 = Vector4(light_temp.position.x, light_temp.position.y, light_temp.position.z, 0) * V;
	GLfloat light_position[3] = { light_pos4.x, light_pos4.y, light_pos4.z };

	//change light direction to view space
	Vector4 light_dir4 = Vector4(light_temp.direction.x, light_temp.direction.y, light_temp.direction.z, 0) * V;
	GLfloat light_direction[3] = { light_dir4.x, light_dir4.y, light_dir4.z };

	// send to shader
	glUniform3fv(iLocKa, 1, Ka);
	glUniform3fv(iLocKd, 1, Kd);
	glUniform3fv(iLocKs, 1, Ks);
	glUniform3fv(iLocDir, 1, light_direction);
	glUniform3fv(iLocPos, 1, light_position);
	glUniform3fv(iLocDiffuse, 1, light_diffuse);
	glUniform3fv(iLocSpecular, 1, light_specular);
	glUniform3fv(iLocAmbient, 1, light_ambient);
	glUniform1i(iLocMode, cur_light_type);
	glUniform1i(iLocAngle, light_angle);
	glUniform1i(iLocShininess, shininess);
	glUniform1i(iLocshademode, shade_mode);
}

// Render function for display rendering
void RenderScene(void) {	
	// clear canvas
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	Matrix4 T, R, S;
	// [TODO] update translation, rotation and scaling
	T = translate(models[cur_idx].position);
	R = rotate(models[cur_idx].rotation);
	S = scaling(models[cur_idx].scale);

	Matrix4 MVP, MV;
	GLfloat mvp[16], mv[16];

	// [TODO] multiply all the matrix
	MVP = project_matrix * view_matrix * (T * R * S);

	// [TODO] row-major ---> column-major
	mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
	mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
	mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
	mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];   mvp[15] = MVP[15];

	// <SELF ADD>
	MV = view_matrix * (T * R * S);

	mv[0] = MV[0];  mv[4] = MV[1];   mv[8] = MV[2];    mv[12] = MV[3];
	mv[1] = MV[4];  mv[5] = MV[5];   mv[9] = MV[6];    mv[13] = MV[7];
	mv[2] = MV[8];  mv[6] = MV[9];   mv[10] = MV[10];  mv[14] = MV[11];
	mv[3] = MV[12]; mv[7] = MV[13];  mv[11] = MV[14];  mv[15] = MV[15];

	// use uniform to send mvp to vertex shader
	glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);
	glUniformMatrix4fv(iLocMV, 1, GL_FALSE, mv);
	for (int i = 0; i < models[cur_idx].shapes.size(); i++) 
	{
		glViewport(0, 0, GLfloat(current_window_width) / 2, current_window_height);
		phong_model(i, view_matrix, 0);
		glBindVertexArray(models[cur_idx].shapes[i].vao);
		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);

		glViewport(GLfloat(current_window_width) / 2, 0, GLfloat(current_window_width) / 2, current_window_height);
		phong_model(i, view_matrix, 1);
		glBindVertexArray(models[cur_idx].shapes[i].vao);
		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);

	}

}


void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// [TODO] Call back function for keyboard
	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_Z:
			cur_idx = (cur_idx - 1 + 5) % 5;
			break;
		case GLFW_KEY_X:
			cur_idx = (cur_idx + 1) % 5;
			break;
		case GLFW_KEY_O:
			setOrthogonal();
			break;
		case GLFW_KEY_P:
			setPerspective();
			break;
		case GLFW_KEY_T:
			cur_mode_type = Transformation;
			cur_trans_mode = GeoTranslation;
			break;
		case GLFW_KEY_S:
			cur_mode_type = Transformation;
			cur_trans_mode = GeoScaling;
			break;
		case GLFW_KEY_R:
			cur_mode_type = Transformation;
			cur_trans_mode = GeoRotation;
			break;
		case GLFW_KEY_L:
			cur_mode_type = Lighting;
			cur_light_type = (LightType)((cur_light_type + 1) % 3);
			break;
		case GLFW_KEY_K:
			cur_mode_type = Lighting;
			cur_light_mode = LightEdit;
			break;
		case GLFW_KEY_J:
			cur_mode_type = Lighting;
			cur_light_mode = ShininessEdit;
			break;
		}
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// [TODO] scroll up positive, otherwise it would be negtive
	const float TICK = 0.01;
	if (cur_mode_type == Transformation) {
		switch (cur_trans_mode) {
		case GeoTranslation:
			models[cur_idx].position += Vector3(0, 0, TICK * yoffset);
			break;
		case GeoScaling:
			models[cur_idx].scale += Vector3(0, 0, TICK * yoffset);
			break;
		case GeoRotation:
			models[cur_idx].rotation += Vector3(0, 0, TICK * yoffset);
			break;
		case None:
			break;
		}
	}
	else if (cur_mode_type == Lighting) {
		if (cur_light_mode == LightEdit) {
			switch (cur_light_type) {
			case Directional:
				Light.directional.diffuse += Vector3(TICK * yoffset, TICK * yoffset, TICK * yoffset);
				break;
			case Point:
				Light.point.diffuse += Vector3(TICK * yoffset, TICK * yoffset, TICK * yoffset);
				break;
			case Spot:
				Light.spot.angle += yoffset;
				break;
			}
		}
		else if(cur_light_mode == ShininessEdit) {
			shininess += yoffset;
		}
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// [TODO] mouse press callback function
	mouse_pressed = (action == GLFW_PRESS);
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	// [TODO] cursor position callback function
	const float TICK = 0.01;

	if (mouse_pressed) {
		if (cur_mode_type == Transformation) {
			switch (cur_trans_mode) {
			case GeoTranslation:
				models[cur_idx].position +=
					Vector3(TICK * (xpos - starting_press_x), -TICK * (ypos - starting_press_y), 0);
				break;
			case GeoScaling:
				models[cur_idx].scale -=
					Vector3(TICK * (xpos - starting_press_x), TICK * (ypos - starting_press_y), 0);
				break;
			case GeoRotation:
				models[cur_idx].rotation -=
					Vector3(TICK * (ypos - starting_press_y), TICK * (xpos - starting_press_x), 0);
				break;
			case None:
				break;
			}
		}
		else if (cur_mode_type == Lighting) {
			if (cur_light_mode == LightEdit) {
				switch (cur_light_type) {
				case Directional:
					Light.directional.position +=
						Vector3(TICK * (xpos - starting_press_x), -TICK * (ypos - starting_press_y), 0);
					break;
				case Point:
					Light.point.position +=
						Vector3(TICK * (xpos - starting_press_x), -TICK * (ypos - starting_press_y), 0);
					break;
				case Spot:
					Light.spot.position +=
						Vector3(TICK * (xpos - starting_press_x), -TICK * (ypos - starting_press_y), 0);
					break;
				}
			}
		}
	}
	starting_press_x = int(xpos);
	starting_press_y = int(ypos);
}

void setShaders()
{
	GLuint v, f, p;
	char *vs = NULL;
	char *fs = NULL;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead("shader.vs");
	fs = textFileRead("shader.fs");

	glShaderSource(v, 1, (const GLchar**)&vs, NULL);
	glShaderSource(f, 1, (const GLchar**)&fs, NULL);

	free(vs);
	free(fs);

	GLint success;
	char infoLog[1000];
	// compile vertex shader
	glCompileShader(v);
	// check for shader compile errors
	glGetShaderiv(v, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(v, 1000, NULL, infoLog);
		std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// compile fragment shader
	glCompileShader(f);
	// check for shader compile errors
	glGetShaderiv(f, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(f, 1000, NULL, infoLog);
		std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// create program object
	p = glCreateProgram();

	// attach shaders to program object
	glAttachShader(p,f);
	glAttachShader(p,v);

	// link program
	glLinkProgram(p);
	// check for linking errors
	glGetProgramiv(p, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(p, 1000, NULL, infoLog);
		std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
	}

	glDeleteShader(v);
	glDeleteShader(f);

	// <SELF ADD>
	iLocMVP = glGetUniformLocation(p, "mvp");
	iLocMV = glGetUniformLocation(p, "mv");
	iLocDir = glGetUniformLocation(p, "light_dir");
	iLocPos = glGetUniformLocation(p, "light_pos");
	iLocKa = glGetUniformLocation(p, "ka");
	iLocKd = glGetUniformLocation(p, "kd");
	iLocKs = glGetUniformLocation(p, "ks");
	iLocMode = glGetUniformLocation(p, "cur_light_type");
	iLocDiffuse = glGetUniformLocation(p, "diffuse");
	iLocSpecular = glGetUniformLocation(p, "specular");
	iLocAmbient = glGetUniformLocation(p, "ambient");
	iLocShininess = glGetUniformLocation(p, "shininess");
	iLocAngle = glGetUniformLocation(p, "angle");
	iLocshademode = glGetUniformLocation(p, "shade_mode");

	if (success)
		glUseProgram(p);
    else
    {
        system("pause");
        exit(123);
    }
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, tinyobj::shape_t* shape)
{
	vector<float> xVector, yVector, zVector;
	float minX = 10000, maxX = -10000, minY = 10000, maxY = -10000, minZ = 10000, maxZ = -10000;

	// find out min and max value of X, Y and Z axis
	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//maxs = max(maxs, attrib->vertices.at(i));
		if (i % 3 == 0)
		{

			xVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minX)
			{
				minX = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxX)
			{
				maxX = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 1)
		{
			yVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minY)
			{
				minY = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxY)
			{
				maxY = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 2)
		{
			zVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minZ)
			{
				minZ = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxZ)
			{
				maxZ = attrib->vertices.at(i);
			}
		}
	}

	float offsetX = (maxX + minX) / 2;
	float offsetY = (maxY + minY) / 2;
	float offsetZ = (maxZ + minZ) / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		if (offsetX != 0 && i % 3 == 0)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetX;
		}
		else if (offsetY != 0 && i % 3 == 1)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetY;
		}
		else if (offsetZ != 0 && i % 3 == 2)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetZ;
		}
	}

	float greatestAxis = maxX - minX;
	float distanceOfYAxis = maxY - minY;
	float distanceOfZAxis = maxZ - minZ;

	if (distanceOfYAxis > greatestAxis)
	{
		greatestAxis = distanceOfYAxis;
	}

	if (distanceOfZAxis > greatestAxis)
	{
		greatestAxis = distanceOfZAxis;
	}

	float scale = greatestAxis / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//std::cout << i << " = " << (double)(attrib.vertices.at(i) / greatestAxis) << std::endl;
		attrib->vertices.at(i) = attrib->vertices.at(i) / scale;
	}
	size_t index_offset = 0;
	for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); f++) {
		int fv = shape->mesh.num_face_vertices[f];

		// Loop over vertices in the face.
		for (size_t v = 0; v < fv; v++) {
			// access to vertex
			tinyobj::index_t idx = shape->mesh.indices[index_offset + v];
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 0]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 1]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 2]);
			// Optional: vertex colors
			colors.push_back(attrib->colors[3 * idx.vertex_index + 0]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 1]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 2]);
			// Optional: vertex normals
			if (idx.normal_index >= 0) {
				normals.push_back(attrib->normals[3 * idx.normal_index + 0]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 1]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 2]);
			}
		}
		index_offset += fv;
	}
}

string GetBaseDir(const string& filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos)
		return filepath.substr(0, filepath.find_last_of("/\\"));
	return "";
}

void LoadModels(string model_path)
{
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	tinyobj::attrib_t attrib;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;
	vector<GLfloat> normals;

	string err;
	string warn;

	string base_dir = GetBaseDir(model_path); // handle .mtl with relative path

#ifdef _WIN32
	base_dir += "\\";
#else
	base_dir += "/";
#endif

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str(), base_dir.c_str());

	if (!warn.empty()) {
		cout << warn << std::endl;
	}

	if (!err.empty()) {
		cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	printf("Load Models Success ! Shapes size %d Material size %d\n", shapes.size(), materials.size());
	model tmp_model;

	vector<PhongMaterial> allMaterial;
	for (int i = 0; i < materials.size(); i++)
	{
		PhongMaterial material;
		material.Ka = Vector3(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
		material.Kd = Vector3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
		material.Ks = Vector3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
		allMaterial.push_back(material);
	}

	for (int i = 0; i < shapes.size(); i++)
	{

		vertices.clear();
		colors.clear();
		normals.clear();
		normalization(&attrib, vertices, colors, normals, &shapes[i]);
		// printf("Vertices size: %d", vertices.size() / 3);

		Shape tmp_shape;
		glGenVertexArrays(1, &tmp_shape.vao);
		glBindVertexArray(tmp_shape.vao);

		glGenBuffers(1, &tmp_shape.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT), &vertices.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		tmp_shape.vertex_count = vertices.size() / 3;

		glGenBuffers(1, &tmp_shape.p_color);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
		glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GL_FLOAT), &colors.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glGenBuffers(1, &tmp_shape.p_normal);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_normal);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GL_FLOAT), &normals.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		// not support per face material, use material of first face
		if (allMaterial.size() > 0)
			tmp_shape.material = allMaterial[shapes[i].mesh.material_ids[0]];
		tmp_model.shapes.push_back(tmp_shape);
	}
	shapes.clear();
	materials.clear();
	models.push_back(tmp_model);
}

void initParameter()
{
	proj.left = -1;
	proj.right = 1;
	proj.top = 1;
	proj.bottom = -1;
	proj.nearClip = 0.001;
	proj.farClip = 100.0;
	proj.fovy = 80;
	proj.aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT / 2;

	main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
	main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);

	// <SELF ADD>
	Light.directional.position = Vector3(1.0f, 1.0f, 1.0f);
	Light.directional.direction = Vector3(0.0f, 0.0f, 0.0f);
	Light.directional.diffuse = Vector3(1.0f, 1.0f, 1.0f);
	Light.directional.specular = Vector3(1.0f, 1.0f, 1.0f);
	Light.point.position = Vector3(0.0f, 2.0f, 1.0f);
	Light.point.diffuse = Vector3(1.0f, 1.0f, 1.0f);
	Light.point.specular = Vector3(1.0f, 1.0f, 1.0f);
	Light.spot.position = Vector3(0.0f, 0.0f, 2.0f);
	Light.spot.direction = Vector3(0.0f, 0.0f, -1.0f);
	Light.spot.diffuse = Vector3(1.0f, 1.0f, 1.0f);
	Light.spot.specular = Vector3(1.0f, 1.0f, 1.0f);
	Light.spot.angle = 30;

	shininess = 64;
	ambient = Vector3(0.15f, 0.15f, 0.15f);

	setViewingMatrix();
	setPerspective();	//set default projection matrix as perspective matrix
}

void setupRC()
{
	// setup shaders
	setShaders();
	initParameter();

	// OpenGL States and Values
	glClearColor(0.2, 0.2, 0.2, 1.0);
	vector<string> model_list{ "../NormalModels/bunny5KN.obj", "../NormalModels/dragon10KN.obj", "../NormalModels/lucy25KN.obj", "../NormalModels/teapot4KN.obj", "../NormalModels/dolphinN.obj"};
	// [TODO] Load five model at here
	for (string model : model_list) {
		LoadModels(model);
	}
}

void glPrintContextInfo(bool printExtension)
{
	cout << "GL_VENDOR = " << (const char*)glGetString(GL_VENDOR) << endl;
	cout << "GL_RENDERER = " << (const char*)glGetString(GL_RENDERER) << endl;
	cout << "GL_VERSION = " << (const char*)glGetString(GL_VERSION) << endl;
	cout << "GL_SHADING_LANGUAGE_VERSION = " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	if (printExtension)
	{
		GLint numExt;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
		cout << "GL_EXTENSIONS =" << endl;
		for (GLint i = 0; i < numExt; i++)
		{
			cout << "\t" << (const char*)glGetStringi(GL_EXTENSIONS, i) << endl;
		}
	}
}


int main(int argc, char **argv)
{
    // initial glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
#endif

    
    // create window
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "109065539 HW2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    
    // load OpenGL function pointer
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
	// register glfw callback functions
    glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);

    glfwSetFramebufferSizeCallback(window, ChangeSize);
	glEnable(GL_DEPTH_TEST);
	// Setup render context
	setupRC();

	// main loop
    while (!glfwWindowShouldClose(window))
    {
        // render
        RenderScene();
        
        // swap buffer from back to front
        glfwSwapBuffers(window);
        
        // Poll input event
        glfwPollEvents();
    }
	
	// just for compatibiliy purposes
	return 0;
}
