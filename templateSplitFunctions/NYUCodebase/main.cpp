#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "ShaderProgram.h"
#include <vector>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

// Let's use this file to practice seperation as well


// Set's up what our program will be using
void setup(Matrix &projectionMatrix, Matrix &viewMatrix, Matrix &modelMatrix){
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
}

// cleans up anything the program was using
void cleanUp(ShaderProgram *program)
{
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
    SDL_Quit();
}

// updates the objects in our program based on input
void update(SDL_Event &event, bool &done)
{
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            done = true;
        }
    }
}
// draws declared objects onto the displayscreen
void render(ShaderProgram *program)
{
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(displayWindow);
}

// processes the input from out program
void processEvents(SDL_Event &event, bool &done)
{
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            done = true;
        }
    }
}

// Loads textures
GLuint LoadTexture(const char *image_path) {
    SDL_Surface *surface = IMG_Load(image_path);
    
    if(surface == NULL){
        printf("bad image\n");
        exit(1);
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, surface->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    SDL_FreeSurface(surface);
    return textureID;
}

int main(int argc, char *argv[])
{
    // Variables can, for now, be global
    SDL_Event event;
    bool done = false;
    Matrix projectionMatrix;
    Matrix modelMatrix;
    Matrix viewMatrix;
    setup(projectionMatrix, viewMatrix, modelMatrix);
    ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    
    // The arrays that will be drawing our data
    std::vector<float> vertexData;
    std::vector<float> textureCoordData;
    
    // Keeping track of frames. time get's updated in update
    float lastFrameTicks = 0.0f;
    float framesPerSecond = 60.0f;
    
    while (!done) {
        update(event, done);
        render(&program);
    }
    
    cleanUp(&program);
    return 0;
}
