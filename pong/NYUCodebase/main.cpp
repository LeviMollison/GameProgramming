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

float radianConverter(float degree){
    return (degree * (3.1415926 / 180.0));
}

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

// Let's try using Entities
class Entity{
public:
    // The Constructor
    Entity(Matrix model, float dir_x, float dir_y, float spe, char* tex_id, float xCord, float yCord, float wid, float hei)
    : modelMatrix(model), direction_x(dir_x), direction_y(dir_y), speed(spe), path(tex_id), x(xCord), y(yCord), width(wid), height(hei)
     {
        textureID = LoadTexture(this->path);
        vertices = {-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f,  -0.5f, -0.5f, 0.5f, -0.5f};
        textureCoords = {0.0, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0};
        }
    // This will help with positioning the objects
    Matrix modelMatrix;
    float direction_x;
    float direction_y;
    float speed;
    
    // for loading the textureID
    char * path;
    GLuint textureID;
    
    // for translation
    float x, y;
    // for scaling
    float width, height;
    // for drawing
    std::vector<float> vertices;
    std::vector<float> textureCoords;
    
    void draw(ShaderProgram &program){
    
        this->modelMatrix.identity();
        this->modelMatrix.Translate(x, y, 0.0f);
        this->modelMatrix.Scale(width, height, 1.0f);
        
        program.setModelMatrix(modelMatrix);
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, this->vertices.data());
        glEnableVertexAttribArray(program.positionAttribute);
        
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, this->textureCoords.data());
        glEnableVertexAttribArray(program.texCoordAttribute);
        
        glBindTexture(GL_TEXTURE_2D, this->textureID);
        glDrawArrays(GL_TRIANGLES, 0, ((int)this->vertices.size() / 2));
    }
};

Matrix modelMatrix;



// Set's up what our program will be using
void setup(){
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
}

void DrawText(ShaderProgram *program, Matrix &modelMatrix, int fontTexture, std::string text, float size, float spacing) {
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
    modelMatrix.identity();
    modelMatrix.Translate(-2.5f, 1.5f, 0.0f);
    program->setModelMatrix(modelMatrix);
    
    glUseProgram(program->programID);
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program->positionAttribute);
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(program->texCoordAttribute);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    
    glDrawArrays(GL_TRIANGLES, 0, (float)(text.size() * 6));
}

// cleans up anything the program was using
void cleanUp(ShaderProgram &program)
{
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
    SDL_Quit();
}

void moveBall(Entity &ball, float &elapsed,float &angle){
    // vector math
    ball.y = sinf(ball.direction_y)*elapsed*ball.speed + ball.y;
    ball.x = cosf(ball.direction_x)*elapsed*ball.speed + ball.x;
}

// updates the objects in our program based on input
void update(std::string &textToDraw, float &lastFrameTicks, float &elapsed, float &angle, Entity &ball, Entity &paddle, Entity &paddle2)
{
    float ticks = (float)SDL_GetTicks()/1000.0f;
    elapsed = ticks - lastFrameTicks;
    lastFrameTicks = ticks;
    angle+=elapsed;
    moveBall(ball, elapsed, angle);
    if (ball.x >= 3.0f){
        textToDraw = "Left Paddle Won!";
    }
    if (ball.x <= -3.0f){
        textToDraw = "Right Paddle Won!";
    }
        
    if (ball.x >= 3.0f || ball.x <= -3.0)
        {
            ball.x = 0.5;
            ball.y = 0.0;
            float dir = (float)(rand() % 10 + 1);
            if (dir >=5.0){
                ball.direction_x = 180;
                ball.direction_y = 45.0;
            }
            else{
                ball.direction_x = 45.0;
                ball.direction_y = 45.0;
            }
            
        }
    if (ball.y >= 1.8f || ball.y <= -1.8)
        {
            ball.direction_y += 180;
        }
    // NOW check for collision with the paddle
    /*
        rec.x - rec.width/2 = left
        rec.x = right
        rec.y - rec.height/2 = top
        rec.y = bot
    */
    // Colliding with paddle 1
    float top, bot, left, right;
    float ptop, pbot, pleft, pright;
    top = ball.y + ball.height / 2.0;
    bot = ball.y - ball.height/2.0;
    left = ball.x - ball.width / 2.0;
    right = ball.x + ball.width/2.0;
    
    ptop = paddle.y + paddle.height/2.0;
    pbot = paddle.y - paddle.height/2.0;
    pleft = paddle.x - paddle.width / 2.0;
    pright = paddle.x + paddle.width/2.0;
   // printf("ball's right x: %f, paddle left's x %f \n", right, pleft);
    if(pbot > top || ptop < bot || pleft > right || pright < left){
        
    }
    else{
        ball.direction_x+=180;
    }
    
    ptop = paddle2.y + paddle2.height/2.0;
    pbot = paddle2.y - paddle2.height/2.0;
    pleft = paddle2.x - paddle2.width / 2.0;
    pright = paddle2.x + paddle2.width/2.0;
    if(pbot > top || ptop < bot || pleft > right || pright < left){
        
    }
    else{
        ball.direction_x+=180;
    }
    
}

// draws declared objects onto the display screen
void render(ShaderProgram &program, std::string &textToDraw, Matrix &modelMatrix, Entity &paddle, Entity &paddle2, Entity &ball, Matrix &viewMatrix, Matrix &projectionMatrix)
{
    glClear(GL_COLOR_BUFFER_BIT);
    
    projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
    glUseProgram(program.programID);
    program.setProjectionMatrix(projectionMatrix);
    program.setViewMatrix(viewMatrix);
    
    paddle.draw(program);
    paddle2.draw(program);
    ball.draw(program);
    
    if (textToDraw != ""){
        DrawText(&program, modelMatrix, LoadTexture("font1.png"), textToDraw, 0.3f, 0.05f);
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    SDL_GL_SwapWindow(displayWindow);
}

// processes the input from out program
void processEvents(SDL_Event &event, bool &done, float &elapsed, Entity &paddle, Entity &paddle2, Entity &ball)
{
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_UP] && paddle.y < 1.8){
            paddle.y = 1*elapsed*paddle.speed + paddle.y;
        }
        if (keys[SDL_SCANCODE_DOWN] && paddle.y > -1.8){
            paddle.y = -1*elapsed*paddle.speed + paddle.y;
        }
        if (keys[SDL_SCANCODE_W] && paddle2.y < 1.8) {
            paddle2.y = 1*elapsed*paddle2.speed + paddle2.y;
        }
        if (keys[SDL_SCANCODE_S] && paddle2.y > -1.8) {
            paddle2.y = -1*elapsed*paddle2.speed + paddle2.y;
        }
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            done = true;
        }
    }
}

int main(int argc, char *argv[])
{
    setup();
    ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    Matrix projectionMatrix;
    Matrix viewMatrix;
    Matrix modelMatrix;
    Entity paddle = Entity(modelMatrix, 1.0f, 1.0f, 4.0f, "white.jpg", 3.0f, 0.5f, 0.3f, 1.0f);
    
    Entity paddle2 = Entity(modelMatrix, 1.0f, 1.0f, 4.0f, "white.jpg", -3.0f, 0.5f, 0.3f, 1.0f);
    
    Entity ball = Entity(modelMatrix, 45.0, 45.0, 2.0f, "ball.png", 0.0f, 0.5f, 0.2f, 0.2f);
    
    // Variables can, for now, be global
    SDL_Event event;
    bool done = false;

    // Keeping track of frames. time get's updated in update
    float lastFrameTicks = 0.0f;
    float elapsed = 0.0f;
    float angle = 0.0f;
    std::string textToDraw = "";

    while (!done) {
        processEvents(event, done, elapsed, paddle, paddle2, ball);
        update(textToDraw, lastFrameTicks, elapsed, angle, ball, paddle, paddle2);
        render(program, textToDraw, modelMatrix, paddle, paddle2, ball, viewMatrix, projectionMatrix);
        
    }
    
    cleanUp(program);
    return 0;
}
