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

#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

#define SPRITE_COUNT_X 30
#define SPRITE_COUNT_Y 30
#define TILE_SIZE 0.5f

using namespace std;

/*
    Space Invaders
    - A bunch, probably around 30, bodies move left, hit the wall then down, then opposite direction, hit the wall then down, until they reach the the player
    - The player can move left and right, shooting bullets (up to 3 visible at a time) 
    - A main menu screen that has a play button, bonus: you lose menu asking to play again
*/

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
    Entity(int spritePos, float x, float y, GLuint textureID):
    spritePos(spritePos), x(x), y(x), textureID(textureID)
    {}
    // is an ID
    GLuint textureID;
    
    // the matrix to modify the object's position
    Matrix matrix;
    float x;
    float y;
    // width and height (scalex and scaley)
    float width;
    float height;
    float rotation;
    
    std::string type;
    
    // Physics
    float max_vel;
    float velocity_x;
    float velocity_y;
    
    float max_accel;
    float acceleration_x;
    float acceleration_y;
    
    float friction_x;
    float friction_y;
    
    // Sprite positioning
    int spritePos;
    vector<float> tileVerts;
    vector<float> tileTexts;

    
    // Position the object
    void position(ShaderProgram *program)
    {
        program->setModelMatrix(matrix);
        matrix.identity();
        matrix.Translate(x, y, 0);
        matrix.Scale(width, height, 1.0f);
    }
    
    void draw(){
        float u = (float)(spritePos % SPRITE_COUNT_X) / (float) SPRITE_COUNT_X;
        float v = (float)(spritePos / SPRITE_COUNT_X) / (float) SPRITE_COUNT_Y;
                
        float spriteWidth = 1.0f / (float) SPRITE_COUNT_X;
        float spriteHeight = 1.0f / (float) SPRITE_COUNT_Y;
                
        tileVerts.insert(tileVerts.end(), {
            TILE_SIZE * x, -TILE_SIZE * y,
            TILE_SIZE * x, (-TILE_SIZE * y)-TILE_SIZE,
            (TILE_SIZE * x)+TILE_SIZE, (-TILE_SIZE * y)-TILE_SIZE,
            TILE_SIZE * x, -TILE_SIZE * y,
            (TILE_SIZE * x)+TILE_SIZE, (-TILE_SIZE * y)-TILE_SIZE,
            (TILE_SIZE * x)+TILE_SIZE, -TILE_SIZE * y
        });
                
        tileTexts.insert(tileTexts.end(), {
            u, v,
            u, v+(spriteHeight),
            u+spriteWidth, v+(spriteHeight),
            u, v,
            u+spriteWidth, v+(spriteHeight),
            u+spriteWidth, v
        });
    }
   
    /*
        Called when the player makes a command to move the player object
        store the direction for spaceships
    */
    int direction;
    void move(float timePerFrame){
    
    }
};


// Make the map a class, store the game in this map object

class Map{
public:

    // Constructor
    Map(GLuint textureID):
    textureID(textureID){}
    // fields for the map
    int mapWidth;
    int mapHeight;
    int tileWidth;
    int tileHeight;
    unsigned char **levelData;
    
    // Testing
    float x = 0;
    float y = 0;
    
    // Tile Verts
    std::vector<float> tileVerts;
    std::vector<float> tileTexts;
    
    // spritesheet
    GLuint textureID;
    
    // maybe keep a vector of entities in the map
    
    bool readHeader(ifstream& levelFile){
        string line;
        mapWidth = -1;
        mapHeight = -1;
        while(getline(levelFile, line)) {
            if(line == "") { break; }
            istringstream sStream(line);
            string key,value;
            getline(sStream, key, '=');
            getline(sStream, value);
            if(key == "width") {
                mapWidth = atoi(value.c_str());
            }
                else if(key == "height"){
                    mapHeight = atoi(value.c_str());
                }
                else if (key == "tilewidth"){
                    tileWidth = atoi(value.c_str());
                }
                else if (key == "tileheight"){
                    tileHeight = atoi(value.c_str());
                }
        }
        if(mapWidth == -1 || mapHeight == -1) {
            return false;
        } else { // allocate our map data
            levelData = new unsigned char*[mapHeight];
            for(int i = 0; i < mapHeight; ++i) {
                levelData[i] = new unsigned char[mapWidth];
            }
            return true;
        }
    }
    bool readLayerData(std::ifstream& levelFile){
        string line;
            while(getline(levelFile, line)) {
                if(line == "") { break; }
                istringstream sStream(line);
                string key,value;
                getline(sStream, key, '=');
                getline(sStream, value);
                if(key == "data") {
                    for(int y=0; y < mapHeight; y++) {
                        getline(levelFile, line);
                        istringstream lineStream(line);
                        string tile;
                        for(int x=0; x < mapWidth; x++) {
                            getline(lineStream, tile, ',');
                            unsigned char val =  (unsigned char)atoi(tile.c_str());
                            if(val > 0) {
        // be careful, the tiles in this format are indexed from 1 not 0
                                levelData[y][x] = val-1;
                            } else {
                                levelData[y][x] = 0;
                            }
                        }
                    }
                }
            }
            return true;
        }
    
    // Place entity where it belongs in levelData
    void placeEntity(std::string& type, float placeX, float placeY){
        
    }

    bool readEntityData(std::ifstream& levelFile){
        string line;
        string type;
        while(getline(levelFile, line)) {
            if(line == "") { break; }
            istringstream sStream(line);
            string key,value;
            getline(sStream, key, '=');
            getline(sStream, value);
            if(key == "type") {
                type = value;
            } else if(key == "location") {
                istringstream lineStream(value);
                string xPosition, yPosition;
                getline(lineStream, xPosition, ',');
                getline(lineStream, yPosition, ',');
                float placeX = atoi(xPosition.c_str())/tileWidth * TILE_SIZE;
                float placeY = atoi(yPosition.c_str())/tileHeight*-TILE_SIZE;
                placeEntity(type, placeX, placeY);
            }
        }
        return true;
    }

    void readMapFile(string& levelFile){
        ifstream infile(levelFile);
    
        string line;
        while(getline(infile, line)){
            if (line == "[header]"){
                if(!readHeader(infile)){
                    return;
                }
            } else
            if (line == "[layer]"){
                readLayerData(infile);
            } else
            if (line == "[object]"){
                readEntityData(infile);
            }
        }
    }

    Matrix viewMatrix;
    Matrix modelMatrix;
    Matrix projectionMatrix;
    // Counts up the amount of tiles that need to be drawn and draw them
    void drawTiles(ShaderProgram *program){
    
        // Matrices
        program->setModelMatrix(modelMatrix);
        program->setViewMatrix(viewMatrix);
        
        glClear(GL_COLOR_BUFFER_BIT);
    
        projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
        glUseProgram(program->programID);
        program->setProjectionMatrix(projectionMatrix);
        // don't forget you need to bind the texture
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        for (int y=0; y < mapHeight; y++){
            for (int x=0; x < mapWidth;  x++){
            if(levelData[y][x] != 0) {
                float u = (float)(((int) levelData[y][x]) % SPRITE_COUNT_X) / (float) SPRITE_COUNT_X;
                float v = (float)(((int) levelData[y][x]) / SPRITE_COUNT_X) / (float) SPRITE_COUNT_Y;
                
                float spriteWidth = 1.0f / (float) SPRITE_COUNT_X;
                float spriteHeight = 1.0f / (float) SPRITE_COUNT_Y;
                
                tileVerts.insert(tileVerts.end(), {
                    TILE_SIZE * x, -TILE_SIZE * y,
                    TILE_SIZE * x, (-TILE_SIZE * y)-TILE_SIZE,
                    (TILE_SIZE * x)+TILE_SIZE, (-TILE_SIZE * y)-TILE_SIZE,
                    TILE_SIZE * x, -TILE_SIZE * y,
                    (TILE_SIZE * x)+TILE_SIZE, (-TILE_SIZE * y)-TILE_SIZE,
                    (TILE_SIZE * x)+TILE_SIZE, -TILE_SIZE * y
                });
                
                tileTexts.insert(tileTexts.end(), {
                    u, v,
                    u, v+(spriteHeight),
                    u+spriteWidth, v+(spriteHeight),
                    u, v,
                    u+spriteWidth, v+(spriteHeight),
                    u+spriteWidth, v
                });
                }
            }
        }
        // Map the vertex array to position attribute
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, tileVerts.data());
        glEnableVertexAttribArray(program->positionAttribute);
        
        // Map the texture array to the texture coordinate reader
        glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, tileTexts.data());
        glEnableVertexAttribArray(program->texCoordAttribute);
        
        // Then actually draw it
        glDrawArrays(GL_TRIANGLES, 0, ( (int)tileVerts.size() / 2));
        
        glDisableVertexAttribArray(program->positionAttribute);
        glDisableVertexAttribArray(program->texCoordAttribute);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
        SDL_GL_SwapWindow(displayWindow);
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
void processEvents(SDL_Event &event, bool &done, float &timePerFrame, Map& game)
{
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            done = true;
        }
    }
    // Manual Scrolling
    if (keys[SDL_SCANCODE_LEFT]){
            game.viewMatrix.identity();
            game.x+=0.5;
            game.viewMatrix.Translate(game.x, game.y, 0);
    }
    if (keys[SDL_SCANCODE_UP]){
            game.viewMatrix.identity();
            game.y-=0.5;
            game.viewMatrix.Translate(game.x, game.y, 0);
    }
    if (keys[SDL_SCANCODE_RIGHT]){
            game.viewMatrix.identity();
            game.x-=0.5;
            game.viewMatrix.Translate(game.x, game.y, 0);
    }
    if (keys[SDL_SCANCODE_DOWN]){
            game.viewMatrix.identity();
            game.y+=0.5;
            game.viewMatrix.Translate(game.x, game.y, 0);
    }
    // Reset Manual
    if (keys[SDL_SCANCODE_SPACE]){
            game.viewMatrix.identity();
            game.x=0;
            game.y=0;
            game.viewMatrix.Translate(game.x, game.y, 0);
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
    float fixedElapsed = 0.0f;
    float ticks;
    float elapsed = 0.0f;
    
    GLuint mapTexture = LoadTexture(RESOURCE_FOLDER"spritesheet_rgba.png");
    Map game = Map(mapTexture);
    
    // Gotta make the game somewhere
    std::string mapFile = RESOURCE_FOLDER"platformDemoMap.txt";
    game.readMapFile(mapFile);
    
    // Grand Finale!
    while (!done){
        ticks = (float)SDL_GetTicks()/1000.0f;
        fixedElapsed = ticks - elapsed;
        elapsed = ticks;
        // update if the main menu is running, then render
        processEvents(event, done, elapsed, game);
        /*
        if(fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
            fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
        }
        while (fixedElapsed >= FIXED_TIMESTEP ) {
            fixedElapsed -= FIXED_TIMESTEP;
            // Run update here
        }
        */
            // Run update again
            // Run render here, everything will be correct
            game.drawTiles(&program);
    }

    cleanUp(&program);
    return 0;
}











