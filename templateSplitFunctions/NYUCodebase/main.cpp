#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

// Let's use this file to practice seperation as well


// Set's up what our program will be using
void setup();
// cleans up anything the program was using
void cleanUp();
// updates the objects in our program based on input
void update(SDL_Event &event, bool &done);
// draws declared objects onto the displayscreen
void render();

int main(int argc, char *argv[])
{
    setup();
    
    // variables that get used can, for now, be global
    SDL_Event event;
    bool done = false;
    
    while (!done) {
        update(event, done);
        render();
    }
    
    cleanUp();
    return 0;
}

void setup(){
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
}

void cleanUp()
{
    SDL_Quit();
}

void update(SDL_Event &event, bool &done)
{
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            done = true;
        }
    }
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(displayWindow);
}
