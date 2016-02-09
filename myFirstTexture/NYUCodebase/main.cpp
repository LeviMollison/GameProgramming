#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "Matrix.h"

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif


SDL_Window* displayWindow;

GLuint LoadTexture(const char *image_path) {
    SDL_Surface *surface = IMG_Load(image_path);
    
    if(surface == NULL){
        printf("bad image\n");
        exit(1);
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_BGRA,GL_UNSIGNED_BYTE, surface->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    SDL_FreeSurface(surface);
    return textureID;
}

float radianConverter(float degree){
    return (degree * (3.1415926 / 180.0));
}

int main(int argc, char *argv[])
{
    
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif

    SDL_Event event;
    bool done = false;
    
    glViewport(0, 0, 640, 360);
    ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

    
    Matrix projectionMatrix;
    Matrix modelMatrix;
    modelMatrix.identity();
    Matrix viewMatrix;
    
    float lastFrameTicks = 0.0f;
    float angle = 0.0f;
    
    // Load the textures our program is gonna use
    GLuint fireTexture = LoadTexture(RESOURCE_FOLDER"images/fire2.png");
    GLuint medalTexture = LoadTexture(RESOURCE_FOLDER"images/flat_medal9.png");
    GLuint tileTexture = LoadTexture(RESOURCE_FOLDER"images/rpgTile181.png");
    
    
    projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
    glUseProgram(program.programID);
    
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        glClearColor(0.7f, 0.3f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // This stuff's for keeping time
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        angle += elapsed;
        
        // Move it where you need it to go Manually
        modelMatrix.identity();
        modelMatrix.Translate(-2.5f, 1.0f, 0.0f);
        
        // The fun stuff to do with it
        modelMatrix.Scale(cosf(angle), sinf(angle), 1.0f);

        
        //This tells the computer yo, do the matrix stuff
        program.setModelMatrix(modelMatrix);
        program.setProjectionMatrix(projectionMatrix);
        program.setViewMatrix(viewMatrix);
        
        // Draw the visible object for players
        float vertices[] = {-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f,  -0.5f, -0.5f, 0.5f, -0.5f};
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program.positionAttribute);
        
        // Now map a texture map to it
        float texCoords[] = {0.0, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0};
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program.texCoordAttribute);
        
        // Now get the actual texture loaded and draw the arrays out
        glBindTexture(GL_TEXTURE_2D, fireTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
        
        /* Now do it again for the other participants in the scene */
        
        // The fun stuff we doing to the Matrix
        modelMatrix.identity();
        // modelMatrix.Rotate(angle);
        
        // Move it where you need it to go Manually
        modelMatrix.identity();
        modelMatrix.Translate(2.5f, 1.0f, 0.0f);

        
        //This tells the computer yo, do the matrix stuff
        program.setModelMatrix(modelMatrix);
        program.setProjectionMatrix(projectionMatrix);
        program.setViewMatrix(viewMatrix);
        
        // Draw the visible object for players
        float secondGuy[] = {-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f,  -0.5f, -0.5f, 0.5f, -0.5f};
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, secondGuy);
        glEnableVertexAttribArray(program.positionAttribute);
        
        // Now map a texture map to it
        float secondTexture[] = {0.0, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0};
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, secondTexture);
        glEnableVertexAttribArray(program.texCoordAttribute);
        
        // Now get the actual texture loaded and draw the arrays out
        glBindTexture(GL_TEXTURE_2D, medalTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
        
        /* Now once more! */
        
        
        // Move it where you need it to go Manually
        modelMatrix.identity();
        modelMatrix.Translate(0.0f, -1.0f, 0.0f);
        
        // The fun stuff to do with it
        modelMatrix.Rotate(angle);

        
        //This tells the computer yo, do the matrix stuff
        program.setModelMatrix(modelMatrix);
        program.setProjectionMatrix(projectionMatrix);
        program.setViewMatrix(viewMatrix);
        
        // Draw the visible object for players
        float thirdGuy[] = {-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f,  -0.5f, -0.5f, 0.5f, -0.5f};
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, thirdGuy);
        glEnableVertexAttribArray(program.positionAttribute);
        
        // Now map a texture map to it
        float thirdTexture[] = {0.0, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0};
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, thirdTexture);
        glEnableVertexAttribArray(program.texCoordAttribute);
        
        // Now get the actual texture loaded and draw the arrays out
        glBindTexture(GL_TEXTURE_2D, tileTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        
        // Clears images? idk keep it on
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        SDL_GL_SwapWindow(displayWindow);
        
        
        
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
    }
    
    SDL_Quit();
    return 0;
}
