/*
 * finalProjectV2_JonathanHandy.cpp
 *
 *  Created on: Oct 18, 2020
 *      Author: 1573734_snhu
 */

// README: Controls
/*
 * Hold alt plus left mouse button and move the mouse left/right or up/down to navigate around object
 * Hold alt plus right mouse button and move mouse up or down to zoom in or out
 * Hold ctrl and left click to change to perspective projection, release ctrl and left click to return to orthographic projection
 */



// Header inclusions
#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>

// GLM math header inclusions
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//SOIL image loader inclusion
#include "SOIL2/SOIL2.h"

using namespace std; // Standard namespace

#define WINDOW_TITLE "Modern OpenGL" // Window title macro

// Shader program macro
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version "\n" #Source
#endif

// Variable declarations for shader, window size initialization, buffer, and array objects
GLint shaderProgram, WindowWidth = 800, WindowHeight = 600;
GLuint VBO, VAO, texture;
GLfloat degrees = glm::radians(0.0f); //converts float to degrees

// Light color
glm::vec3 lightColor(0.0f, 1.0f, 0.0f);
glm::vec3 secondLightColor(1.0f, 1.0f, 1.0f);
//Light position and scale
glm::vec3 lightPosition(1.0f, 0.5f, -3.0f);
glm::vec3 lightScale(0.3f);
                      //ambient   specular    highlight
glm::vec3 lightStrength(0.1f,     1.0f,       0.5f);

GLfloat cameraSpeed = 0.0010f; // Movement speed per frame

GLchar currentKey; //Will store key pressed

int mod; // Variable for modifiers

// Scale variables
GLfloat scale_by_y=2.0f;
GLfloat scale_by_z=2.0f;
GLfloat scale_by_x=2.0f;

GLfloat lastMouseX = 400, lastMouseY = 300; // Locks mouse cursor at the center of the screen
GLfloat mouseXOffset, mouseYOffset, yaw = 0.0f, pitch = 0.0f; // Mouse offset, yaw, and pitch variables
GLfloat sensitivity = 0.005f; // Used for mouse / camera rotation sensitivity
bool mouseDetected = true; // Initially true when mouse movement is detected

// Boolean variables to monitor mouse movement
bool rotate = false;
bool checkMotion = false;
bool checkZoom = false;
bool perspective = false;

// Global vector declarations
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f); // Initial camera position
glm::vec3 CameraUpY = glm::vec3(0.0f, 1.0f, 0.0f); // Temporary y unit vector
glm::vec3 CameraForwardZ = glm::vec3(0.0f, 0.0f, -1.0f); // Temporary z unit vector
glm::vec3 front; // Temporary z unit vector for mouse

// Function prototypes
void UResizeWindow(int, int);
void URenderGraphics(void);
void UCreateShader(void);
void UCreateBuffers(void);
void UMouseClick(int button, int state, int x, int y);
void UMouseMove(int x, int y);
void UOnMotion(int x, int y);
void UGenerateTexture(void);

// Vertex shader source code
const GLchar * vertexShaderSource = GLSL(330,
	layout(location = 0) in vec3 position; // Vertex data from the vertex attrib pointer 0
	layout (location = 2) in vec3 color;

	out vec2 mobileTextureCoordinate;

	// Global variables for the transform matrices
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main() {
		gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices to clip coordinates
		mobileTextureCoordinate = vec2(textureCoordinate.x, 1.0f - textureCoordinate.y); //flips the texture horizontal
	}
);

// Fragment shader source code
const GLchar * fragmentShaderSource = GLSL(330,

	in vec2 mobileTextureCoordinate;

	out vec4 gpuTexture; //variable to pass color data to the GPU

	uniform sampler2D uTexture; //Useful when working with multiple textures

	void main() {

		gpuTexture = texture(uTexture, mobileTextureCoordinate);
	}
);

const GLchar * lightVertexShaderSource = GLSL(330,
	layout (location = 0) in vec3 position; //VAP position 0 for vertex position data
	layout (location = 1) in vec3 normal; //VAP position 1 for normals
	layout (location = 2) in vec2 textureCoordinate;

	out vec3 Normal; //for outgoing normals to fragment shader
	out vec3 FragmentPos; // for outgoing color / pixels to fragment shader
	out vec2 mobileTextureCoordinate; // uv coords for texture

	//uniform / global variables for the transform matrices
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

    void main(){
        gl_Position = projection * view * model * vec4(position, 1.0f);//Transforms vertices into clip coordinates
        Normal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
        FragmentPos = vec3(model * vec4(position, 1.0f)); //Gets fragment / pixel position in world space only (exclude view and projection)
		mobileTextureCoordinate = vec2(textureCoordinate.x, 1.0f - textureCoordinate.y); //flips the texture horizontal
	}
);

const GLchar * lightFragmentShaderSource = GLSL(330,
	in vec3 Normal; //For incoming normals
	in vec3 FragmentPos; //for incoming fragment position
	in vec2 mobileTextureCoordinate;

	out vec4 result; //for outgoing light color to the GPU

	//Uniform / Global variables for object color, light color, light position and camera/view position
	uniform vec3 lightColor;
	uniform vec3 secondLightColor;
	uniform vec3 lightPos;
	uniform vec3 viewPosition;
    uniform vec3 lightStrength;
	uniform sampler2D uTexture; //Useful when working with multiple textures

    void main(){
    	vec3 norm = normalize(Normal); //Normalize vectors to 1 unit
    	vec3 ambient = lightStrength.x * lightColor; //Generate ambient light color
    	vec3 ambientTwo = lightStrength.x * secondLightColor;//Generate second ambient light color
    	vec3 lightDirection = normalize(lightPos - FragmentPos); //Calculate distance (light direction) between light source and fragments/pixels on
    	float impact = max(dot(norm, lightDirection), 0.0); //Calculate diffuse impact by generating dot product of normal and light
    	vec3 diffuse = impact * lightColor; //Generate diffuse light color
    	vec3 viewDir = normalize(viewPosition - FragmentPos); //Calculate view direction
    	vec3 reflectDir = reflect(-lightDirection, norm); //Calculate reflection vector
    	float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), lightStrength.z);
    	vec3 specular = lightStrength.y * specularComponent * lightColor;

    	//Calculate Phong result
    	vec3 phongOne = (ambient + diffuse + specular) * vec3(texture(uTexture, mobileTextureCoordinate));

    	// Second light position
    	lightDirection = normalize(vec3(6.0f, 0.0f, -3.0f)- FragmentPos);
    	impact = max(dot(norm, lightDirection), 0.0); //Calculate diffuse impact by generating dot product of normal and light
    	diffuse = impact * secondLightColor; //Generate diffuse light color
    	viewDir = normalize(viewPosition - FragmentPos); //Calculate view direction
    	reflectDir = reflect(-lightDirection, norm); //Calculate reflection vector
    	specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), lightStrength.z);

    	// Second light
    	vec3 specularTwo = 0.1f * specularComponent * secondLightColor;

    	vec3 phongTwo = (ambientTwo + diffuse + specularTwo) * vec3(texture(uTexture, mobileTextureCoordinate));

    	result = vec4(phongOne + phongTwo, 1.0f); //Send lighting results to GPU
	}
);

// Main program
int main(int argc, char* argv[]) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(WindowWidth, WindowHeight);
	glutCreateWindow(WINDOW_TITLE);

	glutReshapeFunc(UResizeWindow);

	glewExperimental = GL_TRUE;
		if (glewInit() != GLEW_OK) {
			cout << "Failed to initialize GLEW" << endl;
			return -1;
		}

	UCreateShader();

	UCreateBuffers();

	UGenerateTexture();



	// Use the shader program
	glUseProgram(shaderProgram);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set background color

	glutDisplayFunc(URenderGraphics);

	glutPassiveMotionFunc(UMouseMove); // Detects mouse movement

	glutMotionFunc(UOnMotion);

	glutMouseFunc(UMouseClick); // Detects mouse click

	glutMainLoop();

	// Destroys buffer objects once used
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	return 0;
}

void UResizeWindow(int w, int h) {
	WindowWidth = w;
	WindowHeight = h;
	glViewport(0, 0, WindowWidth, WindowHeight);
}

void URenderGraphics(void) {
	glEnable(GL_DEPTH_TEST); // Enable z-depth

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clears the screen

	glBindVertexArray(VAO); // Activate the vertex array object before rendering and transforming

	CameraForwardZ = front; // Replaces camera forward vector with radians normalized as a unit vector


	// Transforms the object
	glm::mat4 model;
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // Place object at the center of the viewport
	model = glm::rotate(model, 45.0f, glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate the object 45 degrees on the x
	model = glm::scale(model, glm::vec3(scale_by_x, scale_by_y, scale_by_z)); // Increase the object size by a scale of 2

	// Transforms the camera
	glm::mat4 view;
	view = glm::lookAt(CameraForwardZ, cameraPosition, CameraUpY);

	glm::mat4 projection;
	// Creates a perspective projection
	if (perspective == true) {
		projection = glm::perspective(45.0f, (GLfloat)WindowWidth / (GLfloat)WindowHeight, 0.1f, 100.0f);
	}
	else {
		// Creates a orthographic projection
		projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
	}

	// Retrieves and passes transform matrices to the shader program
	GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
	GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
	GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint secondLightColorLoc, lightColorLoc, lightPositionLoc, lightStrengthLoc, viewPositionLoc;


	glUniformMatrix4fv(modelLoc,1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc,1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc,1, GL_FALSE, glm::value_ptr(projection));

	lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
	lightPositionLoc = glGetUniformLocation(shaderProgram, "lightPos");
    lightStrengthLoc = glGetUniformLocation(shaderProgram, "lightStrength");
    secondLightColorLoc = glGetUniformLocation(shaderProgram, "secondLightColor");
	viewPositionLoc = glGetUniformLocation(shaderProgram, "viewPosition");

	//pass color, light, and camera data to the cube shader programs corresponding uniforms
	glUniform3f(lightColorLoc, lightColor.r, lightColor.g, lightColor.b);
	glUniform3f(secondLightColorLoc, secondLightColor.r, secondLightColor.g, secondLightColor.b);
	glUniform3f(lightPositionLoc, lightPosition.x, lightPosition.y, lightPosition.z);
	glUniform3f(lightStrengthLoc, lightStrength.x, lightStrength.y, lightStrength.z);
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

	glutPostRedisplay();

	glBindTexture(GL_TEXTURE_2D, texture);

	// Draws the triangles
	glDrawArrays(GL_TRIANGLES, 0, 216); // Draws the triangles that make up the chair

	glBindVertexArray(0); // Deactivate the vertex array object
	glutSwapBuffers(); // Flips the front and back buffers every frame
}

void UCreateShader() {

	//Vertex shader
	GLint vertexShader = glCreateShader(GL_VERTEX_SHADER); // creates the vertex shader
	glShaderSource(vertexShader, 1, &lightVertexShaderSource, NULL); //Attaches the vertex shader to the source code
	glCompileShader(vertexShader); //compiles the vertex shader

	//Fragment shader
	GLint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); //Creates the fragment shader
	glShaderSource(fragmentShader, 1, &lightFragmentShaderSource, NULL); //Attaches the fragment shader to the source code
	glCompileShader(fragmentShader); //compiles the fragment shader

	// Shader Program
	shaderProgram = glCreateProgram(); // Creates the shader program and returns an id
	glAttachShader(shaderProgram, vertexShader); // Attach vertex shader to the shader program
	glAttachShader(shaderProgram, fragmentShader); // Attach fragment shader to the shader program
	glLinkProgram(shaderProgram); // Link vertex and fragment shader to shader program

	// Delete the vertex and fragment shaders once linked
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void UCreateBuffers() {

	// Chair vertices
	GLfloat vertices[] = {
			// center back of seat is 0,0,0
			// Locations are referenced while looking at front of chair

			//Position     	 	 	//Normals				// Texture

			// Seat - Top
			-0.5f, 0.0f, 0.0f,		0.0f, 1.0f, 0.0f,	 	0.0f, 1.0f, // 0 Back left (top)
			-0.5f, 0.0f, 1.0f,		0.0f, 1.0f, 0.0f, 		0.0f, 0.0f, // 2 Front left (top)
			0.5f, 0.0f, 1.0f,		0.0f, 1.0f, 0.0f, 		1.0f, 0.0f, // 3 Front right (top)
			-0.5f, 0.0f, 0.0f,		0.0f, 1.0f, 0.0f, 		0.0f, 1.0f, // 0 Back left (top)
			0.5f, 0.0f, 0.0f,		0.0f, 1.0f, 0.0f, 		1.0f, 1.0f, // 1 Back right (top)
			0.5f, 0.0f, 1.0f,		0.0f, 1.0f, 0.0f, 		1.0f, 0.0f, // 3 Front right (top)

			// Seat - Bottom
			-0.5f, -0.2f, 0.0f,		0.0f, -1.0f, 0.0f, 		0.0f, 1.0f, // 4 Back left (bottom)
			-0.5f, -0.2f, 1.0f,		0.0f, -1.0f, 0.0f, 		0.0f, 0.0f, // 6 Front left (bottom)
			0.5f, -0.2f, 1.0f,		0.0f, -1.0f, 0.0f, 		1.0f, 0.0f, // 7 Front right (bottom)
			-0.5f, -0.2f, 0.0f,		0.0f, -1.0f, 0.0f, 		0.0f, 1.0f, // 4 Back left (bottom)
			0.5f, -0.2f, 0.0f,		0.0f, -1.0f, 0.0f, 		1.0f, 1.0f, // 5 Back right (bottom)
			0.5f, -0.2f, 1.0f,		0.0f, -1.0f, 0.0f, 		1.0f, 0.0f, // 7 Front right (bottom)

			// Seat - Left Side
			-0.5f, 0.0f, 0.0f,		-1.0f, 0.0f, 0.0f, 		1.0f, 1.0f, // 0 Back left (top)
			-0.5f, -0.2f, 0.0f,		-1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, // 4 Back left (bottom)
			-0.5f, -0.2f, 1.0f,		-1.0f, 0.0f, 0.0f, 		0.0f, 0.0f, // 6 Front left (bottom)
			-0.5f, 0.0f, 0.0f,		-1.0f, 0.0f, 0.0f, 		1.0f, 1.0f, // 0 Back left (top)
			-0.5f, 0.0f, 1.0f,		-1.0f, 0.0f, 0.0f, 		1.0f, 0.0f, // 2 Front left (top)
			-0.5f, -0.2f, 1.0f,		-1.0f, 0.0f, 0.0f, 		0.0f, 0.0f, // 6 Front left (bottom)

			// Seat - Back Side
			-0.5f, 0.0f, 0.0f,		0.0f, 0.0f, -1.0f, 		1.0f, 0.0f, // 0 Back left (top)
			-0.5f, -0.2f, 0.0f,		0.0f, 0.0f, -1.0f, 		0.0f, 0.0f, // 4 Back left (bottom)
			0.5f, -0.2f, 0.0f,		0.0f, 0.0f, -1.0f, 		0.0f, 1.0f, // 5 Back right (bottom)
			-0.5f, 0.0f, 0.0f,		0.0f, 0.0f, -1.0f, 		1.0f, 0.0f, // 0 Back left (top)
			0.5f, 0.0f, 0.0f,		0.0f, 0.0f, -1.0f,		1.0f, 1.0f, // 1 Back right (top)
			0.5f, -0.2f, 0.0f,		0.0f, 0.0f, -1.0f, 		0.0f, 1.0f, // 5 Back right (bottom)

			// Seat - Right Side
			0.5f, 0.0f, 0.0f,		1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, // 1 Back right (top)
			0.5f, -0.2f, 0.0f,		1.0f, 0.0f, 0.0f, 		1.0f, 1.0f, // 5 Back right (bottom)
			0.5f, -0.2f, 1.0f,		1.0f, 0.0f, 0.0f, 		1.0f, 0.0f, // 7 Front right (bottom)
			0.5f, 0.0f, 0.0f,		1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, // 1 Back right (top)
			0.5f, 0.0f, 1.0f,		1.0f, 0.0f, 0.0f, 		0.0f, 0.0f, // 3 Front right (top)
			0.5f, -0.2f, 1.0f,		1.0f, 0.0f, 0.0f, 		1.0f, 0.0f, // 7 Front right (bottom)

			// Seat - Front Side
			-0.5f, 0.0f, 1.0f,		0.0f, 0.0f, 1.0f, 		0.0f, 0.0f, // 2 Front left (top)
			-0.5f, -0.2f, 1.0f,		0.0f, 0.0f, 1.0f, 		1.0f, 0.0f, // 6 Front left (bottom)
			0.5f, -0.2f, 1.0f,		0.0f, 0.0f, 1.0f, 		1.0f, 1.0f, // 7 Front right (bottom)
			-0.5f, 0.0f, 1.0f,		0.0f, 0.0f, 1.0f, 		0.0f, 0.0f, // 2 Front left (top)
			0.5f, 0.0f, 1.0f,		0.0f, 0.0f, 1.0f, 		0.0f, 1.0f, // 3 Front right (top)
			0.5f, -0.2f, 1.0f,		0.0f, 0.0f, 1.0f, 		1.0f, 1.0f, // 7 Front right (bottom)

		// ---------------------------------------------

			// Back Rest - Front
			-0.5f, 1.2f, 0.1f,		0.0f, 0.0f, 1.0f, 		0.0f, 0.0f, // 12 Top left (front)
			-0.5f, 0.9f, 0.1f,		0.0f, 0.0f, 1.0f, 		1.0f, 0.0f, // 14 Bottom left (front)
			0.5f, 0.9f, 0.1f,		0.0f, 0.0f, 1.0f, 		1.0f, 1.0f, // 15 Bottom right (front)
			-0.5f, 1.2f, 0.1f,		0.0f, 0.0f, 1.0f, 		0.0f, 0.0f, // 12 Top left (front)
			0.5f, 1.2f, 0.1f,		0.0f, 0.0f, 1.0f, 		0.0f, 1.0f, // 13 Top right (front)
			0.5f, 0.9f, 0.1f,		0.0f, 0.0f, 1.0f, 		1.0f, 1.0f, // 15 Bottom right (front)

			// Back Rest - Back
			-0.5f, 1.2f, 0.0f,		0.0f, 0.0f, -1.0f, 		0.0f, 0.0f, // 8 Top left (back)
			-0.5f, 0.9f, 0.0f,		0.0f, 0.0f, -1.0f, 		1.0f, 0.0f, // 10 Bottom left (back)
			0.5f, 0.9f, 0.0f,		0.0f, 0.0f, -1.0f, 		1.0f, 1.0f, // 11 Bottom right (back)
			-0.5f, 1.2f, 0.0f,		0.0f, 0.0f, -1.0f, 		0.0f, 0.0f, // 8 Top left (back)
			0.5f, 1.2f, 0.0f,		0.0f, 0.0f, -1.0f, 		0.0f, 1.0f, // 9 Top right (back)
			0.5f, 0.9f, 0.0f,		0.0f, 0.0f, -1.0f, 		1.0f, 1.0f, // 11 Bottom right (back)

			// Back Rest - Left Side
			-0.5f, 1.2f, 0.0f,		-1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, // 8 Top left (back)
			-0.5f, 0.9f, 0.0f,		-1.0f, 0.0f, 0.0f, 		0.0f, 0.0f, // 10 Bottom left (back)
			-0.5f, 0.9f, 0.1f,		-1.0f, 0.0f, 0.0f, 		1.0f, 0.0f, // 14 Bottom left (front)
			-0.5f, 1.2f, 0.0f,		-1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, // 8 Top left (back)
			-0.5f, 1.2f, 0.1f,		-1.0f, 0.0f, 0.0f, 		1.0f, 1.0f, // 12 Top left (front)
			-0.5f, 0.9f, 0.1f,		-1.0f, 0.0f, 0.0f, 		1.0f, 0.0f, // 14 Bottom left (front)

			// Back Rest - Right Side
			0.5f, 1.2f, 0.0f,		1.0f, 0.0f, 0.0f, 		1.0f, 1.0f, // 9 Top right (back)
			0.5f, 0.9f, 0.0f,		1.0f, 0.0f, 0.0f, 		1.0f, 0.0f, // 11 Bottom right (back)
			0.5f, 0.9f, 0.1f,		1.0f, 0.0f, 0.0f, 		0.0f, 0.0f, // 15 Bottom right (front)
			0.5f, 1.2f, 0.0f,		1.0f, 0.0f, 0.0f,		1.0f, 1.0f,  // 9 Top right (back)
			0.5f, 1.2f, 0.1f,		1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, // 13 Top right (front)
			0.5f, 0.9f, 0.1f,		1.0f, 0.0f, 0.0f, 		0.0f, 0.0f, // 15 Bottom right (front)

			// Back Rest - Top Side
			-0.5f, 1.2f, 0.0f,		0.0f, 1.0f, 0.0f, 		0.0f, 0.0f, // 8 Top left (back)
			-0.5f, 1.2f, 0.1f,		0.0f, 1.0f, 0.0f, 		1.0f, 0.0f, // 12 Top left (front)
			0.5f, 1.2f, 0.1f,		0.0f, 1.0f, 0.0f, 		1.0f, 1.0f, // 13 Top right (front)
			-0.5f, 1.2f, 0.0f,		0.0f, 1.0f, 0.0f, 		0.0f, 0.0f, // 8 Top left (back)
			0.5f, 1.2f, 0.0f,		0.0f, 1.0f, 0.0f, 		0.0f, 1.0f, // 9 Top right (back)
			0.5f, 1.2f, 0.1f,		0.0f, 1.0f, 0.0f, 		1.0f, 1.0f, // 13 Top right (front)

			// Back Rest - Bottom Side
			-0.5f, 0.9f, 0.0f,		0.0f, -1.0f, 0.0f,		0.0f, 0.0f, // 10 Bottom left (back)
			-0.5f, 0.9f, 0.1f,		0.0f, -1.0f, 0.0f, 		1.0f, 0.0f,// 14 Bottom left (front)
			0.5f, 0.9f, 0.0f,		0.0f, -1.0f, 0.0f, 		0.0f, 1.0f, // 11 Bottom right (back)
			-0.5f, 0.9f, 0.0f,		0.0f, -1.0f, 0.0f, 		0.0f, 0.0f, // 10 Bottom left (back)
			0.5f, 0.9f, 0.1f,		0.0f, -1.0f, 0.0f, 		1.0f, 1.0f, // 15 Bottom right (front)
			0.5f, 0.9f, 0.0f,		0.0f, -1.0f, 0.0f, 		0.0f, 1.0f, // 11 Bottom right (back)

		// ---------------------------------------------

			// Rear Left Leg - Front Side
			-0.5f, 0.9f, 0.1f,		0.0f, 0.0f, 1.0f, 		0.0f, 1.0f, // 14 Bottom left (front)
			-0.5f, -0.9f, 0.1f,		0.0f, 0.0f, 1.0f, 		0.0f, 0.0f, // 19 Bottom left (front)
			-0.4f, -0.9f, 0.1f,		0.0f, 0.0f, 1.0f, 		1.0f, 0.0f, // 21 bottom right (front)
			-0.5f, 0.9f, 0.1f,		0.0f, 0.0f, 1.0f, 		0.0f, 1.0f, // 14 Bottom left (front)
			-0.4f, 0.9f, 0.1f,		0.0f, 0.0f, 1.0f, 		1.0f, 1.0f, // 17 Top right (front)
			-0.4f, -0.9f, 0.1f,		0.0f, 0.0f, 1.0f, 		1.0f, 0.0f, // 21 bottom right (front)

			// Rear Left Leg - Back Side
			-0.5f, 0.9f, 0.0f,		0.0f, 0.0f, -1.0f, 		0.0f, 1.0f, // 10 Bottom left (back)
			-0.5f, -0.9f, 0.0f,		0.0f, 0.0f, -1.0f, 		0.0f, 0.0f, // 18 Bottom left (back)
			-0.4f, -0.9f, 0.0f,		0.0f, 0.0f, -1.0f, 		1.0f, 0.0f, // 20 bottom right (back)
			-0.5f, 0.9f, 0.0f,		0.0f, 0.0f, -1.0f, 		0.0f, 1.0f, // 10 Bottom left (back)
			-0.4f, 0.9f, 0.0f,		0.0f, 0.0f, -1.0f, 		1.0f, 1.0f, // 16 Top right (back)
			-0.4f, -0.9f, 0.0f,		0.0f, 0.0f, -1.0f, 		1.0f, 0.0f, // 20 bottom right (back)

			// Rear Left Leg  - Left Side
			-0.5f, 0.9f, 0.0f,		-1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, // 10 Bottom left (back)
			-0.5f, -0.9f, 0.0f,		-1.0f, 0.0f, 0.0f, 		0.0f, 0.0f, // 18 Bottom left (back)
			-0.5f, -0.9f, 0.1f,		-1.0f, 0.0f, 0.0f, 		1.0f, 0.0f, // 19 Bottom left (front)
			-0.5f, 0.9f, 0.0f,		-1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, // 10 Bottom left (back)
			-0.5f, 0.9f, 0.1f,		-1.0f, 0.0f, 0.0f, 		1.0f, 1.0f, // 14 Bottom left (front)
			-0.5f, -0.9f, 0.1f,		-1.0f, 0.0f, 0.0f, 		1.0f, 0.0f, // 19 Bottom left (front)

			// Rear Left Leg - Right Side
			-0.4f, 0.9f, 0.0f,		1.0f, 0.0f, 0.0f, 		1.0f, 1.0f, // 16 Top right (back)
			-0.4f, 0.9f, 0.1f,		1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, // 17 Top right (front)
			-0.4f, -0.9f, 0.1f,		1.0f, 0.0f, 0.0f, 		0.0f, 0.0f, // 21 bottom right (front)
			-0.4f, 0.9f, 0.0f,		1.0f, 0.0f, 0.0f, 		1.0f, 1.0f, // 16 Top right (back)
			-0.4f, -0.9f, 0.0f,		1.0f, 0.0f, 0.0f, 		1.0f, 0.0f, // 20 bottom right (back)
			-0.4f, -0.9f, 0.1f,		1.0f, 0.0f, 0.0f, 		0.0f, 0.0f, // 21 bottom right (front)

			// Rear Left Leg - Top
			-0.5f, 0.9f, 0.0f,		0.0f, 1.0f, 0.0f,		0.0f, 1.0f, // 10 Bottom left (back)
			-0.5f, 0.9f, 0.1f,		0.0f, 1.0f, 0.0f, 		0.0f, 0.0f, // 14 Bottom left (front)
			-0.4f, 0.9f, 0.1f,		0.0f, 1.0f, 0.0f, 		1.0f, 0.0f, // 17 Top right (front)
			-0.5f, 0.9f, 0.0f,		0.0f, 1.0f, 0.0f, 		0.0f, 1.0f, // 10 Bottom left (back)
			-0.4f, 0.9f, 0.0f,		0.0f, 1.0f, 0.0f, 		1.0f, 1.0f, // 16 Top right (back)
			-0.4f, 0.9f, 0.1f,		0.0f, 1.0f, 0.0f, 		1.0f, 0.0f, // 17 Top right (front)

			// Rear Left Leg - Bottom
			-0.5f, -0.9f, 0.0f,		0.0f, -1.0f, 0.0f, 		0.0f, 1.0f, // 18 Bottom left (back)
			-0.5f, -0.9f, 0.1f,		0.0f, -1.0f, 0.0f, 		0.0f, 0.0f, // 19 Bottom left (front)
			-0.4f, -0.9f, 0.1f,		0.0f, -1.0f, 0.0f, 		1.0f, 0.0f, // 21 bottom right (front)
			-0.5f, -0.9f, 0.0f,		0.0f, -1.0f, 0.0f, 		0.0f, 1.0f, // 18 Bottom left (back)
			-0.4f, -0.9f, 0.0f,		0.0f, -1.0f, 0.0f, 		1.0f, 1.0f, // 20 bottom right (back)
			-0.4f, -0.9f, 0.1f,		0.0f, -1.0f, 0.0f, 		1.0f, 0.0f, // 21 bottom right (front)

		// ---------------------------------------------

			// Rear Right Leg - Front Side
			0.4f, 0.9f, 0.1f,		0.0f, 0.0f, 1.0f, 		0.0f, 1.0f, // 23 Top left (front)
			0.4f, -0.9f, 0.1f,		0.0f, 0.0f, 1.0f, 		0.0f, 0.0f, // 27 bottom left (front)
			0.5f, -0.9f, 0.1f,		0.0f, 0.0f, 1.0f, 		1.0f, 0.0f, // 25 Bottom right (front)
			0.4f, 0.9f, 0.1f,		0.0f, 0.0f, 1.0f, 		0.0f, 1.0f, // 23 Top left (front)
			0.5f, 0.9f, 0.1f,		0.0f, 0.0f, 1.0f, 		1.0f, 1.0f, // 15 Bottom right (front)
			0.5f, -0.9f, 0.1f,		0.0f, 0.0f, 1.0f, 		1.0f, 0.0f, // 25 Bottom right (front)

			// Rear Right Leg - Back Side
			0.4f, 0.9f, 0.0f,		0.0f, 0.0f, -1.0f, 		0.0f, 1.0f, // 22 Top left (back)
			0.4f, -0.9f, 0.0f,		0.0f, 0.0f, -1.0f, 		0.0f, 0.0f, // 26 bottom left (back)
			0.5f, -0.9f, 0.0f,		0.0f, 0.0f, -1.0f, 		1.0f, 0.0f, // 24 Bottom right (back)
			0.4f, 0.9f, 0.0f,		0.0f, 0.0f, -1.0f, 		0.0f, 1.0f, // 22 Top left (back)
			0.5f, 0.9f, 0.0f,		0.0f, 0.0f, -1.0f, 		1.0f, 1.0f, // 11 Bottom right (back)
			0.5f, -0.9f, 0.0f,		0.0f, 0.0f, -1.0f, 		1.0f, 0.0f, // 24 Bottom right (back)

			// Rear Right Leg - Left Side
			0.4f, 0.9f, 0.0f,		-1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, // 22 Top left (back)
			0.4f, -0.9f, 0.0f,		-1.0f, 0.0f, 0.0f, 		0.0f, 0.0f, // 26 bottom left (back)
			0.4f, -0.9f, 0.1f,		-1.0f, 0.0f, 0.0f, 		1.0f, 0.0f, // 27 bottom left (front)
			0.4f, 0.9f, 0.0f,		-1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, // 22 Top left (back)
			0.4f, 0.9f, 0.1f,		-1.0f, 0.0f, 0.0f, 		1.0f, 1.0f, // 23 Top left (front)
			0.4f, -0.9f, 0.1f,		-1.0f, 0.0f, 0.0f, 		1.0f, 0.0f, // 27 bottom left (front)

			// Rear Right Leg - Right Side
			0.5f, 0.9f, 0.1f,		1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, // 15 Bottom right (front)
			0.5f, -0.9f, 0.1f,		1.0f, 0.0f, 0.0f, 		0.0f, 0.0f, // 25 Bottom right (front)
			0.5f, -0.9f, 0.0f,		1.0f, 0.0f, 0.0f, 		1.0f, 0.0f, // 24 Bottom right (back)
			0.5f, 0.9f, 0.1f,		1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, // 15 Bottom right (front)
			0.5f, 0.9f, 0.0f,		1.0f, 0.0f, 0.0f, 		1.0f, 1.0f, // 11 Bottom right (back)
			0.5f, -0.9f, 0.0f,		1.0f, 0.0f, 0.0f, 		1.0f, 0.0f, // 24 Bottom right (back)

			// Rear Right Leg - Top
			0.4f, 0.9f, 0.0f,		0.0f, 1.0f, 0.0f, 		0.0f, 1.0f, // 22 Top left (back)
			0.4f, 0.9f, 0.1f,		0.0f, 1.0f, 0.0f, 		0.0f, 0.0f, // 23 Top left (front)
			0.5f, 0.9f, 0.1f,		0.0f, 1.0f, 0.0f, 		1.0f, 0.0f, // 15 Bottom right (front)
			0.4f, 0.9f, 0.0f,		0.0f, 1.0f, 0.0f, 		0.0f, 1.0f, // 22 Top left (back)
			0.5f, 0.9f, 0.0f,		0.0f, 1.0f, 0.0f, 		1.0f, 1.0f, // 11 Bottom right (back)
			0.5f, 0.9f, 0.1f,		0.0f, 1.0f, 0.0f, 		1.0f, 0.0f, // 15 Bottom right (front)

			// Rear Right Leg - Bottom
			0.4f, -0.9f, 0.0f,		0.0f, -1.0f, 0.0f, 		0.0f, 1.0f, // 26 bottom left (back)
			0.4f, -0.9f, 0.1f,		0.0f, -1.0f, 0.0f, 		0.0f, 0.0f, // 27 bottom left (front)
			0.5f, -0.9f, 0.1f,		0.0f, -1.0f, 0.0f, 		1.0f, 0.0f, // 25 Bottom right (front)
			0.4f, -0.9f, 0.0f,		0.0f, -1.0f, 0.0f, 		0.0f, 1.0f, // 26 bottom left (back)
			0.5f, -0.9f, 0.0f,		0.0f, -1.0f, 0.0f, 		1.0f, 1.0f, // 24 Bottom right (back)
			0.5f, -0.9f, 0.1f,		0.0f, -1.0f, 0.0f, 		1.0f, 0.0f, // 25 Bottom right (front)

		// ---------------------------------------------

			// Left Front Leg - Front
			-0.5f, -0.2f, 1.0f,		0.0f, 0.0f, 1.0f, 		0.0f, 1.0f, // 6 Front left (bottom)
			-0.5f, -0.9f, 1.0f,		0.0f, 0.0f, 1.0f, 		0.0f, 0.0f, // 32 Bottom left (front)
			-0.4f, -0.9f, 1.0f,		0.0f, 0.0f, 1.0f, 		1.0f, 0.0f, // 34 Bottom right (front)
			-0.5f, -0.2f, 1.0f,		0.0f, 0.0f, 1.0f, 		0.0f, 1.0f, // 6 Front left (bottom)
			-0.4f, -0.2f, 1.0f,		0.0f, 0.0f, 1.0f, 		1.0f, 1.0f, // 30 Top right (front)
			-0.4f, -0.9f, 1.0f,		0.0f, 0.0f, 1.0f, 		1.0f, 0.0f, // 34 Bottom right (front)

			// Left Front Leg - Back
			-0.5f, -0.2f, 0.9f,		0.0f, 0.0f, -1.0f, 		0.0f, 1.0f, // 28 Top left (back)
			-0.5f, -0.9f, 0.9f,		0.0f, 0.0f, -1.0f, 		0.0f, 0.0f, // 31 Bottom left (back)
			-0.4f, -0.9f, 0.9f,		0.0f, 0.0f, -1.0f, 		1.0f, 0.0f, // 33 Bottom right (back)
			-0.5f, -0.2f, 0.9f,		0.0f, 0.0f, -1.0f, 		0.0f, 1.0f, // 28 Top left (back)
			-0.4f, -0.2f, 0.9f,		0.0f, 0.0f, -1.0f, 		1.0f, 1.0f, // 29 Top right (back)
			-0.4f, -0.9f, 0.9f,		0.0f, 0.0f, -1.0f, 		1.0f, 0.0f, // 33 Bottom right (back)

			// Left Front Leg - Left Side
			-0.5f, -0.2f, 0.9f,		-1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, // 28 Top left (back)
			-0.5f, -0.9f, 0.9f,		-1.0f, 0.0f, 0.0f, 		0.0f, 0.0f, // 31 Bottom left (back)
			-0.5f, -0.9f, 1.0f,		-1.0f, 0.0f, 0.0f, 		1.0f, 0.0f, // 32 Bottom left (front)
			-0.5f, -0.2f, 0.9f,		-1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, // 28 Top left (back)
			-0.5f, -0.2f, 1.0f,		-1.0f, 0.0f, 0.0f, 		1.0f, 1.0f, // 6 Front left (bottom)
			-0.5f, -0.9f, 1.0f,		-1.0f, 0.0f, 0.0f, 		1.0f, 0.0f, // 32 Bottom left (front)

			// Left Front Leg - Right Side
			-0.4f, -0.2f, 1.0f,		1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, // 30 Top right (front)
			-0.4f, -0.9f, 1.0f,		1.0f, 0.0f, 0.0f, 		0.0f, 0.0f, // 34 Bottom right (front)
			-0.4f, -0.9f, 0.9f,		1.0f, 0.0f, 0.0f, 		1.0f, 0.0f, // 33 Bottom right (back)
			-0.4f, -0.2f, 1.0f,		1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, // 30 Top right (front)
			-0.4f, -0.2f, 0.9f,		1.0f, 0.0f, 0.0f, 		1.0f, 1.0f, // 29 Top right (back)
			-0.4f, -0.9f, 0.9f,		1.0f, 0.0f, 0.0f, 		1.0f, 0.0f, // 33 Bottom right (back)

			// Left Front Leg - Top
			-0.5f, -0.2f, 1.0f,		0.0f, 1.0f, 0.0f, 		0.0f, 0.0f, // 6 Front left (bottom)
			-0.5f, -0.2f, 0.9f,		0.0f, 1.0f, 0.0f, 		0.0f, 1.0f, // 28 Top left (back)
			-0.4f, -0.2f, 0.9f,		0.0f, 1.0f, 0.0f, 		1.0f, 1.0f, // 29 Top right (back)
			-0.5f, -0.2f, 1.0f,		0.0f, 1.0f, 0.0f, 		0.0f, 0.0f, // 6 Front left (bottom)
			-0.4f, -0.2f, 1.0f,		0.0f, 1.0f, 0.0f, 		1.0f, 0.0f, // 30 Top right (front)
			-0.4f, -0.2f, 0.9f,		0.0f, 1.0f, 0.0f, 		1.0f, 1.0f, // 29 Top right (back)

			// Left Front Leg - Bottom
			-0.5f, -0.9f, 0.9f,		0.0f, -1.0f, 0.0f, 		0.0f, 1.0f, // 31 Bottom left (back)
			-0.5f, -0.9f, 1.0f,		0.0f, -1.0f, 0.0f, 		0.0f, 0.0f, // 32 Bottom left (front)
			-0.4f, -0.9f, 1.0f,		0.0f, -1.0f, 0.0f, 		1.0f, 0.0f, // 34 Bottom right (front)
			-0.5f, -0.9f, 0.9f,		0.0f, -1.0f, 0.0f, 		0.0f, 1.0f, // 31 Bottom left (back)
			-0.4f, -0.9f, 0.9f,		0.0f, -1.0f, 0.0f, 		1.0f, 1.0f, // 33 Bottom right (back)
			-0.4f, -0.9f, 1.0f,		0.0f, -1.0f, 0.0f, 		1.0f, 0.0f, // 34 Bottom right (front)

		// ---------------------------------------------

			// Right Front Leg - Front Side
			0.4f, -0.2f, 1.0f,		0.0f, 0.0f, 1.0f, 		0.0f, 1.0f, // 37 Top left (front)
			0.4f, -0.9f, 1.0f,		0.0f, 0.0f, 1.0f, 		0.0f, 0.0f, // 41 Bottom left (front)
			0.5f, -0.9f, 1.0f,		0.0f, 0.0f, 1.0f, 		1.0f, 0.0f, // 39 Bottom right (front)
			0.4f, -0.2f, 1.0f,		0.0f, 0.0f, 1.0f, 		0.0f, 1.0f, // 37 Top left (front)
			0.5f, -0.2f, 1.0f,		0.0f, 0.0f, 1.0f, 		1.0f, 1.0f, // 7 Front right (bottom)
			0.5f, -0.9f, 1.0f,		0.0f, 0.0f, 1.0f, 		1.0f, 0.0f, // 39 Bottom right (front)

			// Right Front Leg - Back Side
			0.4f, -0.2f, 0.9f,		0.0f, 0.0f, -1.0f, 		0.0f, 1.0f, // 36 Top left (back)
			0.4f, -0.9f, 0.9f,		0.0f, 0.0f, -1.0f, 		0.0f, 0.0f, // 38 Bottom left (back)
			0.5f, -0.9f, 0.9f,		0.0f, 0.0f, -1.0f, 		1.0f, 0.0f, // 40 Bottom right (back)
			0.4f, -0.2f, 0.9f,		0.0f, 0.0f, -1.0f, 		0.0f, 1.0f, // 36 Top left (back)
			0.5f, -0.2f, 0.9f,		0.0f, 0.0f, -1.0f, 		1.0f, 1.0f, // 35 Top right (back)
			0.5f, -0.9f, 0.9f,		0.0f, 0.0f, -1.0f, 		1.0f, 0.0f, // 40 Bottom right (back)

			// Right Front Leg - Left Side
			0.4f, -0.2f, 0.9f,		-1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, // 36 Top left (back)
			0.4f, -0.9f, 0.9f,		-1.0f, 0.0f, 0.0f, 		0.0f, 0.0f, // 38 Bottom left (back)
			0.4f, -0.9f, 1.0f,		-1.0f, 0.0f, 0.0f, 		1.0f, 0.0f, // 41 Bottom left (front)
			0.4f, -0.2f, 0.9f,		-1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, // 36 Top left (back)
			0.4f, -0.2f, 1.0f,		-1.0f, 0.0f, 0.0f, 		1.0f, 1.0f, // 37 Top left (front)
			0.4f, -0.9f, 1.0f,		-1.0f, 0.0f, 0.0f, 		1.0f, 0.0f, // 41 Bottom left (front)

			// Right Front Leg - Right Side
			0.5f, -0.2f, 1.0f,		1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, // 7 Front right (bottom)
			0.5f, -0.9f, 1.0f,		1.0f, 0.0f, 0.0f, 		0.0f, 0.0f, // 39 Bottom right (front)
			0.5f, -0.9f, 0.9f,		1.0f, 0.0f, 0.0f, 		1.0f, 0.0f, // 40 Bottom right (back)
			0.5f, -0.2f, 1.0f,		1.0f, 0.0f, 0.0f, 		0.0f, 1.0f, // 7 Front right (bottom)
			0.5f, -0.2f, 0.9f,		1.0f, 0.0f, 0.0f, 		1.0f, 1.0f, // 35 Top right (back)
			0.5f, -0.9f, 0.9f,		1.0f, 0.0f, 0.0f, 		1.0f, 0.0f, // 40 Bottom right (back)

			// Right Front Leg - Top
			0.4f, -0.2f, 0.9f,		0.0f, 1.0f, 0.0f, 		0.0f, 1.0f, // 36 Top left (back)
			0.4f, -0.2f, 1.0f,		0.0f, 1.0f, 0.0f, 		0.0f, 0.0f, // 37 Top left (front)
			0.5f, -0.2f, 1.0f,		0.0f, 1.0f, 0.0f, 		1.0f, 0.0f, // 7 Front right (bottom)
			0.4f, -0.2f, 0.9f,		0.0f, 1.0f, 0.0f, 		0.0f, 1.0f, // 36 Top left (back)
			0.5f, -0.2f, 0.9f,		0.0f, 1.0f, 0.0f, 		1.0f, 1.0f, // 35 Top right (back)
			0.5f, -0.2f, 1.0f,		0.0f, 1.0f, 0.0f, 		1.0f, 0.0f, // 7 Front right (bottom)

			// Right Front Leg - Bottom
			0.4f, -0.9f, 0.9f,		0.0f, -1.0f, 0.0f, 		0.0f, 1.0f, // 38 Bottom left (back)
			0.4f, -0.9f, 1.0f,		0.0f, -1.0f, 0.0f, 		0.0f, 0.0f, // 41 Bottom left (front)
			0.5f, -0.9f, 1.0f,		0.0f, -1.0f, 0.0f, 		1.0f, 0.0f, // 39 Bottom right (front)
			0.4f, -0.9f, 0.9f,		0.0f, -1.0f, 0.0f, 		0.0f, 1.0f, // 38 Bottom left (back)
			0.5f, -0.9f, 0.9f,		0.0f, -1.0f, 0.0f, 		1.0f, 1.0f, // 40 Bottom right (back)
			0.5f, -0.9f, 1.0f,		0.0f, -1.0f, 0.0f, 		1.0f, 0.0f, // 39 Bottom right (front)

	};


	// Generate buffer IDs
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	// Activate the vertex array object before binding and setting any VBOs and vertex attrib pointers
	glBindVertexArray(VAO);

	// Activate the VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // Copy vertices to VBO

	// Set attribute pointer 0 to hold position data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0); // Enables vertex attribute

	//Set attribute pointer 1 to hold Normal data
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	//Set attribute pointer 2 to hold texture data
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0); // Deactivates the VAO which is good practice

}

void UMouseMove(int x, int y) {

	front.x = 10.0f * cos(yaw);
	front.y = 10.0f * sin(pitch);
	front.z = sin(yaw) * cos(pitch) * 10.0f;

}

void UOnMotion(int x, int y) {

	// Boolean to check for alt and mouse button hold
	if (checkMotion) {

		// Get mouse direction
		mouseXOffset = x - lastMouseX;
		mouseYOffset = lastMouseY - y;

		// Update with new coordinates
		lastMouseX = x;
		lastMouseY = y;

		// Apply sensitivity to mouse direction
		mouseXOffset *= sensitivity;
		mouseYOffset *= sensitivity;

		// Get direction of mouse
		if ((yaw != yaw + mouseXOffset) && (pitch == pitch + mouseYOffset)) {

			// Increment x
			yaw += mouseXOffset;
		}
		else if ((pitch != pitch + mouseYOffset) && (yaw == yaw + mouseXOffset)) {

			// Increment y
			pitch += mouseYOffset;
		}

		front.x = 10.0f * cos(yaw);
		front.y = 10.0f * sin(pitch);
		front.z = sin(yaw) * cos(pitch) * 10.0f;

	}

	// Boolean to check for user zoom
	if (checkZoom) {

		// Determine direction of the mouse
		if (lastMouseY > y) {

			// Increment scale values (zoom out)
			scale_by_x += 0.1f;
			scale_by_y += 0.1f;
			scale_by_z += 0.1f;

			//Redisplay
			glutPostRedisplay();

		}
		else {

			// Decrement scale values (zoom in)
			scale_by_x -= 0.1f;
			scale_by_y -= 0.1f;
			scale_by_z -= 0.1f;

			// Set limit for zoom in
			if (scale_by_y < 0.2f) {

				scale_by_x = 0.2f;
				scale_by_y = 0.2f;
				scale_by_z = 0.2f;

			}

			// Redisplay
			glutPostRedisplay();

		}

		// Update x and y
		lastMouseX = x;
		lastMouseY = y;

	}

}

// Implements the UMouseClick function
void UMouseClick(int button, int state, int x, int y) {

	// Check for alt press
	mod = glutGetModifiers();

	// Set variable to false
	checkMotion = false;

	if ((mod == GLUT_ACTIVE_ALT) && (button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN)) {

		// Is moving
		checkMotion = true;

		// Not zooming
		checkZoom = false;

	}
	else if ((mod == GLUT_ACTIVE_ALT) && (button == GLUT_RIGHT_BUTTON) && (state == GLUT_DOWN)) {

		// Not moving
		checkMotion = false;

		// Is zooming
		checkZoom = true;

	}

	if (mod == GLUT_ACTIVE_CTRL) {
		perspective = true;
	}
	else {
		perspective = false;
	}

}

//Generate and load the texture
void UGenerateTexture(){
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	int width, height;

	unsigned char* image = SOIL_load_image("alder.jpg", &width, &height, 0, SOIL_LOAD_RGB);//loads texture file

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0); //Unbind the texture
}









