#define STB_IMAGE_IMPLEMENTATION
#define GLFW_INCLUDE_NONE
#include "stb_image.h"
#include <chrono>
#include <fstream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <random>
#include <sstream>
#include "shader.hpp"
#include "camera.hpp"

bool readyToFire = false;
int remainingAmmo = 6;
int hydraulicPressure = 0;
bool hydraulicsEnabled = false;
float rotationSpeed = 1.0f;
bool externalView = false;
float M_PI = 3.141592653;
float panoramaOffsetX = -0.3f;
float panoramaOffsetY = 0.0f;
int counter = 0;
float targetX[3];
float targetY[3];
float targetZ[3];
bool targetHit[3];
unsigned int VAO_targets[3];
unsigned int VBO_targets[3];
float zoom = 0.5f;
std::chrono::steady_clock::time_point lastFireTime;
bool nightVisionEnabled = false;
bool flashEffect = false;
float flashStartTime = 0.0f;
float flashDuration = 0.5f;
bool flashLight = false;

unsigned int compileShader(GLenum type, const char* source);
unsigned int createShader(const char* vsSource, const char* fsSource);
static unsigned loadImageToTexture(const char* filePath);

void initializeTargets() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dis(-0.3, 0.3);
    static std::uniform_real_distribution<float> dis2(-2.0, 2.0);
    static std::uniform_real_distribution<float> dis3(-6.0, -1.0);
    for (int i = 0; i < 3; ++i) {
        targetX[i] = std::round(dis2(gen) * 10.0) / 10.0;
        targetY[i] = std::round(dis(gen) * 10.0) / 10.0;
        targetZ[i] = dis3(gen);
        targetHit[i] = false;
        std::cout << "Target " << i + 1 << " coordinates: " << targetX[i] << ", " << targetY[i] << ", " << targetZ[i] << "\n";
    }
}

unsigned int compileShader(GLenum type, const char* source)
{
    std::string content = "";
    std::ifstream file(source);
    std::stringstream ss;
    if (file.is_open())
    {
        ss << file.rdbuf();
        file.close();
        std::cout << "Uspjesno procitao fajl sa putanje \"" << source << "\"!" << std::endl;
    }
    else {
        ss << "";
        std::cout << "Greska pri citanju fajla sa putanje \"" << source << "\"!" << std::endl;
    }
    std::string temp = ss.str();
    const char* sourceCode = temp.c_str();

    int shader = glCreateShader(type);

    int success;
    char infoLog[512];
    glShaderSource(shader, 1, &sourceCode, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        if (type == GL_VERTEX_SHADER)
            printf("VERTEX");
        else if (type == GL_FRAGMENT_SHADER)
            printf("FRAGMENT");
        printf(" sejder ima gresku! Greska: \n");
        printf(infoLog);
    }
    return shader;
}

unsigned int createShader(const char* vsSource, const char* fsSource)
{
    unsigned int program;
    unsigned int vertexShader;
    unsigned int fragmentShader;

    program = glCreateProgram();

    vertexShader = compileShader(GL_VERTEX_SHADER, vsSource);
    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource);


    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);
    glValidateProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(program, 512, NULL, infoLog);
        std::cout << "Objedinjeni sejder ima gresku! Greska: \n";
        std::cout << infoLog << std::endl;
    }

    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}

static unsigned loadImageToTexture(const char* filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;
    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, 0);
    if (ImageData != NULL)
    {
        stbi__vertical_flip(ImageData, TextureWidth, TextureHeight, TextureChannels);

        GLint InternalFormat = -1;
        switch (TextureChannels) {
        case 1: InternalFormat = GL_RED; break;
        case 3: InternalFormat = GL_RGB; break;
        case 4: InternalFormat = GL_RGBA; break;
        default: InternalFormat = GL_RGB; break;
        }

        unsigned int Texture;
        glGenTextures(1, &Texture);
        glBindTexture(GL_TEXTURE_2D, Texture);
        glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, TextureWidth, TextureHeight, 0, InternalFormat, GL_UNSIGNED_BYTE, ImageData);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(ImageData);
        return Texture;
    }
    else
    {
        std::cout << "Textura nije ucitana! Putanja texture: " << filePath << std::endl;
        stbi_image_free(ImageData);
        return 0;
    }
}

void drawLampIndicator() {

    glPushMatrix();
    glTranslatef(-0.9f, 0.8f, 0.0f);

    if (readyToFire) {
        glColor3f(0.0f, 1.0f, 0.0f);
    }
    else {
        glColor3f(1.0f, 0.0f, 0.0f);
    }

    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(0.1f, 0.0f);
    glVertex2f(0.1f, 0.1f);
    glVertex2f(0.0f, 0.1f);
    glEnd();

    glPopMatrix();
}

void drawTanksMunition() {
    glPushMatrix();
    glTranslatef(0.8f, 0.8f, 0.0f);

    const float radius = 0.01f;
    const int segments = 100;

    for (int i = 0; i < remainingAmmo; ++i) {
        glBegin(GL_QUADS);
        glColor3f(0.6f, 0.6f, 0.0f);
        glVertex2f(0.0f, 0.0f);
        glColor3f(1.0f, 1.0f, 0.0f);
        glVertex2f(0.02f, 0.0f);
        glVertex2f(0.02f, 0.1f);
        glColor3f(0.6f, 0.6f, 0.0f);
        glVertex2f(0.0f, 0.1f);
        glEnd();
        glTranslatef(0.01f, 0.1f, 0.0f);
        glBegin(GL_TRIANGLE_FAN);
        for (int i = 0; i <= segments; ++i) {
            float angle = static_cast<float>(i) / static_cast<float>(segments) * M_PI;
            float x = radius * std::cos(angle);
            float y = radius * std::sin(angle) * 2;
            glVertex2f(x, y);
        }
        glEnd();
        glTranslatef(-0.01f, -0.1f, -0.0f);
        glTranslatef(0.03f, 0.0f, 0.0f);
    }

    glPopMatrix();
}

void drawVoltmeter() {
    glPushMatrix();
    glTranslatef(-0.85f, -0.9f, 0.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i <= 360; ++i) {
        float angle = static_cast<float>(i) / static_cast<float>(360) * M_PI;
        float x = 0.1f * std::cos(angle);
        float y = 0.1f * std::sin(angle) * 2;
        glVertex2f(x, y);
    }
    glEnd();
    float arrowX = 0.0f;
    float arrowY = 0.0f;
    glColor3f(0.6f, 0.6f, 0.6f);
    glLineWidth(2);
    for (int i = 0; i <= 10; ++i) {
        float angle = static_cast<float>(i) / static_cast<float>(10) * M_PI;
        float x = 0.1f * std::cos(angle);
        float y = 0.1f * std::sin(angle) * 2;
        glBegin(GL_LINES);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(x, y, 0.0f);
        glEnd();
        if (hydraulicPressure == i) {
            arrowX = x;
            arrowY = y;
        }
    }
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i <= 360; ++i) {
        float angle = static_cast<float>(i) / static_cast<float>(360) * M_PI;
        float x = 0.09f * std::cos(angle);
        float y = 0.09f * std::sin(angle) * 2;
        glVertex2f(x, y);
    }
    glEnd();
    glColor3f(0.6f, 0.6f, 0.6f);
    glLineWidth(5);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i <= 360; ++i) {
        float angle = static_cast<float>(i) / static_cast<float>(360) * M_PI;
        float x = 0.1f * std::cos(angle);
        float y = 0.1f * std::sin(angle) * 2;
        glVertex2f(x, y);
    }
    glEnd();
    glColor3f(0.0f, 0.0f, 1.0f);
    if (hydraulicsEnabled) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> dis(-0.002f, 0.002f);
        arrowX += dis(gen);
        arrowY += dis(gen);
    }
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(arrowX, arrowY, 0.0f);
    glEnd();
    glColor3f(0.6f, 0.6f, 0.6f);
    glTranslatef(0.0f, -0.004f, 0.0f);
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i <= 360; ++i) {
        float angle = static_cast<float>(i) / static_cast<float>(360) * M_PI;
        float x = 0.01f * std::cos(angle);
        float y = 0.01f * std::sin(angle) * 2;
        glVertex2f(x, y);
    }
    glEnd();
    glPopMatrix();
}

void drawAim() {
    glLineWidth(2);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINES);
    glVertex3f(-0.01f, panoramaOffsetY, 0.0f);
    glVertex3f(0.01f, panoramaOffsetY, 0.0f);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(0.0f, -0.02f + panoramaOffsetY, 0.0f);
    glVertex3f(0.0f, 0.02f + panoramaOffsetY, 0.0f);
    glEnd();
}

void update() {
    auto currentTime = std::chrono::steady_clock::now();
    auto timeSinceLastFire = std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastFireTime).count();
    if (timeSinceLastFire >= 7.5) {
        readyToFire = true;
    }
}

void render() {
    glUseProgram(0);
    glOrtho(0, 1920, 0, 1080, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    drawLampIndicator();
    drawTanksMunition();
    drawVoltmeter();
    drawAim();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (externalView)
    {
        if (action == GLFW_PRESS) {
            switch (key) {
            case GLFW_KEY_C:
                std::cout << "Returning to tank's cabin view.\n";
                externalView = false;
                break;
            case GLFW_KEY_ESCAPE:
                std::cout << "Closing simulator.\n";
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;
            }
        }
    }
    else
    {
        if (action == GLFW_PRESS) {
            switch (key) {
            case GLFW_KEY_SPACE:
                update();
                if (readyToFire && remainingAmmo > 0) {
                    float aimX = 0;
                    float aimY = panoramaOffsetY;

                    for (int i = 0; i < 3; ++i) {
                        float targetRadius = 0.2f * (1 + zoom);
                        if (glm::distance(glm::vec2(aimX, aimY), glm::vec2(targetX[i], targetY[i])) < targetRadius) {
                            targetHit[i] = true;
                        }
                    }
                    readyToFire = false;
                    flashEffect = true;
                    flashStartTime = glfwGetTime();
                    remainingAmmo--;
                    lastFireTime = std::chrono::steady_clock::now();
                }
                else {
                    std::cout << "Cannon not ready to fire!\n";
                }
                break;
            case GLFW_KEY_V:
                externalView = true;
                nightVisionEnabled = false;
                if (externalView) {
                    std::cout << "Switching to external view.\n";
                }
                break;
            case GLFW_KEY_ESCAPE:
                std::cout << "Closing simulator.\n";
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;
            case GLFW_KEY_UP:
                if (counter < 3) {
                    counter += 1;
                    panoramaOffsetY += 0.1f;
                }
                break;
            case GLFW_KEY_DOWN:
                if (counter > -3) {
                    counter -= 1;
                    panoramaOffsetY -= 0.1f;
                }
                break;
            case GLFW_KEY_LEFT:
                if (hydraulicsEnabled) {
                    panoramaOffsetX -= 0.03f * hydraulicPressure + 0.05f;
                    for (int i = 0; i < 3; ++i) {
                        targetX[i] -= 0.3f * hydraulicPressure + 0.1f;
                        if (targetX[i] < -9.0f) targetX[i] = 9.0f;
                    }
                }
                else {
                    panoramaOffsetX -= 0.01;
                    for (int i = 0; i < 3; ++i) {
                        targetX[i] -= 0.05f;
                        if (targetX[i] < -9.0f) targetX[i] = 9.0f;
                    }
                }
                break;
            case GLFW_KEY_RIGHT:
                if (hydraulicsEnabled) {
                    panoramaOffsetX += 0.03f * hydraulicPressure + 0.05f;
                    for (int i = 0; i < 3; ++i) {
                        targetX[i] += 0.3f * hydraulicPressure + 0.1f;
                        if (targetX[i] > 9.0f) targetX[i] = -9.0f;
                    }
                }
                else {
                    panoramaOffsetX += 0.01f;
                    for (int i = 0; i < 3; ++i) {
                        targetX[i] += 0.05f;
                        if (targetX[i] > 9.0f) targetX[i] = -9.0f;
                    }
                }
                break;
            case GLFW_KEY_MINUS:
                if (hydraulicPressure > 0 and hydraulicsEnabled) {
                    hydraulicPressure -= 1;
                    if (hydraulicPressure == 0) hydraulicsEnabled = false;
                }
                break;
            case GLFW_KEY_EQUAL:
                if (hydraulicPressure < 10 and hydraulicsEnabled) {
                    hydraulicPressure += 1;
                }
                break;
            case GLFW_KEY_COMMA:
                if (zoom < 0.6f) {
                    zoom += 0.05f;
                }
                break;
            case GLFW_KEY_PERIOD:
                if (zoom > 0.5f) {
                    zoom -= 0.05f;
                }
                break;
            case GLFW_KEY_H:
                hydraulicsEnabled = !hydraulicsEnabled;
                hydraulicPressure = 0.0f;
                break;
            case GLFW_KEY_B:
                nightVisionEnabled = !nightVisionEnabled;
                break;
            case GLFW_KEY_F:
                flashLight = !flashLight;
                break;
            }
        }
    }
}

int main() {

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(glfwGetVideoMode(glfwGetPrimaryMonitor())->width, glfwGetVideoMode(glfwGetPrimaryMonitor())->height, "Tank Simulator", glfwGetPrimaryMonitor(), nullptr);

    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glDepthFunc(GL_LESS);

    if (glewInit() != GLEW_OK)
    {
        std::cout << "Failed to initialize GLEW\n";
        return -1;
    }

    //+++++++++++++++BKG++++++++++++++++
    unsigned int unifiedShader = createShader("basic.vert", "basic.frag");

    float vertices[] = {
        -1.0, -1.0,  0.0, 0.0,  
        -1.0,  1.0,  0.0, 1.0,  
         1.0,  1.0,  1.0, 1.0,  

         1.0,  1.0,  1.0, 1.0,  
         1.0, -1.0,  1.0, 0.0, 
        -1.0, -1.0,  0.0, 0.0, 
    };

    unsigned int stride = (2 + 2) * sizeof(float);

    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    unsigned int checkerTexture = loadImageToTexture("bkg.png");
    unsigned int smoke = loadImageToTexture("smoke.png");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindTexture(GL_TEXTURE_2D, checkerTexture);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    unsigned uTexLoc = glGetUniformLocation(unifiedShader, "uTex");
    glUniform1i(uTexLoc, 0);

    //++++++++++SMOKE+++++++++++++

    unsigned int smokeShader = createShader("smoke.vert", "smoke.frag");

    float verticesS[] = {
        0.8, -0.2,  0.0, 0.0,  
        0.8,  0.2,  0.0, 0.2, 
        1.2,  0.2,  0.2, 0.2, 

        1.2,  0.2,  0.2, 0.2,
        1.2, -0.2,  0.2, 0.0, 
        0.8, -0.2,  0.0, 0.0, 
    };

    unsigned int strideS = (2 + 2) * sizeof(float);

    unsigned int VAOS;
    glGenVertexArrays(1, &VAOS);
    glBindVertexArray(VAOS);
    unsigned int VBOS;
    glGenBuffers(1, &VBOS);
    glBindBuffer(GL_ARRAY_BUFFER, VBOS);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), verticesS, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, strideS, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, strideS, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindTexture(GL_TEXTURE_2D, smoke);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    unsigned smokeLoc = glGetUniformLocation(smokeShader, "uTex");
    glUniform1i(smokeLoc, 0);

    // +++++++++++PANORAMA++++++++++++++++++++

    unsigned int unifiedShader2 = createShader("panorama.vert", "panorama.frag");

    float vertices2[] = {
        -0.42, -0.55,  0.0, 0.0,  
        -0.42,  0.55,  0.0, 0.42, 
         0.42,  0.55,  0.42, 0.42, 

         0.42,  0.55,  0.42, 0.42,
         0.42, -0.55,  0.42, 0.0,
        -0.42, -0.55,  0.0, 0.0, 
    };

    unsigned int stride2 = (2 + 2) * sizeof(float);

    unsigned int VAO2;
    glGenVertexArrays(1, &VAO2);
    glBindVertexArray(VAO2);
    unsigned int VBO2;
    glGenBuffers(1, &VBO2);
    glBindBuffer(GL_ARRAY_BUFFER, VBO2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2), vertices2, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride2, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride2, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    unsigned checkerTexture2 = loadImageToTexture("terrain-night.jpg");
    glBindTexture(GL_TEXTURE_2D, checkerTexture2);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    unsigned uTexLoc2 = glGetUniformLocation(unifiedShader2, "uTex");
    unsigned int nightVisionLoc = glGetUniformLocation(unifiedShader2, "nightVisionEnabled");
    glUniform1i(uTexLoc2, 0);

    //+++++++++++++++++++++++CUBES++++++++++++++++++++++++
    Camera camera(glm::vec3(0.0f, 0.0f, 2.0f));
    glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

    float cube[] =
    {   
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };

    unsigned int diffuseMap = loadImageToTexture("container2.png");
    unsigned int specularMap = loadImageToTexture("container2_specular.png");

    Shader lightingShader("lighting.vert", "lighting.frag");

    unsigned int VAO3;
    glGenVertexArrays(1, &VAO3);
    glBindVertexArray(VAO3);

    unsigned int VBO3;
    glGenBuffers(1, &VBO3);
    glBindBuffer(GL_ARRAY_BUFFER, VBO3);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);

    glBindVertexArray(VAO3);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO3);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    lightingShader.use();
    lightingShader.setInt("material.diffuse", 0);
    lightingShader.setInt("material.specular", 1);

    // +++++++++++++++++++++CANNON+++++++++++++++++++++++++++++++++ 

    float rectanglePrism[] =
    {
        -1.0f, -0.125f, -0.125f,  0.0f,  0.0f, -0.5f,
         1.0f, -0.125f, -0.125f,  0.0f,  0.0f, -0.5f,
         1.0f,  0.125f, -0.125f,  0.0f,  0.0f, -0.5f,
         1.0f,  0.125f, -0.125f,  0.0f,  0.0f, -0.5f,
        -1.0f,  0.125f, -0.125f,  0.0f,  0.0f, -0.5f,
        -1.0f, -0.125f, -0.125f,  0.0f,  0.0f, -0.5f,

        -1.0f, -0.125f,  0.125f,  0.0f,  0.0f, 0.5f,
         1.0f, -0.125f,  0.125f,  0.0f,  0.0f, 0.5f,
         1.0f,  0.125f,  0.125f,  0.0f,  0.0f, 0.5f,
         1.0f,  0.125f,  0.125f,  0.0f,  0.0f, 0.5f,
        -1.0f,  0.125f,  0.125f,  0.0f,  0.0f, 0.5f,
        -1.0f, -0.125f,  0.125f,  0.0f,  0.0f, 0.5f,

        -1.0f,  0.125f,  0.125f, -0.5f,  0.0f,  0.0f,
        -1.0f,  0.125f, -0.125f, -0.5f,  0.0f,  0.0f,
        -1.0f, -0.125f, -0.125f, -0.5f,  0.0f,  0.0f,
        -1.0f, -0.125f, -0.125f, -0.5f,  0.0f,  0.0f,
        -1.0f, -0.125f,  0.125f, -0.5f,  0.0f,  0.0f,
        -1.0f,  0.125f,  0.125f, -0.5f,  0.0f,  0.0f,

         1.0f,  0.125f,  0.125f,  0.5f,  0.0f,  0.0f,
         1.0f,  0.125f, -0.125f,  0.5f,  0.0f,  0.0f,
         1.0f, -0.125f, -0.125f,  0.5f,  0.0f,  0.0f,
         1.0f, -0.125f, -0.125f,  0.5f,  0.0f,  0.0f,
         1.0f, -0.125f,  0.125f,  0.5f,  0.0f,  0.0f,
         1.0f,  0.125f,  0.125f,  0.5f,  0.0f,  0.0f,

        -1.0f, -0.125f, -0.125f,  0.0f, -0.5f,  0.0f,
         1.0f, -0.125f, -0.125f,  0.0f, -0.5f,  0.0f,
         1.0f, -0.125f,   0.125f,  0.0f, -0.5f,  0.0f,
         1.0f, -0.125f,   0.125f,  0.0f, -0.5f,  0.0f,
        -1.0f, -0.125f,   0.125f,  0.0f, -0.5f,  0.0f,
        -1.0f, -0.125f, -0.125f,  0.0f, -0.5f,  0.0f,

        -1.0f,  0.125f, -0.125f,  0.0f,  0.5f,  0.0f,
         1.0f,  0.125f, -0.125f,  0.0f,  0.5f,  0.0f,
         1.0f,  0.125f,   0.125f,  0.0f,  0.5f,  0.0f,
         1.0f,  0.125f,   0.125f,  0.0f,  0.5f,  0.0f,
        -1.0f,  0.125f,   0.125f,  0.0f,  0.5f,  0.0f,
        -1.0f,  0.125f, -0.125f,  0.0f,  0.5f,  0.0f
    };

    unsigned int strideCylinder = (3 + 3) * sizeof(float);

    unsigned int VAOcylinder;
    glGenVertexArrays(1, &VAOcylinder);
    glBindVertexArray(VAOcylinder);

    unsigned int VBOcylinder;
    glGenBuffers(1, &VBOcylinder);
    glBindBuffer(GL_ARRAY_BUFFER, VBOcylinder);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectanglePrism), rectanglePrism, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, strideCylinder, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, strideCylinder, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    unsigned int prismShader = createShader("phong.vert", "phong.frag");

    glm::mat4 modelP = glm::mat4(1.0f);
    unsigned int modelLocP = glGetUniformLocation(prismShader, "uM");

    glm::mat4 viewP = glm::mat4(1.0f);
    viewP = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    unsigned int viewLocP = glGetUniformLocation(prismShader, "uV");

    glm::mat4 projectionPP = glm::perspective(glm::radians(60.0f), (float)1920 / (float)1080, 0.1f, 100.0f);
    unsigned int projectionLocP = glGetUniformLocation(prismShader, "uP");

    unsigned int viewPosLocP = glGetUniformLocation(prismShader, "uViewPos");

    unsigned int lightPosLocP = glGetUniformLocation(prismShader, "uLight.pos");
    unsigned int lightALocP = glGetUniformLocation(prismShader, "uLight.kA");
    unsigned int lightDLocP = glGetUniformLocation(prismShader, "uLight.kD");
    unsigned int lightSLocP = glGetUniformLocation(prismShader, "uLight.kS");

    unsigned int materialShineLocP = glGetUniformLocation(prismShader, "uMaterial.shine");
    unsigned int materialALocP = glGetUniformLocation(prismShader, "uMaterial.kA");
    unsigned int materialDLocP = glGetUniformLocation(prismShader, "uMaterial.kD");
    unsigned int materialSLocP = glGetUniformLocation(prismShader, "uMaterial.kS");
    unsigned int nightVisionLoc2P = glGetUniformLocation(prismShader, "nightVisionEnabled");

    glUseProgram(prismShader);

    glUniformMatrix4fv(modelLocP, 1, GL_FALSE, glm::value_ptr(modelP));
    glUniformMatrix4fv(viewLocP, 1, GL_FALSE, glm::value_ptr(viewP));
    glUniformMatrix4fv(projectionLocP, 1, GL_FALSE, glm::value_ptr(projectionPP));

    glUniform3f(viewPosLocP, 0.0, 0.0, 2.0); 

    glUniform3f(lightPosLocP, 0.0, 0.5, 2.0);

    glUniform3f(lightALocP, 0.5, 0.5, 0.5); 
    glUniform3f(lightDLocP, 0.5, 0.5, 0.5); 
    glUniform3f(lightSLocP, 0.5, 0.5, 0.5); 

    glUniform1f(materialShineLocP, 0.6 * 100);
    glUniform3f(materialALocP, 0.1, 0.1, 0.1);
    glUniform3f(materialDLocP, 0.2, 0.2, 0.2);
    glUniform3f(materialSLocP, 1.0, 1.0, 1.0);

    //++++++++++++++++++++++++++++++++++++++++++++

    glfwSetKeyCallback(window, key_callback);
    initializeTargets();
    glCullFace(GL_BACK);

    while (!glfwWindowShouldClose(window))
    {
        update();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        targetX[1] += 0.001f;

        if (!externalView) {
            glUseProgram(unifiedShader2);
            glBindVertexArray(VAO2);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, checkerTexture2);
            glUniform1i(nightVisionLoc, nightVisionEnabled);

            glUniform1f(glGetUniformLocation(unifiedShader2, "offsetX"), panoramaOffsetX);
            glUniform1f(glGetUniformLocation(unifiedShader2, "offsetY"), 1);
            glUniform1f(glGetUniformLocation(unifiedShader2, "textureScale"), 0.5 - zoom/8);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        else {
            glUseProgram(unifiedShader2);
            glBindVertexArray(VAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, checkerTexture2);
            glUniform1i(nightVisionLoc, nightVisionEnabled);
            glUniform1f(glGetUniformLocation(unifiedShader2, "offsetX"), panoramaOffsetX);
            glUniform1f(glGetUniformLocation(unifiedShader2, "offsetY"), 1);
            glUniform1f(glGetUniformLocation(unifiedShader2, "textureScale"), 0.5f);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glEnable(GL_DEPTH_TEST);
        lightingShader.use();
        lightingShader.setBool("nightVisionEnabled", nightVisionEnabled);
        lightingShader.setVec3("viewPos", camera.Position);
        lightingShader.setFloat("material.shininess", 32.0f);

        lightingShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        lightingShader.setVec3("dirLight.ambient", 0.3f, 0.3f, 0.3f); 
        lightingShader.setVec3("dirLight.diffuse", 0.3f, 0.3f, 0.3f); 
        lightingShader.setVec3("dirLight.specular", 0.3f, 0.3f, 0.3f);

        lightingShader.setVec3("spotLight.position", camera.Position);
        lightingShader.setVec3("spotLight.direction", camera.Front);

        if (!targetHit[1]) {
            lightingShader.setVec3("pointLights[0].position", glm::vec3(targetX[1], targetY[1], targetZ[1]));
            lightingShader.setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
            lightingShader.setVec3("pointLights[0].diffuse", 1.0f, 0.0f, 0.0f); 
            lightingShader.setVec3("pointLights[0].specular", 1.0f, 0.0f, 0.0f); 
            lightingShader.setFloat("pointLights[0].constant", 1.0f);
            lightingShader.setFloat("pointLights[0].linear", 0.09f);
            lightingShader.setFloat("pointLights[0].quadratic", 0.032f);
        }
        else {
            lightingShader.setVec3("pointLights[0].ambient", 0.0f, 0.0f, 0.0f);
            lightingShader.setVec3("pointLights[0].diffuse", 0.0f, 0.0f, 0.0f);
            lightingShader.setVec3("pointLights[0].specular", 0.0f, 0.0f, 0.0f);
        }

        lightingShader.setVec3("pointLights[1].position", glm::vec3(0.0f, panoramaOffsetY, -1.0f));
        lightingShader.setVec3("pointLights[1].ambient", 0.0f, 0.0f, 0.0f);
        lightingShader.setVec3("pointLights[1].diffuse", 0.0f, 0.0f, 0.0f);
        lightingShader.setVec3("pointLights[1].specular", 0.0f, 0.0f, 0.0f);
        lightingShader.setFloat("pointLights[1].constant", 1.0f);
        lightingShader.setFloat("pointLights[1].linear", 0.09f);
        lightingShader.setFloat("pointLights[1].quadratic", 0.032f);
        if (!readyToFire) {
            // ++++++++++++blast+++++++++++++++++++
            auto currentTime = std::chrono::high_resolution_clock::now();
            float elapsedTime = std::chrono::duration<float>(currentTime - lastFireTime).count();
            if (elapsedTime < 1.0) {
                lightingShader.setVec3("pointLights[1].ambient", 0.2f, 0.2f, 0.0f);
                lightingShader.setVec3("pointLights[1].diffuse", 1.0f, 1.0f, 1.0f);
                lightingShader.setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
            }
        }
        // spotLight
        if (flashLight) {
            lightingShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
            lightingShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
            lightingShader.setFloat("spotLight.constant", 1.0f);
            lightingShader.setFloat("spotLight.linear", 0.09f);
            lightingShader.setFloat("spotLight.quadratic", 0.032f);
            lightingShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
            lightingShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));
        }
        else {
            lightingShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
            lightingShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
            lightingShader.setFloat("spotLight.constant", 1.0f);
            lightingShader.setFloat("spotLight.linear", 0.09f);
            lightingShader.setFloat("spotLight.quadratic", 0.032f);
            lightingShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(0.0f)));
            lightingShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(0.0f)));
        }

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)1920 / (float)1080, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        // world transformation
        glm::mat4 model = glm::mat4(1.0f);
        lightingShader.setMat4("model", model);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);

        glBindVertexArray(VAO3);

        for (int i = 0; i < 3; ++i) {
            if (targetHit[i]) continue;
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(targetX[i], targetY[i], targetZ[i]));
            float angle = 20.0f * i;
            model = glm::scale(model, glm::vec3(zoom));
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            lightingShader.setMat4("model", model);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glUseProgram(prismShader);
        glBindVertexArray(VAOcylinder);

        glm::mat4 model_cylinder = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, -0.4f + panoramaOffsetY, 0.0f));
        float angle = glm::radians(90.0f);
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(-3.5f, -1.0f, -2.0f));
        model_cylinder = model_cylinder * rotationMatrix;
        unsigned int modelLoc_cylinder = glGetUniformLocation(prismShader, "uM");
        glUniformMatrix4fv(modelLoc_cylinder, 1, GL_FALSE, glm::value_ptr(model_cylinder));

        glDrawArrays(GL_TRIANGLES, 0, sizeof(rectanglePrism));

        if (!readyToFire) {
            // ++++++++++++smoke+++++++++++++++++++
            auto currentTime = std::chrono::high_resolution_clock::now();
            float elapsedTime = std::chrono::duration<float>(currentTime - lastFireTime).count();
            glUseProgram(smokeShader);
            glUniform1f(glGetUniformLocation(smokeShader, "time"), elapsedTime);
            glUniform1f(glGetUniformLocation(smokeShader, "duration"), 3.0);
            if (elapsedTime < 3.0) {
                glBindVertexArray(VAO);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, smoke);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glBindVertexArray(0);
            }
        }

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        if (!externalView)
        {
            // ++++++++++++BKG+++++++++++++++++++
            glUseProgram(unifiedShader);
            glBindVertexArray(VAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, checkerTexture);

            glDrawArrays(GL_TRIANGLES, 0, 6);
            // +++++++++++++++++++++++++++++++
            render();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteTextures(1, &checkerTexture);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(unifiedShader);
    glDeleteTextures(1, &checkerTexture2);
    glDeleteBuffers(1, &VBO2);
    glDeleteVertexArrays(1, &VAO2);
    glDeleteProgram(unifiedShader2);
    glfwTerminate();
    return 0;
}