// This actually includes GLES 3.0 but is named GLES2.
#include <SDL3/SDL.h>
#include <glad/gles2.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include <optional>
#include <vector>

#define CONSTRUCTORS(NAME) \
	NAME(NAME &) = delete; \
	NAME(NAME &&) = default

std::optional<std::vector<Uint8>>
storage_read(SDL_Storage *storage, const char *path) {
	Uint64 length;
	if (SDL_GetStorageFileSize(storage, path, &length) && length > 0) {
		std::vector<Uint8> out(length);
		if (SDL_ReadStorageFile(storage, path, &out[0], length)) {
			return out;
		} else {
			SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "IO loading: %s\n", path);
			return std::optional<std::vector<Uint8>>();
		}
	} else {
		SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Asset not found: %s\n", path);
		return std::optional<std::vector<Uint8>>();
	}
}

std::optional<std::vector<Uint8>> title_storage_read(const char *path) {
	SDL_Storage *storage = SDL_OpenTitleStorage(NULL, 0);
	if (storage == NULL)
		return std::optional<std::vector<Uint8>>();
	while (!SDL_StorageReady(storage))
		SDL_Delay(1);
	std::optional<std::vector<Uint8>> out = storage_read(storage, path);
	SDL_CloseStorage(storage);
	return out;
}

#define GPU_ERROR(...) SDL_LogError(SDL_LOG_CATEGORY_GPU, __VA_ARGS__)
struct InitialisationError {};

namespace gl {
struct Buffer {
	CONSTRUCTORS(Buffer);
	const GLuint handle;
	Buffer()
		: handle([]() {
			  GLuint out;
			  glGenBuffers(1, &out);
			  return out;
		  }()) {}
	~Buffer() { glDeleteBuffers(1, &handle); }
	void bind(GLenum target) const { glBindBuffer(target, handle); }
	static void unbind(GLenum target) { glBindBuffer(target, 0); }
	void data(size_t size, void *data, GLenum usage) const {
		bind(GL_ARRAY_BUFFER);
		glBufferData(GL_ARRAY_BUFFER, size, data, usage);
		unbind(GL_ARRAY_BUFFER);
	}
};
struct VertexArray {
	CONSTRUCTORS(VertexArray);
	const GLuint handle;
	static GLuint genBuffer() {
		GLuint out;
		glGenVertexArrays(1, &out);
		return out;
	}
	VertexArray() : handle(genBuffer()) {}
	~VertexArray() { glDeleteVertexArrays(1, &handle); }
	void bind() const { glBindVertexArray(handle); }
	static void unbind() { glBindVertexArray(0); }
};

struct Shader {
	CONSTRUCTORS(Shader);
	const GLuint handle;
	Shader(const char *source_path, GLenum type)
		: handle([](const char *source_path, GLenum type) {
			  std::optional<std::vector<Uint8>> source
				  = title_storage_read(source_path);
			  if (!source.has_value())
				  throw InitialisationError();
			  const GLchar *sources[] = { (const GLchar *)&(*source)[0] };
			  const GLint lengths[] = { (GLint)source->size() };

			  GLuint shader = glCreateShader(type);
			  if (shader == 0) {
				  GPU_ERROR("Error creating shader %s", source_path);
				  throw InitialisationError();
			  }

			  glShaderSource(shader, 1, sources, lengths);

			  glCompileShader(shader);
			  GLint compile_success;
			  glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_success);

			  if (!compile_success) {
				  int length;
				  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
				  std::vector<char> message(length);
				  glGetShaderInfoLog(shader, length, &length, &message[0]);

				  GPU_ERROR("In shader %s %s", source_path, &message[0]);

				  glDeleteShader(shader);
				  throw InitialisationError();
			  }
			  return shader;
		  }(source_path, type)) {}
	~Shader() { glDeleteShader(handle); }
};
struct Uniform {
	const GLuint location;
	Uniform(GLuint location) : location(location) {}
	void value_1f(GLfloat v0) const { glUniform1f(location, v0); }
	void value_2f(GLfloat v0, GLfloat v1) const {
		glUniform2f(location, v0, v1);
	}
};
struct Attribute {
	const GLuint index;
	Attribute(GLuint location) : index(location) {}
	void attribute(
		const Buffer &buffer,
		GLint size,
		GLenum type,
		GLboolean normalised,
		GLsizei stride,
		size_t offset
	) const {
		buffer.bind(GL_ARRAY_BUFFER);
		glVertexAttribPointer(
			index, size, type, normalised, stride, (const void *)offset
		);
		Buffer::unbind(GL_ARRAY_BUFFER);
	}
	void enable() const { glEnableVertexAttribArray(index); }
	void disable() const { glDisableVertexAttribArray(index); }
};
struct Program {
	CONSTRUCTORS(Program);
	const GLuint handle;
	Program(const Shader &vertex, const Shader &fragment)
		: handle([](GLuint vertex_shader, GLuint fragment_shader) {
			  GLuint program = glCreateProgram();

			  if (program == 0) {
				  GPU_ERROR("Error Creating Shader Program");
				  throw InitialisationError();
			  }

			  glAttachShader(program, vertex_shader);
			  glAttachShader(program, fragment_shader);

			  glLinkProgram(program);
			  GLint link_sucess;
			  glGetProgramiv(program, GL_LINK_STATUS, &link_sucess);

			  if (!link_sucess) {
				  GPU_ERROR("Error Linking program");
				  glDeleteProgram(program);
				  throw InitialisationError();
			  }

			  return program;
		  }(vertex.handle, fragment.handle)) {}
	~Program() { glDeleteProgram(handle); }
	Uniform get_uniform(const char *name) const {
		return Uniform(glGetUniformLocation(handle, name));
	}
	Attribute get_attribute(const char *name) const {
		return Attribute(glGetAttribLocation(handle, name));
	}
	void use() const { glUseProgram(handle); }
};
} // namespace gl

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

struct AppState {
	int width, height;
	SDL_Window *window;
	SDL_GLContext context;
	gl::Program program;
	gl::Uniform time, mouse_position, window_size;
	gl::Buffer tris;
	gl::VertexArray vertex_input;
	bool fullscreen = false;

	AppState(int width, int height)
		: width(width), height(height),
		  window(SDL_CreateWindow(
			  "SDL3 Template",
			  width,
			  height,
			  SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
		  )),
		  context([](SDL_Window *window) {
			  SDL_GL_SetAttribute(
				  SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES
			  );
			  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

			  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
			  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

			  SDL_GLContext context = SDL_GL_CreateContext(window);

			  int version = gladLoadGLES2(SDL_GL_GetProcAddress);
			  const GLubyte *version_string = glGetString(GL_VERSION);
			  if (version == 0) {
				  GPU_ERROR(
					  "Unable to get OpenGL ES version string: %d\n",
					  glGetError()
				  );
				  throw InitialisationError();
			  }
			  SDL_Log("Version string: %s\n", version_string);
			  return context;
		  }(this->window)),
		  program(
			  gl::Shader("assets/shaders/shader.vert", GL_VERTEX_SHADER),
			  gl::Shader("assets/shaders/shader.frag", GL_FRAGMENT_SHADER)
		  ),
		  time(program.get_uniform("time")),
		  mouse_position(program.get_uniform("mouse_position")),
		  window_size(program.get_uniform("window_size")) {
		glClearColor(0, 0, 0, 0);

		tris.data(sizeof(vertices), vertices, GL_STATIC_DRAW);

		gl::Attribute position = program.get_attribute("position");

		vertex_input.bind();
		position.attribute(tris, 3, GL_FLOAT, false, 0, 0);
		position.enable();
		gl::VertexArray::unbind();

		program.use();
	}
	SDL_AppResult iterate() {
		glClear(GL_COLOR_BUFFER_BIT);

		vertex_input.bind();
		time.value_1f(SDL_GetTicks() / 1000.0);
		float x, y;
		SDL_GetMouseState(&x, &y);
		mouse_position.value_2f(x + 0.5, y + 0.5);
		window_size.value_2f(width, height);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		SDL_GL_SwapWindow(window);

		return SDL_APP_CONTINUE;
	}
	SDL_AppResult event(SDL_Event *event) {
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
	~AppState() {
		SDL_GL_DestroyContext(context);
		SDL_DestroyWindow(window);
	}
};

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
	int width = 800, height = 600;
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

	try {
		*appstate = new AppState(width, height);
	} catch (InitialisationError) {
		GPU_ERROR("Failed to initialise graphics");
		return SDL_APP_FAILURE;
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
	return ((AppState *)appstate)->iterate();
}
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
	return ((AppState *)appstate)->event(event);
}
void SDL_AppQuit(void *appstate, SDL_AppResult) {
	delete (AppState *)appstate;
}
