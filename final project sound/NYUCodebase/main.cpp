#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "ShaderProgram.h"
#include <vector>
#include <SDL_mixer.h>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#define SPRITE_COUNT_X 30
#define SPRITE_COUNT_Y 30
#define TILE_SIZE 0.5f
#define LEVEL_HEIGHT 32 // 4 rooms * 8 tiles per room
#define LEVEL_WIDTH 32
enum GameState {menu, game, endScreen};
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

// use enums to determine entity types

// The Entity class, for each object we plan to draw into our program
enum EntityType {};
class Entity
{
public:
    Matrix matrix;
    Matrix view;
    
    Entity(GLuint textureID, int spritePos, std::vector<std::vector<int>>& grid):textureID(textureID), spritePos(spritePos), grid(grid){
        // Make the entity start on top of where the start block is
        startPlayer(grid);
    }
    void update(float elapsed);
    float x = 0.5f;
    float y = 0.5f;
    float width = TILE_SIZE;
    float height = TILE_SIZE;
    float velocity_x;
    float velocity_y;
    float acceleration_x;
    float acceleration_y;
    std::vector<std::vector<int>> grid;
    
    //Drawing
    float size = 2.0;
    float spritePos;
    GLuint textureID;
    
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
        program->setViewMatrix(view);
        view.identity();
        view.Translate(-1.0*TILE_SIZE*x, -1.0*TILE_SIZE*y, 0.0);
        view.Scale(width, height, 1.0f);
    }
    
    void startPlayer(std::vector<std::vector<int>>& grid){
        // Make entity start on top of where the start block is
        for(int gridY = 0; gridY < LEVEL_HEIGHT; gridY++){
            // Horizontal Now, for Checking
            for(int gridX = 0; gridX < LEVEL_WIDTH; gridX++){
               // Now go until you find starting point
               if (grid[gridX][gridY] == 0){
                    x = gridX * TILE_SIZE + TILE_SIZE/2.0;
                    y = gridY * -1.0 * TILE_SIZE + TILE_SIZE/2.0;
                    break;
               }
            }
        }
    }
    
    // One for enemies, the other for things on the grid
    // 0 bottom,1 top,2 left,3 right
    bool collidesWith(int area, std::vector<std::vector<int>>& grid){
        // separate it into parts
        // For every tile in the grid, if the area of the entity is colliding with the tile, return true
        bool collided = false;
        float playerTop = y + height / 2.0f;
        float playerBot = y - height / 2.0f;
        float playerLeft = x - width / 2.0f;
        float playerRight = x + width / 2.0f;
        
        float tileTop;
        float tileBot;
        float tileLeft;
        float tileRight;
        for(int gridY = 0; gridY < LEVEL_HEIGHT; gridY++){
            // Horizontal Now, for Checking
            for(int gridX = 0; gridX < LEVEL_WIDTH; gridX++){
               // Now go until you find starting point
               if (grid[gridX][gridY] == 1){
               tileRight = gridX * TILE_SIZE + TILE_SIZE;
               tileLeft = gridX * TILE_SIZE;
               tileTop = gridY * -1.0 * TILE_SIZE + TILE_SIZE;
               tileBot = gridY * -1.0 * TILE_SIZE;
               // Check Bottom of player
               if(area ==0){
                   if (playerBot < tileTop && playerBot > tileBot && playerRight <= tileRight && playerLeft >= tileLeft)
                       collided = true;
                }
                // Check Top of player
                if(area == 1){
                    if(playerTop > tileBot && playerTop < tileTop && playerRight <= tileRight && playerLeft >= tileLeft)
                        collided = true;
                }
                // Check left of player
                if (area == 2){
                    if(playerLeft < tileRight && playerRight > tileRight && playerTop <= tileTop && playerBot >= tileBot )
                        collided = true;
                }
                // Check right of player
                if (area == 3){
                    if(playerRight > tileLeft && playerLeft < tileLeft && playerTop <= tileTop && playerBot >= tileBot)
                        collided = true;
                }
                }
            }
        }
        return collided;
    }
   
    /*
        Called when the player makes a command to move the player object
        store the direction for spaceships
    */
    int direction;
    void move(float timePerFrame){
        
    }
    
    void draw(ShaderProgram* program){
        
        // Bind the texture to be used
        glBindTexture(GL_TEXTURE_2D, textureID);
    
        float u = (float)(((int) spritePos) % SPRITE_COUNT_X) / (float) SPRITE_COUNT_X;
        float v = (float)(((int) spritePos) / SPRITE_COUNT_X) / (float) SPRITE_COUNT_Y;
    
         float aspect = width / height;
        // Vertices Array
        float vertices[] = {
            -0.5f , -0.5f ,
            0.5f , 0.5f ,
            -0.5f  , 0.5f ,
            0.5f , 0.5f ,
            -0.5f , -0.5f  ,
            0.5f , -0.5f
        };
        
        float textur = 1.0f / (float) SPRITE_COUNT_X;
        GLfloat texCoords[] = {
            u, v+textur,
            u+textur, v,
            u, v,
            u+textur, v,
            u, v+textur,
            u+textur, v+textur
        };
        
        position(program);
    
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

// The solution path is essential to this entire game's production

/*
    The solution path is an entire room wide/tall, orientation determined by random. It starts in the top row (a random start room) and randomly branches out until it hits the bottom row (exit room). If it hits a side that isnt on the bottom it tries immediatly to drop down. Once the solution path is determined, the game will build rooms that match the path. (example, if the path is a horizontal in the first room then the game will build a path that has exists for sure on the left and right.) We could make the solution path an object that stores the potential rooms and the necessary exits as it traverses through the level. 
    
        0: no definite exit, random entrance, fills up empty rooms not touched by solution path
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

class MapCoord{
public:
    MapCoord(int x, int y)
    :x(x), y(y) {}
    int x;
    int y;
};

// This will progress the solution path to a neihbor block
// pass in the current room, and return the next room.
MapCoord progressPath(MapCoord currentRoom){
    // Can go left(1) or right(2) or down(3)
    int dir = rand()%600;
    if (dir >= 500){
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
    if(dir >= 200){
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


std::vector<std::vector<RoomTrail>> solutionPath(){
    // pass into map generator
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
    MapCoord currentRoom = MapCoord(0, 0);
    
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
                int startPlace = rand()% 400;
                if (startPlace > 100){
                    if (startPlace > 200){
                        if (startPlace > 300){
                                startPlace = 3;
                        }
                        else
                            startPlace = 2;
                    }
                    else
                        startPlace =1;
                }
                else
                    startPlace = 0;
                mapPath[startPlace][0] = start;
                currentRoom = MapCoord(startPlace, 0);
            }
            else{
                // Now progress the room and store the journey marks
                currentRoom = progressPath(currentRoom);
                mapPath[currentRoom.x][currentRoom.y] = journey;
                
                // Make sure you didn't reach the end
                if ((currentRoom.y+1)>3){
                    makingPath = false;
                    mapPath[currentRoom.x][currentRoom.y] = end;
                }
            }
    }
    return mapPath;
    
}

/*
    How will room templates work?
    A level will be comprised of 16 rooms (4x4). Each room will be 8x8, making a level a 32x32 monster.
    Rooms will be created after running the solution path creator.
    Have 3 sets of rooms: A starting room, journey rooms, end room, and null rooms.
*/

/*
    Creates 8x8 grid describing how rooms will be layed out
    Room 0: Null Room
    Room 1: Start Room
    Rooms 3: Journey Rooms
        3: left right top bottom
    Room 5: End room
 
*/
std::vector<std::vector<int>> RoomTemplate(RoomTrail temp){
    std::vector<std::vector<int>> grid;
    
    if(temp == off){
        grid = {
            {1,1,1,6,6,1,1,1},
            {1,6,6,3,6,6,5,1},
            {1,6,6,3,6,3,5,1},
            {1,6,3,3,1,3,10,6},
            {1,6,3,4,1,3,10,6},
            {1,6,3,3,3,3,1,1},
            {1,6,3,3,3,1,5,1},
            {1,1,1,6,6,1,1,1}
        };
    }
    else if(temp ==start){
        grid = {
            {6,1,1,3,3,1,1,1},
            {6,3,3,3,3,3,0,1},
            {6,8,1,3,3,1,3,1},
            {6,3,3,3,1,3,3,3},
            {6,3,3,1,3,8,3,3},
            {6,3,3,3,3,3,3,1},
            {6,3,3,3,3,1,3,1},
            {6,1,1,3,3,1,1,6}
        };
    }
    else if (temp ==end){
        grid = {
            {6,1,1,3,3,1,1,6},
            {1,3,3,3,3,3,6,5},
            {1,3,1,3,1,3,1,5},
            {3,3,1,8,5,3,3,3},
            {3,3,1,4,1,3,3,3},
            {1,8,5,3,6,3,1,5},
            {1,3,3,3,3,9,1,5},
            {6,1,1,3,3,1,1,6}
        };
    }
    else if (temp ==journey){
        grid = {
            {6,1,1,3,3,1,1,6},
            {1,3,3,3,3,3,6,5},
            {1,3,1,3,1,3,1,5},
            {3,3,1,8,5,3,3,3},
            {3,3,1,4,1,3,3,3},
            {1,8,5,3,6,3,1,5},
            {1,3,3,3,3,3,1,5},
            {6,1,1,3,3,1,1,6}
        };
    }
    return grid;
}

 void update(GameState& state, ShaderProgram* program, GLuint fontTexture){
        Matrix words;
    }

class Map{
    // Now we need to make the map from the solution path
public:
    Map(GLuint textureID):textureID(textureID){
        path = solutionPath();
        createMap();
    }
    std::vector<std::vector<RoomTrail>> path;
    std::vector<std::vector<int>> grid;
    Matrix projectionMatrix;
    GLuint textureID;
    // for testing purpose
    float x = 0;
    float y = 0;
    
    void resetGrid(){
        grid.clear();
        for(int xAxis = 0; xAxis < LEVEL_WIDTH; xAxis++){
            std::vector<int> dummy;
            for(int yAxis = 0; yAxis < LEVEL_HEIGHT; yAxis++){
                dummy.push_back(0);
            }
            grid.push_back(dummy);
        }
    }
    
    int tileToFill(int templateTile){
         int dir = rand()%1000;
         if(templateTile==5){
             if(dir >= 500)
                templateTile = 1;
            else
                templateTile = 2;
        }
        else if(templateTile==6){
             if(dir >= 500)
                templateTile = 1;
            else
                templateTile = 3;
        }
        else if(templateTile==7){
             if(dir >= 500)
                templateTile = 3;
            else // For now, no enemies
                templateTile = 3;
        }
        else if(templateTile==10){
             if(dir >= 500)
                templateTile = 3;
            else
                templateTile = 8;
        }
        return templateTile;
    }
    
    void createMap(){
        // Go through the path, add room templates as you progress
        // Start with the solution path, keep a current template
        // Horizontal
        resetGrid();
        std::vector<std::vector<int>> templateRoom;
        for(int pathX = 0; pathX < 4; pathX++){
            // Vertical, reset when you hit the end, up till the last one
            for(int pathY = 0; pathY < 4; pathY++){
               templateRoom = RoomTemplate(path[pathX][pathY]);
               for(int gridX = (0 + pathX*8); gridX < (8 + 8*pathX); gridX++){
                    for (int gridY = (0 + pathY*8); gridY < (8 + 8*pathY); gridY++){
                        // don't forget some tiles are RNG based, handle the RNG here
                        int tile = tileToFill(templateRoom[gridX - (8*pathX)][gridY - (pathY*8)]);
                        grid[gridX][gridY] = tile;
                    }
               }
            }
        }
        
    }
    
    int positionInSheet(int gridData){
        if(gridData == 0)
            gridData = 497;
        else if (gridData==1)
            gridData = 123;
        else if (gridData==2)
            gridData = 70;
        else if (gridData == 3)
            gridData = 749;
        else if (gridData ==4)
            gridData = 749; // for now only
        else if (gridData ==8)
            gridData = 78;
        else if (gridData == 9)
            gridData = 280;
        
        return gridData;
    }
    void drawTiles(ShaderProgram *program, Entity& player){
        std::vector<float> tileVerts;
        std::vector<float> tileTexts;
        Matrix model;
    
        projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
        glUseProgram(program->programID);
        program->setProjectionMatrix(projectionMatrix);
        // don't forget you need to bind the texture
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        for (int y=0; y < LEVEL_HEIGHT; y++){
            for (int x=0; x < LEVEL_WIDTH;  x++){
                int positionInSpriteSheet = positionInSheet(grid[x][y]);
                
                // replace levelData call with it's position in spritesheet
                float u = (float)(((int) positionInSpriteSheet) % SPRITE_COUNT_X) / (float) SPRITE_COUNT_X;
                float v = (float)(((int) positionInSpriteSheet) / SPRITE_COUNT_X) / (float) SPRITE_COUNT_Y;
                
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
        
        // player.position(program);
        player.draw(program);
        program->setModelMatrix(model);
        
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
        
    
        SDL_GL_SwapWindow(displayWindow);
    }
};

// Sets the program up
void setup()
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 4096 );
    #ifdef _WINDOWS
        glewInit();
    #endif
}

// processes the input from out program
void processEvents(SDL_Event &event, bool &done, float &timePerFrame, Entity& player, GameState& state)
{
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            done = true;
        }
    }
    if (state == game){
        if (keys[SDL_SCANCODE_UP]){
            player.y+=0.1;
        }
        if (keys[SDL_SCANCODE_DOWN]){
            player.y-=0.1;
        }
        if (keys[SDL_SCANCODE_LEFT]){
            player.x-=0.1;
        }
        if (keys[SDL_SCANCODE_RIGHT]){
            player.x+=0.1;
        }
        if (keys[SDL_SCANCODE_Q]){
            state = menu;
        }
    }
    else
    if (state == menu){
        if (keys[SDL_SCANCODE_P]){
            state = game;
        }
        if (keys[SDL_SCANCODE_B]){
            done = true;
        }
    }
    
}

void render(GameState& state, Map& gameGrid, Entity& player, ShaderProgram* program, GLuint fontTexture){
        Matrix words;
    
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
        Matrix projectionMatrix;
        Matrix viewMatrix;
        projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
        
        glUseProgram(program->programID);
        program->setViewMatrix(viewMatrix);
        program->setProjectionMatrix(projectionMatrix);
    
        if (state == game){
            gameGrid.drawTiles(program, player);
        }
        else if (state == menu){
            DrawText(program, fontTexture, "Welcome to the Maze", 0.3f, 0.001f, -3.25f, 1.0f, words);
            DrawText(program, fontTexture, "To Continue Press P", 0.3f, 0.001f, -3.25f, 0.5f, words);
            DrawText(program, fontTexture, "To Quit Press B", 0.3f, 0.001f, -3.25f, 0.0f, words);
        }

        glDisable(GL_BLEND);
        SDL_GL_SwapWindow(displayWindow);
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
    GameState currentState = menu;
    float fixedElapsed = 0.0f;
    float ticks;
    float elapsed = 0.0f;
    GLuint font_texture;
    GLuint game_texture;
    game_texture = LoadTexture("spritesheet_rgba.png");
    font_texture = LoadTexture("font1.png");
    Map gameGrid = Map(game_texture);
    Entity player = Entity(game_texture, 19, gameGrid.grid);
    Mix_Music *music;
    music = Mix_LoadMUS("steampunkModified.mp3");
    Mix_PlayMusic(music, -1);
    // Grand Finale!
    while (!done){
        ticks = (float)SDL_GetTicks()/1000.0f;
        fixedElapsed = ticks - elapsed;
        elapsed = ticks;
        // Game states as objects or enums?
        // update if the game's running
        update(currentState, &program, font_texture);
            if(fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
                fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
            }
            while (fixedElapsed >= FIXED_TIMESTEP ) {
                fixedElapsed -= FIXED_TIMESTEP;
                // Run proper updates
                update(currentState, &program, font_texture);
            }
                // run update once again
                update(currentState, &program, font_texture);
                // Run render after updates
                processEvents(event, done, fixedElapsed, player, currentState);
                render(currentState, gameGrid, player, &program, font_texture);
        
    }

    cleanUp(&program);
    Mix_FreeMusic(music);
    return 0;
}











