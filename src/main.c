// This actually includes GLES 3.0 but is named GLES2.
#include <SDL3/SDL.h>
#include <glad/gles2.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

SDL_GLContext context;
SDL_Window *window;
int width = 800, height = 600;

struct {
	GLuint program, tri_buffer, vertex_array;
	GLuint time_uniform, mouse_position_uniform, window_size_uniform;
} gl;

#define GPU_ERROR(...) SDL_LogError(SDL_LOG_CATEGORY_GPU, __VA_ARGS__)

uint8_t *
storage_read(SDL_Storage *storage, const char *path, Uint64 *length_out) {
	Uint64 length;
	if (SDL_GetStorageFileSize(storage, path, &length) && length > 0) {
		void *out = SDL_malloc(length);
		if (SDL_ReadStorageFile(storage, path, out, length)) {
			if (length_out != NULL)
				*length_out = length;
			return out;
		} else {
			SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "IO loading: %s\n", path);
			SDL_free(out);
			return NULL;
		}
	} else {
		SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Asset not found: %s\n", path);
		return NULL;
	}
}
uint8_t *title_storage_read(const char *path, Uint64 *length_out) {
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
	GLchar *source
		= (GLchar *)title_storage_read(asset_path, &long_source_length);
	if (source == NULL)
		return 0;
	GLint source_length = long_source_length;
	GLuint shader = glCreateShader(type);

	if (shader == 0) {
		GPU_ERROR("Error creating shader %s", asset_path);
		SDL_free(source);
		return 0;
	}

	const GLchar *s = source;
	glShaderSource(shader, 1, &s, &source_length);
	SDL_free(source);

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
		glDeleteProgram(program);
		return 0;
	}

	return program;
}

GLfloat vertices[][3][3] = {
	{
		{ -1, 1, 0 },
		{ -1, -1, 0 },
		{ 1, -1, 0 },
	},
	{
		{ 1, -1, 0 },
		{ 1, 1, 0 },
		{ -1, 1, 0 },
	},
};
GLuint load_tri() {
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return buffer;
}

bool init_graphics() {
	glClearColor(0, 0, 0, 0);

	gl.tri_buffer = load_tri();

	GLuint vertex_shader, fragment_shader;
	{
		vertex_shader
			= compile_shader("assets/shaders/shader.vert", GL_VERTEX_SHADER);
		if (vertex_shader == 0)
			return false;
		fragment_shader
			= compile_shader("assets/shaders/shader.frag", GL_FRAGMENT_SHADER);
		if (fragment_shader == 0) {
			glDeleteShader(vertex_shader);
			return false;
		}
	}

	gl.program = link_program(vertex_shader, fragment_shader);
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	if (gl.program == 0)
		return false;
#define SET_UNIFOM(NAME) \
	gl.NAME##_uniform = glGetUniformLocation(gl.program, #NAME)
	SET_UNIFOM(time);
	SET_UNIFOM(mouse_position);
	SET_UNIFOM(window_size);
#undef SET_UNIFOM

	GLuint pos_attribute = glGetAttribLocation(gl.program, "pos");

	glGenVertexArrays(1, &gl.vertex_array);
	glBindVertexArray(gl.vertex_array);

	glBindBuffer(GL_ARRAY_BUFFER, gl.tri_buffer);
	glVertexAttribPointer(pos_attribute, 3, GL_FLOAT, false, 0, 0);

	glEnableVertexAttribArray(pos_attribute);

	glUseProgram(gl.program);

	return true;
}

int update_uint_variable(
	const char *name, const char *new_value, int old_value
) {
	char *error_char = NULL;
	int value = SDL_strtoul(new_value, &error_char, 0);
	if (error_char != new_value + SDL_strlen(new_value)
	    || error_char == new_value) {
		SDL_LogWarn(
			SDL_LOG_CATEGORY_APPLICATION,
			"Expected integer for %s, got %s\n",
			name,
			new_value
		);
		value = old_value;
	}
	return value;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		GPU_ERROR("Couldn't initialise video output: %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	for (int i = 1; i < argc; ++i) {
		if (0 == SDL_strcmp(argv[i], "--width")) {
			width = update_uint_variable("--width", argv[++i], width);
		} else if (0 == SDL_strcmp(argv[i], "--height")) {
			height = update_uint_variable("--height", argv[++i], height);
		}
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

	window
		= SDL_CreateWindow("SDL3 Template", width, height, SDL_WINDOW_OPENGL);
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

	if (!init_graphics()) {
		GPU_ERROR("Failed to initialise graphics");
		return SDL_APP_FAILURE;
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {

	glClear(GL_COLOR_BUFFER_BIT);

	glUniform1f(gl.time_uniform, SDL_GetTicks() / 1000.0);
	float x, y;
	SDL_GetMouseState(&x, &y);
	glUniform2f(gl.mouse_position_uniform, x + 0.5, y + 0.5);
	glUniform2f(gl.window_size_uniform, width, height);
	glDrawArrays(GL_TRIANGLES, 0,  6);
	SDL_GL_SwapWindow(window);

	return SDL_APP_CONTINUE;
}
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
	static bool fullscreen;
	switch (event->type) {
	case SDL_EVENT_KEY_DOWN:
		if (event->key.key == SDLK_F)
			SDL_SetWindowFullscreen(window, !fullscreen);
		break;
	case SDL_EVENT_WINDOW_ENTER_FULLSCREEN: fullscreen = true; break;
	case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN: fullscreen = false; break;
	case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
		glViewport(
			0, 0, width = event->window.data1, height = event->window.data2
		);
		break;
	case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;
	}
	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
	glDeleteVertexArrays(1, &gl.vertex_array);
	glDeleteBuffers(1, &gl.tri_buffer);
	glDeleteProgram(gl.program);
	SDL_GL_DestroyContext(context);
	SDL_DestroyWindow(window);
}
