// This actually includes GLES 3.2 but is named GLES2.
#include <glad/gles2.h>
#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

SDL_GLContext context;
SDL_Window *window;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
  int rc = SDL_Init(SDL_INIT_VIDEO);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

  window = SDL_CreateWindow("Test", 800, 600, SDL_WINDOW_OPENGL);
  context = SDL_GL_CreateContext(window);

	{
    int version = gladLoadGLES2(SDL_GL_GetProcAddress);
	  SDL_Log("Loaded GLES %d.%d\n",
            GLAD_VERSION_MAJOR(version),
		        GLAD_VERSION_MINOR(version));
  }
  
  const GLubyte *version = glGetString(GL_VERSION);

  if (version == 0) {
    SDL_LogError(SDL_LOG_CATEGORY_GPU,
                 "Unable to get OpenGL ES version string: %d\n",
                 glGetError());
    return SDL_APP_FAILURE;
  }

  SDL_Log("Version string: %s\n", version);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
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
