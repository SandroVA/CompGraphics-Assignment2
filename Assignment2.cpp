#include "stdafx.h"

#include "..\glew\glew.h"	// include GL Extension Wrangler
#include "..\glfw\glfw3.h"	// include GLFW helper library
#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"
#include <vector>
#include <math.h>

using namespace std;

//All VAOs and VBOs
GLuint VAO, BezierVAO, VBO, BezierVBO, TrainVAO, TrainVerticeVBO, TrainNormalsVBO;
GLuint shaderProgram;

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 800;
int width, height;

glm::mat4 projection_matrix;

glm::mat4 camera_look_at;
glm::vec3 camera_translation = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 look_at = glm::vec3(0.0, 0.0, 0.0);

int numPoints = 0;
int MAX_LINE_LENGTH = 30;
double trainPosition = 0;

vector<glm::vec3> user_clicks;
vector<glm::vec3> bezier_Points;
vector<float> bezier_Points_Rotation;

bool renderLines = true;
bool drawBezierCurve = false;
bool trainMode = false;
bool animateTrain = false;
bool lighting = true;

void getNumberOfPointsNeededToDraw() {
	cout << "Enter the number of points (N) for the Bezier Splines. \n(This number must be greater than 3)" << endl;
	while (numPoints <= 3) {
		cin >> numPoints;
		if (cin.fail() || numPoints <= 3) {
			cin.clear();
			cin.ignore(256, '\n');
			numPoints = 0;
			cout << "Error, please enter an integer greater than 3." << endl;
		}
	}
}

void GetBezierPointsRotation()
{
	for (int i = 0; i < bezier_Points.size() - 1; i++) {
		glm::vec3 from_vector = bezier_Points[i];
		glm::vec3 to_vector = bezier_Points[i+1];
		glm::vec3 diff = to_vector - from_vector;
		float angle = glm::degrees(atan2(diff.x, diff.y));
		bezier_Points_Rotation.push_back(angle);
	}
}

glm::vec3 CalculateBezierPoint(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
	glm::vec3 p = pow((1 - t), 3) * p0; //first term
	p += 3 * pow((1 - t), 2) * t * p1; //second term
	p += 3 * (1 - t) * pow(t, 2) * p2; //third term
	p += pow(t, 3) * p3; //fourth term

	return p;
}

void Subdivide(float u0, float u1, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3)
{
	float umid = (u0 + u1) / 2;
	glm::vec3 result1 = CalculateBezierPoint(u0, p0, p1, p2, p3);
	glm::vec3 result2 = CalculateBezierPoint(u1, p0, p1, p2, p3);

	int distancex = pow(abs(result2.x - result1.x),2);
	int distancey = pow(abs(result2.y - result1.y),2);
	double calcDistance = sqrt(distancex + distancey);
	if (calcDistance > MAX_LINE_LENGTH)
	{
		Subdivide(u0, umid, p0, p1, p2, p3);
		Subdivide(umid, u1, p0, p1, p2, p3);
	}
	else
	{
		bezier_Points.push_back(result1);
	}
}

void GetBezierPoints()
{
	vector<glm::vec3> temp_points;
	glm::vec3 p0;
	glm::vec3 p1;
	glm::vec3 p2;
	glm::vec3 p3;
	for (int i = 0; i < user_clicks.size() - 3; i += 3)
	{
		p0 = user_clicks[i];
		p1 = user_clicks[i + 1];
		p2 = user_clicks[i + 2];
		p3 = user_clicks[i + 3];

		Subdivide(0, 1, p0, p1, p2, p3);
	}

	glm::vec3 result = CalculateBezierPoint(1, p0, p1, p2, p3);
	bezier_Points.push_back(result);
}

void renderTrain(GLuint transformLoc, GLuint viewMatrixLoc, GLuint projectionLoc, GLuint colorLoc, GLuint startPosLoc, GLuint endPosLoc, GLuint lightingLoc, GLuint viewPosLoc) {
	GLfloat rail_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat floor_color[] = { 0.8f, 0.8f, 0.8f, 1.0f };

	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection_matrix));

	if (drawBezierCurve) {
		glUniform3f(startPosLoc, bezier_Points[0].x, bezier_Points[0].y, bezier_Points[0].z);
		glUniform3f(endPosLoc, bezier_Points[bezier_Points.size()-1].x, bezier_Points[bezier_Points.size() - 1].y, bezier_Points[bezier_Points.size() - 1].z);
		glUniform1i(lightingLoc, lighting);

		if (animateTrain) {
			if (trainPosition < bezier_Points.size()-1) {
				camera_translation = bezier_Points[floor(trainPosition)];
				camera_translation.z -= 4;
				look_at = bezier_Points[floor(trainPosition + 1)];
				look_at.z -= 5;
			}
			else {
				camera_translation = bezier_Points[0];
				camera_translation.z -= 4;
				look_at = bezier_Points[1];
				look_at.z -= 5;
				trainPosition = 0;
				animateTrain = false;
			}
		}
		else {
			trainPosition = 0;
			camera_translation = bezier_Points[0];
			camera_translation.z -= 4;
			look_at = bezier_Points[1];
			look_at.z -= 5;
		}
		glUniform3f(viewPosLoc, camera_translation.x, camera_translation.y, camera_translation.z);
		camera_look_at = glm::lookAt(camera_translation, look_at, glm::vec3(0.0, 0.0, -1.0));

		glUniformMatrix4fv(viewMatrixLoc, 1, GL_FALSE, glm::value_ptr(camera_look_at));

		glBindVertexArray(TrainVAO);

		glProgramUniform4fv(shaderProgram, colorLoc, 1, floor_color);
		glm::mat4 floor_location;
		floor_location = glm::translate(floor_location, glm::vec3(bezier_Points[0].x, bezier_Points[0].y, 0.0));
		glm::vec3 floor_scale = glm::vec3(1100, 1100, -0.2);
		floor_location = glm::scale(floor_location, floor_scale);
		glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(floor_location));
		glDrawArrays(GL_TRIANGLES, 0, 36);

		glProgramUniform4fv(shaderProgram, colorLoc, 1, rail_color);
		for (int i = 0; i < bezier_Points.size(); i++) {
			glm::mat4 rail_location;
			rail_location = glm::translate(rail_location, bezier_Points[i]);
			if (i < bezier_Points.size() - 1)
			{
				//rail_location = glm::rotate(rail_location, bezier_Points_Rotation[i], glm::vec3(0.0f, 0.0f, 1.0f));
			}
			glm::vec3 rail_scale = glm::vec3(3.4,1,-0.2);
			rail_location = glm::scale(rail_location, rail_scale);
			glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(rail_location));

			glDrawArrays(GL_TRIANGLES, 0, 36);
		}
		glBindVertexArray(0);
		trainPosition += 0.07;
	}
}

void renderBezier(GLuint transformLoc, GLuint viewMatrixLoc, GLuint projectionLoc, GLuint colorLoc, GLuint lightingLoc) {
	GLfloat user_click_color[] = { 0.2f, 0.6f, 1.0f, 1.0f };
	GLfloat bezier_curve_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };

	glUniformMatrix4fv(viewMatrixLoc, 1, GL_FALSE, glm::value_ptr(camera_look_at));
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection_matrix));

	if (drawBezierCurve) {
		glBindVertexArray(BezierVAO);
		glProgramUniform4fv(shaderProgram, colorLoc, 1, bezier_curve_color);
		glUniform1i(lightingLoc, false);

		glBindBuffer(GL_ARRAY_BUFFER, BezierVBO);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

		glm::mat4 point_location;
		glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(point_location));

		if (renderLines) {
			glDrawArrays(GL_LINE_STRIP, 0, bezier_Points.size());
		}
		else {
			glDrawArrays(GL_POINTS, 0, bezier_Points.size());
		}

		glBindVertexArray(0);
	}

	glBindVertexArray(VAO);
	glProgramUniform4fv(shaderProgram, colorLoc, 1, user_click_color);
	for (int i = 0; i < user_clicks.size(); i++) {
		glm::mat4 point_location;
		point_location = glm::translate(point_location, user_clicks[i]);
		glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(point_location));

		glDrawArrays(GL_POINTS, 0, 3);
	}
	glBindVertexArray(0);
}

void framebuffer_size_callback(GLFWwindow* window, int w, int h) {
	glViewport(0, 0, w, h);
	height = h;
	width = w;

	// Update the Projection matrix after a window resize event
	if (trainMode) {
		projection_matrix = glm::perspective(45.0f, (float)w / (float)h, 0.1f, 1000.0f);
	}
	else {
		projection_matrix = glm::ortho(0.0f, (float)w, (float)h, 0.0f, 0.1f, 100.0f);
	}
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_T && action == GLFW_PRESS) {
		if (trainMode) {
			cout << "Entering Bezier Mode." << endl;
			camera_translation = glm::vec3(0.0f, 0.0f, 1.0f);
			camera_look_at = glm::lookAt(camera_translation, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
			trainMode = false;
			animateTrain = false;
			trainPosition = 0;
			//Projection matrix so that everything is properly sized
			projection_matrix = glm::ortho(0.0f, (float)width, (float)height, 0.0f, 0.1f, 100.0f);
		}
		else if(drawBezierCurve){
			cout << "Entering Train Mode." << endl;
			//Projection matrix so that everything is properly sized
			projection_matrix = glm::perspective(45.0f, (float)width / (float)height, 0.1f, 1000.0f);
			trainMode = true;
		}
	}

	if (trainMode) {
		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && !animateTrain) {
			animateTrain = true;
		}
		if (key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS && animateTrain) {
			trainPosition = 0;
			animateTrain = false;
		}
		if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
			if (lighting) {
				lighting = false;
			}
			else {
				lighting = true;
			}
		}
	}
	else {
		if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
			if (user_clicks.size() == numPoints) {
				bezier_Points.clear();
				bezier_Points_Rotation.clear();
				GetBezierPoints();
				GetBezierPointsRotation();
				drawBezierCurve = true;

				glGenVertexArrays(1, &BezierVAO);
				glGenBuffers(1, &BezierVBO);
				// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
				glBindVertexArray(BezierVAO);

				glBindBuffer(GL_ARRAY_BUFFER, BezierVBO);
				glBufferData(GL_ARRAY_BUFFER, bezier_Points.size() * sizeof(glm::vec3), &bezier_Points[0], GL_STATIC_DRAW);

				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
				glEnableVertexAttribArray(0);

				glBindBuffer(GL_ARRAY_BUFFER, 0); // Note that this is allowed, the call to glVertexAttribPointer registered VBO as the currently bound vertex buffer object so afterwards we can safely unbind

				glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs), remember: do NOT unbind the EBO, keep it bound to this VAO
			}
			else {
				cout << "Please enter more points." << endl;
			}
		}

		if (key == GLFW_KEY_P && action == GLFW_PRESS) {
			cout << "Switching to points view." << endl;
			renderLines = false;
		}

		if (key == GLFW_KEY_L && action == GLFW_PRESS) {
			cout << "Switching to line strip view." << endl;
			renderLines = true;
		}

		if (key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS) {
			drawBezierCurve = false;
			bezier_Points.clear();
			user_clicks.clear();
			numPoints = 0;
			getNumberOfPointsNeededToDraw();
		}
	}
}

static void mouse_callback(GLFWwindow* window, int button, int action, int mods) {
	if (!trainMode) {
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			if (GLFW_PRESS == action) {
				if (user_clicks.size() < numPoints) {
					double xpos, ypos;
					glfwGetCursorPos(window, &xpos, &ypos);
					user_clicks.push_back(glm::vec3(xpos, ypos, 0.0f));
					cout << "Point Pressed: (" << xpos << "," << ypos << "), total points now: " << user_clicks.size() << endl;
				}
				else {
					cout << "You have reached the maximum number of points. \nPlease press BackSpace to reset the program" << endl;
				}
			}
		}
	}
}

// The MAIN function, from here we start the application and run the game loop
int main() {
	cout << "Starting GLFW context, OpenGL 3.3" << endl;
	// Init GLFW
	glfwInit();
	// Set all the required options for GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

	// Create a GLFWwindow object that we can use for GLFW's functions
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Assignment 2", nullptr, nullptr);
	if (window == nullptr) {
		cout << "Failed to create GLFW window" << endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	// Set the required callback functions
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
	glewExperimental = GL_TRUE;
	// Initialize GLEW to setup the OpenGL Function pointers
	if (glewInit() != GLEW_OK) {
		cout << "Failed to initialize GLEW" << endl;
		return -1;
	}

	// Define the viewport dimensions
	glfwGetFramebufferSize(window, &width, &height);

	glViewport(0, 0, width, height);

	// Build and compile our shader program
	// Vertex shader

	// Read the Vertex Shader code from the file
	string vertex_shader_path = "vertex.shader";
	string VertexShaderCode;
	ifstream VertexShaderStream(vertex_shader_path, ios::in);

	if (VertexShaderStream.is_open()) {
		string Line = "";
		while (getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}
	else {
		printf("Impossible to open %s. Are you in the right directory ?\n", vertex_shader_path.c_str());
		getchar();
		exit(-1);
	}

	// Read the Fragment Shader code from the file
	string fragment_shader_path = "fragment.shader";
	string FragmentShaderCode;
	ifstream FragmentShaderStream(fragment_shader_path, ios::in);

	if (FragmentShaderStream.is_open()) {
		string Line = "";
		while (getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}
	else {
		printf("Impossible to open %s. Are you in the right directory?\n", fragment_shader_path.c_str());
		getchar();
		exit(-1);
	}

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(vertexShader, 1, &VertexSourcePointer, NULL);
	glCompileShader(vertexShader);
	// Check for compile time errors
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(fragmentShader, 1, &FragmentSourcePointer, NULL);
	glCompileShader(fragmentShader);
	// Check for compile time errors
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
	}
	// Link shaders
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Check for linking errors
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
	}
	glDeleteShader(vertexShader); //free up memory
	glDeleteShader(fragmentShader);

	glUseProgram(shaderProgram);

	GLfloat point_vertex[] = {
		0, 0, 0
	};

	GLfloat cube_Normals[] = {
		0.0f,0.0f,-1.0f,
		0.0f,0.0f,-1.0f,
		0.0f,0.0f,-1.0f,
		0.0f,0.0f,-1.0f,
		0.0f,0.0f,-1.0f,
		0.0f,0.0f,-1.0f,
		0.0f,0.0f,1.0f,
		0.0f,0.0f,1.0f,
		0.0f,0.0f,1.0f,
		0.0f,0.0f,1.0f,
		0.0f,0.0f,1.0f,
		0.0f,0.0f,1.0f,
		-1.0f,0.0f,0.0f,
		-1.0f,0.0f,0.0f,
		-1.0f,0.0f,0.0f,
		-1.0f,0.0f,0.0f,
		-1.0f,0.0f,0.0f,
		-1.0f,0.0f,0.0f,
		1.0f,0.0f,0.0f,
		1.0f,0.0f,0.0f,
		1.0f,0.0f,0.0f,
		1.0f,0.0f,0.0f,
		1.0f,0.0f,0.0f,
		1.0f,0.0f,0.0f,
		0.0f,-1.0f,0.0f,
		0.0f,-1.0f,0.0f,
		0.0f,-1.0f,0.0f,
		0.0f,-1.0f,0.0f,
		0.0f,-1.0f,0.0f,
		0.0f,-1.0f,0.0f,
		0.0f,1.0f,0.0f,
		0.0f,1.0f,0.0f,
		0.0f,1.0f,0.0f,
		0.0f,1.0f,0.0f,
		0.0f,1.0f,0.0f,
		0.0f,1.0f,0.0f,
	};

	GLfloat cube[] = {
		-1.0f, -1.0f, -1.0f, 
		1.0f, -1.0f, -1.0f, 
		1.0f,  1.0f, 0.0f, 
		1.0f,  1.0f, -1.0f, 
		- 1.0f,  1.0f, -1.0f, 
		- 1.0f, -1.0f, -1.0f, 
		- 1.0f, -1.0f,  1.0f, 
		1.0f, -1.0f,  1.0f, 
		1.0f,  1.0f,  1.0f, 
		1.0f,  1.0f,  1.0f, 
		- 1.0f,  1.0f,  1.0f, 
		- 1.0f, -1.0f,  1.0f, 
		- 1.0f,  1.0f,  1.0f, 
		- 1.0f,  1.0f, -1.0f, 
		- 1.0f, -1.0f, -1.0f, 
		- 1.0f, -1.0f, -1.0f,
		- 1.0f, -1.0f,  1.0f,
		- 1.0f,  1.0f,  1.0f, 
		1.0f,  1.0f,  1.0f, 
		1.0f,  1.0f, -1.0f,
		1.0f, -1.0f, -1.0f, 
		1.0f, -1.0f, -1.0f, 
		1.0f, -1.0f,  1.0f, 
		1.0f,  1.0f,  1.0f, 
		- 1.0f, -1.0f, -1.0f, 
		1.0f, -1.0f, -1.0f, 
		1.0f, -1.0f,  1.0f, 
		1.0f, -1.0f,  1.0f, 
		- 1.0f, -1.0f,  1.0f, 
		- 1.0f, -1.0f, -1.0f, 
		- 1.0f,  1.0f, -1.0f, 
		1.0f,  1.0f, -1.0f, 
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f, 
		- 1.0f,  1.0f,  1.0f, 
		- 1.0f,  1.0f, -1.0f, 
	};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(point_vertex), point_vertex, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0); // Note that this is allowed, the call to glVertexAttribPointer registered VBO as the currently bound vertex buffer object so afterwards we can safely unbind

	glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs), remember: do NOT unbind the EBO, keep it bound to this VAO

	glGenVertexArrays(1, &TrainVAO);
	glGenBuffers(1, &TrainVerticeVBO);
	glGenBuffers(1, &TrainNormalsVBO);
	// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
	glBindVertexArray(TrainVAO);

	glBindBuffer(GL_ARRAY_BUFFER, TrainVerticeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	
	glBindBuffer(GL_ARRAY_BUFFER, TrainNormalsVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_Normals), cube_Normals, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0); // Note that this is allowed, the call to glVertexAttribPointer registered VBO as the currently bound vertex buffer object so afterwards we can safely unbind

	glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs), remember: do NOT unbind the EBO, keep it bound to this VAO


	glPointSize(10);
	glLineWidth(10);

	//MVP
	GLuint transformLoc = glGetUniformLocation(shaderProgram, "model_matrix");
	GLuint viewMatrixLoc = glGetUniformLocation(shaderProgram, "view_matrix");
	GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection_matrix");
	GLuint colorLoc = glGetUniformLocation(shaderProgram, "color");
	GLuint startPosLoc = glGetUniformLocation(shaderProgram, "startPos");
	GLuint endPosLoc = glGetUniformLocation(shaderProgram, "endPos");
	GLuint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
	GLuint lightingLoc = glGetUniformLocation(shaderProgram, "useLighting");

	//Used to make the camera always look at the spinner
	camera_look_at = glm::lookAt(camera_translation, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	//Projection matrix so that everything is properly sized
	projection_matrix = glm::ortho(0.0f, (float)width, (float)height, 0.0f, 0.1f, 100.0f);

	getNumberOfPointsNeededToDraw();

	// Game loop
	while (!glfwWindowShouldClose(window)) {
		// Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
		glfwPollEvents();

		// Render
		// Clear the colorbuffer
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		if (trainMode) {
			renderTrain(transformLoc, viewMatrixLoc, projectionLoc, colorLoc, startPosLoc, endPosLoc, lightingLoc, viewPosLoc);
		}
		else {
			renderBezier(transformLoc, viewMatrixLoc, projectionLoc, colorLoc, lightingLoc);
		}

		// Swap the screen buffers
		glfwSwapBuffers(window);
	}

	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
	return 0;
}
