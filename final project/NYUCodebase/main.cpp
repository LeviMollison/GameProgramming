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

/*
    Final Project: Sonic Knock Off
    - Will be a 16x16 grid of rooms (how big are the rooms? debatable but 8x8 don't sound too big
*/

SDL_Window* displayWindow;

// Convert from degrees to radians
float radianConverter(float degree){
    return (degree * (3.1415926 / 180.0));
}

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

// use enums to determine entity types

// The Entity class, for each object we plan to draw into our program
enum EntityType {};
class Entity
{
public:
    
    // Let the spritesheet class handle the drawing
    SpriteSheet sprite;
    Matrix matrix;
    
    Entity();
    void update(float elapsed);
    void render(ShaderProgram *program);
    bool collidesWith(Entity *entity);
    float x;
    float y;
    float width;
    float height;
    float velocity_x;
    float velocity_y;
    float acceleration_x;
    float acceleration_y;
    
    EntityType entityType;
    bool collidedTop = false;
    bool collidedBottom = false;
    bool collidedLeft = false;
    bool collidedRight = false;
    
    // Position the object
    void position(ShaderProgram *program){
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
    void move(float timePerFrame){
        
    }
};

// The solution path is essential to this entire game's production

/*
    The solution path is an entire room wide/tall, orientation determined by random. It starts in the top row (a random start room) and randomly branches out until it hits the bottom row (exit room). If it hits a side that isnt on the bottom it tries immediatly to drop down. Once the solution path is determined, the game will build rooms that match the path. (example, if the path is a horizontal in the first room then the game will build a path that has exists for sure on the left and right.) We could make the solution path an object that stores the potential rooms and the necessary exits as it traverses through the level. 
    
        0: no definite exit, random entrance, fills up empty rooms not touched by solution path
        1: left and right only
        2: left right and top
        3: left right and bottom
        4: left right top bottom
        
        Can allow the solution path object to interact with the room templates, forcing entrances that need to be open to remain that way
*/
enum RoomTrail {start, journey, end, off};
#define LEVEL_HEIGHT 32 // 4 rooms * 8 tiles per room
#define LEVEL_WIDTH 32
/*
    Building the solution path object
    
    1,2,3,4
    5,6,7,8
    9,10,11,12
    13,14,15,16
    
    Create a solution path
    mark rooms that are chosen to be along the solution path: starting room (has a safe spot), journey rooms, end room (has an end point)
        - 2D array / vector route: use enums
    Rooms that have a safe /end spot will randomly choose one of the treasure areas and turn them into the starting spot
    pass the room templates and the marked rooms into the map generator
*/

class MapCoordinate{
public:
    MapCoordinate();
    MapCoordinate(int x, int y)
    :x(x), y(y) {}
    int x;
    int y;
};

// This will progress the solution path to a neihbor block
// pass in the current room, and return the next room.
MapCoordinate progressPath(MapCoordinate currentRoom){
    // Can go left(1) or right(2) or down(3)
    int dir = rand()%3 + 1;
    if (dir == 1){
        if ((currentRoom.x -1) < 0){
            currentRoom.y +=1;
            return currentRoom;
        }
        else{
            currentRoom.x-=1;
            return currentRoom;
        }
    }
    else
    if(dir == 2){
            if ((currentRoom.x +1) > 3){
                currentRoom.y +=1;
                return currentRoom;
            }
            else{
                currentRoom.x+=1;
                return currentRoom;
            }
        }
    else{
        currentRoom.y +=1;
        return currentRoom;
    }
    
}

void solutionPath(){
    std::vector<std::vector<RoomTrail>> mapPath;
    
    // Start with some dummy data
    int height = 4;
    for (int i=0;i<height;i++)
    {
        std::vector<RoomTrail> row;
        row.push_back(off);
        row.push_back(off);
        row.push_back(off);
        row.push_back(off);
        mapPath.push_back(row);
    }
    bool beginning = true;
    bool makingPath = true;
    MapCoordinate startRoom;
    MapCoordinate currentRoom;
    MapCoordinate endRoom;
    
    // Now run the randomizer
    
    /* 
        Instead use a while loop and continue until you reach the end of the level. Send the current map
        coordinate to a function that checks it's neibors and progresses it. each time itll change what state that
        room is in, where the next room is, and if it reaches the end will stop the path
    */
    while (makingPath){
         // If it's the first run
            if (beginning){
                beginning = false;
                int startPlace = rand() % 4;
                mapPath[startPlace][0] = start;
                startRoom = MapCoordinate(startPlace, 0);
                currentRoom = MapCoordinate(startPlace, 0);
            }
            else{
                // Now progress the room and store the journey marks
                currentRoom = progressPath(currentRoom);
                mapPath[currentRoom.x][currentRoom.y] = journey;
                
                // Make sure you didn't reach the end
                if ((currentRoom.y+1)>3){
                    makingPath = false;
                    mapPath[currentRoom.x][currentRoom.y] = end;
                    endRoom = MapCoordinate(currentRoom.x, currentRoom.y);
                }
            }
    }
    
}

/*
    How will room templates work?
    A level will be comprised of 16 rooms (4x4). Each room will be 8x8, making a level a 32x32 monster.
    Rooms will be created after running the solution path creator.
*/


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
    GLuint font_texture;
    GLuint game_texture;
    
    // Grand Finale!
    while (!done){
        ticks = (float)SDL_GetTicks()/1000.0f;
        fixedElapsed = ticks - elapsed;
        elapsed = ticks;
        // Game states as objects or enums?
        // update if the game's running
            if(fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
                fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
            }
            while (fixedElapsed >= FIXED_TIMESTEP ) {
                fixedElapsed -= FIXED_TIMESTEP;
                // Run proper updates
            }
                // run update once again
                // Run render after updates
    }

    cleanUp(&program);
    return 0;
}











