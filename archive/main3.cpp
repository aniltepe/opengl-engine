//
//  main3.cpp
//  OpenGLTest4
//
//  Created by Nazım Anıl Tepe on 12.04.2021.
//
// Scene loading from path; with nested Objects and parent-child relationship

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

enum struct ObjectType : int {
    Scene, Model, Light, Camera, Joint, Text
};
enum struct LightType : int {
    point, directional, spotlight
};
struct Transform {
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
};
struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    bool texture;
    unsigned int diffuseTex;
    unsigned int specularTex;
    string diffuseTexBase64;
    string specularTexBase64;
    float shininess;
};
struct Layout {
    float x;
    float y;
    int width;
    int height;
};
struct Shader {
    vector<float> vertices;
    vector<unsigned int> faces;
    int vertexCount;
    int faceCount;
    unsigned int vao;
    unsigned int vbo;
    unsigned int ebo;
    int shaderID;
    string vertexShader;
    string fragmentShader;
};
struct Light {
    LightType lightType;
    float constant;
    float linear;
    float quadratic;
    float cutOff;
    float outerCutOff;
};
struct Camera {
    float fov;
    float minDistance;
    float maxDistance;
    float moveSpeed;
};
struct Object {
    ObjectType type;
    string name;
    unsigned int index;
    map<string, string> dictionary;
    Object* superObject = NULL;
    vector<Object*> subObjects;
    Object* objectPtr = NULL;
    Shader shader;
    Light light;
    Camera camera;
    Material material;
    Transform transform;
    Layout layout;
    string additionalInfo;
};
struct Scene : Object {
    Object* cameraObj;
    vector<Object*> modelObjs;
    vector<Object*> lightObjs;
};
struct Character {
    unsigned int TextureID;
    glm::ivec2   Size;
    glm::ivec2   Bearing;
    unsigned int Advance;
};

int objIndex = 0;
Scene scene;
vector<Object> objects;
map<GLchar, Character> characters;

void createScene(string path);
Object* processObject(vector<string> rows, string name);
void processObjectTypes(Object* objPtr);
void createShaders();
void createBuffers();
void processDiscreteInput(GLFWwindow* window, int key, int scancode, int action, int mods);
void processContinuousInput(GLFWwindow* window);
void resizeFramebuffer(GLFWwindow* window, int width, int height);
int captureScreenshot();
vector<unsigned char> base64_decode(string const& encoded_string);
glm::vec3 rotateVectorAroundAxis(glm::vec3 vector, glm::vec3 axis, float angle);

template <class T>
vector<T> processAttributeArray(string s) {
    vector<T> values;
    long space = 0;
    string delimiter = " ";
    while ((space = s.find(delimiter)) != string::npos) {
        string next = s.substr(0, space);
        if (next != "")
            values.push_back(strcmp(typeid(T).name(), "unsigned int") == 1 ? stoi(next) : stof(next));
        s.erase(0, space + delimiter.length());
    }
    if (s != "")
        values.push_back(strcmp(typeid(T).name(), "unsigned int") == 1 ? stoi(s) : stof(s));
    return values;
}

unsigned int polygonMode = GL_FILL;

glm::mat4 projection;
glm::mat4 view;
glm::mat4 model;

float lastFrame = 0.0f;
int frameCount = 0;
int fps = 0;

int main()
{
    createScene("/Users/nazimaniltepe/Documents/Projects/opengl-nscene/OpenGLTest4/scene2.sce");
    
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(scene.Object::layout.width, scene.Object::layout.height, "OpenGL", NULL, NULL);
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

        glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glPolygonMode(GL_FRONT_AND_BACK, polygonMode);

        processContinuousInput(window);

        projection = glm::perspective(glm::radians(scene.cameraObj->camera.fov), (float)scene.Object::layout.width / (float)scene.Object::layout.height, scene.cameraObj->camera.minDistance, scene.cameraObj->camera.maxDistance);
        view = lookAt(scene.cameraObj->transform.position, scene.cameraObj->transform.position + scene.cameraObj->transform.front, scene.cameraObj->transform.up);

        for (int i = 0; i < scene.modelObjs.size(); i++) {
            glUseProgram(scene.modelObjs[i]->shader.shaderID);
            glUniformMatrix4fv(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));
            glUniformMatrix4fv(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, "view"), 1, GL_FALSE, value_ptr(view));
            model = glm::translate(glm::mat4(1.0f), scene.modelObjs[i]->transform.position);
            model = glm::scale(model, scene.modelObjs[i]->transform.scale);
            glm::mat4 rotation = glm::mat4(scene.modelObjs[i]->transform.right.x, scene.modelObjs[i]->transform.right.y, scene.modelObjs[i]->transform.right.z, 0,
                              scene.modelObjs[i]->transform.up.x, scene.modelObjs[i]->transform.up.y, scene.modelObjs[i]->transform.up.z, 0,
                              scene.modelObjs[i]->transform.front.x, scene.modelObjs[i]->transform.front.y, scene.modelObjs[i]->transform.front.z, 0,
                              0, 0, 0, 1);
            model *= rotation;
            glUniformMatrix4fv(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, "model"), 1, GL_FALSE,  value_ptr(model));
            glUniform3fv(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, "cameraPos"), 1, value_ptr(scene.cameraObj->transform.position));

            glUniform3fv(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, "modelMaterial.ambient"), 1, value_ptr(scene.modelObjs[i]->material.ambient));
            glUniform3fv(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, "modelMaterial.diffuse"), 1, value_ptr(scene.modelObjs[i]->material.diffuse));
            glUniform3fv(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, "modelMaterial.specular"), 1, value_ptr(scene.modelObjs[i]->material.specular));
            glUniform1i(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, "modelMaterial.texture"), (int)scene.modelObjs[i]->material.texture);
            glUniform1f(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, "modelMaterial.shininess"), scene.modelObjs[i]->material.shininess);

            if (scene.modelObjs[i]->material.texture) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, scene.modelObjs[i]->material.diffuseTex);
                if (scene.modelObjs[i]->material.specularTexBase64 != "") {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, scene.modelObjs[i]->material.specularTex);
                }
            }

            for (int j = 0; j < scene.lightObjs.size(); j++) {
                glUniform1i(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, ("lights[" + to_string(j) + "].lightType").c_str()), static_cast<int>(scene.lightObjs[j]->light.lightType));
                glUniform3fv(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, ("lights[" + to_string(j) + "].direction").c_str()), 1, value_ptr(scene.lightObjs[j]->transform.front));
                glUniform3fv(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, ("lights[" + to_string(j) + "].position").c_str()), 1, value_ptr(scene.lightObjs[j]->transform.position));
                glUniform3fv(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, "modelMaterial.specular"), 1, value_ptr(scene.modelObjs[i]->material.specular));
                glUniform1f(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, ("lights[" + to_string(j) + "].constant").c_str()), scene.lightObjs[j]->light.constant);
                glUniform1f(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, ("lights[" + to_string(j) + "].linear").c_str()), scene.lightObjs[j]->light.linear);
                glUniform1f(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, ("lights[" + to_string(j) + "].quadratic").c_str()), scene.lightObjs[j]->light.quadratic);
                glUniform1f(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, ("lights[" + to_string(j) + "].cutOff").c_str()), scene.lightObjs[j]->light.cutOff);
                glUniform1f(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, ("lights[" + to_string(j) + "].outerCutOff").c_str()), scene.lightObjs[j]->light.outerCutOff);
                glUniform3fv(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, ("lights[" + to_string(j) + "].material.ambient").c_str()), 1, value_ptr(scene.lightObjs[j]->material.ambient));
                glUniform3fv(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, ("lights[" + to_string(j) + "].material.diffuse").c_str()), 1, value_ptr(scene.lightObjs[j]->material.diffuse));
                glUniform3fv(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, ("lights[" + to_string(j) + "].material.specular").c_str()), 1, value_ptr(scene.lightObjs[j]->material.specular));
            }
            glBindVertexArray(scene.modelObjs[i]->shader.vao);
            if (scene.modelObjs[i]->shader.faces.size() > 0)
                glDrawElements(GL_TRIANGLES, scene.modelObjs[i]->shader.vertexCount, GL_UNSIGNED_INT, 0);
            else
                glDrawArrays(GL_TRIANGLES, 0, scene.modelObjs[i]->shader.vertexCount);
        }
        for (int i = 0; i < scene.lightObjs.size(); i++) {
            if (scene.lightObjs[i]->shader.vertices.size() > 0) {
                glUseProgram(scene.lightObjs[i]->shader.shaderID);
                glUniformMatrix4fv(glGetUniformLocation(scene.lightObjs[i]->shader.shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));
                glUniformMatrix4fv(glGetUniformLocation(scene.lightObjs[i]->shader.shaderID, "view"), 1, GL_FALSE, value_ptr(view));
                model = glm::translate(glm::mat4(1.0f), scene.lightObjs[i]->transform.position);
                model = glm::scale(model, scene.lightObjs[i]->transform.scale);
                glm::mat4 rotation = glm::mat4(scene.lightObjs[i]->transform.right.x, scene.lightObjs[i]->transform.right.y, scene.lightObjs[i]->transform.right.z, 0,
                                  scene.lightObjs[i]->transform.up.x, scene.lightObjs[i]->transform.up.y, scene.lightObjs[i]->transform.up.z, 0,
                                  scene.lightObjs[i]->transform.front.x, scene.lightObjs[i]->transform.front.y, scene.lightObjs[i]->transform.front.z, 0,
                                  0, 0, 0, 1);
                model *= rotation;
                glUniformMatrix4fv(glGetUniformLocation(scene.lightObjs[i]->shader.shaderID, "model"), 1, GL_FALSE,  value_ptr(model));
                glUniform3fv(glGetUniformLocation(scene.lightObjs[i]->shader.shaderID, "color"), 1, value_ptr(scene.lightObjs[i]->material.diffuse / 0.8f));
                glBindVertexArray(scene.lightObjs[i]->shader.vao);
                glDrawArrays(GL_TRIANGLES, 0, scene.lightObjs[i]->shader.vertexCount);
            }
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    for (int i = 0; i < scene.modelObjs.size(); i++) {
        glDeleteProgram(scene.modelObjs[i]->shader.shaderID);
        glDeleteBuffers(1, &scene.modelObjs[i]->shader.vbo);
        glDeleteVertexArrays(1, &scene.modelObjs[i]->shader.vao);
    }
    for (int i = 0; i < scene.lightObjs.size(); i++) {
        if (scene.lightObjs[i]->shader.vertices.size() > 0) {
            glDeleteProgram(scene.lightObjs[i]->shader.shaderID);
            glDeleteBuffers(1, &scene.lightObjs[i]->shader.vbo);
            glDeleteVertexArrays(1, &scene.lightObjs[i]->shader.vao);
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
    
    Object* sceneObjPtr = processObject(rows, "TestScene");
    scene.Object::operator=(*sceneObjPtr);
    for (int i = int(scene.Object::subObjects.size()) - 1; i >= 0; i--) {
        if (scene.Object::subObjects[i]->type == ObjectType::Model) {
            scene.modelObjs.push_back(scene.Object::subObjects[i]);
            scene.Object::subObjects.erase(scene.Object::subObjects.begin() + i);
        }
        else if (scene.Object::subObjects[i]->type == ObjectType::Light) {
            scene.lightObjs.push_back(scene.Object::subObjects[i]);
            scene.Object::subObjects.erase(scene.Object::subObjects.begin() + i);
        }
        else if (scene.Object::subObjects[i]->type == ObjectType::Camera) {
            scene.cameraObj = scene.Object::subObjects[i];
            scene.Object::subObjects.erase(scene.Object::subObjects.begin() + i);
        }
    }
}

Object* processObject(vector<string> rows, string name)
{
    Object* objPtr = new Object();
    objPtr->objectPtr = objPtr;
    objPtr->index = objIndex++;
    objPtr->name = name;
    for (int i = 0; i < rows.size(); i++) {
        if (rows[i].find(":") != string::npos) {
            string pairKey = rows[i].substr(0, rows[i].find(":"));
            pairKey.erase(pairKey.begin(), find_if(pairKey.begin(), pairKey.end(), [](unsigned char c) {
                return !isspace(c);
            }));
            string pairValue = rows[i].substr(rows[i].find(":") + 1);
            pairValue.erase(pairValue.begin(), find_if(pairValue.begin(), pairValue.end(), [](unsigned char c) {
                return !isspace(c);
            }));
            objPtr->dictionary.insert(pair<string, string>(pairKey, pairValue));
        }
        else {
            vector<string>::const_iterator itr;
            function<bool(string)> endsWith = [&](string s) {
                rows[i].erase(rows[i].begin(), find_if(rows[i].begin(), rows[i].end(), [](unsigned char ch) { return !isspace(ch); }));
                string word = "/" + rows[i];
                if (s.length() >= word.length())
                    return (0 == s.compare(s.length() - word.length(), word.length(), word));
                else
                    return false;
            };
            itr = find_if(rows.begin() + i, rows.end(), endsWith);
            if (itr != rows.end()) {
                vector<string>::const_iterator first = rows.begin() + i + 1;
                vector<string>::const_iterator last = rows.begin() + (itr - rows.begin());
                vector<string> newRows(first, last);
                Object* subObjPtr = processObject(newRows, rows[i]);
                subObjPtr->superObject = objPtr;
                objPtr->subObjects.push_back(subObjPtr);
                i = int(itr - rows.begin());
            }
        }
    }
    processObjectTypes(objPtr);
    objects.push_back(*objPtr);
    return objPtr;
}

void processObjectTypes(Object* objPtr)
{
    map<string, string>::iterator itr;
    for (const auto &entry : objPtr->dictionary) {
        if (entry.first == "type")
            objPtr->type = static_cast<ObjectType>(stoi(entry.second));
        else if (entry.first == "ltyp")
            objPtr->light.lightType = static_cast<LightType>(stoi(entry.second));
        else if (entry.first == "wd")
            objPtr->layout.width = stoi(entry.second);
        else if (entry.first == "hg")
            objPtr->layout.height = stoi(entry.second);
        else if (entry.first == "cnst")
            objPtr->light.constant = stof(entry.second);
        else if (entry.first == "lnr")
            objPtr->light.linear = stof(entry.second);
        else if (entry.first == "quad")
            objPtr->light.quadratic = stof(entry.second);
        else if (entry.first == "cut")
            objPtr->light.cutOff = stof(entry.second);
        else if (entry.first == "ocut")
            objPtr->light.outerCutOff = stof(entry.second);
        else if (entry.first == "fov")
            objPtr->camera.fov = stof(entry.second);
        else if (entry.first == "mind")
            objPtr->camera.minDistance = stof(entry.second);
        else if (entry.first == "maxd")
            objPtr->camera.maxDistance = stof(entry.second);
        else if (entry.first == "mvsp")
            objPtr->camera.moveSpeed = stof(entry.second);
        else if (entry.first == "mtdf")
            objPtr->material.diffuseTexBase64 = entry.second;
        else if (entry.first == "mtsp")
            objPtr->material.specularTexBase64 = entry.second;
        else if (entry.first == "v")
            objPtr->shader.vertices = processAttributeArray<float>(entry.second);
        else if (entry.first == "f")
            objPtr->shader.faces = processAttributeArray<unsigned int>(entry.second);
        else if (entry.first == "mtrl") {
            vector<float> sequence = processAttributeArray<float>(entry.second);
            objPtr->material.texture = sequence[0] == 0.0 ? false : true;
            objPtr->material.ambient = glm::vec3(sequence[1], sequence[2], sequence[3]);
            objPtr->material.diffuse = glm::vec3(sequence[4], sequence[5], sequence[6]);
            objPtr->material.specular = glm::vec3(sequence[7], sequence[8], sequence[9]);
            objPtr->material.shininess = sequence[10];
        }
        else if (entry.first == "trns") {
            vector<float> sequence = processAttributeArray<float>(entry.second);
            objPtr->transform.position = glm::vec3(sequence[0], sequence[1], sequence[2]);
            objPtr->transform.scale = glm::vec3(sequence[3], sequence[4], sequence[5]);
            objPtr->transform.front = glm::vec3(sequence[6], sequence[7], sequence[8]);
            objPtr->transform.up = glm::vec3(sequence[9], sequence[10], sequence[11]);
            objPtr->transform.right = glm::vec3(sequence[12], sequence[13], sequence[14]);
        }
    }
}

void createShaders()
{
    for (int i = 0; i < scene.modelObjs.size(); i++) {
        scene.modelObjs[i]->shader.vertexShader = "#version 330 core\n";
        scene.modelObjs[i]->shader.vertexShader += "layout(location = 0) in vec3 vPos;\n";
        scene.modelObjs[i]->shader.vertexShader += "layout(location = 1) in vec3 vNormal;\n";
        scene.modelObjs[i]->shader.vertexShader += (scene.modelObjs[i]->material.texture) ? "layout(location = 2) in vec2 vTexCoord;\n" : "";
        scene.modelObjs[i]->shader.vertexShader += "out vec3 FragPos;\n";
        scene.modelObjs[i]->shader.vertexShader += "out vec3 Normal;\n";
        scene.modelObjs[i]->shader.vertexShader += (scene.modelObjs[i]->material.texture) ? "out vec2 TexCoord;\n" : "";
        scene.modelObjs[i]->shader.vertexShader += "uniform mat4 model;\n";
        scene.modelObjs[i]->shader.vertexShader += "uniform mat4 view;\n";
        scene.modelObjs[i]->shader.vertexShader += "uniform mat4 projection;\n";
        scene.modelObjs[i]->shader.vertexShader += "void main() {\n";
        scene.modelObjs[i]->shader.vertexShader += "gl_Position = projection * view * model * vec4(vPos, 1.0f);\n";
        scene.modelObjs[i]->shader.vertexShader += "FragPos = vec3(model * vec4(vPos, 1.0f));\n";
        scene.modelObjs[i]->shader.vertexShader += "Normal = vNormal;\n";
        scene.modelObjs[i]->shader.vertexShader += (scene.modelObjs[i]->material.texture) ? "TexCoord = vTexCoord;\n" : "";
        scene.modelObjs[i]->shader.vertexShader += "}\0";

        scene.modelObjs[i]->shader.fragmentShader = "#version 330 core\n";
        scene.modelObjs[i]->shader.fragmentShader += "out vec4 FragColor;\n";
        scene.modelObjs[i]->shader.fragmentShader += "in vec3 FragPos;\n";
        scene.modelObjs[i]->shader.fragmentShader += "in vec3 Normal;\n";
        scene.modelObjs[i]->shader.fragmentShader += (scene.modelObjs[i]->material.texture) ? "in vec2 TexCoord;\n" : "";
        scene.modelObjs[i]->shader.fragmentShader += "struct Material {\n";
        scene.modelObjs[i]->shader.fragmentShader += "vec3 ambient;\n";
        scene.modelObjs[i]->shader.fragmentShader += "vec3 diffuse;\n";
        scene.modelObjs[i]->shader.fragmentShader += "vec3 specular;\n";
        scene.modelObjs[i]->shader.fragmentShader += "bool texture;\n";
        scene.modelObjs[i]->shader.fragmentShader += "sampler2D diffuseTex;\n";
        scene.modelObjs[i]->shader.fragmentShader += "sampler2D specularTex;\n";
        scene.modelObjs[i]->shader.fragmentShader += "float shininess;\n";
        scene.modelObjs[i]->shader.fragmentShader += "};\n";
        scene.modelObjs[i]->shader.fragmentShader += "struct Light {\n";
        scene.modelObjs[i]->shader.fragmentShader += "int lightType;\n";
        scene.modelObjs[i]->shader.fragmentShader += "vec3 direction;\n";
        scene.modelObjs[i]->shader.fragmentShader += "vec3 position;\n";
        scene.modelObjs[i]->shader.fragmentShader += "float constant;\n";
        scene.modelObjs[i]->shader.fragmentShader += "float linear;\n";
        scene.modelObjs[i]->shader.fragmentShader += "float quadratic;\n";
        scene.modelObjs[i]->shader.fragmentShader += "float cutOff;\n";
        scene.modelObjs[i]->shader.fragmentShader += "float outerCutOff;\n";
        scene.modelObjs[i]->shader.fragmentShader += "Material material;\n";
        scene.modelObjs[i]->shader.fragmentShader += "};\n";
        scene.modelObjs[i]->shader.fragmentShader += "uniform vec3 cameraPos;\n";
        scene.modelObjs[i]->shader.fragmentShader += "uniform Material modelMaterial;\n";
        scene.modelObjs[i]->shader.fragmentShader += "uniform Light lights[" + to_string(scene.lightObjs.size()) + "];\n";
        scene.modelObjs[i]->shader.fragmentShader += "vec3 CalculateLight(Light light, vec3 normal, vec3 viewDir, vec3 fragPos);\n";
        scene.modelObjs[i]->shader.fragmentShader += "void main() {\n";
        scene.modelObjs[i]->shader.fragmentShader += "vec3 norm = normalize(Normal);\n";
        scene.modelObjs[i]->shader.fragmentShader += "vec3 viewDir = normalize(cameraPos - FragPos);\n";
        scene.modelObjs[i]->shader.fragmentShader += "vec3 result = vec3(0.0f);\n";
        scene.modelObjs[i]->shader.fragmentShader += "for(int i = 0; i < lights.length(); i++)\n";
        scene.modelObjs[i]->shader.fragmentShader += "result += CalculateLight(lights[i], norm, viewDir, FragPos);\n";
        scene.modelObjs[i]->shader.fragmentShader += "FragColor = vec4(result, 1.0f);\n";
        scene.modelObjs[i]->shader.fragmentShader += "}\n";
        scene.modelObjs[i]->shader.fragmentShader += "vec3 CalculateLight(Light light, vec3 normal, vec3 viewDir, vec3 fragPos) {\n";
        scene.modelObjs[i]->shader.fragmentShader += "vec3 lightDir = normalize(light.position - fragPos);\n";
        scene.modelObjs[i]->shader.fragmentShader += "if (light.lightType == 1)\n";
        scene.modelObjs[i]->shader.fragmentShader += "lightDir = normalize(-light.direction);\n";
        scene.modelObjs[i]->shader.fragmentShader += "float diffStrength = max(dot(normal, lightDir), 0.0);\n";
        scene.modelObjs[i]->shader.fragmentShader += "vec3 reflectDir = reflect(-lightDir, normal);\n";
        scene.modelObjs[i]->shader.fragmentShader += "float specStrength = pow(max(dot(viewDir, reflectDir), 0.0), modelMaterial.shininess);\n";
        scene.modelObjs[i]->shader.fragmentShader += "vec3 ambient = light.material.ambient * modelMaterial.ambient;\n";
        scene.modelObjs[i]->shader.fragmentShader += "vec3 diffuse = light.material.diffuse * diffStrength * modelMaterial.diffuse;\n";
        scene.modelObjs[i]->shader.fragmentShader += "vec3 specular = light.material.specular * specStrength * modelMaterial.specular;\n";
        scene.modelObjs[i]->shader.fragmentShader += (scene.modelObjs[i]->material.texture) ? "ambient = light.material.ambient * vec3(texture(modelMaterial.diffuseTex, TexCoord)) * modelMaterial.diffuse;\n" : "";
        scene.modelObjs[i]->shader.fragmentShader += (scene.modelObjs[i]->material.texture) ? "diffuse = light.material.diffuse * diffStrength * vec3(texture(modelMaterial.diffuseTex, TexCoord)) * modelMaterial.diffuse;\n" : "";
        scene.modelObjs[i]->shader.fragmentShader += (scene.modelObjs[i]->material.texture) ? ((scene.modelObjs[i]->material.specularTexBase64 != "") ? "specular = light.material.specular * specStrength * vec3(texture(modelMaterial.specularTex, TexCoord));\n" : "") : "";
        scene.modelObjs[i]->shader.fragmentShader += "if (light.lightType != 1) {\n";
        scene.modelObjs[i]->shader.fragmentShader += "float distance = length(light.position - fragPos);\n";
        scene.modelObjs[i]->shader.fragmentShader += "float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));\n";
        scene.modelObjs[i]->shader.fragmentShader += "ambient *= attenuation;\n";
        scene.modelObjs[i]->shader.fragmentShader += "diffuse *= attenuation;\n";
        scene.modelObjs[i]->shader.fragmentShader += "specular *= attenuation;\n";
        scene.modelObjs[i]->shader.fragmentShader += "if (light.lightType == 2) {\n";
        scene.modelObjs[i]->shader.fragmentShader += "float theta = dot(lightDir, normalize(-light.direction));\n";
        scene.modelObjs[i]->shader.fragmentShader += "float epsilon = light.cutOff - light.outerCutOff;\n";
        scene.modelObjs[i]->shader.fragmentShader += "float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);\n";
        scene.modelObjs[i]->shader.fragmentShader += "ambient *= intensity;\n";
        scene.modelObjs[i]->shader.fragmentShader += "diffuse *= intensity;\n";
        scene.modelObjs[i]->shader.fragmentShader += "specular *= intensity;\n";
        scene.modelObjs[i]->shader.fragmentShader += "}\n";
        scene.modelObjs[i]->shader.fragmentShader += "}\n";
        scene.modelObjs[i]->shader.fragmentShader += "return (ambient + diffuse + specular);\n";
        scene.modelObjs[i]->shader.fragmentShader += "}\0";
        
        int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        const char *vertexShaderSource = scene.modelObjs[i]->shader.vertexShader.c_str();
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        const char *fragmentShaderSource = scene.modelObjs[i]->shader.fragmentShader.c_str();
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        scene.modelObjs[i]->shader.shaderID = glCreateProgram();
        glAttachShader(scene.modelObjs[i]->shader.shaderID, vertexShader);
        glAttachShader(scene.modelObjs[i]->shader.shaderID, fragmentShader);
        glLinkProgram(scene.modelObjs[i]->shader.shaderID);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }
    
    for (int i = 0; i < scene.lightObjs.size(); i++) {
        if (scene.lightObjs[i]->shader.vertices.size() > 0) {
            scene.lightObjs[i]->shader.vertexShader = "#version 330 core\n";
            scene.lightObjs[i]->shader.vertexShader += "layout(location = 0) in vec3 vPos;\n";
            scene.lightObjs[i]->shader.vertexShader += "uniform mat4 model;\n";
            scene.lightObjs[i]->shader.vertexShader += "uniform mat4 view;\n";
            scene.lightObjs[i]->shader.vertexShader += "uniform mat4 projection;\n";
            scene.lightObjs[i]->shader.vertexShader += "void main() {\n";
            scene.lightObjs[i]->shader.vertexShader += "gl_Position = projection * view * model * vec4(vPos, 1.0f);\n";
            scene.lightObjs[i]->shader.vertexShader += "}\0";

            scene.lightObjs[i]->shader.fragmentShader = "#version 330 core\n";
            scene.lightObjs[i]->shader.fragmentShader += "out vec4 FragColor;\n";
            scene.lightObjs[i]->shader.fragmentShader += "uniform vec3 color;\n";
            scene.lightObjs[i]->shader.fragmentShader += "void main() {\n";
            scene.lightObjs[i]->shader.fragmentShader += "FragColor = vec4(color, 1.0f);\n";
            scene.lightObjs[i]->shader.fragmentShader += "}\0";

            int vertexShader = glCreateShader(GL_VERTEX_SHADER);
            const char *vertexShaderSource = scene.lightObjs[i]->shader.vertexShader.c_str();
            glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
            glCompileShader(vertexShader);
            int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
            const char *fragmentShaderSource = scene.lightObjs[i]->shader.fragmentShader.c_str();
            glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
            glCompileShader(fragmentShader);
            scene.lightObjs[i]->shader.shaderID = glCreateProgram();
            glAttachShader(scene.lightObjs[i]->shader.shaderID, vertexShader);
            glAttachShader(scene.lightObjs[i]->shader.shaderID, fragmentShader);
            glLinkProgram(scene.lightObjs[i]->shader.shaderID);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
        }
    }
}

void createBuffers()
{
    for (int i = 0; i < scene.modelObjs.size(); i++) {
        glGenVertexArrays(1, &scene.modelObjs[i]->shader.vao);
        glGenBuffers(1, &scene.modelObjs[i]->shader.vbo);
        if (scene.modelObjs[i]->shader.faces.size() > 0)
            glGenBuffers(1, &scene.modelObjs[i]->shader.ebo);
        glBindVertexArray(scene.modelObjs[i]->shader.vao);
        glBindBuffer(GL_ARRAY_BUFFER, scene.modelObjs[i]->shader.vbo);
        glBufferData(GL_ARRAY_BUFFER, scene.modelObjs[i]->shader.vertices.size() * sizeof(float), &scene.modelObjs[i]->shader.vertices[0], GL_DYNAMIC_DRAW);
        if (scene.modelObjs[i]->shader.faces.size() > 0) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, scene.modelObjs[i]->shader.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, scene.modelObjs[i]->shader.faces.size() * sizeof(float), &scene.modelObjs[i]->shader.faces[0], GL_DYNAMIC_DRAW);
        }
        int attCount = scene.modelObjs[i]->material.texture ? 8 : 6;
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, attCount * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, attCount * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        if (scene.modelObjs[i]->material.texture) {
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, attCount * sizeof(float), (void*)(6 * sizeof(float)));
            glEnableVertexAttribArray(2);
        }
        glBindVertexArray(0);

        if (scene.modelObjs[i]->material.texture) {
            int width, height, nrChannels;
            vector<unsigned char> decoded = base64_decode(scene.modelObjs[i]->material.diffuseTexBase64);
            unsigned char *data = stbi_load_from_memory(&decoded[0], decoded.size(), &width, &height, &nrChannels, 0);
            glGenTextures(1, &scene.modelObjs[i]->material.diffuseTex);
            glBindTexture(GL_TEXTURE_2D, scene.modelObjs[i]->material.diffuseTex);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(data);

            if (scene.modelObjs[i]->material.specularTexBase64 != "") {
                vector<unsigned char> decoded_ = base64_decode(scene.modelObjs[i]->material.specularTexBase64);
                unsigned char *data_ = stbi_load_from_memory(&decoded_[0], decoded_.size(), &width, &height, &nrChannels, 0);
                glGenTextures(1, &scene.modelObjs[i]->material.specularTex);
                glBindTexture(GL_TEXTURE_2D, scene.modelObjs[i]->material.specularTex);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_);
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
                glGenerateMipmap(GL_TEXTURE_2D);
                stbi_image_free(data_);
            }

            glUseProgram(scene.modelObjs[i]->shader.shaderID);
            glUniform1i(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, "modelMaterial.diffuseTex"), 0);
            if (scene.modelObjs[i]->material.specularTexBase64 != "")
                glUniform1i(glGetUniformLocation(scene.modelObjs[i]->shader.shaderID, "modelMaterial.specularTex"), 1);
        }
        
        if (scene.modelObjs[i]->shader.faces.size() > 0)
            scene.modelObjs[i]->shader.vertexCount = scene.modelObjs[i]->shader.faces.size();
        else
            scene.modelObjs[i]->shader.vertexCount = scene.modelObjs[i]->material.texture ? scene.modelObjs[i]->shader.vertices.size() / 8 : scene.modelObjs[i]->shader.vertices.size() / 6;
    }
    for (int i = 0; i < scene.lightObjs.size(); i++) {
        if (scene.lightObjs[i]->shader.vertices.size() > 0) {
            glGenVertexArrays(1, &scene.lightObjs[i]->shader.vao);
            glGenBuffers(1, &scene.lightObjs[i]->shader.vbo);
            glBindVertexArray(scene.lightObjs[i]->shader.vao);
            glBindBuffer(GL_ARRAY_BUFFER, scene.lightObjs[i]->shader.vbo);
            glBufferData(GL_ARRAY_BUFFER, scene.lightObjs[i]->shader.vertices.size() * sizeof(float), &scene.lightObjs[i]->shader.vertices[0], GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glBindVertexArray(0);
            
            scene.lightObjs[i]->shader.vertexCount = scene.lightObjs[i]->shader.vertices.size() / 3;
        }
    }
    for (int i = 0; i < scene.joints.size(); i++) {
        glGenVertexArrays(1, &scene.joints[i].vao);
        glGenBuffers(1, &scene.joints[i].vbo);
        glBindVertexArray(scene.joints[i].vao);
        glBindBuffer(GL_ARRAY_BUFFER, scene.joints[i].vbo);
        glBufferData(GL_ARRAY_BUFFER, scene.joints[i].vertices.size() * sizeof(float), &scene.joints[i].vertices[0], GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
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
        glm::vec3 offset = scene.cameraObj->transform.front * scene.cameraObj->camera.moveSpeed;
        scene.cameraObj->transform.position = scene.cameraObj->transform.position + offset;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        glm::vec3 offset = scene.cameraObj->transform.front * scene.cameraObj->camera.moveSpeed;
        scene.cameraObj->transform.position = scene.cameraObj->transform.position - offset;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        glm::vec3 offset = scene.cameraObj->transform.right * scene.cameraObj->camera.moveSpeed;
        scene.cameraObj->transform.position = scene.cameraObj->transform.position - offset;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        glm::vec3 offset = scene.cameraObj->transform.right * scene.cameraObj->camera.moveSpeed;
        scene.cameraObj->transform.position = scene.cameraObj->transform.position + offset;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        glm::vec3 offset = scene.cameraObj->transform.up * scene.cameraObj->camera.moveSpeed;
        scene.cameraObj->transform.position = scene.cameraObj->transform.position + offset;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        glm::vec3 offset = scene.cameraObj->transform.up * scene.cameraObj->camera.moveSpeed;
        scene.cameraObj->transform.position = scene.cameraObj->transform.position - offset;
    }


    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        scene.cameraObj->transform.up = rotateVectorAroundAxis(scene.cameraObj->transform.up, scene.cameraObj->transform.right, 0.5f);
        scene.cameraObj->transform.front = rotateVectorAroundAxis(scene.cameraObj->transform.front, scene.cameraObj->transform.right, 0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        scene.cameraObj->transform.up = rotateVectorAroundAxis(scene.cameraObj->transform.up, scene.cameraObj->transform.right, -0.5f);
        scene.cameraObj->transform.front = rotateVectorAroundAxis(scene.cameraObj->transform.front, scene.cameraObj->transform.right, -0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        scene.cameraObj->transform.front = rotateVectorAroundAxis(scene.cameraObj->transform.front, scene.cameraObj->transform.up, 0.5f);
        scene.cameraObj->transform.right = rotateVectorAroundAxis(scene.cameraObj->transform.right, scene.cameraObj->transform.up, 0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        scene.cameraObj->transform.front = rotateVectorAroundAxis(scene.cameraObj->transform.front, scene.cameraObj->transform.up, -0.5f);
        scene.cameraObj->transform.right = rotateVectorAroundAxis(scene.cameraObj->transform.right, scene.cameraObj->transform.up, -0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        scene.cameraObj->transform.right = rotateVectorAroundAxis(scene.cameraObj->transform.right, scene.cameraObj->transform.front, -0.5f);
        scene.cameraObj->transform.up = rotateVectorAroundAxis(scene.cameraObj->transform.up, scene.cameraObj->transform.front, -0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        scene.cameraObj->transform.right = rotateVectorAroundAxis(scene.cameraObj->transform.right, scene.cameraObj->transform.front, 0.5f);
        scene.cameraObj->transform.up = rotateVectorAroundAxis(scene.cameraObj->transform.up, scene.cameraObj->transform.front, 0.5f);
    }

//    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
//        int selectedLine = 29;
//        glm::vec3 axis = glm::vec3(0.0f, 0.0f, 1.0f);
//        float angle = 0.1f;
//        glm::vec3 lineBegin = glm::vec3(scene.joints[0].vertices[(selectedLine - 1) * 6],
//                                     scene.joints[0].vertices[(selectedLine - 1) * 6 + 1],
//                                        scene.joints[0].vertices[(selectedLine - 1) * 6 + 2]);
//        glm::vec3 lineEnd = glm::vec3(scene.joints[0].vertices[(selectedLine - 1) * 6 + 3],
//                                     scene.joints[0].vertices[(selectedLine - 1) * 6 + 4],
//                                        scene.joints[0].vertices[(selectedLine - 1) * 6 + 5]);
//        glm::vec3 vector = lineEnd - lineBegin;
//        glm::vec3 rotated = vector * cos(glm::radians(angle)) + cross(axis, vector) * sin(glm::radians(angle)) + axis * dot(axis, vector) * (1.0f - cos(glm::radians(angle)));
//        glm::vec3 newEnd = lineBegin + rotated;
//        scene.joints[0].vertices[(selectedLine - 1) * 6 + 3] = newEnd.x;
//        scene.joints[0].vertices[(selectedLine - 1) * 6 + 4] = newEnd.y;
//        scene.joints[0].vertices[(selectedLine - 1) * 6 + 5] = newEnd.z;
//        glBindBuffer(GL_ARRAY_BUFFER, scene.joints[0].vbo);
//        glBufferData(GL_ARRAY_BUFFER, scene.joints[0].vertices.size() * sizeof(float), &scene.joints[0].vertices[0], GL_DYNAMIC_DRAW);
//    }
//    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
//        int selectedLine = 29;
//        glm::vec3 axis = glm::vec3(0.0f, 0.0f, 1.0f);
//        float angle = -0.1f;
//        glm::vec3 lineBegin = glm::vec3(scene.joints[0].vertices[(selectedLine - 1) * 6],
//                                     scene.joints[0].vertices[(selectedLine - 1) * 6 + 1],
//                                        scene.joints[0].vertices[(selectedLine - 1) * 6 + 2]);
//        glm::vec3 lineEnd = glm::vec3(scene.joints[0].vertices[(selectedLine - 1) * 6 + 3],
//                                     scene.joints[0].vertices[(selectedLine - 1) * 6 + 4],
//                                        scene.joints[0].vertices[(selectedLine - 1) * 6 + 5]);
//        glm::vec3 vector = lineEnd - lineBegin;
//        glm::vec3 rotated = vector * cos(glm::radians(angle)) + cross(axis, vector) * sin(glm::radians(angle)) + axis * dot(axis, vector) * (1.0f - cos(glm::radians(angle)));
//        glm::vec3 newEnd = lineBegin + rotated;
//        scene.joints[0].vertices[(selectedLine - 1) * 6 + 3] = newEnd.x;
//        scene.joints[0].vertices[(selectedLine - 1) * 6 + 4] = newEnd.y;
//        scene.joints[0].vertices[(selectedLine - 1) * 6 + 5] = newEnd.z;
//        glBindBuffer(GL_ARRAY_BUFFER, scene.joints[0].vbo);
//        glBufferData(GL_ARRAY_BUFFER, scene.joints[0].vertices.size() * sizeof(float), &scene.joints[0].vertices[0], GL_DYNAMIC_DRAW);
//    }



    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        if (scene.cameraObj->camera.fov < 180.0f)
            scene.cameraObj->camera.fov += 0.5f;
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        if (scene.cameraObj->camera.fov > 0.0f)
            scene.cameraObj->camera.fov -= 0.5f;
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

vector<unsigned char> base64_decode(string const& encoded_string)
{
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

glm::vec3 rotateVectorAroundAxis(glm::vec3 vector, glm::vec3 axis, float angle)
{
    return vector * cos(glm::radians(angle)) + cross(axis, vector) * sin(glm::radians(angle)) + axis * dot(axis, vector) * (1.0f - cos(glm::radians(angle)));
}
