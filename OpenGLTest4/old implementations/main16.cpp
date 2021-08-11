//
//  main14.cpp
//  OpenGLTest3
//
//  Created by Nazım Anıl Tepe on 12.12.2020.
//  Copyright © 2020 Nazım Anıl Tepe. All rights reserved.
//
// cube with multiple textures

#include <glew.h>
#include <glfw3.h>

#include <glm.hpp>
using namespace glm;
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <cmath>

float vertices[] = {
    //front
    -100.0f, -100.0f, 100.0f,     -1.0f, -1.0f, 1.0f,   0.0f, 0.0f,     0,
    100.0f, -100.0f, 100.0f,      1.0f, -1.0f, 1.0f,    1.0f, 0.0f,     0,
    100.0f, 100.0f, 100.0f,       1.0f, 1.0f, 1.0f,     1.0f, 1.0f,     0,
    -100.0f, 100.0f, 100.0f,      -1.0f, 1.0f, 1.0f,    0.0f, 1.0f,     0,
    
    //left
    -100.0f, -100.0f, -100.0f,    -1.0f, -1.0f, -1.0f,  0.0f, 0.0f,     1,
    -100.0f, -100.0f, 100.0f,     -1.0f, -1.0f, 1.0f,   1.0f, 0.0f,     1,
    -100.0f, 100.0f, 100.0f,      -1.0f, 1.0f, 1.0f,    1.0f, 1.0f,     1,
    -100.0f, 100.0f, -100.0f,     -1.0f, 1.0f, -1.0f,   0.0f, 1.0f,     1,
    
    //right
    100.0f, -100.0f, 100.0f,      1.0f, -1.0f, 1.0f,    0.0f, 0.0f,     2,
    100.0f, -100.0f, -100.0f,     1.0f, -1.0f, -1.0f,   1.0f, 0.0f,     2,
    100.0f, 100.0f, -100.0f,      1.0f, 1.0f, -1.0f,    1.0f, 1.0f,     2,
    100.0f, 100.0f, 100.0f,       1.0f, 1.0f, 1.0f,     0.0f, 1.0f,     2,

    //back
    100.0f, -100.0f, -100.0f,     1.0f, -1.0f, -1.0f,   0.0f, 0.0f,     3,
    -100.0f, -100.0f, -100.0f,    -1.0f, -1.0f, -1.0f,  1.0f, 0.0f,     3,
    -100.0f, 100.0f, -100.0f,     -1.0f, 1.0f, -1.0f,   1.0f, 1.0f,     3,
    100.0f, 100.0f, -100.0f,      1.0f, 1.0f, -1.0f,    0.0f, 1.0f,     3,
    
    //top
    -100.0f, 100.0f, 100.0f,      -1.0f, 1.0f, 1.0f,    0.0f, 0.0f,     4,
    100.0f, 100.0f, 100.0f,       1.0f, 1.0f, 1.0f,     1.0f, 0.0f,     4,
    100.0f, 100.0f, -100.0f,      1.0f, 1.0f, -1.0f,    1.0f, 1.0f,     4,
    -100.0f, 100.0f, -100.0f,     -1.0f, 1.0f, -1.0f,   0.0f, 1.0f,     4,
    
    //bottom
    -100.0f, -100.0f, -100.0f,    -1.0f, -1.0f, -1.0f,  0.0f, 0.0f,     5,
    100.0f, -100.0f, -100.0f,     1.0f, -1.0f, -1.0f,   1.0f, 0.0f,     5,
    100.0f, -100.0f, 100.0f,      1.0f, -1.0f, 1.0f,    1.0f, 1.0f,     5,
    -100.0f, -100.0f, 100.0f,     -1.0f, -1.0f, 1.0f,   0.0f, 1.0f,     5,
};

unsigned int indices[] = {
    0, 1, 2,
    2, 3, 0,
    4, 5, 6,
    6, 7, 4,
    8, 9, 10,
    10, 11, 8,
    12, 13, 14,
    14, 15, 12,
    16, 17, 18,
    18, 19, 16,
    20, 21, 22,
    22, 23, 20
};

unsigned int polygonMode = 0;

void processInput(GLFWwindow *window);
void resizeFramebuffer(GLFWwindow* window, int width, int height);
void initShader();
vec3 rotateVectorAroundAxis(vec3 vector, vec3 axis, float angle);
void yaw(float angle);
void pitch(float angle);
void roll(float angle);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

unsigned int VBO, VAO, EBO;
unsigned int lightVAO;

float fieldOfView   =  45.0f;

vec3 cameraPos = vec3(0.0f, 0.0f, 500.0f);
vec3 cameraFront = vec3(0.0f, 0.0f, -1.0f);
vec3 cameraRight = vec3(1.0f, 0.0f, 0.0f);
vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);

float moveDistance = 5.0f;

mat4 projection;
mat4 view;
mat4 model;

glm::vec3 lightPos = glm::vec3(0.0f, 500.0f, 500.0f);

int shaderProgram;
const char *vertexShaderSource = "#version 330 core\n"
    "layout(location = 0) in vec3 vPos;\n"
    "layout(location = 1) in vec3 vNormal;\n"
    "layout(location = 2) in vec2 vTexCoord;\n"
    "layout(location = 3) in float vOrder;\n"
    "out vec2 TexCoord;\n"
    "out float Order;\n"
    "out vec3 FragPos;\n"
    "out vec3 Normal;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = projection * view * model * vec4(vPos, 1.0f);\n"
    "   TexCoord = vTexCoord;\n"
    "   Order = vOrder;\n"
    "   FragPos = vec3(model * vec4(vPos, 1.0f));\n"
    "   Normal = vNormal;\n"
    "}\0";
const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 TexCoord;\n"
    "in float Order;\n"
    "in vec3 FragPos;\n"
    "in vec3 Normal;\n"
    "uniform vec3 lightPos;\n"
    "uniform sampler2D textures[6];\n"
    "void main()\n"
    "{\n"
    "   float ambientStrength = 0.1f;\n"
    "   vec3 ambient = ambientStrength * vec3(1.0f);\n"
    "   vec3 norm = normalize(Normal);\n"
    "   vec3 lightDir = normalize(lightPos - FragPos);\n"
    "   float diffuseStrength = max(dot(norm, lightDir), 0.0f);\n"
    "   vec3 diffuse = diffuseStrength * vec3(1.0f);\n"
    "   int order = int(Order);\n"
    "   vec3 ambTex = ambient * vec3(texture(textures[order], TexCoord));\n"
    "   vec3 difTex = diffuse * vec3(texture(textures[order], TexCoord));\n"
    "   vec3 result = ambTex + difTex;\n"
    "   FragColor = vec4(result, 1.0f);\n"
    "}\0";

int lightShaderProgram;
const char *lightVertexShaderSource = "#version 330 core\n"
    "layout(location = 0) in vec3 vPos;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = projection * view * model * vec4(vPos, 1.0f);\n"
    "}\0";
const char *lightFragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f);\n"
    "}\0";


int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGLTest3", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, resizeFramebuffer);
    
    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }
    
    glEnable(GL_DEPTH_TEST);
    
    initShader();
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    stbi_set_flip_vertically_on_load(true);
    unsigned int textures[6];
    std::string path = "/Users/nazimaniltepe/Documents/Projects/OpenGLTest3/OpenGLTest3/";
    for(int i = 0; i < 6; i++) {
        glGenTextures(1, &textures[i]);
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        
        std::string fullpath = path + std::to_string(i + 1) + ".jpg";
        int width, height, nrChannels;
        unsigned char *data = stbi_load(fullpath.c_str(), &width, &height, &nrChannels, 0);
        
        if (data) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        else
            std::cout << "Failed to load texture" << std::endl;
        
        stbi_image_free(data);
        
        glUseProgram(shaderProgram);
        std::string texName = "textures[" + std::to_string(i) + "]";
        glUniform1i(glGetUniformLocation(shaderProgram, texName.c_str()), i);
    }
    
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        processInput(window);
        
        if (polygonMode == 0)
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        else if (polygonMode == 1)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else if (polygonMode == 2) {
            glPointSize(7);
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        }
        
        projection = perspective(radians(fieldOfView), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
        view = lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        model = mat4(1.0f);
        
        glUseProgram(shaderProgram);
        for (int i = 0; i < 6; i++) {
            glActiveTexture(GL_TEXTURE0 + (i * 0x0001));
            glBindTexture(GL_TEXTURE_2D, textures[i]);
        }
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE,  value_ptr(model));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        
        glUseProgram(lightShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(lightShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(lightShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.1f));
        glUniformMatrix4fv(glGetUniformLocation(lightShaderProgram, "model"), 1, GL_FALSE,  glm::value_ptr(model));
        glBindVertexArray(lightVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &lightVAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    
    glfwTerminate();
    return 0;
}


void initShader()
{
    int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    int vertexShader_ = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader_, 1, &lightVertexShaderSource, NULL);
    glCompileShader(vertexShader_);
    int fragmentShader_ = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader_, 1, &lightFragmentShaderSource, NULL);
    glCompileShader(fragmentShader_);
    lightShaderProgram = glCreateProgram();
    glAttachShader(lightShaderProgram, vertexShader_);
    glAttachShader(lightShaderProgram, fragmentShader_);
    glLinkProgram(lightShaderProgram);
    glDeleteShader(vertexShader_);
    glDeleteShader(fragmentShader_);
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    
    
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        cameraPos = vec3(0.0f, 0.0f, 5.0f);
        cameraFront = vec3(0.0f, 0.0f, -1.0f);
        cameraRight = vec3(1.0f, 0.0f, 0.0f);
        cameraUp = vec3(0.0f, 1.0f, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        cameraPos = vec3(-5.0f, 0.0f, 0.0f);
        cameraFront = vec3(1.0f, 0.0f, 0.0f);
        cameraRight = vec3(0.0f, 0.0f, 1.0f);
        cameraUp = vec3(0.0f, 1.0f, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        cameraPos = vec3(5.0f, 0.0f, 0.0f);
        cameraFront = vec3(-1.0f, 0.0f, 0.0f);
        cameraRight = vec3(0.0f, 0.0f, -1.0f);
        cameraUp = vec3(0.0f, 1.0f, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
        cameraPos = vec3(0.0f, 0.0f, -5.0f);
        cameraFront = vec3(0.0f, 0.0f, 1.0f);
        cameraRight = vec3(-1.0f, 0.0f, 0.0f);
        cameraUp = vec3(0.0f, 1.0f, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) {
        cameraPos = vec3(0.0f, 5.0f, 0.0f);
        cameraFront = vec3(0.0f, -1.0f, 0.0f);
        cameraRight = vec3(1.0f, 0.0f, 0.0f);
        cameraUp = vec3(0.0f, 0.0f, -1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) {
        cameraPos = vec3(0.0f, -5.0f, 0.0f);
        cameraFront = vec3(0.0f, 1.0f, 0.0f);
        cameraRight = vec3(1.0f, 0.0f, 0.0f);
        cameraUp = vec3(0.0f, 0.0f, 1.0f);
    }
    
        
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        vec3 offset = cameraFront * moveDistance;
        cameraPos = cameraPos + offset;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        vec3 offset = cameraFront * moveDistance;
        cameraPos = cameraPos - offset;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        vec3 offset = cameraRight * moveDistance;
        cameraPos = cameraPos - offset;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        vec3 offset = cameraRight * moveDistance;
        cameraPos = cameraPos + offset;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        vec3 offset = cameraUp * moveDistance;
        cameraPos = cameraPos + offset;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        vec3 offset = cameraUp * moveDistance;
        cameraPos = cameraPos - offset;
    }
    
    
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        pitch(1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        pitch(-1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        yaw(1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        yaw(-1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
        roll(-1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
        roll(1.0f);
    }
    
    
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        if (fieldOfView < 180.0f)
            fieldOfView += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        if (fieldOfView > 0.0f)
            fieldOfView -= 1.0f;
    }
    
    
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        polygonMode++;
        if (polygonMode == 3)
            polygonMode = 0;
    }
}

vec3 rotateVectorAroundAxis(vec3 vector, vec3 axis, float angle)
{
    return vector * cos(radians(angle)) + cross(axis, vector) * sin(radians(angle)) + axis * dot(axis, vector) * (1.0f - cos(radians(angle)));
}

void yaw(float angle) {
    cameraFront = rotateVectorAroundAxis(cameraFront, cameraUp, angle);
    cameraRight = rotateVectorAroundAxis(cameraRight, cameraUp, angle);
}

void pitch(float angle) {
    cameraUp = rotateVectorAroundAxis(cameraUp, cameraRight, angle);
    cameraFront = rotateVectorAroundAxis(cameraFront, cameraRight, angle);
}

void roll(float angle) {
    cameraRight = rotateVectorAroundAxis(cameraRight, cameraFront, angle);
    cameraUp = rotateVectorAroundAxis(cameraUp, cameraFront, angle);
}

void resizeFramebuffer(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
