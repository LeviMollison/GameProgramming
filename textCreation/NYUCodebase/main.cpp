#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <vector>
#include "Matrix.h"
#include "ShaderProgram.h"

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

// Let's use this file to practice seperation as well: draw hello world!

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

// Set's up what our program will be using
void setup(Matrix &projectionMatrix, Matrix &viewMatrix, Matrix &modelMatrix){
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(0, 0, 640, 360);
    projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
}

// cleans up anything the program was using
void cleanUp(ShaderProgram *program)
{
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
    SDL_Quit();
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

//draws text to the screen
void drawText(std::vector<float> &vertexData, std::vector<float> &textureCoordData, std::string text, float size, float spacing)
{
    float texture_size = 1.0f/16.0f;
    for (int i  =0; i<text.size(); i++)
    {
        // the x coordinate position of the texture on the image map
        float texture_u = (float)(((int)text[i])%16) / 16.0f;
        // the y coordinate position of the texture on the image map
        float texture_v = (float)(((int)text[i])/16)/16.0f;
        // The 6 vertex points of the letter being drawn
        vertexData.insert(vertexData.end(), {
            ((size+spacing)*i) + (-0.5f * size), 0.5f *size,
            ((size+spacing)*i) + (-0.5f * size), -0.5f *size,
            ((size+spacing)*i) + (0.5f * size), 0.5f *size,
            ((size+spacing)*i) + (0.5f * size), -0.5f *size,
            ((size+spacing)*i) + (0.5f * size), 0.5f *size,
            ((size+spacing)*i) + (-0.5f * size), -0.5f *size
        });
        
        // Will map a texture coordinate to each created vertex point, fully forming the letter
        textureCoordData.insert(textureCoordData.end(), {
            texture_u, texture_v,
            texture_u, texture_v + texture_size,
            texture_u + texture_size, texture_v,
            texture_u + texture_size, texture_v + texture_size,
            texture_u + texture_size, texture_v,
            texture_u, texture_v + texture_size
        });
    }
}

// draws declared objects onto the displayscreen
void render(ShaderProgram *program, std::vector<float> &vertexData, std::vector<float> &textureCoordData, GLuint fontTexture, std::string text)
{
    glClearColor(0.3, 0.7, 0.8, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    
    drawText(vertexData, textureCoordData, text, 1.2f, 0.5f);
    
    glUseProgram(program->programID);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, textureCoordData.data());
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glDrawArrays(GL_TRIANGLES, 0, (int)text.size() * 6);
    
    // Clears images? idk keep it on
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    SDL_GL_SwapWindow(displayWindow);
}

// will update the program based on input
void update(Matrix &projectionMatrix, Matrix &viewMatrix, Matrix &modelMatrix, float &lastFrameTicks, const float &framesPerSecond)
{
    float ticks = (float)SDL_GetTicks()/1000.0f;
    float elapsed = ticks - lastFrameTicks;
    lastFrameTicks = ticks;
}

int main(int argc, char *argv[])
{
    // variables that get used can, for now, be global
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
    
    // Load the font Texture
    GLuint fontTexture = LoadTexture(RESOURCE_FOLDER"images/font1.png");
    
    while (!done) {
        processEvents(event, done);
        update(projectionMatrix, viewMatrix, modelMatrix, lastFrameTicks, framesPerSecond);
        render(&program, vertexData, textureCoordData, fontTexture, "Hello World");
    }
    cleanUp(&program);
    
    return 0;
}

