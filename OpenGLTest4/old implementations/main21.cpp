//
//  main21.cpp
//  OpenGLTest3
//
//  Created by Nazım Anıl Tepe on 14.02.2021.
//  Copyright © 2021 Nazım Anıl Tepe. All rights reserved.
//
// flat shading vs. smooth shading on a cube

#include <glew.h>
#include <glfw3.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <cmath>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>


//flat shading

float vertices[] = {
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,

    -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,

    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,

    0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
    0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
    0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,

    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f
};


//smooth shading

//float vertices[] = {
//    -0.5f, -0.5f, 0.5f, -1.0f, -1.0f, 1.0f,
//    0.5f, -0.5f, 0.5f, 1.0f, -1.0f, 1.0f,
//    0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f,
//    0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f,
//    -0.5f, 0.5f, 0.5f, -1.0f, 1.0f, 1.0f,
//    -0.5f, -0.5f, 0.5f, -1.0f, -1.0f, 1.0f,
//
//    -0.5f, -0.5f, -0.5f, -1.0f, -1.0f, -1.0f,
//    -0.5f, -0.5f, 0.5f, -1.0f, -1.0f, 1.0f,
//    -0.5f, 0.5f, 0.5f, -1.0f, 1.0f, 1.0f,
//    -0.5f, 0.5f, 0.5f, -1.0f, 1.0f, 1.0f,
//    -0.5f, 0.5f, -0.5f, -1.0f, 1.0f, -1.0f,
//    -0.5f, -0.5f, -0.5f, -1.0f, -1.0f, -1.0f,
//
//    0.5f, -0.5f, 0.5f, 1.0f, -1.0f, 1.0f,
//    0.5f, -0.5f, -0.5f, 1.0f, -1.0f, -1.0f,
//    0.5f, 0.5f, -0.5f, 1.0f, 1.0f, -1.0f,
//    0.5f, 0.5f, -0.5f, 1.0f, 1.0f, -1.0f,
//    0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f,
//    0.5f, -0.5f, 0.5f, 1.0f, -1.0f, 1.0f,
//
//    0.5f, -0.5f, -0.5f, 1.0f, -1.0f, -1.0f,
//    -0.5f, -0.5f, -0.5f, -1.0f, -1.0f, -1.0f,
//    -0.5f, 0.5f, -0.5f, -1.0f, 1.0f, -1.0f,
//    -0.5f, 0.5f, -0.5f, -1.0f, 1.0f, -1.0f,
//    0.5f, 0.5f, -0.5f, 1.0f, 1.0f, -1.0f,
//    0.5f, -0.5f, -0.5f, 1.0f, -1.0f, -1.0f,
//
//    -0.5f, 0.5f, 0.5f, -1.0f, 1.0f, 1.0f,
//    0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f,
//    0.5f, 0.5f, -0.5f, 1.0f, 1.0f, -1.0f,
//    0.5f, 0.5f, -0.5f, 1.0f, 1.0f, -1.0f,
//    -0.5f, 0.5f, -0.5f, -1.0f, 1.0f, -1.0f,
//    -0.5f, 0.5f, 0.5f, -1.0f, 1.0f, 1.0f,
//
//    -0.5f, -0.5f, -0.5f, -1.0f, -1.0f, -1.0f,
//    0.5f, -0.5f, -0.5f, 1.0f, -1.0f, -1.0f,
//    0.5f, -0.5f, 0.5f, 1.0f, -1.0f, 1.0f,
//    0.5f, -0.5f, 0.5f, 1.0f, -1.0f, 1.0f,
//    -0.5f, -0.5f, 0.5f, -1.0f, -1.0f, 1.0f,
//    -0.5f, -0.5f, -0.5f, -1.0f, -1.0f, -1.0f
//};

int shaderProgram;
const char *vertexShaderSource = "#version 330 core\n"
    "layout(location = 0) in vec3 vPos;\n"
    "layout(location = 1) in vec3 vNormal;\n"
    "out vec3 FragPos;\n"
    "out vec3 Normal;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "   FragPos = vec3(model * vec4(vPos, 1.0f));\n"
    "   Normal = vNormal;\n"
    "   gl_Position = projection * view * model * vec4(vPos, 1.0f);\n"
    "}\0";
const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec3 FragPos;\n"
    "in vec3 Normal;\n"
    "uniform vec3 lightPos;\n"
    "void main()\n"
    "{\n"
    "   float ambientStrength = 0.1f;\n"
    "   vec3 ambient = ambientStrength * vec3(1.0f);\n"
    "   vec3 norm = normalize(Normal);\n"
    "   vec3 lightDir = normalize(lightPos - FragPos);\n"
    "   float diffuseStrength = max(dot(norm, lightDir), 0.0f);\n"
    "   vec3 diffuse = diffuseStrength * vec3(1.0f);\n"
    "   vec3 result = (ambient + diffuse) * vec3(0.2f, 0.7f, 0.4f);\n"
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



void processDiscreteInput(GLFWwindow* window, int key, int scancode, int action, int mods);
void processContinuousInput(GLFWwindow* window);
void initShader();
glm::vec3 rotateVectorAroundAxis(glm::vec3 vector, glm::vec3 axis, float angle);
void yaw(float angle);
void pitch(float angle);
void roll(float angle);
void resizeFramebuffer(GLFWwindow* window, int width, int height);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

unsigned int VBO;
unsigned int VAO;
unsigned int lightVAO;

unsigned int polygonMode = GL_FILL;
float fieldOfView   =  45.0f;
float moveDistance = 0.02f;

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraRight = glm::vec3(1.0f, 0.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

glm::mat4 projection;
glm::mat4 view;
glm::mat4 model;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec3 lightPos = glm::vec3(2.0f, 1.2f, 0.0f);

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, resizeFramebuffer);
    glfwSetKeyCallback(window, processDiscreteInput);
    
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

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
        
        processContinuousInput(window);
        
        projection = glm::perspective(glm::radians(fieldOfView), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        view = lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        model = glm::mat4(1.0f);
        
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE,  glm::value_ptr(model));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        
        
        glUseProgram(lightShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(lightShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(lightShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.01f));
        glUniformMatrix4fv(glGetUniformLocation(lightShaderProgram, "model"), 1, GL_FALSE,  glm::value_ptr(model));
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &lightVAO);
    
    
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

void processDiscreteInput(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    
    
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
        cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
        cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
        cameraRight = glm::vec3(1.0f, 0.0f, 0.0f);
        cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
        fieldOfView = 45.0f;
    }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
        cameraPos = glm::vec3(-3.0f, 0.0f, 0.0f);
        cameraFront = glm::vec3(1.0f, 0.0f, 0.0f);
        cameraRight = glm::vec3(0.0f, 0.0f, 1.0f);
        cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
        fieldOfView = 45.0f;
    }
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
        cameraPos = glm::vec3(3.0f, 0.0f, 0.0f);
        cameraFront = glm::vec3(-1.0f, 0.0f, 0.0f);
        cameraRight = glm::vec3(0.0f, 0.0f, -1.0f);
        cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
        fieldOfView = 45.0f;
    }
    if (key == GLFW_KEY_4 && action == GLFW_PRESS) {
        cameraPos = glm::vec3(0.0f, 0.0f, -3.0f);
        cameraFront = glm::vec3(0.0f, 0.0f, 1.0f);
        cameraRight = glm::vec3(-1.0f, 0.0f, 0.0f);
        cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
        fieldOfView = 45.0f;
    }
    if (key == GLFW_KEY_5 && action == GLFW_PRESS) {
        cameraPos = glm::vec3(0.0f, 3.0f, 0.0f);
        cameraFront = glm::vec3(0.0f, -1.0f, 0.0f);
        cameraRight = glm::vec3(1.0f, 0.0f, 0.0f);
        cameraUp = glm::vec3(0.0f, 0.0f, -1.0f);
        fieldOfView = 45.0f;
    }
    if (key == GLFW_KEY_6 && action == GLFW_PRESS) {
        cameraPos = glm::vec3(0.0f, -3.0f, 0.0f);
        cameraFront = glm::vec3(0.0f, 1.0f, 0.0f);
        cameraRight = glm::vec3(1.0f, 0.0f, 0.0f);
        cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);
        fieldOfView = 45.0f;
    }
    
       
    if (key == GLFW_KEY_M && action == GLFW_PRESS) {
        if (polygonMode == GL_FILL)
            polygonMode = GL_POINT;
        else if (polygonMode == GL_POINT)
            polygonMode = GL_LINE;
        else if (polygonMode == GL_LINE)
            polygonMode = GL_FILL;
    }
}

void processContinuousInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        glm::vec3 offset = cameraFront * moveDistance;
        cameraPos = cameraPos + offset;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        glm::vec3 offset = cameraFront * moveDistance;
        cameraPos = cameraPos - offset;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        glm::vec3 offset = cameraRight * moveDistance;
        cameraPos = cameraPos - offset;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        glm::vec3 offset = cameraRight * moveDistance;
        cameraPos = cameraPos + offset;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        glm::vec3 offset = cameraUp * moveDistance;
        cameraPos = cameraPos + offset;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        glm::vec3 offset = cameraUp * moveDistance;
        cameraPos = cameraPos - offset;
    }
    
    
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        pitch(0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        pitch(-0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        yaw(0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        yaw(-0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        roll(-0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        roll(0.5f);
    }
    
    
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        if (fieldOfView < 180.0f)
            fieldOfView += 0.5f;
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        if (fieldOfView > 0.0f)
            fieldOfView -= 0.5f;
    }
}

glm::vec3 rotateVectorAroundAxis(glm::vec3 vector, glm::vec3 axis, float angle)
{
    return vector * cos(glm::radians(angle)) + cross(axis, vector) * sin(glm::radians(angle)) + axis * dot(axis, vector) * (1.0f - cos(glm::radians(angle)));
}

void yaw(float angle)
{
    cameraFront = rotateVectorAroundAxis(cameraFront, cameraUp, angle);
    cameraRight = rotateVectorAroundAxis(cameraRight, cameraUp, angle);
}

void pitch(float angle)
{
    cameraUp = rotateVectorAroundAxis(cameraUp, cameraRight, angle);
    cameraFront = rotateVectorAroundAxis(cameraFront, cameraRight, angle);
}

void roll(float angle)
{
    cameraRight = rotateVectorAroundAxis(cameraRight, cameraFront, angle);
    cameraUp = rotateVectorAroundAxis(cameraUp, cameraFront, angle);
}

void resizeFramebuffer(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

