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

// Load desired texture into the program
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

void DrawText(ShaderProgram *program, GLuint &fontTexture, std::string text, float size, float spacing, float xCord, float yCord, Matrix &matrix) {
    float texture_size = 1.0/16.0f;
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    for(int i=0; i < text.size(); i++) {
        float texture_x = (float)(((int)text[i]) % 16) / 16.0f;
        float texture_y = (float)(((int)text[i]) / 16) / 16.0f;
        vertexData.insert(vertexData.end(), {
            ((size+spacing) * i) + (-0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
        });
        texCoordData.insert(texCoordData.end(), {
            texture_x, texture_y,
            texture_x, texture_y + texture_size,
            texture_x + texture_size, texture_y,
            texture_x + texture_size, texture_y + texture_size,
            texture_x + texture_size, texture_y,
            texture_x, texture_y + texture_size,
        }); }
    matrix.identity();
    matrix.Translate(xCord, yCord, 0.0f);
    program->setModelMatrix(matrix);
    
    
    glUseProgram(program->programID);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glDrawArrays(GL_TRIANGLES, 0, (float)(text.size() * 6));
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

// Map the object's vertices to where they belong on the spritesheet and draw the object
class SpriteSheet
{
public:
    SpriteSheet();
    SpriteSheet(GLuint texID, float uCoord, float vCoord, float wid, float hei, float sze)
    : textureID(texID), u(uCoord), v(vCoord), width(wid), height(hei), size(sze)
    {}
    // left-x: (x position / image width), right-x: (x-position / image width)+(width/image width)
    // top-y: (y position / image height), bottom-y: (y-position / image height)+(height of image / image width)
    GLuint textureID;
    float u;
    float v;
    float width;
    float height;
    float size;
    
    void draw(ShaderProgram *program)
    {
        // Bind the texture to be used
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        // Texture Array
        GLfloat texCoords[] = {
            u, v+height,
            u+width, v,
            u, v,
            u+width, v,
            u, v+height,
            u+width, v+height
        };
        // The size of the image related to the aspect of the UV map
        float aspect = width / height;
        // Vertices Array
        float vertices[] = {
        -0.5f * size * aspect, -0.5f * size,
        0.5f * size * aspect, 0.5f * size,
        -0.5f * size * aspect, 0.5f * size,
        0.5f * size * aspect, 0.5f * size,
        -0.5f * size * aspect, -0.5f * size ,
        0.5f * size * aspect, -0.5f * size
        };
        // Map the vertex array to position attribute
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program->positionAttribute);
        
        // Map the texture array to the texture coordinate reader
        glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program->texCoordAttribute);
        
        // Then actually draw it
        glDrawArrays(GL_TRIANGLES, 0, ( (sizeof(vertices) / sizeof(float)) / 2));
        
        glDisableVertexAttribArray(program->positionAttribute);
        glDisableVertexAttribArray(program->texCoordAttribute);
    }
};

// The Entity class, for each object we plan to draw into our program
class Entity
{
public:
    // Time to create them!
    Entity(SpriteSheet sprite, Matrix matrix, float x, float y, float width, float height, float rotation, float max_vel, bool affectedByPlayer, bool bullet, int direction)
    :sprite(sprite), matrix(matrix), x(x), y(y), width(width), height(height), rotation(rotation), max_vel(max_vel), affectedByPlayer(affectedByPlayer), bullet(bullet), direction(direction) {
        alive = true;
        usable = true;
        if (affectedByPlayer == true && !bullet){
            GLuint bullet_texture = LoadTexture(RESOURCE_FOLDER"sheet.png");
            SpriteSheet bullet_sprite = SpriteSheet(bullet_texture, 809.0f/1024.0f, 437.0f/1024.0f, 19.0f/1024.0f, 30.0f/1024.0f, 0.5f);
            Matrix new_matrix;
            for (int i = 0; i < 2; i++){
                Entity newBullet = Entity(bullet_sprite, new_matrix, -5.0f, -5.0f, 0.1, 0.2, 90.0f, 1.0f, true, true, 1.0);
                bullets.push_back(newBullet);
            }
        }
    }
    // Let the spritesheet class handle the drawing
    SpriteSheet sprite;
    
    // the matrix to modify the object's position
    Matrix matrix;
    float x;
    float y;
    // width and height (scalex and scaley)
    float width;
    float height;
    float rotation;
    
    // Physics
    float max_vel;
    float velocity_x;
    float velocity_y;
    
    float max_accel;
    float acceleration_x;
    float acceleration_y;
    
    float friction_x;
    float friction_y;
    
    // Tells if object is affected by player input
    bool affectedByPlayer;
    bool alive;
    
    // Gotta store the bullets somewhere
    std::vector<Entity> bullets;
    bool bullet;
    bool usable;
    
    // Position the object
    void position(ShaderProgram *program)
    {
        program->setModelMatrix(matrix);
        matrix.identity();
        matrix.Translate(x, y, 0);
        matrix.Scale(width, height, 1.0f);
    }
   
    /*
        Called when the player makes a command to move the player object
        store the direction for spaceships
    */
    int direction;
    void move(float timePerFrame)
    {
        if (!bullet){
            velocity_x = max_vel;
            x+= (float) direction * velocity_x * timePerFrame;
            y+= velocity_y * timePerFrame;
            velocity_x = 0;
            velocity_y = 0;
        }
        else{
            velocity_y = max_vel;
            y+= (float)direction * velocity_y * timePerFrame;
            x+= velocity_x *timePerFrame;
            velocity_x = 0;
            velocity_y = 0;
        }
    }
};



// Convert from degrees to radians
float radianConverter(float degree){
    return (degree * (3.1415926 / 180.0));
}

// Sets the program up
void setup()
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    #ifdef _WINDOWS
        glewInit();
    #endif
}



// processes the input from out program
void processEvents(SDL_Event &event, bool &done, float &timePerFrame)
{
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            done = true;
        }
    }
}


// cleans up anything the program was using
void cleanUp(ShaderProgram *program)
{
    SDL_Quit();
}

int main(int argc, char *argv[])
{
    setup();
    ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    
    #define FIXED_TIMESTEP 0.0166666f
    #define MAX_TIMESTEPS 6
    
    SDL_Event event;
    bool done = false;
    int currentState = 0;
    float fixedElapsed = 0.0f;
    float ticks;
    float elapsed = 0.0f;
    
    // Grand Finale!
    while (!done){
        ticks = (float)SDL_GetTicks()/1000.0f;
        fixedElapsed = ticks - elapsed;
        elapsed = ticks;
        // update if the main menu is running, then render
        if (currentState == 0){
            if(fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
                fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
            }
            while (fixedElapsed >= FIXED_TIMESTEP ) {
                fixedElapsed -= FIXED_TIMESTEP;
                // Update function here
            }
                //Update here
                // Render here
        }
    }

    cleanUp(&program);
    return 0;
}











