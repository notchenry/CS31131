#ifdef _WINDOWS
#include <cstdlib>
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>

#include <vector>
#include "Matrix.h"
#include "ShaderProgram.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_OVER};

GLuint LoadTexture(const char* filepath){
    int w,h,comp;
    unsigned char* image = stbi_load(filepath, &w, &h, &comp, STBI_rgb_alpha);
    
    if(image == NULL){
        std::cout << "Unable to load image. Make sure the path is corret\n";
        assert(false);
    }
    
    GLuint retTexture;
    glGenTextures(1, &retTexture);
    glBindTexture(GL_TEXTURE_2D, retTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    stbi_image_free(image);
    return retTexture;
}

class SheetSprite{
public:
    SheetSprite();
    SheetSprite(unsigned int textureID, float u, float v, float width, float height, float
                size):textureID(textureID), u(u), v(v), width(width), height(height), size(size){ }
    
    unsigned int textureID;
    float u;
    float v;
    float width;
    float height;
    float size;
    
    void Draw(ShaderProgram *program) const {
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        GLfloat texCoords[] = {
            u, v+height,
            u+width, v,
            u, v,
            u+width, v,
            u, v+height,
            u+width, v+height
        };
        
        
        float aspect = width / height;
        float vertices[] = {
            -0.5f * size * aspect, -0.5f * size,
            0.5f * size * aspect, 0.5f * size,
            -0.5f * size * aspect, 0.5f * size,
            0.5f * size * aspect, 0.5f * size,
            -0.5f * size * aspect, -0.5f * size ,
            0.5f * size * aspect, -0.5f * size};
        
        glUseProgram(program->programID);
        
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program->positionAttribute);
        glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program->texCoordAttribute);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glDisableVertexAttribArray(program->positionAttribute);
        glDisableVertexAttribArray(program->texCoordAttribute);
    }
    
};

void DrawText(ShaderProgram *program, GLuint fontTexture, std::string text, float size, float spacing) {
    
    float texture_size = 1.0/16.0f;
    
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    
    for(int i=0; i < text.size(); i++) {
        
        int spriteIndex = (int)text[i];
        
        float texture_x = (float)(spriteIndex % 16) / 16.0f;
        float texture_y = (float)(spriteIndex / 16) / 16.0f;
        
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
        });
        
    }
    
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    
    glUseProgram(program->programID);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program->positionAttribute);
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(program->texCoordAttribute);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    
    glDrawArrays(GL_TRIANGLES, 0, text.size()*6.0);
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
    
}
class Vector3{
public:
    Vector3(float x, float y, float z):x(x), y(y), z(z){};
    
    float x;
    float y;
    float z;
};

int bullet_count = 0;

class Entity{
public:
    
    Entity(float positionX, float positionY, float sizeX, float sizeY):position(positionX, positionY, 0.0f), size(sizeX, sizeY,0.0f){};
    
    void  displayText(ShaderProgram* program, GLuint fontTexture, const std::string& text){
        Matrix projectionMatrix;
        Matrix modelMatrix;
        projectionMatrix.SetOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);
        modelMatrix.Translate(position.x, position.y, position.z);
        Matrix viewMatrix;
        
        program->SetProjectionMatrix(projectionMatrix);
        program->SetModelMatrix(modelMatrix);
        program->SetViewMatrix(viewMatrix);
        
        DrawText(program, fontTexture, text, size.x, -size.x/5);
    }
    
    void Draw(ShaderProgram* program, const SheetSprite& sprite) const {
        Matrix projectionMatrix;
        projectionMatrix.SetOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);
        Matrix modelMatrix;
        modelMatrix.Translate(position.x, position.y, position.z);
        modelMatrix.Scale(size.x, size.y, size.z);
        Matrix viewMatrix;
        
        glUseProgram(program->programID);
        
        program->SetProjectionMatrix(projectionMatrix);
        program->SetModelMatrix(modelMatrix);
        program->SetViewMatrix(viewMatrix);
        
        sprite.Draw(program);
    }
    
    
    void shootBullets(std::vector<Entity>& bullets){
        bullets[bullet_count].position.x = position.x + position.y ;
        bullets[bullet_count].position.y = position.y * 0.4f;
        bullets[bullet_count].speed = -1.5f;
        bullet_count++;
    }
    
    Vector3 position;
    Vector3 size;
    float speed;
};

void RenderGame(ShaderProgram* program, const std::vector<Entity>& invaders, const SheetSprite& invaderSprite, const Entity& player, const SheetSprite& playerSprite, const std::vector<Entity>& bullets, const SheetSprite& bulletSprite ){
    
    for(GLsizei i=0; i<invaders.size(); i++){
        invaders[i].Draw(program, invaderSprite) ;
    }
    
    for(GLsizei i=0; i<bullets.size(); i++){
        bullets[i].Draw(program, bulletSprite);
    }
    
    player.Draw(program, playerSprite);
    
}

float ticks = 0.0f;

void UpdateGame(std::vector<Entity>& invaders, Entity& player, std::vector<Entity>& bullets, float elapsed, GameMode& gameMode){
    
    ticks += elapsed;
    
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    
    if(keys[SDL_SCANCODE_LEFT]){
        player.position.x -= elapsed * 1.5f;
        if(player.position.x <= -3.55+player.size.x*0.5*0.2){
            player.position.x = -3.55+player.size.x*0.5*0.2;
        }
        
    } else if(keys[SDL_SCANCODE_RIGHT]){
        player.position.x += elapsed * 1.5f;
        if(player.position.x >= 3.55-player.size.x*0.5*0.2){
            player.position.x = 3.55-player.size.x*0.5*0.2;
        }
    }
    
    for (GLsizei i=0; i<bullets.size(); i++){
        bullets[i].position.y += bullets[i].speed * elapsed;
    }
    
    if(ticks >= 2.0f){
        for(GLsizei i=0; i<invaders.size(); i++){
            if(i%5 == 0){
                invaders[i].shootBullets(bullets);
            }
        }
        ticks-=2.0f;
    }
    
    
    for(GLsizei i=0; i<bullets.size(); i++){
        
        Entity bullet = bullets[i];
        for(GLsizei j=0; j<invaders.size(); j++){
            
            Entity invader = invaders[j];
            if(bullet.position.x+bullet.size.x*0.2*0.5 < invader.position.x-invader.size.x*0.2*0.5 || bullet.position.x-bullet.size.x*0.2*0.5 > invader.position.x+invader.size.x*0.2*0.5 || bullet.position.y+bullet.size.y*0.2*0.5 < invader.position.y-invader.size.y*0.2*0.5 || bullet.position.y-bullet.size.y*0.2*0.5 > invader.position.y+invader.size.y*0.2*0.5){
            }else{
                if(bullets[i].speed >= 0){
                    bullets[i].position.x = -200.0f;
                    invaders[j].position.x = -200.0f;
                }
            }
            
            if(bullet.position.x+bullet.size.x*0.2*0.5 < player.position.x-player.size.x*0.2*0.5 || bullet.position.x-bullet.size.x*0.2*0.5 > player.position.x+player.size.x*0.2*0.5 || bullet.position.y+bullet.size.y*0.2*0.5 < player.position.y-player.size.y*0.2*0.5 || bullet.position.y-bullet.size.y*0.2*0.5 > player.position.y+player.size.y*0.2*0.5){
            }else{
                if(bullets[i].speed <= 0){
                    gameMode = STATE_GAME_OVER;
                }
            }
            
        }
        
    }
    
}

void renderMenu(ShaderProgram* program, ShaderProgram* program_untextured, Entity& font, GLuint fontTexture){
    font. displayText(program, fontTexture, "CLICK TO PLAY ");
}

void renderLose(ShaderProgram* progam, Entity& loserFont, GLuint fontTexture){
    loserFont. displayText(progam, fontTexture, "YOU LOSE");
}

void setup(ShaderProgram* program, ShaderProgram* program_untextured){
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(0, 0, 1280, 720);
    glClearColor(0.1f, 0.1f, 0.7f, 1.0f);
    
    program->Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    program_untextured->Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    LoadTexture(RESOURCE_FOLDER"darkPurple.png");
}

void ProcessGameInput(SDL_Event* event, bool& gameOver, std::vector<Entity>& bullets, const Entity& player){
    while (SDL_PollEvent(event)){
        if(event->type == SDL_QUIT || event->type == SDL_WINDOWEVENT_CLOSE){
            gameOver = true;
        }else if (event->type == SDL_KEYDOWN){
            if(event->key.keysym.scancode == SDL_SCANCODE_SPACE){
                bullets[bullet_count].position.x = player.position.x;
                bullets[bullet_count].position.y = player.position.y + player.size.y*0.5*0.2;
                bullets[bullet_count].speed = 5.0f;
                bullet_count+=2;
            }
        }
    }
}


void processMenuInputInput(SDL_Event* event, bool& gameOver, GameMode& gameMode){
    std::cout << "PLEASE LEFT CLICK TO START THE GAME" << std::endl;
    while (SDL_PollEvent(event)) {
        if (event->type == SDL_QUIT || event->type == SDL_WINDOWEVENT_CLOSE) {
            gameOver = true;
        }else if(event->type == SDL_MOUSEBUTTONDOWN){
            if(event->button.button == 1){
                gameMode = STATE_GAME_LEVEL;}
        }
    }
}
void processLoseInput(SDL_Event* event, bool& gameOver){
    while(SDL_PollEvent(event)){
        if(event->type == SDL_QUIT || event->type == SDL_WINDOWEVENT_CLOSE){
            gameOver = true;
        }
    }
}

void Quit(){
    SDL_Quit();
}
int main(int argc, char *argv[])
{
    ShaderProgram program;
    ShaderProgram program_untextured;
    setup(&program, &program_untextured);
    
    GLuint fontTexture = LoadTexture(RESOURCE_FOLDER"fonttexture.png");
    GLuint spriteSheetTexture = LoadTexture(RESOURCE_FOLDER"sheettexture.png");
    
    SheetSprite invaderSprite = SheetSprite(spriteSheetTexture, 222.0f/1024.0f, 0.0f/1024.0f, 103.0f/1024.0f, 84.0f/1024.0f, 0.2);
    SheetSprite playerSprite = SheetSprite(spriteSheetTexture, 336.0f/1024.0f, 309.0f/1024.0f, 98.0f/1024.0f, 75.0f/1024.0f, 0.2);
    SheetSprite bulletSprite = SheetSprite(spriteSheetTexture, 848.0f/1024.0f, 684.0f/1024.0f, 13.0f/1024.0f, 54.0f/1024.0f, 0.2);
    
    
    SDL_Event event;
    bool gameOver = false;
    
    GameMode mode = STATE_MAIN_MENU;
    Entity font(-1.08f, 0.0f, 0.3f, 0.3f);
    Entity loserFont(-1.08f, 0.0f, 0.3f, 0.3f);
    Entity player(01.0f, -1.3f, 1.5f, 1.5f);
    
    std::vector<Entity> invaders;
    
    
    float initialX = -3.55f-0.5*0.2*1.5f;
    float initialY = 1.8f;
    
    for(GLsizei j=0; j<4; j++){
        for(GLsizei i=0; i<11; i++){
            Entity newInvader(initialX, initialY, 1.5f, 1.5f);
            initialX+=0.5*1.3f;
            invaders.push_back(newInvader);
        }
        initialX = -3.55f-0.5*0.25f*1.5f;
        initialY-=0.5f;
    }
    
    
    std::vector<Entity> bullets;
    
    for(GLsizei i=0; i<2000 ; i++){
        Entity newBullets(-2000.0f, 0.0f, 1.5f, 1.5f);
        bullets.push_back(newBullets);
    }
    
    
    float FrameTicks = 0.0f;
    while (!gameOver) {
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - FrameTicks;
        FrameTicks = ticks;
        
        if(mode == STATE_MAIN_MENU){
            processMenuInputInput(&event, gameOver, mode);
        }else if(mode == STATE_GAME_LEVEL){
            ProcessGameInput(&event, gameOver, bullets, player);
        }else if(mode == STATE_GAME_OVER){
            processLoseInput(&event, gameOver);
        }
        glClear(GL_COLOR_BUFFER_BIT);
        
        if(mode == STATE_MAIN_MENU){
            renderMenu(&program, &program_untextured, font, fontTexture);
        }else if(mode == STATE_GAME_LEVEL){
            UpdateGame(invaders, player, bullets, elapsed, mode);
            RenderGame(&program, invaders, invaderSprite, player, playerSprite, bullets, bulletSprite);
        }else if(mode == STATE_GAME_OVER){
            renderLose(&program, loserFont, fontTexture);
        }
        
        SDL_GL_SwapWindow(displayWindow);
    }
    Quit();
    return 0;
}


