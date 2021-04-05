//
//  main2.cpp
//  OpenGLTest4
//
//  Created by Nazım Anıl Tepe on 22.03.2021.
//
// scene loading from path

#include <glew.h>
#include <glfw3.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <cmath>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <map>
#include <time.h>
using namespace std;

#include <unistd.h>
#define GetCurrentDir getcwd

//#include "base64.h"

struct Transform {
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    void yaw(float angle) {
        front = rotateVectorAroundAxis(front, up, angle);
        right = rotateVectorAroundAxis(right, up, angle);
    }
    void pitch(float angle) {
        up = rotateVectorAroundAxis(up, right, angle);
        front = rotateVectorAroundAxis(front, right, angle);
    }
    void roll(float angle) {
        right = rotateVectorAroundAxis(right, front, angle);
        up = rotateVectorAroundAxis(up, front, angle);
    }
private:
    glm::vec3 rotateVectorAroundAxis(glm::vec3 vector, glm::vec3 axis, float angle)
    {
        return vector * cos(glm::radians(angle)) + cross(axis, vector) * sin(glm::radians(angle)) + axis * dot(axis, vector) * (1.0f - cos(glm::radians(angle)));
    }
};
struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    bool texture;
    unsigned int diffuseTex;
    unsigned int specularTex;
    float shininess;
};
enum struct nType : int {
    nScene, nModel, nLight, nCamera, nJoint, nText
};
enum struct nLightType : int {
    point, directional, spotlight
};
struct nObject {
    nType type;
    string name;
    unsigned int index;
};
struct nModel : nObject {
    vector<float> vertices {};
    int vertexCount;
    unsigned int vao;
    unsigned int vbo;
    int shader;
    string vertexShader;
    string fragmentShader;
    Transform transform;
    Material material;
};
struct nLight : nObject {
    vector<float> vertices {};
    int vertexCount;
    unsigned int vao;
    unsigned int vbo;
    int shader;
    string vertexShader;
    string fragmentShader;
    nLightType lightType;
    float constant;
    float linear;
    float quadratic;
    float cutOff;
    float outerCutOff;
    Material material;
    Transform transform;
};
struct nCamera : nObject {
    float fov;
    float minDistance;
    float maxDistance;
    float moveSpeed;
    Transform transform;
};
struct nJoint : nObject {
    vector<float> vertices {};
    unsigned int vao;
    unsigned int vbo;
    int shader;
    string vertexShader;
    string fragmentShader;
    Transform transform;
    Material material;
};
struct nText : nObject {
    string text;
    Transform transform;
    Material material;
};
struct nScene : nObject {
    unsigned int scrWidth;
    unsigned int scrHeight;
    vector<nModel> models {};
    vector<nLight> lights {};
    nCamera camera;
};
struct Character {
    unsigned int TextureID;
    glm::ivec2   Size;
    glm::ivec2   Bearing;
    unsigned int Advance;
};

nScene scene;
map<GLchar, Character> characters;

void createScene(string path);
void createShaders();
void createBuffers();
void processDiscreteInput(GLFWwindow* window, int key, int scancode, int action, int mods);
void processContinuousInput(GLFWwindow* window);
void resizeFramebuffer(GLFWwindow* window, int width, int height);
int captureScreenshot();
vector<unsigned char> base64_decode(string const& encoded_string);

unsigned int polygonMode = GL_FILL;

glm::mat4 projection;
glm::mat4 view;
glm::mat4 model;

float lastFrame = 0.0f;
int frameCount = 0;
int fps = 0;

int main()
{
    createScene("/Users/nazimaniltepe/Documents/Projects/opengl-nscene/OpenGLTest4/scene1.nsce");
    
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(scene.scrWidth, scene.scrHeight, "OpenGL", NULL, NULL);
    if (window == NULL) {
        cout << "Failed to create GLFW window" << endl;
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
    
    createShaders();
    createBuffers();
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        frameCount++;
        if (currentFrame - lastFrame >= 1.0) {
            fps = frameCount;
            frameCount = 0;
            lastFrame = currentFrame;
        }
        
        glClearColor(0.6f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
        
        processContinuousInput(window);
        
        projection = glm::perspective(glm::radians(scene.camera.fov), (float)scene.scrWidth / (float)scene.scrHeight, scene.camera.minDistance, scene.camera.maxDistance);
        view = lookAt(scene.camera.transform.position, scene.camera.transform.position + scene.camera.transform.front, scene.camera.transform.up);
        
        for (int i = 0; i < scene.models.size(); i++) {
            glUseProgram(scene.models[i].shader);
            glUniformMatrix4fv(glGetUniformLocation(scene.models[i].shader, "projection"), 1, GL_FALSE, value_ptr(projection));
            glUniformMatrix4fv(glGetUniformLocation(scene.models[i].shader, "view"), 1, GL_FALSE, value_ptr(view));
            model = glm::translate(glm::mat4(1.0f), scene.models[i].transform.position);
            model = glm::scale(model, scene.models[i].transform.scale);
            glm::mat4 rotation = glm::mat4(scene.models[i].transform.right.x, scene.models[i].transform.right.y, scene.models[i].transform.right.z, 0,
                              scene.models[i].transform.up.x, scene.models[i].transform.up.y, scene.models[i].transform.up.z, 0,
                              scene.models[i].transform.front.x, scene.models[i].transform.front.y, scene.models[i].transform.front.z, 0,
                              0, 0, 0, 1);
            model *= rotation;
            glUniformMatrix4fv(glGetUniformLocation(scene.models[i].shader, "model"), 1, GL_FALSE,  value_ptr(model));
            glUniform3fv(glGetUniformLocation(scene.models[i].shader, "cameraPos"), 1, value_ptr(scene.camera.transform.position));
            
            glUniform3fv(glGetUniformLocation(scene.models[i].shader, "modelMaterial.ambient"), 1, value_ptr(scene.models[i].material.ambient));
            glUniform3fv(glGetUniformLocation(scene.models[i].shader, "modelMaterial.diffuse"), 1, value_ptr(scene.models[i].material.diffuse));
            glUniform3fv(glGetUniformLocation(scene.models[i].shader, "modelMaterial.specular"), 1, value_ptr(scene.models[i].material.specular));
            glUniform1i(glGetUniformLocation(scene.models[i].shader, "modelMaterial.texture"), (int)scene.models[i].material.texture);
            glUniform1f(glGetUniformLocation(scene.models[i].shader, "modelMaterial.shininess"), scene.models[i].material.shininess);
            
            if (scene.models[i].material.texture) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, scene.models[0].material.diffuseTex);
            }
            
            for (int j = 0; j < scene.lights.size(); j++) {
                glUniform1i(glGetUniformLocation(scene.models[i].shader, ("lights[" + to_string(j) + "].lightType").c_str()), static_cast<int>(scene.lights[j].lightType));
                glUniform3fv(glGetUniformLocation(scene.models[i].shader, ("lights[" + to_string(j) + "].direction").c_str()), 1, value_ptr(scene.lights[j].transform.front));
                glUniform3fv(glGetUniformLocation(scene.models[i].shader, ("lights[" + to_string(j) + "].position").c_str()), 1, value_ptr(scene.lights[j].transform.position));
                glUniform3fv(glGetUniformLocation(scene.models[i].shader, "modelMaterial.specular"), 1, value_ptr(scene.models[i].material.specular));
                glUniform1f(glGetUniformLocation(scene.models[i].shader, ("lights[" + to_string(j) + "].constant").c_str()), scene.lights[j].constant);
                glUniform1f(glGetUniformLocation(scene.models[i].shader, ("lights[" + to_string(j) + "].linear").c_str()), scene.lights[j].linear);
                glUniform1f(glGetUniformLocation(scene.models[i].shader, ("lights[" + to_string(j) + "].quadratic").c_str()), scene.lights[j].quadratic);
                glUniform1f(glGetUniformLocation(scene.models[i].shader, ("lights[" + to_string(j) + "].cutOff").c_str()), scene.lights[j].cutOff);
                glUniform1f(glGetUniformLocation(scene.models[i].shader, ("lights[" + to_string(j) + "].outerCutOff").c_str()), scene.lights[j].outerCutOff);
                glUniform3fv(glGetUniformLocation(scene.models[i].shader, ("lights[" + to_string(j) + "].material.ambient").c_str()), 1, value_ptr(scene.lights[j].material.ambient));
                glUniform3fv(glGetUniformLocation(scene.models[i].shader, ("lights[" + to_string(j) + "].material.diffuse").c_str()), 1, value_ptr(scene.lights[j].material.diffuse));
                glUniform3fv(glGetUniformLocation(scene.models[i].shader, ("lights[" + to_string(j) + "].material.specular").c_str()), 1, value_ptr(scene.lights[j].material.specular));
            }
            glBindVertexArray(scene.models[i].vao);
            glDrawArrays(GL_TRIANGLES, 0, scene.models[i].vertexCount);
        }
        for (int i = 0; i < scene.lights.size(); i++) {
            if (scene.lights[i].vertices.size() > 0) {
                glUseProgram(scene.lights[i].shader);
                glUniformMatrix4fv(glGetUniformLocation(scene.lights[i].shader, "projection"), 1, GL_FALSE, value_ptr(projection));
                glUniformMatrix4fv(glGetUniformLocation(scene.lights[i].shader, "view"), 1, GL_FALSE, value_ptr(view));
                model = glm::translate(glm::mat4(1.0f), scene.lights[i].transform.position);
                model = glm::scale(model, scene.lights[i].transform.scale);
                glm::mat4 rotation = glm::mat4(scene.lights[i].transform.right.x, scene.lights[i].transform.right.y, scene.lights[i].transform.right.z, 0,
                                  scene.lights[i].transform.up.x, scene.lights[i].transform.up.y, scene.lights[i].transform.up.z, 0,
                                  scene.lights[i].transform.front.x, scene.lights[i].transform.front.y, scene.lights[i].transform.front.z, 0,
                                  0, 0, 0, 1);
                model *= rotation;
                glUniformMatrix4fv(glGetUniformLocation(scene.lights[i].shader, "model"), 1, GL_FALSE,  value_ptr(model));
                glUniform3fv(glGetUniformLocation(scene.lights[i].shader, "color"), 1, value_ptr(scene.lights[i].material.diffuse / 0.8f));
                glBindVertexArray(scene.lights[i].vao);
                glDrawArrays(GL_TRIANGLES, 0, scene.lights[i].vertexCount);
            }
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    for (int i = 0; i < scene.models.size(); i++) {
        glDeleteProgram(scene.models[i].shader);
        glDeleteBuffers(1, &scene.models[i].vbo);
        glDeleteVertexArrays(1, &scene.models[i].vao);
    }
    for (int i = 0; i < scene.lights.size(); i++) {
        if (scene.lights[i].vertices.size() > 0) {
            glDeleteProgram(scene.lights[i].shader);
            glDeleteBuffers(1, &scene.lights[i].vbo);
            glDeleteVertexArrays(1, &scene.lights[i].vao);
        }
    }
    
    glfwTerminate();
    return 0;
}

void createScene(string path)
{
    string line;
    ifstream file(path);
    if (file) {
        file.seekg(0, file.end);
        long length = file.tellg();
        file.seekg(0, file.beg);
        char* buffer = new char[length];
        file.read(buffer, length);
        file.close();
        line = buffer;
        delete[] buffer;
    }
    vector<string> rows {};
    long backslashPos = 0;
    string backslash = "\n";
    while ((backslashPos = line.find(backslash)) != string::npos) {
        string row = line.substr(0, backslashPos);
        if (row != "")
            rows.push_back(row);
        line.erase(0, backslashPos + backslash.length());
    }
    long eofPos = 0;
    if ((eofPos = line.find("\377")) != string::npos) {
        rows.push_back(line.substr(0, eofPos));
    }
    rows.push_back(line);
    int index = 0;
    for (int i = 0; i < rows.size(); i++) {
        if (rows[i].find("/") == string::npos && rows[i].find(":") == string::npos && !regex_match(rows[i], regex("[\\s]*"))) {
            map<string, string> dictionary;
            int j = i + 1;
            while (rows[j].rfind("/" + rows[i], 0) != 0) {
                if (regex_match(rows[j], regex("[\\s]*"))) {
                    j++;
                    continue;
                }
                string pairKey = rows[j].substr(0, rows[j].find(":"));
                pairKey.erase(pairKey.begin(), find_if(pairKey.begin(), pairKey.end(), [](unsigned char c) {
                    return !isspace(c);
                }));
                string pairValue = rows[j].substr(rows[j].find(":") + 1);
                pairValue.erase(pairValue.begin(), find_if(pairValue.begin(), pairValue.end(), [](unsigned char c) {
                    return !isspace(c);
                }));
                dictionary.insert(pair<string, string>(pairKey, pairValue));
                j++;
            }
            if (stoi(dictionary.at("type")) == static_cast<int>(nType::nScene)) {
                scene.type = nType::nScene;
                scene.name = rows[i];
                scene.index = index++;
                scene.scrWidth = stoi(dictionary.at("scwd"));
                scene.scrHeight = stoi(dictionary.at("schg"));
            }
            else if (stoi(dictionary.at("type")) == static_cast<int>(nType::nModel)) {
                nModel model;
                model.type = nType::nModel;
                model.name = rows[i];
                model.index = index++;
                long space = 0;
                string delimiter = " ";
                string s = dictionary.at("v");
                while ((space = s.find(delimiter)) != string::npos) {
                    string f = s.substr(0, space);
                    if (f != "")
                        model.vertices.push_back(stof(f));
                    s.erase(0, space + delimiter.length());
                }
                if (s != "")
                    model.vertices.push_back(stof(s));
                if (dictionary.find("trns") != dictionary.end()) {
                    long space = 0;
                    string delimiter = " ";
                    string s = dictionary.at("trns");
                    vector<float> sequence {};
                    while ((space = s.find(delimiter)) != string::npos) {
                        string f = s.substr(0, space);
                        if (f != "")
                            sequence.push_back(stof(f));
                        s.erase(0, space + delimiter.length());
                    }
                    sequence.push_back(stof(s));
                    model.transform.position = glm::vec3(sequence[0], sequence[1], sequence[2]);
                    model.transform.scale = glm::vec3(sequence[3], sequence[4], sequence[5]);
                    model.transform.front = glm::vec3(sequence[6], sequence[7], sequence[8]);
                    model.transform.up = glm::vec3(sequence[9], sequence[10], sequence[11]);
                    model.transform.right = glm::vec3(sequence[12], sequence[13], sequence[14]);
                }
                if (dictionary.find("mtrl") != dictionary.end()) {
                    long space = 0;
                    string delimiter = " ";
                    string s = dictionary.at("mtrl");
                    vector<float> sequence {};
                    while ((space = s.find(delimiter)) != string::npos) {
                        string f = s.substr(0, space);
                        if (f != "")
                            if (f == "true" || f == "false") {
                                istringstream is_bool(f);
                                is_bool >> boolalpha >> model.material.texture;
                            }
                            else
                                sequence.push_back(stof(f));
                        s.erase(0, space + delimiter.length());
                    }
                    sequence.push_back(stof(s));
                    model.material.ambient = glm::vec3(sequence[0], sequence[1], sequence[2]);
                    model.material.diffuse = glm::vec3(sequence[3], sequence[4], sequence[5]);
                    model.material.specular = glm::vec3(sequence[6], sequence[7], sequence[8]);
                    model.material.shininess = sequence[9];
                }
                
                model.vertexCount = model.material.texture ? model.vertices.size() / 8 : model.vertices.size() / 6;
                
                scene.models.push_back(model);
            }
            else if (stoi(dictionary.at("type")) == static_cast<int>(nType::nLight)) {
                nLight light;
                light.type = nType::nLight;
                light.name = rows[i];
                light.index = index++;
                light.lightType = static_cast<nLightType>(stoi(dictionary.at("ltyp")));
                if (light.lightType != nLightType::directional) {
                    light.constant = stof(dictionary.at("cnst"));
                    light.linear = stof(dictionary.at("lnr"));
                    light.quadratic = stof(dictionary.at("quad"));
                    if (light.lightType == nLightType::spotlight) {
                        light.cutOff = stof(dictionary.at("cut"));
                        light.outerCutOff = stof(dictionary.at("ocut"));
                    }
                }
                if (dictionary.find("v") != dictionary.end()) {
                    long space = 0;
                    string delimiter = " ";
                    string s = dictionary.at("v");
                    while ((space = s.find(delimiter)) != string::npos) {
                        string f = s.substr(0, space);
                        if (f != "")
                            light.vertices.push_back(stof(f));
                        s.erase(0, space + delimiter.length());
                    }
                    if (s != "")
                        light.vertices.push_back(stof(s));
                    light.vertexCount = light.vertices.size() / 3;
                }
                if (dictionary.find("trns") != dictionary.end()) {
                    long space = 0;
                    string delimiter = " ";
                    string s = dictionary.at("trns");
                    vector<float> sequence {};
                    while ((space = s.find(delimiter)) != string::npos) {
                        string f = s.substr(0, space);
                        if (f != "")
                            sequence.push_back(stof(f));
                        s.erase(0, space + delimiter.length());
                    }
                    sequence.push_back(stof(s));
                    light.transform.position = glm::vec3(sequence[0], sequence[1], sequence[2]);
                    light.transform.scale = glm::vec3(sequence[3], sequence[4], sequence[5]);
                    light.transform.front = glm::vec3(sequence[6], sequence[7], sequence[8]);
                    light.transform.up = glm::vec3(sequence[9], sequence[10], sequence[11]);
                    light.transform.right = glm::vec3(sequence[12], sequence[13], sequence[14]);
                }
                if (dictionary.find("mtrl") != dictionary.end()) {
                    long space = 0;
                    string delimiter = " ";
                    string s = dictionary.at("mtrl");
                    vector<float> sequence {};
                    while ((space = s.find(delimiter)) != string::npos) {
                        string f = s.substr(0, space);
                        if (f != "")
                            if (f == "true" || f == "false") {
                                istringstream is_bool(f);
                                is_bool >> boolalpha >> light.material.texture;
                            }
                            else
                                sequence.push_back(stof(f));
                        s.erase(0, space + delimiter.length());
                    }
                    sequence.push_back(stof(s));
                    light.material.ambient = glm::vec3(sequence[0], sequence[1], sequence[2]);
                    light.material.diffuse = glm::vec3(sequence[3], sequence[4], sequence[5]);
                    light.material.specular = glm::vec3(sequence[6], sequence[7], sequence[8]);
                    light.material.shininess = sequence[9];
                }
                
                scene.lights.push_back(light);
            }
            else if (stoi(dictionary.at("type")) == static_cast<int>(nType::nCamera)) {
                nCamera camera;
                camera.type = nType::nCamera;
                camera.name = rows[i];
                camera.index = index++;
                camera.fov = stof(dictionary.at("fov"));
                camera.minDistance = stof(dictionary.at("mind"));
                camera.maxDistance = stof(dictionary.at("maxd"));
                camera.moveSpeed = stof(dictionary.at("mvsp"));
                long space = 0;
                string delimiter = " ";
                string s = dictionary.at("trns");
                vector<float> sequence {};
                while ((space = s.find(delimiter)) != string::npos) {
                    string f = s.substr(0, space);
                    if (f != "")
                        sequence.push_back(stof(f));
                    s.erase(0, space + delimiter.length());
                }
                sequence.push_back(stof(s));
                camera.transform.position = glm::vec3(sequence[0], sequence[1], sequence[2]);
                camera.transform.front = glm::vec3(sequence[6], sequence[7], sequence[8]);
                camera.transform.up = glm::vec3(sequence[9], sequence[10], sequence[11]);
                camera.transform.right = glm::vec3(sequence[12], sequence[13], sequence[14]);
            
                scene.camera = camera;
            }
        }
    }
}

void createShaders()
{
    for (int i = 0; i < scene.models.size(); i++) {
        scene.models[i].vertexShader = "#version 330 core\n";
        scene.models[i].vertexShader += "layout(location = 0) in vec3 vPos;\n";
        scene.models[i].vertexShader += "layout(location = 1) in vec3 vNormal;\n";
        scene.models[i].vertexShader += (scene.models[i].material.texture) ? "layout(location = 2) in vec2 vTexCoord;\n" : "";
        scene.models[i].vertexShader += "out vec3 FragPos;\n";
        scene.models[i].vertexShader += "out vec3 Normal;\n";
        scene.models[i].vertexShader += (scene.models[i].material.texture) ? "out vec2 TexCoord;\n" : "";
        scene.models[i].vertexShader += "uniform mat4 model;\n";
        scene.models[i].vertexShader += "uniform mat4 view;\n";
        scene.models[i].vertexShader += "uniform mat4 projection;\n";
        scene.models[i].vertexShader += "void main() {\n";
        scene.models[i].vertexShader += "gl_Position = projection * view * model * vec4(vPos, 1.0f);\n";
        scene.models[i].vertexShader += "FragPos = vec3(model * vec4(vPos, 1.0f));\n";
        scene.models[i].vertexShader += "Normal = vNormal;\n";
//        scene.models[i].vertexShader += "Normal = mat3(transpose(inverse(model))) * vNormal;\n";
        scene.models[i].vertexShader += (scene.models[i].material.texture) ? "TexCoord = vTexCoord;\n" : "";
        scene.models[i].vertexShader += "}\0";

        scene.models[i].fragmentShader = "#version 330 core\n";
        scene.models[i].fragmentShader += "out vec4 FragColor;\n";
        scene.models[i].fragmentShader += "in vec3 FragPos;\n";
        scene.models[i].fragmentShader += "in vec3 Normal;\n";
        scene.models[i].fragmentShader += (scene.models[i].material.texture) ? "in vec2 TexCoord;\n" : "";
        scene.models[i].fragmentShader += "struct Material {\n";
        scene.models[i].fragmentShader += "vec3 ambient;\n";
        scene.models[i].fragmentShader += "vec3 diffuse;\n";
        scene.models[i].fragmentShader += "vec3 specular;\n";
        scene.models[i].fragmentShader += "bool texture;\n";
        scene.models[i].fragmentShader += "sampler2D diffuseTex;\n";
        scene.models[i].fragmentShader += "sampler2D specularTex;\n";
        scene.models[i].fragmentShader += "float shininess;\n";
        scene.models[i].fragmentShader += "};\n";
        scene.models[i].fragmentShader += "struct Light {\n";
        scene.models[i].fragmentShader += "int lightType;\n";
        scene.models[i].fragmentShader += "vec3 direction;\n";
        scene.models[i].fragmentShader += "vec3 position;\n";
        scene.models[i].fragmentShader += "float constant;\n";
        scene.models[i].fragmentShader += "float linear;\n";
        scene.models[i].fragmentShader += "float quadratic;\n";
        scene.models[i].fragmentShader += "float cutOff;\n";
        scene.models[i].fragmentShader += "float outerCutOff;\n";
        scene.models[i].fragmentShader += "Material material;\n";
        scene.models[i].fragmentShader += "};\n";
        scene.models[i].fragmentShader += "uniform vec3 cameraPos;\n";
        scene.models[i].fragmentShader += "uniform Material modelMaterial;\n";
        scene.models[i].fragmentShader += "uniform Light lights[" + to_string(scene.lights.size()) + "];\n";
        scene.models[i].fragmentShader += "vec3 CalculateLight(Light light, vec3 normal, vec3 viewDir, vec3 fragPos);\n";
        scene.models[i].fragmentShader += "void main() {\n";
        scene.models[i].fragmentShader += "vec3 norm = normalize(Normal);\n";
        scene.models[i].fragmentShader += "vec3 viewDir = normalize(cameraPos - FragPos);\n";
        scene.models[i].fragmentShader += "vec3 result = vec3(0.0f);\n";
        scene.models[i].fragmentShader += "for(int i = 0; i < lights.length(); i++)\n";
        scene.models[i].fragmentShader += "result += CalculateLight(lights[i], norm, viewDir, FragPos);\n";
        scene.models[i].fragmentShader += "FragColor = vec4(result, 1.0f);\n";
        scene.models[i].fragmentShader += "}\n";
        scene.models[i].fragmentShader += "vec3 CalculateLight(Light light, vec3 normal, vec3 viewDir, vec3 fragPos) {\n";
        scene.models[i].fragmentShader += "vec3 lightDir = normalize(light.position - fragPos);\n";
        scene.models[i].fragmentShader += "if (light.lightType == 1)\n";
        scene.models[i].fragmentShader += "lightDir = normalize(-light.direction);\n";
        scene.models[i].fragmentShader += "float diffStrength = max(dot(normal, lightDir), 0.0);\n";
        scene.models[i].fragmentShader += "vec3 reflectDir = reflect(-lightDir, normal);\n";
        scene.models[i].fragmentShader += "float specStrength = pow(max(dot(viewDir, reflectDir), 0.0), modelMaterial.shininess);\n";
        scene.models[i].fragmentShader += "vec3 ambient = light.material.ambient * modelMaterial.ambient;\n";
        scene.models[i].fragmentShader += "vec3 diffuse = light.material.diffuse * diffStrength * modelMaterial.diffuse;\n";
        scene.models[i].fragmentShader += "vec3 specular = light.material.specular * specStrength * modelMaterial.specular;\n";
        scene.models[i].fragmentShader += (scene.models[i].material.texture) ? "ambient = light.material.ambient * vec3(texture(modelMaterial.diffuseTex, TexCoord));\n" : "";
        scene.models[i].fragmentShader += (scene.models[i].material.texture) ? "diffuse = light.material.diffuse * diffStrength * vec3(texture(modelMaterial.diffuseTex, TexCoord));\n" : "";
        scene.models[i].fragmentShader += (scene.models[i].material.texture) ? "specular = light.material.specular * specStrength * vec3(texture(modelMaterial.specularTex, TexCoord));\n" : "";
        scene.models[i].fragmentShader += "if (light.lightType != 1) {\n";
        scene.models[i].fragmentShader += "float distance = length(light.position - fragPos);\n";
        scene.models[i].fragmentShader += "float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));\n";
        scene.models[i].fragmentShader += "ambient *= attenuation;\n";
        scene.models[i].fragmentShader += "diffuse *= attenuation;\n";
        scene.models[i].fragmentShader += "specular *= attenuation;\n";
        scene.models[i].fragmentShader += "if (light.lightType == 2) {\n";
        scene.models[i].fragmentShader += "float theta = dot(lightDir, normalize(-light.direction));\n";
        scene.models[i].fragmentShader += "float epsilon = light.cutOff - light.outerCutOff;\n";
        scene.models[i].fragmentShader += "float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);\n";
        scene.models[i].fragmentShader += "ambient *= intensity;\n";
        scene.models[i].fragmentShader += "diffuse *= intensity;\n";
        scene.models[i].fragmentShader += "specular *= intensity;\n";
        scene.models[i].fragmentShader += "}\n";
        scene.models[i].fragmentShader += "}\n";
        scene.models[i].fragmentShader += "return (ambient + diffuse + specular);\n";
        scene.models[i].fragmentShader += "}\0";
        
        int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        const char *vertexShaderSource = scene.models[i].vertexShader.c_str();
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        glGetError();
        int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        const char *fragmentShaderSource = scene.models[i].fragmentShader.c_str();
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        glGetError();
        scene.models[i].shader = glCreateProgram();
        glAttachShader(scene.models[i].shader, vertexShader);
        glGetError();
        glAttachShader(scene.models[i].shader, fragmentShader);
        glGetError();
        glLinkProgram(scene.models[i].shader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }
    for (int i = 0; i < scene.lights.size(); i++) {
        if (scene.lights[i].vertices.size() > 0) {
            scene.lights[i].vertexShader = "#version 330 core\n";
            scene.lights[i].vertexShader += "layout(location = 0) in vec3 vPos;\n";
            scene.lights[i].vertexShader += "uniform mat4 model;\n";
            scene.lights[i].vertexShader += "uniform mat4 view;\n";
            scene.lights[i].vertexShader += "uniform mat4 projection;\n";
            scene.lights[i].vertexShader += "void main() {\n";
            scene.lights[i].vertexShader += "gl_Position = projection * view * model * vec4(vPos, 1.0f);\n";
            scene.lights[i].vertexShader += "}\0";
            
            scene.lights[i].fragmentShader = "#version 330 core\n";
            scene.lights[i].fragmentShader += "out vec4 FragColor;\n";
            scene.lights[i].fragmentShader += "uniform vec3 color;\n";
            scene.lights[i].fragmentShader += "void main() {\n";
            scene.lights[i].fragmentShader += "FragColor = vec4(color, 1.0f);\n";
            scene.lights[i].fragmentShader += "}\0";
            
            int vertexShader = glCreateShader(GL_VERTEX_SHADER);
            const char *vertexShaderSource = scene.lights[i].vertexShader.c_str();
            glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
            glCompileShader(vertexShader);
            int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
            const char *fragmentShaderSource = scene.lights[i].fragmentShader.c_str();
            glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
            glCompileShader(fragmentShader);
            scene.lights[i].shader = glCreateProgram();
            glAttachShader(scene.lights[i].shader, vertexShader);
            glAttachShader(scene.lights[i].shader, fragmentShader);
            glLinkProgram(scene.lights[i].shader);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
        }
    }
}

void createBuffers()
{
    for (int i = 0; i < scene.models.size(); i++) {
        glGenVertexArrays(1, &scene.models[i].vao);
        glGenBuffers(1, &scene.models[i].vbo);
        glBindVertexArray(scene.models[i].vao);
        glBindBuffer(GL_ARRAY_BUFFER, scene.models[i].vbo);
        glBufferData(GL_ARRAY_BUFFER, scene.models[i].vertices.size() * sizeof(float), &scene.models[i].vertices[0], GL_DYNAMIC_DRAW);
        int attCount = scene.models[i].material.texture ? 8 : 6;
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, attCount * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, attCount * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        if (scene.models[i].material.texture) {
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, attCount * sizeof(float), (void*)(6 * sizeof(float)));
            glEnableVertexAttribArray(2);
        }
        glBindVertexArray(0);
        
        if (scene.models[i].material.texture) {
            string b64 = "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";
            
            int width, height, nrChannels;
            vector<unsigned char> decoded = base64_decode(b64);
            unsigned char *data = stbi_load_from_memory(&decoded[0], decoded.size(), &width, &height, &nrChannels, 0);
            glGenTextures(1, &scene.models[0].material.diffuseTex);
            glBindTexture(GL_TEXTURE_2D, scene.models[0].material.diffuseTex);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT );
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(data);
            
            glUseProgram(scene.models[0].shader);
            glUniform1i(glGetUniformLocation(scene.models[0].shader, "modelMaterial.diffuseTex"), 0);
            
        }
    }
    for (int i = 0; i < scene.lights.size(); i++) {
        if (scene.lights[i].vertices.size() > 0) {
            glGenVertexArrays(1, &scene.lights[i].vao);
            glGenBuffers(1, &scene.lights[i].vbo);
            glBindVertexArray(scene.lights[i].vao);
            glBindBuffer(GL_ARRAY_BUFFER, scene.lights[i].vbo);
            glBufferData(GL_ARRAY_BUFFER, scene.lights[i].vertices.size() * sizeof(float), &scene.lights[i].vertices[0], GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glBindVertexArray(0);
        }
    }
}

void processDiscreteInput(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    
    if (key == GLFW_KEY_M && action == GLFW_PRESS) {
        if (polygonMode == GL_FILL)
            polygonMode = GL_POINT;
        else if (polygonMode == GL_POINT)
            polygonMode = GL_LINE;
        else if (polygonMode == GL_LINE)
            polygonMode = GL_FILL;
    }
    
    if (key == GLFW_KEY_B && action == GLFW_PRESS) {
        captureScreenshot();
    }
}

void processContinuousInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        glm::vec3 offset = scene.camera.transform.front * scene.camera.moveSpeed;
        scene.camera.transform.position = scene.camera.transform.position + offset;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        glm::vec3 offset = scene.camera.transform.front * scene.camera.moveSpeed;
        scene.camera.transform.position = scene.camera.transform.position - offset;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        glm::vec3 offset = scene.camera.transform.right * scene.camera.moveSpeed;
        scene.camera.transform.position = scene.camera.transform.position - offset;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        glm::vec3 offset = scene.camera.transform.right * scene.camera.moveSpeed;
        scene.camera.transform.position = scene.camera.transform.position + offset;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        glm::vec3 offset = scene.camera.transform.up * scene.camera.moveSpeed;
        scene.camera.transform.position = scene.camera.transform.position + offset;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        glm::vec3 offset = scene.camera.transform.up * scene.camera.moveSpeed;
        scene.camera.transform.position = scene.camera.transform.position - offset;
    }
    
    
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        scene.camera.transform.pitch(0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        scene.camera.transform.pitch(-0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        scene.camera.transform.yaw(0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        scene.camera.transform.yaw(-0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        scene.camera.transform.roll(-0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        scene.camera.transform.roll(0.5f);
    }
    
    
    
    if (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS) {
        scene.models[2].transform.yaw(-1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) {
        scene.models[2].transform.yaw(1.0f);
    }
    
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        glm::vec3 offset = scene.lights[0].transform.front * scene.camera.moveSpeed;
        scene.lights[0].transform.position = scene.lights[0].transform.position - offset;
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        glm::vec3 offset = scene.lights[0].transform.front * scene.camera.moveSpeed;
        scene.lights[0].transform.position = scene.lights[0].transform.position + offset;
    }
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        glm::vec3 offset = scene.lights[0].transform.right * scene.camera.moveSpeed;
        scene.lights[0].transform.position = scene.lights[0].transform.position - offset;
    }
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
        glm::vec3 offset = scene.lights[0].transform.right * scene.camera.moveSpeed;
        scene.lights[0].transform.position = scene.lights[0].transform.position + offset;
    }
    
    
    
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        if (scene.camera.fov < 180.0f)
            scene.camera.fov += 0.5f;
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        if (scene.camera.fov > 0.0f)
            scene.camera.fov -= 0.5f;
    }
}

void resizeFramebuffer(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

int captureScreenshot()
{
    stbi_flip_vertically_on_write(true);
    
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    int x = viewport[0];
    int y = viewport[1];
    int width = viewport[2];
    int height = viewport[3];

    char *data = (char*) malloc((size_t) (width * height * 3)); // 3 components (R, G, B)

    if (!data)
        return 0;

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
    static char basename[30];
    time_t t = time(NULL);
    strftime(basename, 30, "%Y%m%d_%H%M%S.png", localtime(&t));
    int saved = stbi_write_png(basename, width, height, 3, data, 0);

    free(data);

    return saved;
}



vector<unsigned char> base64_decode(string const& encoded_string) {
    string base64_chars =
                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                 "abcdefghijklmnopqrstuvwxyz"
                 "0123456789+/";
    
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    vector<unsigned char> ret;
    
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
