// This actually includes GLES 3.0 but is named GLES2.
#include <SDL3/SDL.h>
#include <glad/gles2.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

SDL_GLContext context;
SDL_Window *window;

#define GPU_ERROR(...) SDL_LogError(SDL_LOG_CATEGORY_GPU, __VA_ARGS__)

void *storage_read(SDL_Storage *storage, const char *path, Uint64 *length_out) {
	Uint64 length;
	if (SDL_GetStorageFileSize(storage, path, &length) && length > 0) {
		void *out = SDL_malloc(length);
		if (SDL_ReadStorageFile(storage, path, out, length)) {
			if (length_out != NULL)
				*length_out = length;
			return out;
		} else {
			SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "IO loading: %s\n", path);
			return NULL;
		}
	} else {
		SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Asset not found: %s\n", path);
		return NULL;
	}
}
void *title_storage_read(const char *path, Uint64 *length_out) {
	SDL_Storage *storage = SDL_OpenTitleStorage(NULL, 0);
	if (storage == NULL)
		return NULL;
	while (!SDL_StorageReady(storage))
		SDL_Delay(1);
	void *out = storage_read(storage, path, length_out);
	SDL_CloseStorage(storage);
	return out;
}

GLuint compile_shader(const char *asset_path, GLenum type) {
	Uint64 long_source_length;
	const GLchar *source = title_storage_read(asset_path, &long_source_length);
	if (source == NULL)
		return 0;
	GLint source_length = long_source_length;
	GLuint shader = glCreateShader(type);

	if (shader == 0) {
		GPU_ERROR("Error creating shader %s", asset_path);
		return 0;
	}

	glShaderSource(shader, 1, &source, &source_length);
	glCompileShader(shader);

	GLint compile_success;

	glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_success);

	if (!compile_success) {
		int length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		char *message = SDL_malloc(length);
		glGetShaderInfoLog(shader, length, &length, message);

		GPU_ERROR("In shader %s %s", asset_path, message);

		SDL_free(message);
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint link_program(GLuint vertex_shader, GLuint fragment_shader) {
	GLuint program = glCreateProgram();

	if (program == 0) {
		GPU_ERROR("Error Creating Shader Program");
		return 0;
	}

	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	glLinkProgram(program);

	GLint link_sucess;

	glGetProgramiv(program, GL_LINK_STATUS, &link_sucess);

	if (!link_sucess) {
		GPU_ERROR("Error Linking program");
		glDetachShader(program, vertex_shader);
		glDetachShader(program, fragment_shader);
		glDeleteProgram(program);

		return 0;
	}

	return program;
}

GLuint load_tri() {
	GLfloat vertices[] = { -0.7, -0.7, 0, 0.7, -0.7, 0, 0, 0.7, 0 };

	GLuint buffer;

	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return buffer;
}

bool init_graphics() {
	glClearColor(0, 0, 0, 0);

	GLuint buffer = load_tri();

	GLuint vertex_shader, fragment_shader;
	{
		vertex_shader
			= compile_shader("assets/shaders/shader.vert", GL_VERTEX_SHADER);
		if (vertex_shader == 0)
			return false;
		fragment_shader
			= compile_shader("assets/shaders/shader.frag", GL_FRAGMENT_SHADER);
		if (fragment_shader == 0)
			return false;
	}

	GLuint program = link_program(vertex_shader, fragment_shader);

	GLuint pos_attribute_position = glGetAttribLocation(program, "pos");

	GLuint vertex_array;
	glGenVertexArrays(1, &vertex_array);

	glBindVertexArray(vertex_array);

	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glVertexAttribPointer(pos_attribute_position, 3, GL_FLOAT, false, 0, 0);

	glEnableVertexAttribArray(pos_attribute_position);

	glUseProgram(program);

	return true;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
	int rc = SDL_Init(SDL_INIT_VIDEO);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	window = SDL_CreateWindow("Test", 800, 600, SDL_WINDOW_OPENGL);
	context = SDL_GL_CreateContext(window);

	{
		int version = gladLoadGLES2(SDL_GL_GetProcAddress);
		SDL_Log(
			"Loaded GLES %d.%d\n",
			GLAD_VERSION_MAJOR(version),
			GLAD_VERSION_MINOR(version)
		);
	}

	const GLubyte *version = glGetString(GL_VERSION);

	if (version == 0) {
		GPU_ERROR("Unable to get OpenGL ES version string: %d\n", glGetError());
		return SDL_APP_FAILURE;
	}

	SDL_Log("Version string: %s\n", version);
	SDL_Log("Base: %s\n", SDL_GetBasePath());
	SDL_Log("Pref: %s\n", SDL_GetPrefPath("sdl3-test", "jackfaller.xyz"));

	if (!init_graphics()) {
		GPU_ERROR("Failed to initialise graphics");
		return SDL_APP_FAILURE;
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	SDL_GL_SwapWindow(window);

	return SDL_APP_CONTINUE;
}
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
	switch (event->type) {
	case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;
	}
	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
	SDL_DestroyWindow(window);
	SDL_GL_DestroyContext(context);
}
