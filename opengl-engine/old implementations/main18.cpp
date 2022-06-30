//
//  main14.cpp
//  OpenGLTest3
//
//  Created by Nazım Anıl Tepe on 30.08.2021.
//  Copyright © 2021 Nazım Anıl Tepe. All rights reserved.
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
#include <vector>

float vertices[] = {
    //front
    -100.0f, -100.0f, 100.0f,
    100.0f, -100.0f, 100.0f,
    100.0f, 100.0f, 100.0f,
    -100.0f, 100.0f, 100.0f,
    
    //left
    -100.0f, -100.0f, -100.0f,
    -100.0f, -100.0f, 100.0f,
    -100.0f, 100.0f, 100.0f,
    -100.0f, 100.0f, -100.0f,
    
    //right
    100.0f, -100.0f, 100.0f,
    100.0f, -100.0f, -100.0f,
    100.0f, 100.0f, -100.0f,
    100.0f, 100.0f, 100.0f,

    //back
    100.0f, -100.0f, -100.0f,
    -100.0f, -100.0f, -100.0f,
    -100.0f, 100.0f, -100.0f,
    100.0f, 100.0f, -100.0f,
    
    //top
    -100.0f, 100.0f, 100.0f,
    100.0f, 100.0f, 100.0f,
    100.0f, 100.0f, -100.0f,
    -100.0f, 100.0f, -100.0f,
    
    //bottom
    -100.0f, -100.0f, -100.0f,
    100.0f, -100.0f, -100.0f,
    100.0f, -100.0f, 100.0f,
    -100.0f, -100.0f, 100.0f
};

float normals[] = {
    //front
    -1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    
    //left
    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, -1.0f,
    
    //right
    1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    1.0f, 1.0f, 1.0f,

    //back
    1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    
    //top
    -1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    
    //bottom
    -1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f
};

float texcoords[] = {
    //front
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f,
    
    //left
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f,
    
    //right
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f,

    //back
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f,
    
    //top
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f,
    
    //bottom
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f
};

float orders[] = {
  0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
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
std::vector<unsigned char> base64_decode(std::string const& encoded_string);

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
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) * 3, vertices, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), &vertices);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), sizeof(normals), &normals);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeof(normals), sizeof(texcoords), &texcoords);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeof(normals) + sizeof(texcoords), sizeof(orders), &orders);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(sizeof(vertices)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(sizeof(vertices) + sizeof(normals)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(float), (void*)(sizeof(vertices) + sizeof(normals) + sizeof(texcoords)));
    glEnableVertexAttribArray(3);
    
    
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    stbi_set_flip_vertically_on_load(true);
//    unsigned int textures[6];
    std::vector<unsigned int> textures;
//    std::string path = "/Users/nazimaniltepe/Documents/Projects/OpenGLTest3/OpenGLTest3/";
    std::vector<std::string> texStrings;
    texStrings.push_back("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");
    texStrings.push_back("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");
    texStrings.push_back("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");
    texStrings.push_back("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");
    texStrings.push_back("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");
    texStrings.push_back("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");
    for(int i = 0; i < texStrings.size(); i++) {
        textures.push_back(*new unsigned int());
        glGenTextures(1, &textures[i]);
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
//        std::string fullpath = path + std::to_string(i + 1) + ".jpg";
        int width, height, nrChannels;
//        unsigned char *data = stbi_load(fullpath.c_str(), &width, &height, &nrChannels, 0);
        std::vector<unsigned char> decoded = base64_decode(texStrings[i]);
        unsigned char *data = stbi_load_from_memory(&decoded[0], int(decoded.size()), &width, &height, &nrChannels, 0);
        
        if (data) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
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
        for (int i = 0; i < texStrings.size(); i++) {
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

std::vector<unsigned char> base64_decode(std::string const& encoded_string)
{
    std::string base64_chars =
                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                 "abcdefghijklmnopqrstuvwxyz"
                 "0123456789+/";
    
    long in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::vector<unsigned char> ret;
    
    while (in_len-- && ( encoded_string[in_] != '=') && (isalnum(encoded_string[in_]) || (encoded_string[in_] == '+') || (encoded_string[in_] == '/'))) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i ==4) {
            for (i = 0; i <4; i++)
            char_array_4[i] = base64_chars.find(char_array_4[i]);
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            
            for (i = 0; (i < 3); i++)
            ret.push_back(char_array_3[i]);
            i = 0;
        }
    }
    
    if (i) {
        for (j = i; j <4; j++)
        char_array_4[j] = 0;
        
        for (j = 0; j <4; j++)
        char_array_4[j] = base64_chars.find(char_array_4[j]);
        
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
        
        for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
    }
    
    return ret;
}
