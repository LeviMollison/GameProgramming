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
    Entity(SpriteSheet sprite, Matrix matrix, float x, float y, float width, float height, float rotation, float max_vel, bool affectedByPlayer, bool bullet, int direction, Mix_Chunk *shot)
    :sprite(sprite), matrix(matrix), x(x), y(y), width(width), height(height), rotation(rotation), max_vel(max_vel), affectedByPlayer(affectedByPlayer), bullet(bullet), direction(direction){
        alive = true;
        usable = true;
        if (affectedByPlayer == true && !bullet){
            GLuint bullet_texture = LoadTexture(RESOURCE_FOLDER"sheet.png");
            SpriteSheet bullet_sprite = SpriteSheet(bullet_texture, 809.0f/1024.0f, 437.0f/1024.0f, 19.0f/1024.0f, 30.0f/1024.0f, 0.5f);
            Matrix new_matrix;
            for (int i = 0; i < 2; i++){
                Entity newBullet = Entity(bullet_sprite, new_matrix, -5.0f, -5.0f, 0.1, 0.2, 90.0f, 1.0f, true, true, 1.0, shot);
                newBullet.shot = shot;
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
    
    // For shooting the bullet sound
    Mix_Chunk *shot;
    
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

/*
    We'll use objects as our game states
    Game states run where the game currently is. It renders all objects related to that game state, as well as actions
    0 == main menu
    1 == playing the game
*/

class GameState
{
public:
    GameState(int gameState, bool active)
    :gameState(gameState), active(active)
    {}

    std::vector<Entity> stateObjects;
    int gameState;
    bool active;
    // What if you win?
    int amountOfAliveInvaders;
    /* 
        Draws all objects to the screen
        Keep all methods that require elements from the entity in the entity itself
        Because we don't have a game that shifts background, viewmatrix will remain the same
    */
    void render(ShaderProgram *program, GLuint &fontTexture)
    {
        glClear(GL_COLOR_BUFFER_BIT);
        Matrix projectionMatrix;
        Matrix viewMatrix;
        Matrix modelMatrix;
        projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
        
        glUseProgram(program->programID);
        program->setViewMatrix(viewMatrix);
        program->setProjectionMatrix(projectionMatrix);
        
        if (gameState==1 && active){
            for(int i=0; i< stateObjects.size(); i++)
            {
                if (stateObjects[i].alive){
                    stateObjects[i].position(program);
                    stateObjects[i].sprite.draw(program);
                   
                    if (stateObjects[i].affectedByPlayer && !stateObjects[i].bullet){
                        for (int j=0; j<stateObjects[i].bullets.size(); j++) {
                            stateObjects[i].bullets[j].position(program);
                            stateObjects[i].bullets[j].sprite.draw(program);
                        }
                    }
                }
            }
        }
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        
        glDisable(GL_BLEND);
        // Now handle the main menu
        if (gameState == 0){
        /*
            Draw the text: Welcome! then, To get started press p, good luck
        */
            DrawText(program, fontTexture, "Welcome!", 0.3f, 0.05f, -2.0f, 1.0f, modelMatrix);
            DrawText(program, fontTexture, "To Get Started", 0.3f, 0.05f, -2.0f, 0.5f, modelMatrix);
            DrawText(program, fontTexture, "Press P", 0.3f, 0.05f, -2.0f, 0.0f, modelMatrix);
            DrawText(program, fontTexture, "Good Luck", 0.3f, 0.05f, -2.0f, -0.5f, modelMatrix);
        }
        if (gameState == 1 && !active){
            // Draw The Phrase "Game over, play again? (press p)
            if (amountOfAliveInvaders > 0){
                DrawText(program, fontTexture, "Game Over.", 0.3f, 0.05f, -2.0f, 1.0f, modelMatrix);
                DrawText(program, fontTexture, "Play Again?", 0.3f, 0.05f, -2.0f, 0.5f, modelMatrix);
                DrawText(program, fontTexture, "(press p)", 0.3f, 0.05f, -2.0f, 0.0f, modelMatrix);
            }
            else if (amountOfAliveInvaders <= 0){
                DrawText(program, fontTexture, "You Won!", 0.3f, 0.05f, -2.0f, 1.0f, modelMatrix);
                DrawText(program, fontTexture, "Play Again?", 0.3f, 0.05f, -2.0f, 0.5f, modelMatrix);
                DrawText(program, fontTexture, "(press p)", 0.3f, 0.05f, -2.0f, 0.0f, modelMatrix);
            }
        }
        
    
        SDL_GL_SwapWindow(displayWindow);
    }

    /*
        Updates the game based on the players input and collision interaction of bullets and ships
        Don't worry about correct time. This will only run based on correct time
    */
    
    void update(ShaderProgram *program, GLuint &fontTexture, SDL_Event &event, bool &done, float fixedElapsed, GameState &innactiveState, GLuint &game_texture, int &currentState, Mix_Chunk *shot);
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
    Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 4096 );
    #ifdef _WINDOWS
        glewInit();
    #endif
}

/*
    Recreate the objects arrays
*/
void reset(GameState &state, GLuint &gameTexture, Mix_Chunk *shot){
    if (state.gameState == 1){
        state.stateObjects.clear();
        // Create the player and his bullets
        SpriteSheet playerSprite = SpriteSheet(gameTexture, 211.0f/1024.0f, 941.0f/1024.0f, 99.0f/1024.0f, 75.0f/1024.0f, 0.3f);
        Matrix matrix;
        Entity player = Entity(playerSprite, matrix, 0.0f, -1.5f, 0.5f, 0.5f, 90.0f, 5.0f, true, false, 1, shot);
        state.stateObjects.push_back(player);
        
        // Create the 30 invaders
        SpriteSheet invader = SpriteSheet(gameTexture, 423.0f/1024.0f, 728.0f/1024.0f, 93.0f/1024.0f, 84.0f/1024.0f, 0.2f);
        float x_pos = -3.3f;
        float y_pos = 1.8f;
        float current_dir = 1;
        for (int i = 0; i < 30; i++){
            Matrix new_matrix;
            Entity new_invader = Entity(invader, new_matrix, x_pos, y_pos, 0.5f, 0.5f, -90.0f, 1.0f, false, false, current_dir, shot);
            state.stateObjects.push_back(new_invader);
            x_pos+=0.5;
            if (i % 10 == 0){
                x_pos = -3.3;
                y_pos -= 0.5;
                current_dir *= -1;
            }
            
        }
        state.amountOfAliveInvaders = (int)state.stateObjects.size() - 1;
    }
}

// processes the input from out program
void processEvents(SDL_Event &event, bool &done, float &timePerFrame, GameState& state, GameState& innactiveState, int &currentState, GLuint &game_texture, Mix_Chunk *shot)
{
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            done = true;
        }
        // Shoot the bullets. Was happening to fast, so needed to "poll" it down
        if (event.type == SDL_KEYDOWN && state.gameState == 1 && state.active && keys[SDL_SCANCODE_SPACE]){
            for (int i = 0; i<state.stateObjects.size(); i++){
                if (state.stateObjects[i].affectedByPlayer && !state.stateObjects[i].bullet){
                    bool foundEmptyBullet =false;
                    for (int j=0;j<state.stateObjects[i].bullets.size();j++){
                        if (state.stateObjects[i].bullets[j].usable && !foundEmptyBullet){
                            state.stateObjects[i].bullets[j].usable = false;
                            state.stateObjects[i].bullets[j].x = state.stateObjects[i].x;
                            state.stateObjects[i].bullets[j].y = state.stateObjects[i].y+0.5;
                            Mix_PlayChannel(-1, state.stateObjects[i].bullets[j].shot, 0);
                            foundEmptyBullet = true;
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
    // Handle player interaction with the game
    if (state.gameState == 1 && state.active){
        if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]){
            for (int i = 0; i<state.stateObjects.size(); i++) {
                if (state.stateObjects[i].affectedByPlayer && !state.stateObjects[i].bullet) {
                    state.stateObjects[i].direction = 1;
                    state.stateObjects[i].move(timePerFrame);
                    if(state.stateObjects[i].x >= 3.4){
                        state.stateObjects[i].x = 3.4;
                    }
                }
            }
        }
        if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]) {
            for (int i = 0; i<state.stateObjects.size(); i++) {
                if (state.stateObjects[i].affectedByPlayer && !state.stateObjects[i].bullet) {
                    state.stateObjects[i].direction = -1;
                    state.stateObjects[i].move(timePerFrame);
                    if(state.stateObjects[i].x <= -3.4){
                        state.stateObjects[i].x = -3.4;
                    }
                }
            }
        }
    }
    // Handle game over
    if (state.gameState == 1 && !state.active){
         if (keys[SDL_SCANCODE_P]){
            state.active = true;
            currentState = state.gameState;
            // Need to reset the game state
            reset(state, game_texture, shot);
         }
    }
    // Handle player interaction with menu
    if (state.gameState == 0){
        if (keys[SDL_SCANCODE_P]){
            currentState = innactiveState.gameState;
            innactiveState.active = true;
        }
    }
}


inline void GameState::update(ShaderProgram *program, GLuint &fontTexture, SDL_Event &event, bool &done, float fixedElapsed, GameState &innactiveState, GLuint &game_texture, int &currentState, Mix_Chunk *shot){
    processEvents(event, done, fixedElapsed, *this, innactiveState, currentState, game_texture, shot);
    if (gameState == 1 && active){
        for (int i = 0; i < stateObjects.size(); i++) {
            if (!stateObjects[i].affectedByPlayer){
                // Handle movement of space ships
                if (!stateObjects[i].bullet && stateObjects[i].alive){
                    stateObjects[i].move(fixedElapsed);
                    if (stateObjects[i].x >= 3.50 || stateObjects[i].x < -3.50){
                        stateObjects[i].direction *= -1;
                        stateObjects[i].move(fixedElapsed);
                        stateObjects[i].y -=0.3f;
                    }
                }
                else if (!stateObjects[i].bullet && !stateObjects[i].alive){
                    stateObjects[i].x = -10.0f;
                    stateObjects[i].y=-10.0f;
                }
            }
            // Handle bullet colliding with space invader when bullet is shot
            if (stateObjects[i].affectedByPlayer && !stateObjects[i].bullet){
                for (int bulletIdx = 0; bulletIdx < stateObjects[i].bullets.size(); bulletIdx++){
                    if (!stateObjects[i].bullets[bulletIdx].usable){
                        float bulletTop, bulletBot, bulletLeft, bulletRight;
                        float invaderTop, invaderBot, invaderLeft, invaderRight;
                    
                        bulletTop = stateObjects[i].bullets[bulletIdx].y + stateObjects[i].bullets[bulletIdx].height / 2.0f;
                        bulletBot = stateObjects[i].bullets[bulletIdx].y - stateObjects[i].bullets[bulletIdx].height / 2.0f;
                        bulletLeft = stateObjects[i].bullets[bulletIdx].x - stateObjects[i].bullets[bulletIdx].width / 2.0f;
                        bulletRight = stateObjects[i].bullets[bulletIdx].x + stateObjects[i].bullets[bulletIdx].width / 2.0f;
                    
                        for (int invaderIdx = 0; invaderIdx < stateObjects.size(); invaderIdx++){
                           if (!stateObjects[invaderIdx].affectedByPlayer && !stateObjects[invaderIdx].bullet){
                                invaderTop = stateObjects[invaderIdx].y + stateObjects[invaderIdx].height / 2.0f;
                                invaderBot = stateObjects[invaderIdx].y - stateObjects[invaderIdx].height / 2.0f;
                                invaderLeft = stateObjects[invaderIdx].x - stateObjects[invaderIdx].width / 2.0f;
                                invaderRight = stateObjects[invaderIdx].x + stateObjects[invaderIdx].width / 2.0f;
                               
                                // Now finally check collision
                                if (!(bulletBot > invaderTop) && !(bulletTop < invaderBot) && !(bulletLeft > invaderRight) && !(bulletRight < invaderLeft)){
                                    amountOfAliveInvaders--;
                                    stateObjects[i].bullets[bulletIdx].usable = true;
                                    stateObjects[i].bullets[bulletIdx].x = -5;
                                    stateObjects[i].bullets[bulletIdx].y = -5;
                                    stateObjects[invaderIdx].alive = false;
                                }
                            }
                        }
                        // Move the bullet after checking if where it is was colliding
                        if (!stateObjects[i].bullets[bulletIdx].usable){
                            stateObjects[i].bullets[bulletIdx].move(fixedElapsed);
                            if (stateObjects[i].bullets[bulletIdx].y > 2.0){
                                stateObjects[i].bullets[bulletIdx].y = -5;
                                stateObjects[i].bullets[bulletIdx].x = -5;
                                stateObjects[i].bullets[bulletIdx].usable = true;
                            }
                        }
                    }
                }
            }
            /*
                Handle game over state (player collision with space ship)
                First, Check if the player is colliding with any of the alive invaders
                Then, render the game over screen
            */
            if (stateObjects[i].affectedByPlayer && !stateObjects[i].bullet){
                Entity player = stateObjects[i];
                float playerTop, playerBot, playerLeft, playerRight;
                float invaderTop, invaderBot, invaderLeft, invaderRight;
                
                playerTop = player.y + player.height / 2.0f;
                playerBot = player.y - player.height / 2.0f;
                playerLeft = player.x - player.width / 2.0f;
                playerRight = player.x + player.width / 2.0f;
                 for (int invaderIdx = 0; invaderIdx < stateObjects.size(); invaderIdx++){
                    if (!stateObjects[invaderIdx].affectedByPlayer && !stateObjects[invaderIdx].bullet && stateObjects[invaderIdx].alive){
                        invaderTop = stateObjects[invaderIdx].y + stateObjects[invaderIdx].height / 2.0f;
                        invaderBot = stateObjects[invaderIdx].y - stateObjects[invaderIdx].height / 2.0f;
                        invaderLeft = stateObjects[invaderIdx].x - stateObjects[invaderIdx].width / 2.0f;
                        invaderRight = stateObjects[invaderIdx].x + stateObjects[invaderIdx].width / 2.0f;
                        
                         if (!(playerBot > invaderTop) && !(playerTop < invaderBot) && !(playerLeft > invaderRight) && !(playerRight < invaderLeft)){
                                    active = false;
                                    player.alive = false;
                                }
                    }
                    if (amountOfAliveInvaders <=0){
                        active = false;
                    }
                 }
            }
        }
    }
    if (gameState == 1 && !active){
        /* 
            What to do when the game is over?
            Make all the bullets usable again
            Ask the player if they want to play again, and if so tell them to press p
            Once they play again, set everyone to be alive again and to their original positions
        */
        for (int playerIdx = 0; playerIdx < stateObjects.size(); playerIdx++){
            if (stateObjects[playerIdx].affectedByPlayer && !stateObjects[playerIdx].bullet){
                Entity player = stateObjects[playerIdx];
                for (int playerBulletsIdx = 0; playerBulletsIdx<player.bullets.size(); playerBulletsIdx++){
                    player.bullets[playerBulletsIdx].usable = true;
                    player.bullets[playerBulletsIdx].x = -5;
                    player.bullets[playerBulletsIdx].y = -5;
                }
                break;
            }
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
    GLuint font_texture = LoadTexture(RESOURCE_FOLDER"font1.png");
    GLuint game_texture = LoadTexture(RESOURCE_FOLDER"sheet.png");
    GameState mainMenu = GameState(0, true);
    GameState gameItself = GameState(1, false);
    // Testing Music
    Mix_Music *music;
    music = Mix_LoadMUS("steampunkModified.mp3");
    Mix_PlayMusic(music, -1);
    Mix_Chunk *bullet;
    bullet = Mix_LoadWAV("laser_shot.wav");
    
    
    reset(gameItself, game_texture, bullet);
    
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
                mainMenu.update(&program, font_texture, event, done, FIXED_TIMESTEP, gameItself, game_texture, currentState, bullet);
            }
                mainMenu.update(&program, font_texture, event, done, fixedElapsed, gameItself, game_texture, currentState, bullet);
                mainMenu.render(&program, font_texture);
        }
        // update if the game's running
        if (currentState == 1){
            if(fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
                fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
            }
            while (fixedElapsed >= FIXED_TIMESTEP ) {
                fixedElapsed -= FIXED_TIMESTEP;
                gameItself.update(&program, font_texture, event, done, FIXED_TIMESTEP, mainMenu, game_texture, currentState, bullet);
            }
                gameItself.update(&program, font_texture, event, done, fixedElapsed, mainMenu, game_texture, currentState, bullet);
                gameItself.render(&program, font_texture);
        }
    }

    cleanUp(&program);
 
    Mix_FreeMusic(music);
    Mix_FreeChunk(bullet);
    return 0;
}











