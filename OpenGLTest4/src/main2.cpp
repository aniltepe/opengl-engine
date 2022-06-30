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
    string diffuseTexBase64;
    string specularTexBase64;
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
    int vertexCount;
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
    vector<nJoint> joints {};
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
        
        glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
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
                glBindTexture(GL_TEXTURE_2D, scene.models[i].material.diffuseTex);
                if (scene.models[i].material.specularTexBase64 != "") {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, scene.models[i].material.specularTex);
                }
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
        
        for (int i = 0; i < scene.joints.size(); i++) {
            if (scene.joints[i].vertices.size() > 0) {
                glUseProgram(scene.joints[i].shader);
                glUniformMatrix4fv(glGetUniformLocation(scene.joints[i].shader, "projection"), 1, GL_FALSE, value_ptr(projection));
                glUniformMatrix4fv(glGetUniformLocation(scene.joints[i].shader, "view"), 1, GL_FALSE, value_ptr(view));
                model = glm::translate(glm::mat4(1.0f), scene.joints[i].transform.position);
                model = glm::scale(model, scene.joints[i].transform.scale);
                glm::mat4 rotation = glm::mat4(scene.joints[i].transform.right.x, scene.joints[i].transform.right.y, scene.joints[i].transform.right.z, 0,
                                  scene.joints[i].transform.up.x, scene.joints[i].transform.up.y, scene.joints[i].transform.up.z, 0,
                                  scene.joints[i].transform.front.x, scene.joints[i].transform.front.y, scene.joints[i].transform.front.z, 0,
                                  0, 0, 0, 1);
                model *= rotation;
                glUniformMatrix4fv(glGetUniformLocation(scene.joints[i].shader, "model"), 1, GL_FALSE,  value_ptr(model));
                glUniform3fv(glGetUniformLocation(scene.joints[i].shader, "color"), 1, value_ptr(scene.joints[i].material.diffuse));
                glBindVertexArray(scene.joints[i].vao);
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
    for (int i = 0; i < scene.joints.size(); i++) {
        glDeleteProgram(scene.joints[i].shader);
        glDeleteBuffers(1, &scene.joints[i].vbo);
        glDeleteVertexArrays(1, &scene.joints[i].vao);
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
                    
                    if (model.material.texture) {
                        if (dictionary.find("mtdf") != dictionary.end()) {
                            model.material.diffuseTexBase64 = dictionary.at("mtdf");
                        }
                        if (dictionary.find("mtsp") != dictionary.end()) {
                            model.material.specularTexBase64 = dictionary.at("mtsp");
                        }
                    }
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
//                                istringstream is_bool(f);
//                                is_bool >> boolalpha >> light.material.texture;
                                light.material.texture = false;
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
            else if (stoi(dictionary.at("type")) == static_cast<int>(nType::nJoint)) {
                nJoint joint;
                joint.type = nType::nJoint;
                joint.name = rows[i];
                joint.index = index++;
                long space = 0;
                string delimiter = " ";
                string s = dictionary.at("v");
                while ((space = s.find(delimiter)) != string::npos) {
                    string f = s.substr(0, space);
                    if (f != "")
                        joint.vertices.push_back(stof(f));
                    s.erase(0, space + delimiter.length());
                }
                if (s != "")
                    joint.vertices.push_back(stof(s));
                joint.vertexCount = joint.vertices.size() / 3;
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
                    joint.transform.position = glm::vec3(sequence[0], sequence[1], sequence[2]);
                    joint.transform.scale = glm::vec3(sequence[3], sequence[4], sequence[5]);
                    joint.transform.front = glm::vec3(sequence[6], sequence[7], sequence[8]);
                    joint.transform.up = glm::vec3(sequence[9], sequence[10], sequence[11]);
                    joint.transform.right = glm::vec3(sequence[12], sequence[13], sequence[14]);
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
//                                istringstream is_bool(f);
//                                is_bool >> boolalpha >> joint.material.texture;
                                joint.material.texture = false;
                            }
                            else
                                sequence.push_back(stof(f));
                        s.erase(0, space + delimiter.length());
                    }
                    sequence.push_back(stof(s));
                    joint.material.ambient = glm::vec3(sequence[0], sequence[1], sequence[2]);
                    joint.material.diffuse = glm::vec3(sequence[3], sequence[4], sequence[5]);
                    joint.material.specular = glm::vec3(sequence[6], sequence[7], sequence[8]);
                    joint.material.shininess = sequence[9];
                }
                
                scene.joints.push_back(joint);
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
        scene.models[i].fragmentShader += (scene.models[i].material.texture) ? ((scene.models[i].material.specularTexBase64 != "") ? "specular = light.material.specular * specStrength * vec3(texture(modelMaterial.specularTex, TexCoord));\n" : "") : "";
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
    
    for (int i = 0; i < scene.joints.size(); i++) {
        if (scene.joints[i].vertices.size() > 0) {
            scene.joints[i].vertexShader = "#version 330 core\n";
            scene.joints[i].vertexShader += "layout(location = 0) in vec3 vPos;\n";
            scene.joints[i].vertexShader += "uniform mat4 model;\n";
            scene.joints[i].vertexShader += "uniform mat4 view;\n";
            scene.joints[i].vertexShader += "uniform mat4 projection;\n";
            scene.joints[i].vertexShader += "void main() {\n";
            scene.joints[i].vertexShader += "gl_Position = projection * view * model * vec4(vPos, 1.0f);\n";
            scene.joints[i].vertexShader += "}\0";
            
            scene.joints[i].fragmentShader = "#version 330 core\n";
            scene.joints[i].fragmentShader += "out vec4 FragColor;\n";
            scene.joints[i].fragmentShader += "uniform vec3 color;\n";
            scene.joints[i].fragmentShader += "void main() {\n";
            scene.joints[i].fragmentShader += "FragColor = vec4(color, 1.0f);\n";
            scene.joints[i].fragmentShader += "}\0";
            
            int vertexShader = glCreateShader(GL_VERTEX_SHADER);
            const char *vertexShaderSource = scene.joints[i].vertexShader.c_str();
            glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
            glCompileShader(vertexShader);
            int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
            const char *fragmentShaderSource = scene.joints[i].fragmentShader.c_str();
            glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
            glCompileShader(fragmentShader);
            scene.joints[i].shader = glCreateProgram();
            glAttachShader(scene.joints[i].shader, vertexShader);
            glAttachShader(scene.joints[i].shader, fragmentShader);
            glLinkProgram(scene.joints[i].shader);
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
            int width, height, nrChannels;
            vector<unsigned char> decoded = base64_decode(scene.models[i].material.diffuseTexBase64);
            unsigned char *data = stbi_load_from_memory(&decoded[0], decoded.size(), &width, &height, &nrChannels, 0);
            glGenTextures(1, &scene.models[i].material.diffuseTex);
            glBindTexture(GL_TEXTURE_2D, scene.models[i].material.diffuseTex);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(data);
            
            if (scene.models[i].material.specularTexBase64 != "") {
                vector<unsigned char> decoded_ = base64_decode(scene.models[i].material.specularTexBase64);
                unsigned char *data_ = stbi_load_from_memory(&decoded_[0], decoded_.size(), &width, &height, &nrChannels, 0);
                glGenTextures(1, &scene.models[i].material.specularTex);
                glBindTexture(GL_TEXTURE_2D, scene.models[i].material.specularTex);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_);
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
                glGenerateMipmap(GL_TEXTURE_2D);
                stbi_image_free(data_);
            }
            
            glUseProgram(scene.models[i].shader);
            glUniform1i(glGetUniformLocation(scene.models[i].shader, "modelMaterial.diffuseTex"), 0);
            if (scene.models[i].material.specularTexBase64 != "")
                glUniform1i(glGetUniformLocation(scene.models[i].shader, "modelMaterial.specularTex"), 1);
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
    
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        int selectedLine = 29;
        glm::vec3 axis = glm::vec3(0.0f, 0.0f, 1.0f);
        float angle = 0.1f;
        glm::vec3 lineBegin = glm::vec3(scene.joints[0].vertices[(selectedLine - 1) * 6],
                                     scene.joints[0].vertices[(selectedLine - 1) * 6 + 1],
                                        scene.joints[0].vertices[(selectedLine - 1) * 6 + 2]);
        glm::vec3 lineEnd = glm::vec3(scene.joints[0].vertices[(selectedLine - 1) * 6 + 3],
                                     scene.joints[0].vertices[(selectedLine - 1) * 6 + 4],
                                        scene.joints[0].vertices[(selectedLine - 1) * 6 + 5]);
        glm::vec3 vector = lineEnd - lineBegin;
        glm::vec3 rotated = vector * cos(glm::radians(angle)) + cross(axis, vector) * sin(glm::radians(angle)) + axis * dot(axis, vector) * (1.0f - cos(glm::radians(angle)));
        glm::vec3 newEnd = lineBegin + rotated;
        scene.joints[0].vertices[(selectedLine - 1) * 6 + 3] = newEnd.x;
        scene.joints[0].vertices[(selectedLine - 1) * 6 + 4] = newEnd.y;
        scene.joints[0].vertices[(selectedLine - 1) * 6 + 5] = newEnd.z;
        glBindBuffer(GL_ARRAY_BUFFER, scene.joints[0].vbo);
        glBufferData(GL_ARRAY_BUFFER, scene.joints[0].vertices.size() * sizeof(float), &scene.joints[0].vertices[0], GL_DYNAMIC_DRAW);
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        int selectedLine = 29;
        glm::vec3 axis = glm::vec3(0.0f, 0.0f, 1.0f);
        float angle = -0.1f;
        glm::vec3 lineBegin = glm::vec3(scene.joints[0].vertices[(selectedLine - 1) * 6],
                                     scene.joints[0].vertices[(selectedLine - 1) * 6 + 1],
                                        scene.joints[0].vertices[(selectedLine - 1) * 6 + 2]);
        glm::vec3 lineEnd = glm::vec3(scene.joints[0].vertices[(selectedLine - 1) * 6 + 3],
                                     scene.joints[0].vertices[(selectedLine - 1) * 6 + 4],
                                        scene.joints[0].vertices[(selectedLine - 1) * 6 + 5]);
        glm::vec3 vector = lineEnd - lineBegin;
        glm::vec3 rotated = vector * cos(glm::radians(angle)) + cross(axis, vector) * sin(glm::radians(angle)) + axis * dot(axis, vector) * (1.0f - cos(glm::radians(angle)));
        glm::vec3 newEnd = lineBegin + rotated;
        scene.joints[0].vertices[(selectedLine - 1) * 6 + 3] = newEnd.x;
        scene.joints[0].vertices[(selectedLine - 1) * 6 + 4] = newEnd.y;
        scene.joints[0].vertices[(selectedLine - 1) * 6 + 5] = newEnd.z;
        glBindBuffer(GL_ARRAY_BUFFER, scene.joints[0].vbo);
        glBufferData(GL_ARRAY_BUFFER, scene.joints[0].vertices.size() * sizeof(float), &scene.joints[0].vertices[0], GL_DYNAMIC_DRAW);
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
