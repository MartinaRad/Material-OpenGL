#include <GL\glew.h>

#include <SDL.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <gl\GLU.h>

#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Shader.h"
#include "Camera.h"

bool init();
bool initGL();
void render();
void renderGlass();
void renderWindow();
GLuint CreateCube(float, GLuint&);
GLuint CreateSquare(float, GLuint&);
void DrawObject(GLuint id);
bool LoadTexture(const char* filename, GLuint& texID);
void close();

SDL_Window* gWindow = NULL;
SDL_GLContext gContext;

//shaders
Shader gCubeShader;
Shader gLightShader;
Shader gTransparent;

//OpenGL ids
GLuint gVAO, gVBO, wVAO;
GLuint diffuseMapID, specularMapID, transparentMapID, windowMapID;

// camera
Camera camera(glm::vec3(0.0f, 1.5f, 7.0f));
float lastX = -1;
float lastY = -1;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(1.3f, 2.0f, 3.0f);

//glass window
glm::vec3 glassPos(0.7f, 1.5f, 2.5f);

//event handlers
void HandleKeyDown(const SDL_KeyboardEvent& key);
void HandleMouseMotion(const SDL_MouseMotionEvent& motion);
void HandleMouseWheel(const SDL_MouseWheelEvent& wheel);

int main(int argc, char* args[])
{
	init();

	SDL_Event e;
	bool quit = false;
	while (!quit)
	{
		// per-frame time logic
		float currentFrame = SDL_GetTicks() / 1000.0f ;
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			//User requests quit
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
			switch (e.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_KEYDOWN:
				if (e.key.keysym.sym == SDLK_ESCAPE)
				{
					quit = true;
				}
				else {
					HandleKeyDown(e.key);
				}
				break;
			case SDL_MOUSEMOTION:
				HandleMouseMotion(e.motion);
				break;
			case SDL_MOUSEWHEEL:
				HandleMouseWheel(e.wheel);
				break;
			}
		}

		glEnable(GL_LIGHTING);
		glDisable(GL_BLEND);

		render();

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		renderWindow();
		renderGlass();
		
		glEnable(GL_LIGHTING);
		glDepthFunc(GL_LEQUAL);
		renderWindow();
		renderGlass();
		

		SDL_GL_SwapWindow(gWindow);
	}

	close();

	return 0;
}

void HandleKeyDown(const SDL_KeyboardEvent& key)
{
	switch (key.keysym.sym)
	{
	case SDLK_UP:
		camera.ProcessKeyboard(FORWARD, deltaTime);
		break;
	case SDLK_DOWN:
		camera.ProcessKeyboard(BACKWARD, deltaTime);
		break;
	case SDLK_LEFT:
		camera.ProcessKeyboard(LEFT, deltaTime);
		break;
	case SDLK_RIGHT:
		camera.ProcessKeyboard(RIGHT, deltaTime);
		break;
	case SDLK_s:
		lightPos.y -= 0.2f;
		break;
	case SDLK_w:
		lightPos.y += 0.2f;
		break;
	case SDLK_a:
		lightPos.x -= 0.2f;
		break;
	case SDLK_d:
		lightPos.x += 0.2f;
		break;
	}
}

void HandleMouseMotion(const SDL_MouseMotionEvent& motion)
{
	if (firstMouse)
	{
		lastX = motion.x;
		lastY = motion.y;
		firstMouse = false;
	}
	else
	{
		camera.ProcessMouseMovement(motion.x - lastX, lastY - motion.y);
		lastX = motion.x;
		lastY = motion.y;
	}
}

void HandleMouseWheel(const SDL_MouseWheelEvent& wheel)
{
	camera.ProcessMouseScroll(wheel.y);
}

bool init()
{
	//Initialization flag
	bool success = true;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//Use OpenGL 3.3
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);


		gWindow = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600,
			SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
		if (gWindow == NULL)
		{
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			//Create context
			gContext = SDL_GL_CreateContext(gWindow);
			if (gContext == NULL)
			{
				printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
				success = false;
			}
			else
			{
				if (SDL_GL_SetSwapInterval(1) < 0)
				{
					printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
				}

				//Initialize OpenGL
				if (!initGL())
				{
					printf("Unable to initialize OpenGL!\n");
					success = false;
				}
			}
		}
	}

	return success;
}

bool initGL()
{
	bool success = true;
	GLenum error = GL_NO_ERROR;

	glewInit();

	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		success = false;
		printf("Error initializing OpenGL! %s\n", gluErrorString(error));
	}

	glClearColor(0.35f, 0.35f, 0.35f, 1.0f);


	if (!LoadTexture("container2.png", diffuseMapID))
	{
		printf("Could not load texture \"container2.png\"");
	}
	if (!LoadTexture("container2_specular.png", specularMapID))
	{
		printf("Could not load texture \"container2_specular.png\"");
	}
	if (!LoadTexture("glass.png", transparentMapID))
	{
		printf("Could not load texture \"glass.png\"");
	}
	if (!LoadTexture("window.png", windowMapID))
	{
		printf("Could not load texture \"window.png\"");
	}


	gCubeShader.Load("./shaders/cube.vert", "./shaders/cube.frag");
	gLightShader.Load("./shaders/light.vert", "./shaders/light.frag");
	gTransparent.Load("./shaders/cube.vert", "./shaders/transp.frag");

	gCubeShader.use();
	gCubeShader.setInt("material.diffuse", 0);
	gCubeShader.setInt("material.specular", 1);

	gVAO = CreateCube(1.0f, gVBO);
	wVAO = CreateSquare(1.0f, gVBO);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	return success;
}

void close()
{
	glDeleteProgram(gCubeShader.ID);
	glDeleteProgram(gTransparent.ID);

	glDeleteVertexArrays(1, &gVAO);
	glDeleteVertexArrays(1, &wVAO);

	glDeleteBuffers(1, &gVBO);

	SDL_GL_DeleteContext(gContext);
	
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;

	SDL_Quit();
}

void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Cube
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), 4.0f / 3.0f, 0.1f, 100.0f);

	glm::mat3 normalMat = glm::transpose(glm::inverse(model));

	glUseProgram(gCubeShader.ID);
		//transformations
	gCubeShader.setMat4("model", model);
	gCubeShader.setMat4("view", view);
	gCubeShader.setMat4("proj", proj);
	gCubeShader.setMat3("normalMat", normalMat);

		//lighting
	gCubeShader.setVec3("light.diffuse", 1.0f, 1.0f, 1.0f);
	gCubeShader.setVec3("light.position", lightPos);
	gCubeShader.setVec3("viewPos", camera.Position);

		//material properties
	gCubeShader.setVec3("material.ambient", 0.2125f, 0.1275f, 0.054f);
	gCubeShader.setVec3("material.diffuse", 0.714f, 0.4284f, 0.18144f);
	gCubeShader.setVec3("material.specular", 0.393548f, 0.271906f, 0.166721f);
	gCubeShader.setFloat("material.shininess", 0.2f * 32);

		//material coefficients
	gCubeShader.setFloat("material.ka", 0.1f);
	gCubeShader.setFloat("material.kd", 1.0f);
	gCubeShader.setFloat("material.ks", 1.0f);

		// bind diffuse map
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, diffuseMapID);
		// bind specular map
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, specularMapID);

	DrawObject(gVAO);

	glUseProgram(gCubeShader.ID);
	gCubeShader.setMat4("projection", proj);
	gCubeShader.setMat4("view", view);
	model = glm::mat4(1.0);
	model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -0.5f));
	gCubeShader.setMat4("model", model);
	DrawObject(gVAO);


	//Lightsource
	glUseProgram(gLightShader.ID);
	gLightShader.setMat4("projection", proj);
	gLightShader.setMat4("view", view);
	model = glm::mat4(1.0);
	model = glm::translate(model, lightPos);
	model = glm::scale(model, glm::vec3(0.2f));
	gLightShader.setMat4("model", model);

	DrawObject(gVAO);
}

void renderGlass()
{
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), 4.0f / 3.0f, 0.1f, 100.0f);

	glm::mat3 normalMat = glm::transpose(glm::inverse(model));

	glUseProgram(gTransparent.ID);

	//transformations
	model = glm::translate(model, glm::vec3(0.0f, 0.7f, 0.0f));

	model = glm::scale(model, glm::vec3(0.5f));
	gTransparent.setMat4("model", model);
	gTransparent.setMat4("view", view);
	gTransparent.setMat4("proj", proj);
	gTransparent.setMat3("normalMat", normalMat);

	//lighting
	gTransparent.setVec3("light.diffuse", 1.0f, 1.0f, 1.0f);
	gTransparent.setVec3("light.position", lightPos);
	gTransparent.setVec3("viewPos", camera.Position);

	//material properties
	gTransparent.setVec3("material.ambient", 0.0f, 0.0f, 0.0f);
	gTransparent.setVec3("material.diffuse", 0.0f, 0.0f, 0.0f);
	gTransparent.setVec3("material.specular", 0.5f, 0.5f, 0.5f);
	gTransparent.setFloat("material.shininess", 0.6f * 128);

	//material coefficients
	gTransparent.setFloat("material.ka", 0.1f);
	gTransparent.setFloat("material.kd", 1.0f);
	gTransparent.setFloat("material.ks", 1.0f);
	
	// bind diffuse map
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, transparentMapID);

	DrawObject(gVAO);
}

void renderWindow()
{
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), 4.0f / 3.0f, 0.1f, 100.0f);
	glm::mat3 normalMat = glm::transpose(glm::inverse(model));

	glUseProgram(gTransparent.ID);

	model = glm::translate(model, glassPos);
	gTransparent.setMat4("model", model);
	gTransparent.setMat4("view", view);
	gTransparent.setMat4("proj", proj);
	gTransparent.setMat3("normalMat", normalMat);

	//lighting
	gTransparent.setVec3("light.diffuse", 1.0f, 1.0f, 1.0f);
	gTransparent.setVec3("light.position", lightPos);
	gTransparent.setVec3("viewPos", camera.Position);

	//material properties
	gTransparent.setVec3("material.ambient", 0.0f, 0.0f, 0.0f);
	gTransparent.setVec3("material.diffuse", 0.0f, 0.0f, 0.0f);
	gTransparent.setVec3("material.specular", 0.7f, 0.7f, 0.7f);
	gTransparent.setFloat("material.shininess", 0.6f * 128);

	//material coefficients
	gTransparent.setFloat("material.ka", 0.1f);
	gTransparent.setFloat("material.kd", 1.0f);
	gTransparent.setFloat("material.ks", 1.0f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, windowMapID);

	DrawObject(wVAO);

	//second window

	glUseProgram(gTransparent.ID);
	gTransparent.setMat4("projection", proj);
	gTransparent.setMat4("view", view);
	model = glm::mat4(1.0);
	model = glm::translate(model, glm::vec3(-0.3f, 1.5f, 2.5f));
	gTransparent.setMat4("model", model);
	
	DrawObject(wVAO);

	//third window
	glUseProgram(gTransparent.ID);
	gTransparent.setMat4("projection", proj);
	gTransparent.setMat4("view", view);
	model = glm::mat4(1.0);
	model = glm::translate(model, glm::vec3(-1.0f, 0.0f, 1.0f));
	gTransparent.setMat4("model", model);

	DrawObject(wVAO);
}

GLuint CreateCube(float width, GLuint& VBO)
{
	float vertices[] = 
	{
		// positions          // normals           // texture coords
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
		0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
	};

	GLuint VAO;
	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

	return VAO;
}

GLuint CreateSquare(float width, GLuint& VBO)
{
	float vertices[] =
	{
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
	};

	GLuint VAO;
	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return VAO;
}

void DrawObject(GLuint vaoID)
{
	glBindVertexArray(vaoID);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}


bool LoadTexture(const char* filename, GLuint& texID)
{
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); //these are the default values for warping
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	GLint width, height, channels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* img_data = stbi_load(filename, &width, &height, &channels, 0);

	if (!img_data)
	{
		return false;
	}

	//3 channels - rgb, 4 channels - RGBA
	GLenum format;
	switch (channels)
	{
	case 4:
		format = GL_RGBA;
		break;
	default:
		format = GL_RGB;
		break;
	}
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, img_data);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(img_data);

	return true;
}