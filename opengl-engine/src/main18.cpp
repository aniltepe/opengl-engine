//
//  main18.cpp
//  OpenGLTest4
//
//  Created by Nazım Anıl Tepe on 27.05.2022.
//

#include <glew.h>
#include <glfw3.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/string_cast.hpp>

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


enum struct ObjectType : int {
    Scene, Model, Light, Camera, Joint, Text, Cubemap, Framebuffer, Region
};
enum struct LightType : int {
    point, directional, spotlight
};
enum struct FboType : int {
    inverse, graysc, kernel, shadow, custom
};
struct Transform {
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 left;
};
struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
    bool texture;
    vector<string> texturesBase64;
    vector<unsigned int> textures;
    vector<int> specMapIndexes;
    vector<int> normMapIndexes;
};
struct Layout {
    float x;
    float y;
    float width;
    float height;
    float size;
    glm::vec3 color;
};
struct Shader {
    vector<float> vertices;
    vector<float> normals;
    vector<float> texCoords;
    vector<float> texOrders;
    vector<float> texQuantities;
    vector<float> tangents;
    vector<unsigned int> faces;
    int vertexCount;
    int instanceCount;
    unsigned int vao;
    unsigned int vbo;
    unsigned int ebo;
    unsigned int fbo;
    unsigned int rbo;
    unsigned int ibo;
    int shaderID;
    string vertexShader;
    string fragmentShader;
    string geometryShader;
};
struct Light {
    LightType lightType;
    float constant;
    float linear;
    float quadratic;
    float cutOff;
    float outerCutOff;
    glm::mat4 lightSpace;
};
struct Camera {
    float fov;
    float minDistance;
    float maxDistance;
    float moveSpeed;
};
struct Bone {
    vector<unsigned int> indices;
    vector<float> weights;
    float rollDegree = 0.0f;
    glm::vec3 locationOffset = glm::vec3(0.0f);
    glm::vec3 rotationDegrees = glm::vec3(0.0f);
    glm::vec3 rotationXAxis;
    glm::vec3 rotationYAxis;
    glm::vec3 rotationZAxis;
};
struct Style {
    string text;
    string font;
    vector<float> kernel;
    FboType fboType;
};
struct Instance {
    vector<float> translate;
    vector<float> scale;
    vector<float> front;
    vector<float> up;
    vector<float> left;
    bool instanced = false;
    vector<glm::mat4> instanceMatrices;
};
struct Animation {
    string animAttr;
    vector<float> values;
    vector<float> timestamps;
    string objPtr;
    vector<float> initValue;
};
struct Object {
    ObjectType type;
    string name;
    unsigned int index;
    bool hidden;
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
    Bone bone;
    Style style;
    Instance instance;
    vector<Animation*> animations;
};
struct Character {
    unsigned int textureID;
    glm::ivec2   size;
    glm::ivec2   bearing;
    unsigned int advance;
};

string WORK_DIR = "/Users/nazimaniltepe/Documents/Projects/opengl-engine/opengl-engine/";
string SCENE_DIR = "/Users/nazimaniltepe/Documents/Projects/opengl-scene/src/";
int objIndex = 0;
vector<Object> objects;
Object* cameraPtr;
vector<Object*> cameraPtrs;
Object* fboPtr;
vector<Object*> fboPtrs;
vector<Object*> rootJoPtrs;
map<GLchar, Character> characters;
string shading = "phong";
bool gammaCorrection = false;
bool multiSampling = false;
bool showJoints = true;
bool shadows = false;
vector<Object*> shadowFboPtrs;
float shadowFarPlane = 25.0;
vector<Animation*> animationPtrs;
bool animationLoop = true;
float animationStart = -1;
bool animationReset = true;
unsigned int polygonMode = GL_FILL;
bool keyCommandSticked = false;
int keyZeroIndex = 0;

glm::mat4 projection;
glm::mat4 view;
glm::mat4 textprojection;

FT_Library ft;
FT_Face face;


Object* createScene(string path);
Object* createObject(vector<string> rows, string name);
void createProperties(Object* objPtr);
void setShaders(Object* objPtr);
void setBuffers(Object* objPtr);
void drawScene(Object* objPtr);
void deleteScene(Object* objPtr);
void drawShadows(Object* objPtr, Object* shadowPtr, bool hidden);
void processAnimationFrames();
void processDiscreteInput(GLFWwindow* window, int key, int scancode, int action, int mods);
void processContinuousInput(GLFWwindow* window);
void resizeFramebuffer(GLFWwindow* window, int width, int height);
int captureScreenshot();
vector<unsigned char> base64Decode(string const& encoded_string);
void renderText(std::string text, float x, float y, float scale, glm::vec3 color);
glm::vec3 rotateVectorAroundAxis(glm::vec3 vector, glm::vec3 axis, float angle);
glm::vec3 rotateVectorAroundAxises(glm::vec3 vector, glm::mat3 axises, glm::vec3 degrees);
void setPoseTransformation(string rootjoint);
void setJointDegrees(string joint, glm::vec3 degrees);
void setJointOffset(string joint, glm::vec3 offset);
void setHumanPose();
void resetHumanPose();
void calculateTangents(Object* objPtr);
void manipulateHuman();

template <class T>
vector<T> processAttributeArray(string s)
{
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

int main()
{
    Object* scene = createScene(SCENE_DIR + "scene18.sce");
    
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(scene->layout.width, scene->layout.height, "OpenGL", NULL, NULL);
    if (window == NULL) {
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, resizeFramebuffer);
    glfwSetKeyCallback(window, processDiscreteInput);
    int windowHeight, windowWidth;
    glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
    float WIDTH_FACTOR = scene->layout.width / windowWidth;
    float HEIGHT_FACTOR = scene->layout.height / windowHeight;
    scene->layout.width = windowWidth;
    scene->layout.height = windowHeight;

    
    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    function<void(Object*)> inheritProperties = [&inheritProperties](Object* obj) {
        if (obj->type == ObjectType::Model || obj->type == ObjectType::Light || obj->type == ObjectType::Joint) {
            if (obj->dictionary.find("trnsf") == obj->dictionary.end()) {
                obj->dictionary.insert(pair<string, string>("trnsf", obj->superObject->dictionary.at("trnsf")));
                obj->transform = obj->superObject->transform;
                if (obj->type == ObjectType::Joint) {
                    obj->bone.rotationXAxis = glm::vec3(obj->transform.left);
                    obj->bone.rotationYAxis = glm::vec3(obj->transform.up);
                    obj->bone.rotationZAxis = glm::vec3(obj->transform.front);
                    
                    obj->bone.rotationXAxis = rotateVectorAroundAxis(obj->bone.rotationXAxis, obj->bone.rotationYAxis, obj->bone.rollDegree);
                    obj->bone.rotationZAxis = rotateVectorAroundAxis(obj->bone.rotationZAxis, obj->bone.rotationYAxis, obj->bone.rollDegree);
                }
            }
            if (obj->superObject->instance.instanced && !obj->instance.instanced) {
//            if (obj->superObject->instance.instanced && !obj->instance.instanced && obj->type != ObjectType::Joint) {
                obj->instance.instanced = true;
                obj->instance.translate = obj->superObject->instance.translate;
                obj->instance.scale = obj->superObject->instance.scale;
                obj->instance.front = obj->superObject->instance.front;
                obj->instance.up = obj->superObject->instance.up;
                obj->instance.left = obj->superObject->instance.left;
            }
            if (obj->dictionary.find("mtrl") == obj->dictionary.end() && obj->superObject->dictionary.find("mtrl") != obj->dictionary.end()) {
                obj->dictionary.insert(pair<string, string>("mtrl", obj->superObject->dictionary.at("mtrl")));
                obj->material.ambient = obj->superObject->material.ambient;
                obj->material.diffuse = obj->superObject->material.diffuse;
                obj->material.specular = obj->superObject->material.specular;
                obj->material.shininess = obj->superObject->material.shininess;
            }
        }
        for (int i = 0; i < obj->subObjects.size(); i++)
            inheritProperties(obj->subObjects[i]);
    };
    inheritProperties(scene);
    
    cameraPtr = cameraPtrs[0];
    
    if (shadowFboPtrs.size() > 0) {
        shadows = true;
        int index = 0;
        for (int i = 0; i < objects.size(); i++) {
            if (objects[i].objectPtr->type != ObjectType::Light)
                continue;
            Object light = objects[i];
            if (index > 0) {
                Object* objPtr = new Object();
                objPtr->objectPtr = objPtr;
                objPtr->index = objIndex++;
                objPtr->name = shadowFboPtrs[0]->name + to_string(index);
                objPtr->type = shadowFboPtrs[0]->type;
                objPtr->style.fboType = shadowFboPtrs[0]->style.fboType;
                objPtr->layout = shadowFboPtrs[0]->layout;
                objPtr->shader.vertices.push_back(0.0);
                objects.push_back(*objPtr);
                scene->subObjects.push_back(objPtr);
                objPtr->superObject = scene;
                shadowFboPtrs.push_back(objPtr);
            }
            shadowFboPtrs[index]->light.lightType = light.objectPtr->light.lightType;
            index++;
        }
    }
    
    
    setShaders(scene);
    setBuffers(scene);
    
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
//    glEnable(GL_CULL_FACE);
    glPointSize(10.0);
    if (multiSampling)
        glEnable(GL_MULTISAMPLE);
    
    vector<Object*> rootJoints;
    function<void(Object*)> initializeRootJoints = [&rootJoints, &initializeRootJoints](Object* obj) {
        if (obj->objectPtr->type == ObjectType::Joint && obj->objectPtr->superObject->type == ObjectType::Model) {
            rootJoints.push_back(obj->objectPtr);
            setPoseTransformation(obj->objectPtr->name);
        }
        for (int i = 0; i < obj->subObjects.size(); i++)
            initializeRootJoints(obj->subObjects[i]);
    };
    initializeRootJoints(scene);
    rootJoPtrs = { rootJoints };
    
    int elapsedFrameCount = 0;
    float lastFrame = 0.0f;
    while (!glfwWindowShouldClose(window))
    {
//        cout << "Frame " << elapsedFrameCount << endl;
//        cout << "Last Frame Time: " << lastFrame << endl;
        elapsedFrameCount++;
        float currentFrame = glfwGetTime();
        float timeDelta = currentFrame - lastFrame;
        lastFrame = currentFrame;
        float fps = 1 / timeDelta;
//        cout << "Current Frame Time: " << currentFrame << endl;
//        cout << "FPS: " << fps << endl;
        
        processAnimationFrames();
        
        processContinuousInput(window);
        glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
        
        projection = glm::perspective(glm::radians(cameraPtr->camera.fov), scene->layout.width / scene->layout.height, cameraPtr->camera.minDistance, cameraPtr->camera.maxDistance);
        view = lookAt(cameraPtr->transform.position, cameraPtr->transform.position + cameraPtr->transform.front, cameraPtr->transform.up);
        textprojection = glm::ortho(0.0f, scene->layout.width * WIDTH_FACTOR, 0.0f, scene->layout.height * HEIGHT_FACTOR);
        
        if (shadows) {
            float shadowNearPlane = 0.0f;
            vector<Object>::iterator light = objects.begin();
            while ((light = find_if(light, objects.end(), [] (Object obj) { return obj.type == ObjectType::Light; })) != objects.end()) {
                int index = int(light - objects.begin());
                glViewport(0, 0, shadowFboPtrs[index]->layout.width, shadowFboPtrs[index]->layout.height);
                glBindFramebuffer(GL_FRAMEBUFFER, shadowFboPtrs[index]->shader.fbo);
                glClear(GL_DEPTH_BUFFER_BIT);
                if (light->objectPtr->light.lightType == LightType::point || light->objectPtr->light.lightType == LightType::spotlight) {
                    glm::mat4 shadowProjection = glm::perspective(glm::radians(90.0f), shadowFboPtrs[index]->layout.width / shadowFboPtrs[index]->layout.height, shadowNearPlane, shadowFarPlane);
                    vector<glm::mat4> shadowTransforms;
                    shadowTransforms.push_back(shadowProjection * lookAt(light->objectPtr->transform.position, light->objectPtr->transform.position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
                    shadowTransforms.push_back(shadowProjection * lookAt(light->objectPtr->transform.position, light->objectPtr->transform.position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
                    shadowTransforms.push_back(shadowProjection * lookAt(light->objectPtr->transform.position, light->objectPtr->transform.position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
                    shadowTransforms.push_back(shadowProjection * lookAt(light->objectPtr->transform.position, light->objectPtr->transform.position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
                    shadowTransforms.push_back(shadowProjection * lookAt(light->objectPtr->transform.position, light->objectPtr->transform.position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
                    shadowTransforms.push_back(shadowProjection * lookAt(light->objectPtr->transform.position, light->objectPtr->transform.position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
                    
                    for (unsigned int i = 0; i < 6; i++) {
                        glUseProgram(shadowFboPtrs[index]->shader.shaderID);
                        glUniformMatrix4fv(glGetUniformLocation(shadowFboPtrs[index]->shader.shaderID, ("shadowTransforms[" + to_string(i) + "]").c_str()), 1, GL_FALSE, value_ptr(shadowTransforms[i]));
                    }
                    glUniform1f(glGetUniformLocation(shadowFboPtrs[index]->shader.shaderID, "farPlane"), shadowFarPlane);
                    glUniform3fv(glGetUniformLocation(shadowFboPtrs[index]->shader.shaderID, "lightPos"), 1, value_ptr(light->objectPtr->transform.position));
                }
                else if (light->objectPtr->light.lightType == LightType::directional) {
                    glm::mat4 shadowProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, shadowNearPlane, shadowFarPlane);
                    light->objectPtr->light.lightSpace = shadowProjection * lookAt(light->objectPtr->transform.position, light->objectPtr->transform.position + light->objectPtr->transform.front, light->objectPtr->transform.up);
                    glUseProgram(shadowFboPtrs[index]->shader.shaderID);
                    glUniformMatrix4fv(glGetUniformLocation(shadowFboPtrs[index]->shader.shaderID, "shadowTransform"), 1, GL_FALSE, value_ptr(light->objectPtr->light.lightSpace));
                }
                
                drawShadows(scene, shadowFboPtrs[index]->objectPtr, light->objectPtr->hidden);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                light++;
            }
            glViewport(0, 0, scene->layout.width, scene->layout.height);
        }
        
        
        if (fboPtr != NULL && fboPtr->hidden == false) {
            glBindFramebuffer(GL_FRAMEBUFFER, fboPtr->shader.fbo);
        }
        glEnable(GL_DEPTH_TEST);
//        glClearColor(0.28f, 0.28f, 0.28f, 1.0f);
        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        
        drawScene(scene);
        
        if (fboPtr != NULL && fboPtr->hidden == false) {
            if (multiSampling) {
                glBindFramebuffer(GL_READ_FRAMEBUFFER, fboPtr->shader.fbo);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboPtr->shader.ebo);  // intermediate fbo olarak ebo kullanıldı
                glBlitFramebuffer(0, 0, scene->layout.width, scene->layout.height, 0, 0, scene->layout.width, scene->layout.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDisable(GL_DEPTH_TEST);
            glClear(GL_COLOR_BUFFER_BIT);
            glUseProgram(fboPtr->shader.shaderID);
            glBindVertexArray(fboPtr->shader.vao);
            if (multiSampling) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, fboPtr->material.textures[1]);
            }
            else
                glBindTexture(GL_TEXTURE_2D, fboPtr->material.textures[0]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
 
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    deleteScene(scene);
    
    glfwTerminate();
    return 0;
}

Object* createScene(string path)
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
    
    return createObject(rows, "TestScene");
}

Object* createObject(vector<string> rows, string name)
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
            bool hidden = false;
            rows[i].erase(rows[i].begin(), find_if(rows[i].begin(), rows[i].end(), [](unsigned char ch) { return !isspace(ch); }));
            if (rows[i].rfind("#", 0) == 0) {
                rows[i].erase(0, 1);
                hidden = true;
            }
            vector<string>::const_iterator itr;
            function<bool(string)> endsWith = [&](string s) {
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
                Object* subObjPtr = createObject(newRows, rows[i]);
                subObjPtr->superObject = objPtr;
                subObjPtr->hidden = hidden;
                objPtr->subObjects.push_back(subObjPtr);
                i = int(itr - rows.begin());
            }
            else
                cout << "couldn't find closure for " << rows[i] << endl;
        }
    }
    createProperties(objPtr);
    objects.push_back(*objPtr);
    return objPtr;
}

void createProperties(Object* objPtr)
{
    for (const auto &entry : objPtr->dictionary) {
        if (entry.first == "type")
            objPtr->type = static_cast<ObjectType>(stoi(entry.second));
        else if (entry.first == "shad")
            shading = entry.second;
        else if (entry.first == "gama")
            gammaCorrection = entry.second == "true" ? true : false;
        else if (entry.first == "msaa")
            multiSampling = entry.second == "true" ? true : false;
        else if (entry.first == "ltyp")
            objPtr->light.lightType = static_cast<LightType>(stoi(entry.second));
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
        else if (entry.first == "w")
            objPtr->bone.weights = processAttributeArray<float>(entry.second);
        else if (entry.first == "f")
            objPtr->shader.faces = processAttributeArray<unsigned int>(entry.second);
        else if (entry.first == "i")
            objPtr->bone.indices = processAttributeArray<unsigned int>(entry.second);
        else if (entry.first == "v")
            objPtr->shader.vertices = processAttributeArray<float>(entry.second);
        else if (entry.first == "n")
            objPtr->shader.normals = processAttributeArray<float>(entry.second);
        else if (entry.first == "t")
            objPtr->shader.texCoords = processAttributeArray<float>(entry.second);
        else if (entry.first == "to")
            objPtr->shader.texOrders = processAttributeArray<float>(entry.second);
        else if (entry.first == "tq")
            objPtr->shader.texQuantities = processAttributeArray<float>(entry.second);
        else if (entry.first == "kern")
            objPtr->style.kernel = processAttributeArray<float>(entry.second);
        else if (entry.first == "instrns") {
            objPtr->instance.translate = processAttributeArray<float>(entry.second);
            objPtr->instance.instanced = true;
        }
        else if (entry.first == "insscal") {
            objPtr->instance.scale = processAttributeArray<float>(entry.second);
            objPtr->instance.instanced = true;
        }
        else if (entry.first == "insfron") {
            objPtr->instance.front = processAttributeArray<float>(entry.second);
            objPtr->instance.instanced = true;
        }
        else if (entry.first == "insup") {
            objPtr->instance.up = processAttributeArray<float>(entry.second);
            objPtr->instance.instanced = true;
        }
        else if (entry.first == "insleft") {
            objPtr->instance.left = processAttributeArray<float>(entry.second);
            objPtr->instance.instanced = true;
        }
        else if (entry.first == "info")
            objPtr->style.text = entry.second;
        else if (entry.first == "font")
            objPtr->style.font = entry.second;
        else if (entry.first == "fbtyp")
            objPtr->style.fboType = static_cast<FboType>(stoi(entry.second));
        else if (entry.first == "mtrl") {
            vector<float> sequence = processAttributeArray<float>(entry.second);
            objPtr->material.ambient = glm::vec3(sequence[0], sequence[1], sequence[2]);
            objPtr->material.diffuse = glm::vec3(sequence[3], sequence[4], sequence[5]);
            objPtr->material.specular = glm::vec3(sequence[6], sequence[7], sequence[8]);
            objPtr->material.shininess = sequence[9];
        }
        else if (entry.first == "trnsf") {
            vector<float> sequence = processAttributeArray<float>(entry.second);
            objPtr->transform.position = glm::vec3(sequence[0], sequence[1], sequence[2]);
            objPtr->transform.scale = glm::vec3(sequence[3], sequence[4], sequence[5]);
            objPtr->transform.front = glm::vec3(sequence[6], sequence[7], sequence[8]);
            objPtr->transform.up = glm::vec3(sequence[9], sequence[10], sequence[11]);
            objPtr->transform.left = glm::vec3(sequence[12], sequence[13], sequence[14]);
        }
        else if (entry.first == "lout") {
            vector<float> sequence = processAttributeArray<float>(entry.second);
            objPtr->layout.width = sequence[0];
            objPtr->layout.height = sequence[1];
            objPtr->layout.x = sequence[2];
            objPtr->layout.y = sequence[3];
            objPtr->layout.size = sequence[4];
        }
        else if (entry.first == "ipos") {
            vector<float> sequence = processAttributeArray<float>(entry.second);
            objPtr->bone.rollDegree = sequence[0];
            objPtr->bone.rotationDegrees = glm::vec3(sequence[1], sequence[2], sequence[3]);
            objPtr->bone.locationOffset = glm::vec3(sequence[4], sequence[5], sequence[6]);
        }
        else if (entry.first.rfind("tex", 0) == 0) {
            objPtr->material.texturesBase64.push_back(entry.second);
            objPtr->material.texture = true;
            if (0 == entry.first.compare(entry.first.length() - 2, 2, "nr"))
                objPtr->material.normMapIndexes.push_back((int)objPtr->material.texturesBase64.size() - 1);
            else if (0 == entry.first.compare(entry.first.length() - 2, 2, "sp"))
                objPtr->material.specMapIndexes.push_back((int)objPtr->material.texturesBase64.size() - 1);
        }
        else if (entry.first.rfind("anim", 0) == 0) {
            string attr = entry.first;
            attr = attr.replace(0, 4, "");
            if (0 == attr.compare(attr.length() - 2, 2, "ts")) {
                attr = attr.replace(attr.length() - 2, 2, "");
                vector<Animation*>::iterator it = find_if(objPtr->animations.begin(), objPtr->animations.end(), [attr] (Animation* anim) { return anim->animAttr == attr; });
                Animation* a = *it;
                a->timestamps = processAttributeArray<float>(entry.second);
            }
            else {
                vector<Animation*>::iterator it = find_if(objPtr->animations.begin(), objPtr->animations.end(), [attr] (Animation* anim) { return anim->animAttr == attr; });
                if (it == objPtr->animations.end()) {
                    Animation* animPtr = new Animation();
                    animPtr->animAttr = attr;
                    animPtr->values = processAttributeArray<float>(entry.second);
                    ostringstream oss;
                    oss << objPtr;
                    animPtr->objPtr = oss.str();
                    objPtr->animations.push_back(animPtr);
                    animationPtrs.push_back(animPtr);
                }
                else {
                    Animation* a = *it;
                    a->timestamps = processAttributeArray<float>(entry.second);
                }
            }
        }
        else if (entry.first == "colo") {
            vector<float> sequence = processAttributeArray<float>(entry.second);
            objPtr->layout.color = glm::vec3(sequence[0], sequence[1], sequence[2]);
        }
    }
    if (objPtr->type == ObjectType::Camera)
        cameraPtrs.push_back(objPtr);
    else if (objPtr->type == ObjectType::Framebuffer && objPtr->style.fboType != FboType::shadow)
        fboPtrs.push_back(objPtr);
    else if (objPtr->type == ObjectType::Framebuffer && objPtr->style.fboType == FboType::shadow)
        shadowFboPtrs.push_back(objPtr);
}

void setShaders(Object* objPtr)
{
    if (objPtr->type == ObjectType::Scene) {
//        cout << "object " + objPtr->name + " is not drawable, passing buffer phase" << endl;
    }
    else if (objPtr->shader.vertices.size() == 0 && objPtr->type != ObjectType::Text) {
//        cout << "object " + objPtr->name + " has no vertices, passing shader phase" << endl;
    }
    else {
//        cout << "object " + objPtr->name + " is drawable, processing shader phase" << endl;
        
        objPtr->shader.vertexShader = "#version 330 core\n";
        objPtr->shader.fragmentShader = "#version 330 core\n";
        
        objPtr->shader.vertexShader += (objPtr->type != ObjectType::Text && objPtr->type != ObjectType::Framebuffer) ? "layout(location = 0) in vec3 vPos;\n" : "";
        objPtr->shader.vertexShader += (objPtr->type == ObjectType::Text) ? "layout(location = 0) in vec4 vPos;\n" : "";
        objPtr->shader.vertexShader += (objPtr->type == ObjectType::Text) ? "out vec2 TexCoord;\n" : "";
        
        objPtr->shader.vertexShader += (objPtr->type == ObjectType::Cubemap) ? "out vec3 TexCoord;\n" : "";
        
        if (objPtr->type == ObjectType::Framebuffer && objPtr->style.fboType != FboType::shadow) {
            objPtr->shader.vertexShader += "layout(location = 0) in vec2 vPos;\n";
            objPtr->shader.vertexShader += "layout(location = 1) in vec2 vTexCoord;\n";
            objPtr->shader.vertexShader += "out vec2 TexCoord;\n";
            objPtr->shader.vertexShader += "void main() {\n";
            objPtr->shader.vertexShader += "\tTexCoord = vTexCoord;\n";
            objPtr->shader.vertexShader += "\tgl_Position = vec4(vPos.x, vPos.y, 0.0, 1.0);\n";
        }
        else if (objPtr->type == ObjectType::Framebuffer && objPtr->style.fboType == FboType::shadow) {
            objPtr->shader.vertexShader += "layout(location = 0) in vec3 vPos;\n";
            objPtr->shader.vertexShader += "layout(location = 1) in vec3 vNormal;\n";
            objPtr->shader.vertexShader += "layout(location = 2) in vec2 vTexCoord;\n";
            objPtr->shader.vertexShader += "layout(location = 3) in float vTexOrder;\n";
            objPtr->shader.vertexShader += "layout(location = 4) in float vTexQty;\n";
            objPtr->shader.vertexShader += objPtr->light.lightType != LightType::directional ? "out VS_OUT { vec2 TexCoord; float TexOrder; float TexQty; } vs_out;\n" : "";
            objPtr->shader.vertexShader += objPtr->light.lightType == LightType::directional ? "out vec2 TexCoord;\n" : "";
            objPtr->shader.vertexShader += objPtr->light.lightType == LightType::directional ? "out float TexOrder;\n" : "";
            objPtr->shader.vertexShader += objPtr->light.lightType == LightType::directional ? "out float TexQty;\n" : "";
            objPtr->shader.vertexShader += objPtr->light.lightType == LightType::directional ? "uniform mat4 shadowTransform;\n" : "";
            objPtr->shader.vertexShader += "uniform mat4 model;\n";
            objPtr->shader.vertexShader += "void main() {\n";
            objPtr->shader.vertexShader += objPtr->light.lightType != LightType::directional ? "\tvs_out.TexCoord = vTexCoord;\n" : "\tTexCoord = vTexCoord;\n";
            objPtr->shader.vertexShader += objPtr->light.lightType != LightType::directional ? "\tvs_out.TexOrder = vTexOrder;\n" : "\tTexOrder = vTexOrder;\n";
            objPtr->shader.vertexShader += objPtr->light.lightType != LightType::directional ? "\tvs_out.TexQty = vTexQty;\n" : "\tTexQty = vTexQty;\n";
            objPtr->shader.vertexShader += objPtr->light.lightType != LightType::directional ? "\tgl_Position = model * vec4(vPos, 1.0);\n" : "";
            
            objPtr->shader.vertexShader += objPtr->light.lightType == LightType::directional ? "\tgl_Position = shadowTransform * model * vec4(vPos, 1.0);\n" : "";
        }
        
        
        if (objPtr->type == ObjectType::Model) {
            objPtr->shader.vertexShader += "layout(location = 1) in vec3 vNormal;\n";
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "layout(location = 2) in vec2 vTexCoord;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "layout(location = 3) in float vTexOrder;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "layout(location = 4) in float vTexQty;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.normMapIndexes.size() > 0) ? "layout(location = 5) in vec3 vTangent;\n" : "";
            if (objPtr->instance.instanced) {
                objPtr->shader.vertexShader += (objPtr->material.texture) ? ((objPtr->material.normMapIndexes.size() > 0) ? "layout(location = 6) in mat4 instanceMatrix;\n" : "layout(location = 5) in mat4 instanceMatrix;\n") : "layout(location = 2) in mat4 instanceMatrix;\n";
                objPtr->shader.vertexShader += (objPtr->material.texture) ? ((objPtr->material.normMapIndexes.size() > 0) ? "layout(location = 10) in mat4 instanceRotationMatrix;\n" : "layout(location = 9) in mat4 instanceRotationMatrix;\n") : "layout(location = 6) in mat4 instanceRotationMatrix;\n";
            }
            objPtr->shader.vertexShader += "out vec3 VertexPos;\n";
            objPtr->shader.vertexShader += "out vec3 FragPos;\n";
            objPtr->shader.vertexShader += "out vec3 Normal;\n";
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "out vec2 TexCoord;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "out float TexOrder;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "out float TexQty;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.normMapIndexes.size() > 0) ? "out vec3 Tangent;\n" : "";
        }
        
        if (objPtr->type == ObjectType::Joint && objPtr->instance.instanced)
            objPtr->shader.vertexShader += "layout(location = 1) in mat4 instanceMatrix;\n";
        
        if (objPtr->type != ObjectType::Framebuffer) {
            objPtr->shader.vertexShader += (objPtr->type != ObjectType::Text && objPtr->type != ObjectType::Cubemap) ? "uniform mat4 model;\n" : "";
            objPtr->shader.vertexShader += (objPtr->type != ObjectType::Text) ? "uniform mat4 view;\n" : "";
            objPtr->shader.vertexShader += "uniform mat4 projection;\n";
            objPtr->shader.vertexShader += (objPtr->type == ObjectType::Model) ? "uniform mat4 rotation;\n" : "";
            objPtr->shader.vertexShader += "void main() {\n";
            if (objPtr->instance.instanced)
                objPtr->shader.vertexShader += "\tgl_Position = projection * view * model * instanceMatrix * vec4(vPos, 1.0f);\n";
            else
                objPtr->shader.vertexShader += (objPtr->type != ObjectType::Text && objPtr->type != ObjectType::Cubemap) ? "\tgl_Position = projection * view * model * vec4(vPos, 1.0f);\n" : "";
            objPtr->shader.vertexShader += (objPtr->type == ObjectType::Text) ? "\tgl_Position = projection * vec4(vPos.xy, 0.0, 1.0);\n" : "";
            objPtr->shader.vertexShader += (objPtr->type == ObjectType::Text) ? "\tTexCoord = vPos.zw;\n" : "";
        }
        
        if (objPtr->type == ObjectType::Model) {
            objPtr->shader.vertexShader += "\tVertexPos = vPos;\n";
            if (objPtr->instance.instanced) {
                objPtr->shader.vertexShader += "\tFragPos = vec3(model * instanceMatrix * vec4(vPos, 1.0f));\n";
                objPtr->shader.vertexShader += "\tNormal = vec3(rotation * instanceRotationMatrix * vec4(vNormal, 1.0f));\n";
            }
            else {
                objPtr->shader.vertexShader += "\tFragPos = vec3(model * vec4(vPos, 1.0f));\n";
                objPtr->shader.vertexShader += "\tNormal = vec3(rotation * vec4(vNormal, 0.0f));\n";
            }
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "\tTexCoord = vTexCoord;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "\tTexOrder = vTexOrder;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "\tTexQty = vTexQty;\n" : "";
            if (objPtr->material.normMapIndexes.size() > 0) {
                if (objPtr->instance.instanced)
                    objPtr->shader.vertexShader += "\tTangent = vec3(rotation * instanceRotationMatrix * vec4(vTangent, 1.0f));\n";
                else
                    objPtr->shader.vertexShader += "\tTangent = vec3(rotation * vec4(vTangent, 0.0f));\n";
            }
        }
        objPtr->shader.vertexShader += "}\0";
        
        
        
        
        if (objPtr->type == ObjectType::Framebuffer && objPtr->style.fboType == FboType::shadow) {
            int maxTexSize = 0, maxSpecTexSize = 0, maxNormalMapSize = 0;
            vector<Object>::iterator it = objects.begin();
            while ((it = find_if(it, objects.end(), [] (Object obj) { return obj.type == ObjectType::Model && obj.material.texture; })) != objects.end()) {
                if (it->objectPtr->material.texturesBase64.size() > maxTexSize) maxTexSize = int(it->objectPtr->material.texturesBase64.size());
                if (it->objectPtr->material.specMapIndexes.size() > maxSpecTexSize) maxSpecTexSize = int(it->objectPtr->material.specMapIndexes.size());
                if (it->objectPtr->material.normMapIndexes.size() > maxNormalMapSize) maxNormalMapSize = int(it->objectPtr->material.normMapIndexes.size());
                it++;
            }
            objPtr->shader.fragmentShader += objPtr->light.lightType != LightType::directional ? "in vec4 FragPos;\n" : "";
            objPtr->shader.fragmentShader += maxTexSize > 0 ? "in vec2 TexCoord;\n" : "";
            objPtr->shader.fragmentShader += maxTexSize > 0 ? "in float TexOrder;\n" : "";
            objPtr->shader.fragmentShader += maxTexSize > 0 ? "in float TexQty;\n" : "";
            objPtr->shader.fragmentShader += objPtr->light.lightType != LightType::directional ? "uniform vec3 lightPos;\n" : "";
            objPtr->shader.fragmentShader += objPtr->light.lightType != LightType::directional ? "uniform float farPlane;\n" : "";
            objPtr->shader.fragmentShader += maxTexSize > 0 ? "uniform sampler2D textures[" + to_string(maxTexSize) + "];\n" : "";
            objPtr->shader.fragmentShader += maxSpecTexSize > 0 ? "uniform int specMapIndexes[" + to_string(maxSpecTexSize) + "];\n" : "";
            objPtr->shader.fragmentShader += maxNormalMapSize > 0 ? "uniform int normMapIndexes[" + to_string(maxNormalMapSize) + "];\n" : "";
            objPtr->shader.fragmentShader += "void main() {\n";
            objPtr->shader.fragmentShader += objPtr->light.lightType != LightType::directional ? "\tfloat lightDistance = length(FragPos.xyz - lightPos);\n" : "";
            objPtr->shader.fragmentShader += objPtr->light.lightType != LightType::directional ? "\tlightDistance = lightDistance / farPlane;\n" : "";
            objPtr->shader.fragmentShader += "\tint complexOrder = int(TexOrder);\n";
            objPtr->shader.fragmentShader += "\tint texQuantity = int(TexQty);\n";
            objPtr->shader.fragmentShader += "\tfor (int i = 0; i < texQuantity; i++) {\n";
            objPtr->shader.fragmentShader += "\t\tint remainder = complexOrder;\n";
            objPtr->shader.fragmentShader += "\t\tint division;\n";
            objPtr->shader.fragmentShader += "\t\tfor (int j = texQuantity; j > i; j--) {\n";
            objPtr->shader.fragmentShader += "\t\t\tdivision = int(remainder / pow(2, 4 * (j - 1)));\n";
            objPtr->shader.fragmentShader += "\t\t\tremainder = remainder - int(division * pow(2, 4 * (j - 1)));\n";
            objPtr->shader.fragmentShader += "\t\t}\n";
            objPtr->shader.fragmentShader += "\t\tint order = division;\n";
            objPtr->shader.fragmentShader += "\t\tbool specMap = false;\n";
            objPtr->shader.fragmentShader += "\t\tbool normMap = false;\n";
            objPtr->shader.fragmentShader += (maxSpecTexSize > 0) ? "\t\tfor (int j = 0; j < specMapIndexes.length(); j++)\n" : "";
            objPtr->shader.fragmentShader += (maxSpecTexSize > 0) ? "\t\t\tif (specMapIndexes[j] == order)\n" : "";
            objPtr->shader.fragmentShader += (maxSpecTexSize > 0) ? "\t\t\t\tspecMap = true;\n" : "";
            objPtr->shader.fragmentShader += (maxNormalMapSize > 0) ? "\t\tfor (int j = 0; j < normMapIndexes.length(); j++)\n" : "";
            objPtr->shader.fragmentShader += (maxNormalMapSize > 0) ? "\t\t\tif (normMapIndexes[j] == order)\n" : "";
            objPtr->shader.fragmentShader += (maxNormalMapSize > 0) ? "\t\t\t\tnormMap = true;\n" : "";
            objPtr->shader.fragmentShader += "\t\tif (normMap || specMap)\n";
            objPtr->shader.fragmentShader += "\t\t\tcontinue;\n";
            objPtr->shader.fragmentShader += "\t\tvec4 texv4 = texture(textures[order], TexCoord);\n";
            objPtr->shader.fragmentShader += "\t\tif (texv4.a == 0.0) discard;\n";
            objPtr->shader.fragmentShader += "\t}\n";
            objPtr->shader.fragmentShader += objPtr->light.lightType != LightType::directional ? "\tgl_FragDepth = lightDistance;\n" : "";
            
            objPtr->shader.fragmentShader += objPtr->light.lightType == LightType::directional ? "\tgl_FragDepth = gl_FragCoord.z;\n" : "";
        }
        else
            objPtr->shader.fragmentShader += "out vec4 FragColor;\n";
            
        if (objPtr->type == ObjectType::Model) {
            objPtr->shader.fragmentShader += "in vec3 VertexPos;\n";
            objPtr->shader.fragmentShader += "in vec3 FragPos;\n";
            objPtr->shader.fragmentShader += "in vec3 Normal;\n";
            objPtr->shader.fragmentShader += (objPtr->material.texture) ? "in vec2 TexCoord;\n" : "";
            objPtr->shader.fragmentShader += (objPtr->material.texture) ? "in float TexOrder;\n" : "";
            objPtr->shader.fragmentShader += (objPtr->material.texture) ? "in float TexQty;\n" : "";
            objPtr->shader.fragmentShader += (objPtr->material.normMapIndexes.size() > 0) ? "in mat3 NormalMatrix;\n" : "";
            objPtr->shader.fragmentShader += (objPtr->material.normMapIndexes.size() > 0) ? "in vec3 Tangent;\n" : "";
            objPtr->shader.fragmentShader += "struct Material {\n";
            objPtr->shader.fragmentShader += "\tvec3 ambient;\n";
            objPtr->shader.fragmentShader += "\tvec3 diffuse;\n";
            objPtr->shader.fragmentShader += "\tvec3 specular;\n";
            objPtr->shader.fragmentShader += "\tfloat shininess;\n";
            objPtr->shader.fragmentShader += "};\n";
            objPtr->shader.fragmentShader += "struct Light {\n";
            objPtr->shader.fragmentShader += "\tint lightType;\n";
            objPtr->shader.fragmentShader += "\tvec3 direction;\n";
            objPtr->shader.fragmentShader += "\tvec3 position;\n";
            objPtr->shader.fragmentShader += "\tfloat constant;\n";
            objPtr->shader.fragmentShader += "\tfloat linear;\n";
            objPtr->shader.fragmentShader += "\tfloat quadratic;\n";
            objPtr->shader.fragmentShader += "\tfloat cutOff;\n";
            objPtr->shader.fragmentShader += "\tfloat outerCutOff;\n";
            objPtr->shader.fragmentShader += "\tMaterial material;\n";
            objPtr->shader.fragmentShader += "\tmat4 lightSpace;\n";
            objPtr->shader.fragmentShader += "};\n";
//            objPtr->shader.fragmentShader += (objPtr->material.texture) ? "uniform sampler2D textures[" + to_string(objPtr->material.texturesBase64.size()) + "];\n" : "";
            if (objPtr->material.texture)
                for (int i = 0; i < objPtr->material.texturesBase64.size(); i++)
                    objPtr->shader.fragmentShader += "uniform sampler2D texture" + to_string(i) + ";\n";
            objPtr->shader.fragmentShader += (objPtr->material.specMapIndexes.size() > 0) ? "uniform int specMapIndexes[" + to_string(objPtr->material.specMapIndexes.size()) + "];\n" : "";
            objPtr->shader.fragmentShader += (objPtr->material.normMapIndexes.size() > 0) ? "uniform int normMapIndexes[" + to_string(objPtr->material.normMapIndexes.size()) + "];\n" : "";
            objPtr->shader.fragmentShader += "uniform vec3 cameraPos;\n";
            objPtr->shader.fragmentShader += "uniform Material modelMaterial;\n";
            objPtr->shader.fragmentShader += "uniform Light lights[" + to_string(count_if(objects.begin(), objects.end(), [] (Object obj) { return obj.type == ObjectType::Light; })) + "];\n";
            objPtr->shader.fragmentShader += "uniform int shading;\n";
            objPtr->shader.fragmentShader += "uniform bool gamma;\n";
            objPtr->shader.fragmentShader += "mat3 TBN;\n";
            objPtr->shader.fragmentShader += "vec4 CalculateLight(vec3 normal, vec3 viewDir, int lightIndex);\n";
            
            if (shadows) {
                for (int i = 0; i < shadowFboPtrs.size(); i++) {
                    if (shadowFboPtrs[i]->light.lightType == LightType::point || shadowFboPtrs[i]->light.lightType == LightType::spotlight)
                        objPtr->shader.fragmentShader += "uniform samplerCube shadowDepthMap" + to_string(i) + ";\n";
                    else if (shadowFboPtrs[i]->light.lightType == LightType::directional)
                        objPtr->shader.fragmentShader += "uniform sampler2D shadowDepthMap" + to_string(i) + ";\n";
                }
                objPtr->shader.fragmentShader += "uniform float farPlane;\n";
                objPtr->shader.fragmentShader += "uniform bool shadows;\n";
                objPtr->shader.fragmentShader += "float CalculateShadow(int lightIndex);\n";
//                objPtr->shader.fragmentShader += "vec3 gridSamplingDisk[20] = vec3[](vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1), vec3(1, 1, -1), vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1), vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0), vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1), vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, -1, -1), vec3(0, 1, -1));\n";
                objPtr->shader.fragmentShader += "vec3 gridSamplingDisk[20] = vec3[](vec3(0.2, 0.2, 0.2), vec3(0.2, -0.2, 0.2), vec3(-0.2, -0.2, 0.2), vec3(-0.2, 0.2, 0.2), vec3(0.2, 0.2, -0.2), vec3(0.2, -0.2, -0.2), vec3(-0.2, -0.2, -0.2), vec3(-0.2, 0.2, -0.2), vec3(0.2, 0.2, 0), vec3(0.2, -0.2, 0), vec3(-0.2, -0.2, 0), vec3(-0.2, 0.2, 0), vec3(0.2, 0, 0.2), vec3(-0.2, 0, 0.2), vec3(0.2, 0, -0.2), vec3(-0.2, 0, -0.2), vec3(0, 0.2, 0.2), vec3(0, -0.2, 0.2), vec3(0, -0.2, -0.2), vec3(0, 0.2, -0.2));\n";
            }
            
            objPtr->shader.fragmentShader += "void main() {\n";
            objPtr->shader.fragmentShader += "\tvec3 N = normalize(Normal);\n";
            if (objPtr->material.normMapIndexes.size() > 0) {
                objPtr->shader.fragmentShader += "\tvec3 T = normalize(Tangent);\n";
                objPtr->shader.fragmentShader += "\tT = normalize(T - dot(T, N) * N);\n";
                objPtr->shader.fragmentShader += "\tvec3 B = cross(N, T);\n";
                objPtr->shader.fragmentShader += "\tTBN = mat3(T, B, N);\n";
                objPtr->shader.fragmentShader += "\tint complexOrder = int(TexOrder);\n";
                objPtr->shader.fragmentShader += "\tint texQuantity = int(TexQty);\n";
                objPtr->shader.fragmentShader += "\tfor (int i = 0; i < texQuantity; i++) {\n";
                objPtr->shader.fragmentShader += "\t\tint remainder = complexOrder;\n";
                objPtr->shader.fragmentShader += "\t\tint division;\n";
                objPtr->shader.fragmentShader += "\t\tfor (int j = texQuantity; j > i; j--) {\n";
                objPtr->shader.fragmentShader += "\t\t\tdivision = int(remainder / pow(2, 4 * (j - 1)));\n";
                objPtr->shader.fragmentShader += "\t\t\tremainder = remainder - int(division * pow(2, 4 * (j - 1)));\n";
                objPtr->shader.fragmentShader += "\t\t}\n";
                objPtr->shader.fragmentShader += "\t\tint order = division;\n";
                objPtr->shader.fragmentShader += "\t\tbool normMap = false;\n";
                objPtr->shader.fragmentShader += "\t\tfor (int j = 0; j < normMapIndexes.length(); j++)\n";
                objPtr->shader.fragmentShader += "\t\t\tif (normMapIndexes[j] == order)\n";
                objPtr->shader.fragmentShader += "\t\t\t\tnormMap = true;\n";
                objPtr->shader.fragmentShader += "\t\tif (normMap) {\n";
                objPtr->shader.fragmentShader += "\t\t\tvec4 texv4 = vec4(0.0);\n";
                for (int i = 0; i < objPtr->material.texturesBase64.size(); i++)
                    objPtr->shader.fragmentShader += "\t\t\tif (order == " + to_string(i) + ") texv4 = texture(texture" + to_string(i) + ", TexCoord);\n";
//                objPtr->shader.fragmentShader += "\t\t\tvec4 texv4 = texture(textures[order], TexCoord);\n";
                objPtr->shader.fragmentShader += "\t\t\tvec3 norm = texv4.rgb;\n";
                objPtr->shader.fragmentShader += "\t\t\tnorm = normalize(norm * 2.0 - 1.0);\n";
                objPtr->shader.fragmentShader += "\t\t\tN = normalize(TBN * norm);\n";
                objPtr->shader.fragmentShader += "\t\t\tbreak;\n";
                objPtr->shader.fragmentShader += "\t\t}\n";
                objPtr->shader.fragmentShader += "\t}\n";
            }
            objPtr->shader.fragmentShader += "\tvec3 viewDir = normalize(cameraPos - FragPos);\n";
            objPtr->shader.fragmentShader += "\tvec4 result = vec4(0.0f);\n";
            objPtr->shader.fragmentShader += "\tfor (int i = 0; i < lights.length(); i++)\n";
            objPtr->shader.fragmentShader += "\t\tif (lights[i].lightType != -1)\n";
            objPtr->shader.fragmentShader += "\t\t\tresult += CalculateLight(N, viewDir, i);\n";
            objPtr->shader.fragmentShader += "\tif (gamma)\n";
            objPtr->shader.fragmentShader += "\t\tresult.xyz = pow(result.xyz, vec3(1.0/2.2));\n";
            objPtr->shader.fragmentShader += "\tFragColor = result;\n";
            objPtr->shader.fragmentShader += "}\n";
            objPtr->shader.fragmentShader += "vec4 CalculateLight(vec3 normal, vec3 viewDir, int lightIndex) {\n";
            objPtr->shader.fragmentShader += "\tfloat shadow = 0.0;\n";
            objPtr->shader.fragmentShader += shadows ? "\tshadow = CalculateShadow(lightIndex);\n" : "";
            objPtr->shader.fragmentShader += "\tvec3 lightPos = lights[lightIndex].position;\n";
            objPtr->shader.fragmentShader += "\tvec3 lightDir = normalize(lightPos - FragPos);\n";
            objPtr->shader.fragmentShader += "\tif (lights[lightIndex].lightType == 1)\n";
            objPtr->shader.fragmentShader += "\t\tlightDir = normalize(-lights[lightIndex].direction);\n";
            objPtr->shader.fragmentShader += "\tfloat diffStrength = max(dot(normal, lightDir), 0.0);\n";
//            objPtr->shader.fragmentShader += "\tfloat diffStrength = max(dot(lightDir, normal), 0.0);\n";
            objPtr->shader.fragmentShader += "\tvec3 reflectDir = reflect(-lightDir, normal);\n";
            objPtr->shader.fragmentShader += "\tvec3 halfwayDir = normalize(lightDir + viewDir);\n";
            objPtr->shader.fragmentShader += "\tfloat specStrength = 0.0f;\n";
            objPtr->shader.fragmentShader += "\tif (shading == 0)\n";
            objPtr->shader.fragmentShader += "\t\tspecStrength = pow(max(dot(viewDir, reflectDir), 0.0), modelMaterial.shininess);\n";
            objPtr->shader.fragmentShader += "\telse if (shading == 1)\n";
            objPtr->shader.fragmentShader += "\t\tspecStrength = pow(max(dot(normal, halfwayDir), 0.0), modelMaterial.shininess);\n";
            
            objPtr->shader.fragmentShader += "\tvec4 ambient = vec4(-1.0);\n";
            objPtr->shader.fragmentShader += "\tvec4 diffuse = vec4(-1.0);\n";
            objPtr->shader.fragmentShader += "\tvec4 specular = vec4(0.0);\n";
            if (objPtr->material.texture) {
                objPtr->shader.fragmentShader += "\tint complexOrder = int(TexOrder);\n";
                objPtr->shader.fragmentShader += "\tint texQuantity = int(TexQty);\n";
                objPtr->shader.fragmentShader += "\tfor (int i = 0; i < texQuantity; i++) {\n";
                objPtr->shader.fragmentShader += "\t\tint remainder = complexOrder;\n";
                objPtr->shader.fragmentShader += "\t\tint division;\n";
                objPtr->shader.fragmentShader += "\t\tfor (int j = texQuantity; j > i; j--) {\n";
                objPtr->shader.fragmentShader += "\t\t\tdivision = int(remainder / pow(2, 4 * (j - 1)));\n";
                objPtr->shader.fragmentShader += "\t\t\tremainder = remainder - int(division * pow(2, 4 * (j - 1)));\n";
                objPtr->shader.fragmentShader += "\t\t}\n";
                objPtr->shader.fragmentShader += "\t\tint order = division;\n";
                objPtr->shader.fragmentShader += "\t\tbool specMap = false;\n";
                objPtr->shader.fragmentShader += "\t\tbool normMap = false;\n";
                objPtr->shader.fragmentShader += (objPtr->material.specMapIndexes.size() > 0) ? "\t\tfor (int j = 0; j < specMapIndexes.length(); j++)\n" : "";
                objPtr->shader.fragmentShader += (objPtr->material.specMapIndexes.size() > 0) ? "\t\t\tif (specMapIndexes[j] == order)\n" : "";
                objPtr->shader.fragmentShader += (objPtr->material.specMapIndexes.size() > 0) ? "\t\t\t\tspecMap = true;\n" : "";
                objPtr->shader.fragmentShader += (objPtr->material.normMapIndexes.size() > 0) ? "\t\tfor (int j = 0; j < normMapIndexes.length(); j++)\n" : "";
                objPtr->shader.fragmentShader += (objPtr->material.normMapIndexes.size() > 0) ? "\t\t\tif (normMapIndexes[j] == order)\n" : "";
                objPtr->shader.fragmentShader += (objPtr->material.normMapIndexes.size() > 0) ? "\t\t\t\tnormMap = true;\n" : "";
                objPtr->shader.fragmentShader += "\t\tif (normMap)\n";
                objPtr->shader.fragmentShader += "\t\t\tcontinue;\n";
                objPtr->shader.fragmentShader += "\t\tvec4 texv4 = vec4(0.0);\n";
                for (int i = 0; i < objPtr->material.texturesBase64.size(); i++)
                    objPtr->shader.fragmentShader += "\t\tif (order == " + to_string(i) + ") texv4 = texture(texture" + to_string(i) + ", TexCoord);\n";
//                objPtr->shader.fragmentShader += "\t\tvec4 texv4 = texture(textures[order], TexCoord);\n";
                objPtr->shader.fragmentShader += "\t\tif (specMap)\n";
                objPtr->shader.fragmentShader += "\t\t\tspecular = vec4(lights[lightIndex].material.specular, 1.0f) * specStrength * texv4;\n";
                objPtr->shader.fragmentShader += "\t\telse {\n";
                objPtr->shader.fragmentShader += "\t\t\tambient = vec4(lights[lightIndex].material.ambient, 1.0f) * texv4 * vec4(modelMaterial.diffuse, 1.0f);\n";
                objPtr->shader.fragmentShader += "\t\t\tdiffuse = vec4(lights[lightIndex].material.diffuse, 1.0f) * diffStrength * texv4 * vec4(modelMaterial.diffuse, 1.0f);\n";
                objPtr->shader.fragmentShader += "\t\t}\n";
                objPtr->shader.fragmentShader += "\t}\n";
            }
            objPtr->shader.fragmentShader += "\tif (ambient == vec4(-1.0)) ambient = vec4(lights[lightIndex].material.ambient, 1.0f) * vec4(modelMaterial.ambient, 1.0f);\n";
            objPtr->shader.fragmentShader += "\tif (diffuse == vec4(-1.0)) diffuse = vec4(lights[lightIndex].material.diffuse, 1.0f) * diffStrength * vec4(modelMaterial.diffuse, 1.0f);\n";
//            objPtr->shader.fragmentShader += "\tif (specular == vec4(0.0)) specular += vec4(lights[lightIndex].material.specular, 1.0f) * specStrength * vec4(modelMaterial.specular, 1.0f);\n";
            
            objPtr->shader.fragmentShader += "\tif (lights[lightIndex].lightType != 1) {\n";
            objPtr->shader.fragmentShader += "\t\tfloat distance = length(lightPos - FragPos);\n";
            objPtr->shader.fragmentShader += "\t\tfloat attenuation = 1.0 / (lights[lightIndex].constant + lights[lightIndex].linear * distance + lights[lightIndex].quadratic * (distance * distance));\n";
            objPtr->shader.fragmentShader += "\t\tambient.xyz *= attenuation;\n";
            objPtr->shader.fragmentShader += "\t\tdiffuse.xyz *= attenuation;\n";
            objPtr->shader.fragmentShader += "\t\tspecular.xyz *= attenuation;\n";
            
            objPtr->shader.fragmentShader += "\t\tif (gamma) {\n";
            objPtr->shader.fragmentShader += "\t\tambient.xyz *= 1.0 / (distance * distance);\n";
            objPtr->shader.fragmentShader += "\t\tdiffuse.xyz *= 1.0 / (distance * distance);\n";
            objPtr->shader.fragmentShader += "\t\tspecular.xyz *= 1.0 / (distance * distance);\n";
            objPtr->shader.fragmentShader += "\t\t}\n";
            objPtr->shader.fragmentShader += "\t\tif (lights[lightIndex].lightType == 2) {\n";
            objPtr->shader.fragmentShader += "\t\t\tfloat theta = dot(lightDir, normalize(-lights[lightIndex].direction));\n";
            objPtr->shader.fragmentShader += "\t\t\tfloat epsilon = lights[lightIndex].cutOff - lights[lightIndex].outerCutOff;\n";
            objPtr->shader.fragmentShader += "\t\t\tfloat intensity = 0.0;\n";
            objPtr->shader.fragmentShader += "\t\t\tintensity += clamp((theta - lights[lightIndex].outerCutOff) / epsilon, 0.0, 1.0);\n";
            objPtr->shader.fragmentShader += "\t\t\tambient.xyz *= intensity;\n";
            objPtr->shader.fragmentShader += "\t\t\tdiffuse.xyz *= intensity;\n";
            objPtr->shader.fragmentShader += "\t\t\tspecular.xyz *= intensity;\n";
            objPtr->shader.fragmentShader += "\t\t}\n";
            objPtr->shader.fragmentShader += "\t}\n";
            objPtr->shader.fragmentShader += "\treturn (ambient + (1.0 - shadow) * (diffuse + specular));\n";
            
            if (shadows) {
                objPtr->shader.fragmentShader += "}\n";
                objPtr->shader.fragmentShader += "float CalculateShadow(int lightIndex) {\n";
                objPtr->shader.fragmentShader += "\tvec3 fragToLight = FragPos - lights[lightIndex].position;\n";
                objPtr->shader.fragmentShader += "\tif (lights[lightIndex].lightType == 1) {\n";
                objPtr->shader.fragmentShader += "\t\tvec4 lightSpaceVec = lights[lightIndex].lightSpace * vec4(FragPos, 1.0);\n";
                objPtr->shader.fragmentShader += "\t\tfragToLight = lightSpaceVec.xyz / lightSpaceVec.w;\n";
                objPtr->shader.fragmentShader += "\t\tfragToLight = fragToLight * 0.5 + 0.5;\n";
                objPtr->shader.fragmentShader += "\t}\n";
                objPtr->shader.fragmentShader += "\tfloat currentDepth = length(fragToLight);\n";
                objPtr->shader.fragmentShader += "\tif (lights[lightIndex].lightType == 1) currentDepth = fragToLight.z;\n";
                objPtr->shader.fragmentShader += "\tfloat closestDepth = 0.0;\n";
                objPtr->shader.fragmentShader += "\tfloat bias = 0.05;\n";
//                objPtr->shader.fragmentShader += "\tif (lights[lightIndex].lightType == 1) bias = 0.05;\n";
//                for (int i = 0; i < shadowFboPtrs.size(); i++)
//                    objPtr->shader.fragmentShader += "\tif (lightIndex == " + to_string(i) + ") closestDepth = texture(shadowDepthMap" + to_string(i) + ", fragToLight).r;\n";
//                objPtr->shader.fragmentShader += "\tclosestDepth *= farPlane;\n";
//                objPtr->shader.fragmentShader += "\treturn currentDepth - bias > closestDepth ? 1.0 : 0.0;\n";
                objPtr->shader.fragmentShader += "\tfloat shadow = 0.0;\n";
                objPtr->shader.fragmentShader += "\tint samples = 20;\n";
                objPtr->shader.fragmentShader += "\tfloat viewDistance = length(cameraPos - FragPos);\n";
                objPtr->shader.fragmentShader += "\tfloat diskRadius = (1.0 + (viewDistance / farPlane)) / 25.0;\n";
                objPtr->shader.fragmentShader += "\tfor(int i = 0; i < samples; ++i) {\n";
                for (int i = 0; i < shadowFboPtrs.size(); i++) {
                    if (shadowFboPtrs[i]->light.lightType == LightType::point || shadowFboPtrs[i]->light.lightType == LightType::spotlight)
                        objPtr->shader.fragmentShader += "\t\tif (lightIndex == " + to_string(i) + ") closestDepth = texture(shadowDepthMap" + to_string(i) + ", fragToLight + gridSamplingDisk[i] * diskRadius).r;\n";
                    else if (shadowFboPtrs[i]->light.lightType == LightType::directional)
                        objPtr->shader.fragmentShader += "\t\tif (lightIndex == " + to_string(i) + ") closestDepth = texture(shadowDepthMap" + to_string(i) + ", fragToLight.xy + gridSamplingDisk[i].xy * diskRadius).r;\n";
                }
                objPtr->shader.fragmentShader += "\t\tclosestDepth *= farPlane;\n";
                objPtr->shader.fragmentShader += "\t\tif(currentDepth - bias > closestDepth) shadow += 1.0;\n";
                objPtr->shader.fragmentShader += "\t}\n";
                objPtr->shader.fragmentShader += "\treturn shadow / float(samples);\n";
            }
        }
        else if (objPtr->type == ObjectType::Light) {
            objPtr->shader.fragmentShader += "uniform vec3 color;\n";
            objPtr->shader.fragmentShader += "void main() {\n";
            objPtr->shader.fragmentShader += "\tFragColor = vec4(color, 1.0f);\n";
        }
        else if (objPtr->type == ObjectType::Joint) {
            objPtr->shader.fragmentShader += "void main() {\n";
            objPtr->shader.fragmentShader += "\tFragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);\n";
        }
        else if (objPtr->type == ObjectType::Text) {
            objPtr->shader.fragmentShader += "in vec2 TexCoord;\n";
            objPtr->shader.fragmentShader += "uniform vec3 textColor;\n";
            objPtr->shader.fragmentShader += "uniform sampler2D text;\n";
            objPtr->shader.fragmentShader += "void main() {\n";
            objPtr->shader.fragmentShader += "\tvec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoord).r);\n";
            objPtr->shader.fragmentShader += "\tFragColor = vec4(textColor, 1.0) * sampled;\n";
        }
        else if (objPtr->type == ObjectType::Framebuffer && objPtr->style.fboType != FboType::shadow) {
            objPtr->shader.fragmentShader += "in vec2 TexCoord;\n";
            objPtr->shader.fragmentShader += "uniform sampler2D screenTexture;\n";
            objPtr->shader.fragmentShader += "const float offset = 1.0 / 300.0;\n";
            objPtr->shader.fragmentShader += "void main() {\n";
            if (objPtr->style.fboType == FboType::kernel || objPtr->style.kernel.size() > 0) {
                objPtr->shader.fragmentShader += "\tvec2 offsets[9] = vec2[](vec2(-offset, offset), vec2(0.0f, offset), vec2(offset, offset), vec2(-offset, 0.0f), vec2(0.0f, 0.0f), vec2(offset, 0.0f), vec2(-offset, -offset), vec2(0.0f, -offset), vec2(offset, -offset));\n";
                objPtr->shader.fragmentShader += "\tfloat kernel[9] = float[](";
                for (int i = 0; i < objPtr->style.kernel.size(); i++)
                    objPtr->shader.fragmentShader += to_string(objPtr->style.kernel[i]) + ",";
                objPtr->shader.fragmentShader.pop_back();
                objPtr->shader.fragmentShader += ");\n";
                objPtr->shader.fragmentShader += "\tvec3 sampleTex[9];\n";
                objPtr->shader.fragmentShader += "\tfor(int i = 0; i < 9; i++)\n";
                objPtr->shader.fragmentShader += "\t\tsampleTex[i] = vec3(texture(screenTexture, TexCoord.st + offsets[i]));\n";
                objPtr->shader.fragmentShader += "\tvec3 col = vec3(0.0);\n";
                objPtr->shader.fragmentShader += "\tfor(int i = 0; i < 9; i++)\n";
                objPtr->shader.fragmentShader += "\t\tcol += sampleTex[i] * kernel[i];\n";
                objPtr->shader.fragmentShader += "\tFragColor = vec4(col, 1.0);\n";
            }
            else if (objPtr->style.fboType == FboType::inverse)
                objPtr->shader.fragmentShader += "\tFragColor = vec4(vec3(1.0 - texture(screenTexture, TexCoord)), 1.0);\n";
            else if (objPtr->style.fboType == FboType::graysc) {
                objPtr->shader.fragmentShader += "\tFragColor = texture(screenTexture, TexCoord);\n";
                objPtr->shader.fragmentShader += "\tfloat average = 0.2126 * FragColor.r + 0.7152 * FragColor.g + 0.0722 * FragColor.b;\n";
                objPtr->shader.fragmentShader += "\tFragColor = vec4(average, average, average, 1.0);\n";
            }
            else {
                objPtr->shader.fragmentShader += "\tvec3 col = texture(screenTexture, TexCoord).rgb;\n";
                objPtr->shader.fragmentShader += "\tFragColor = vec4(col, 1.0);\n";
            }
        }
        objPtr->shader.fragmentShader += "}\0";
        
        if (objPtr->type == ObjectType::Framebuffer && objPtr->style.fboType == FboType::shadow) {
            if (objPtr->light.lightType != LightType::directional) {
                objPtr->shader.geometryShader = "#version 330 core\n";
                objPtr->shader.geometryShader += "layout(triangles) in;\n";
                objPtr->shader.geometryShader += "layout(triangle_strip, max_vertices=18) out;\n";
                objPtr->shader.geometryShader += "in VS_OUT { vec2 TexCoord; float TexOrder; float TexQty; } gs_in[];\n";
                objPtr->shader.geometryShader += "uniform mat4 shadowTransforms[6];\n";
                objPtr->shader.geometryShader += "out vec4 FragPos;\n";
                objPtr->shader.geometryShader += "out vec2 TexCoord;\n";
                objPtr->shader.geometryShader += "out float TexOrder;\n";
                objPtr->shader.geometryShader += "out float TexQty;\n";
                objPtr->shader.geometryShader += "void main() {\n";
                objPtr->shader.geometryShader += "\tfor(int face = 0; face < 6; ++face) {\n";
                objPtr->shader.geometryShader += "\t\tgl_Layer = face;\n";
                objPtr->shader.geometryShader += "\t\tfor(int i = 0; i < 3; ++i) {\n";
                objPtr->shader.geometryShader += "\t\t\tFragPos = gl_in[i].gl_Position;\n";
                objPtr->shader.geometryShader += "\t\t\tTexCoord = gs_in[i].TexCoord;\n";
                objPtr->shader.geometryShader += "\t\t\tTexOrder = gs_in[i].TexOrder;\n";
                objPtr->shader.geometryShader += "\t\t\tTexQty = gs_in[i].TexQty;\n";
                objPtr->shader.geometryShader += "\t\t\tgl_Position = shadowTransforms[face] * FragPos;\n";
                objPtr->shader.geometryShader += "\t\t\tEmitVertex();\n";
                objPtr->shader.geometryShader += "\t\t}\n";
                objPtr->shader.geometryShader += "\t\tEndPrimitive();\n";
                objPtr->shader.geometryShader += "\t}\n";
                objPtr->shader.geometryShader += "}\0";
            }
        }
        
        
        int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        const char *vertexShaderSource = objPtr->shader.vertexShader.c_str();
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        int success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
        }
        int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        const char *fragmentShaderSource = objPtr->shader.fragmentShader.c_str();
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
        }
        int geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
        if (!objPtr->shader.geometryShader.empty()) {
            const char *geometryShaderSource = objPtr->shader.geometryShader.c_str();
            glShaderSource(geometryShader, 1, &geometryShaderSource, NULL);
            glCompileShader(geometryShader);
            int success;
            char infoLog[512];
            glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(geometryShader, 512, NULL, infoLog);
                cout << "ERROR::SHADER::GEOMETRY::COMPILATION_FAILED\n" << infoLog << endl;
            }
        }
        objPtr->shader.shaderID = glCreateProgram();
        glAttachShader(objPtr->shader.shaderID, vertexShader);
        glAttachShader(objPtr->shader.shaderID, fragmentShader);
        if (!objPtr->shader.geometryShader.empty())
            glAttachShader(objPtr->shader.shaderID, geometryShader);
        glLinkProgram(objPtr->shader.shaderID);
        glGetProgramiv(objPtr->shader.shaderID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(objPtr->shader.shaderID, 512, NULL, infoLog);
            cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        if (!objPtr->shader.geometryShader.empty())
            glDeleteShader(geometryShader);
    }
    
//    if (objPtr->name == "jo1") {
//        cout << objPtr->shader.vertexShader << endl;
//        cout << objPtr->shader.geometryShader << endl;
//        cout << objPtr->shader.fragmentShader << endl;
//    }
    
    for (int i = 0; i < objPtr->subObjects.size(); i++)
        setShaders(objPtr->subObjects[i]);
}

void setBuffers(Object* objPtr)
{
    if (objPtr->type == ObjectType::Scene) {
//        cout << "object " + objPtr->name + " is not drawable, passing buffer phase" << endl;
    }
    else if (objPtr->shader.vertices.size() == 0 && objPtr->type != ObjectType::Text) {
//        cout << "object " + objPtr->name + " has no vertices, passing buffer phase" << endl;
    }
    else {
//        cout << "object " + objPtr->name + " is drawable, processing buffer phase" << endl;
        
        
        if (objPtr->type == ObjectType::Framebuffer) {
            if (objPtr->style.fboType != FboType::shadow) {
                glBufferData(GL_ARRAY_BUFFER, objPtr->shader.vertices.size() * 2 * sizeof(float), NULL, GL_STATIC_DRAW);
                glBufferSubData(GL_ARRAY_BUFFER, 0, objPtr->shader.vertices.size() * sizeof(float), &objPtr->shader.vertices[0]);
                glBufferSubData(GL_ARRAY_BUFFER, objPtr->shader.vertices.size() * sizeof(float), objPtr->shader.texCoords.size() * sizeof(float), &objPtr->shader.texCoords[0]);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(objPtr->shader.vertices.size() * sizeof(float)));
                glEnableVertexAttribArray(1);
            }
            glGenFramebuffers(1, &objPtr->shader.fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, objPtr->shader.fbo);
            objPtr->objectPtr->material.textures.push_back(*new unsigned int());
            glGenTextures(1, &objPtr->material.textures[0]);
            GLenum textype = objPtr->style.fboType == FboType::shadow ? GL_TEXTURE_CUBE_MAP : (multiSampling ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D);
            textype = objPtr->style.fboType == FboType::shadow && objPtr->light.lightType == LightType::directional ? GL_TEXTURE_2D : textype;
            glBindTexture(textype, objPtr->material.textures[0]);
            if (objPtr->style.fboType == FboType::shadow) {
                if (objPtr->light.lightType == LightType::point || objPtr->light.lightType == LightType::spotlight) {
                    for (unsigned int i = 0; i < 6; ++i)
                        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, objPtr->layout.width, objPtr->layout.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
                    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
                }
                else if (objPtr->light.lightType == LightType::directional) {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, objPtr->layout.width, objPtr->layout.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                }
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, objPtr->material.textures[0], 0);
                glDrawBuffer(GL_NONE);
                glReadBuffer(GL_NONE);
                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                    cout << "ERROR::FRAMEBUFFER:: Shadow framebuffer is not complete!" << endl;
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
            else {
                vector<Object>::iterator sce = find_if(objects.begin(), objects.end(), [] (Object obj) { return obj.type == ObjectType::Scene; });
                if (multiSampling)
                    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, sce->objectPtr->layout.width, sce->objectPtr->layout.height, GL_TRUE);
                else {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sce->objectPtr->layout.width, sce->objectPtr->layout.height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                }
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textype, objPtr->material.textures[0], 0);
                if (!multiSampling) {
                    glUseProgram(objPtr->shader.shaderID);
                    glUniform1i(glGetUniformLocation(objPtr->shader.shaderID, "screenTexture"), 0);
                }
                glGenRenderbuffers(1, &objPtr->shader.rbo);
                glBindRenderbuffer(GL_RENDERBUFFER, objPtr->shader.rbo);
                if (multiSampling)
                    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, sce->objectPtr->layout.width, sce->objectPtr->layout.height);
                else
                    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, sce->objectPtr->layout.width, sce->objectPtr->layout.height);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, objPtr->shader.rbo);
                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                    cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                
                if (multiSampling) {
                    glGenFramebuffers(1, &objPtr->shader.ebo);  // intermediate fbo olarak ebo kullanıldı
                    glBindFramebuffer(GL_FRAMEBUFFER, objPtr->shader.ebo);
                    objPtr->objectPtr->material.textures.push_back(*new unsigned int());
                    glGenTextures(1, &objPtr->material.textures[1]);
                    glBindTexture(GL_TEXTURE_2D, objPtr->material.textures[1]);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sce->objectPtr->layout.width, sce->objectPtr->layout.height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, objPtr->material.textures[1], 0);
                    glUseProgram(objPtr->shader.shaderID);
                    glUniform1i(glGetUniformLocation(objPtr->shader.shaderID, "screenTexture"), 0);
                    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                        cout << "ERROR::FRAMEBUFFER:: Intermediate framebuffer is not complete!" << endl;
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                }
            }
            return;
        }
        
        glGenVertexArrays(1, &objPtr->shader.vao);
        glGenBuffers(1, &objPtr->shader.vbo);
        if (objPtr->shader.faces.size() > 0)
            glGenBuffers(1, &objPtr->shader.ebo);
        glBindVertexArray(objPtr->shader.vao);
        glBindBuffer(GL_ARRAY_BUFFER, objPtr->shader.vbo);
        if (objPtr->type == ObjectType::Joint && objPtr->superObject->type == ObjectType::Joint) {
            objPtr->shader.vertices.insert(objPtr->shader.vertices.begin(),
                                           objPtr->superObject->shader.vertices.end() - 3,
                                           objPtr->superObject->shader.vertices.end());
        }
        if (objPtr->type != ObjectType::Text && objPtr->type != ObjectType::Cubemap) {
            int attrCount = objPtr->material.texture ? (objPtr->material.normMapIndexes.size() > 0 ? 13 : 10) : 6;
            if (objPtr->type == ObjectType::Joint || objPtr->type == ObjectType::Light)
                attrCount = 3;
            glBufferData(GL_ARRAY_BUFFER, objPtr->shader.vertices.size() / 3 * attrCount * sizeof(float), NULL, GL_DYNAMIC_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, objPtr->shader.vertices.size() * sizeof(float), &objPtr->shader.vertices[0]);
            if (objPtr->type == ObjectType::Model)
                glBufferSubData(GL_ARRAY_BUFFER, objPtr->shader.vertices.size() * sizeof(float), objPtr->shader.normals.size() * sizeof(float), &objPtr->shader.normals[0]);
            if (objPtr->material.texture) {
                glBufferSubData(GL_ARRAY_BUFFER, (objPtr->shader.vertices.size() + objPtr->shader.normals.size()) * sizeof(float), objPtr->shader.texCoords.size() * sizeof(float), &objPtr->shader.texCoords[0]);
                if (objPtr->shader.texOrders.size() == 0)
                    for(int i = 0; i < objPtr->shader.vertices.size() / 3; i++)
                        objPtr->shader.texOrders.push_back(0.0f);
                else if (objPtr->shader.texOrders.size() == 1)
                    for(int i = 0; i < objPtr->shader.vertices.size() / 3 - 1; i++)
                        objPtr->shader.texOrders.push_back(objPtr->shader.texOrders[0]);
                glBufferSubData(GL_ARRAY_BUFFER, (objPtr->shader.vertices.size() + objPtr->shader.normals.size() + objPtr->shader.texCoords.size()) * sizeof(float), objPtr->shader.texOrders.size() * sizeof(float), &objPtr->shader.texOrders[0]);
                if (objPtr->shader.texQuantities.size() == 0)
                    for(int i = 0; i < objPtr->shader.vertices.size() / 3; i++)
                        objPtr->shader.texQuantities.push_back(1.0f);
                else if (objPtr->shader.texQuantities.size() == 1)
                    for(int i = 0; i < objPtr->shader.vertices.size() / 3 - 1; i++)
                        objPtr->shader.texQuantities.push_back(objPtr->shader.texQuantities[0]);
                glBufferSubData(GL_ARRAY_BUFFER, (objPtr->shader.vertices.size() + objPtr->shader.normals.size() + objPtr->shader.texCoords.size() + objPtr->shader.texOrders.size()) * sizeof(float), objPtr->shader.texQuantities.size() * sizeof(float), &objPtr->shader.texQuantities[0]);
                
                if (objPtr->material.normMapIndexes.size() > 0) {
                    if (objPtr->shader.tangents.size() == 0)
                        calculateTangents(objPtr);
                    glBufferSubData(GL_ARRAY_BUFFER, (objPtr->shader.vertices.size() + objPtr->shader.normals.size() + objPtr->shader.texCoords.size() + objPtr->shader.texOrders.size() + objPtr->shader.texQuantities.size()) * sizeof(float), objPtr->shader.tangents.size() * sizeof(float), &objPtr->shader.tangents[0]);
                }
            }
            
            if (objPtr->instance.instanced) {
                if (objPtr->instance.translate.size() >= 3)
                    objPtr->shader.instanceCount = int(objPtr->instance.translate.size() / 3);
                else if (objPtr->instance.scale.size() >= 3)
                    objPtr->shader.instanceCount = int(objPtr->instance.scale.size() / 3);
                else if (objPtr->instance.front.size() >= 3)
                    objPtr->shader.instanceCount = int(objPtr->instance.front.size() / 3);
                else if (objPtr->instance.up.size() >= 3)
                    objPtr->shader.instanceCount = int(objPtr->instance.up.size() / 3);
                else if (objPtr->instance.left.size() >= 3)
                    objPtr->shader.instanceCount = int(objPtr->instance.left.size() / 3);
                
                for (int i = 0; i < objPtr->shader.instanceCount; i++) {
                    glm::mat4 insmatrix, insrotmatrix;
                    glm::vec3 translate, scale, front, up, left;
                    translate = (objPtr->instance.translate.size() > 3) ? glm::vec3(objPtr->instance.translate[i * 3], objPtr->instance.translate[i * 3 + 1], objPtr->instance.translate[i * 3 + 2]) : (objPtr->instance.translate.size() == 3 ? glm::vec3(objPtr->instance.translate[0], objPtr->instance.translate[1], objPtr->instance.translate[2]) : glm::vec3(0.0f));
                    scale = (objPtr->instance.scale.size() > 3) ? glm::vec3(objPtr->instance.scale[i * 3], objPtr->instance.scale[i * 3 + 1], objPtr->instance.scale[i * 3 + 2]) : (objPtr->instance.scale.size() == 3 ? glm::vec3(objPtr->instance.scale[0], objPtr->instance.scale[1], objPtr->instance.scale[2]) : glm::vec3(1.0f));
                    front = (objPtr->instance.front.size() > 3) ? glm::vec3(objPtr->instance.front[i * 3], objPtr->instance.front[i * 3 + 1], objPtr->instance.front[i * 3 + 2]) : (objPtr->instance.front.size() == 3 ? glm::vec3(objPtr->instance.front[0], objPtr->instance.front[1], objPtr->instance.front[2]) : glm::vec3(0.0f, 0.0f, 1.0f));
                    up = (objPtr->instance.up.size() > 3) ? glm::vec3(objPtr->instance.up[i * 3], objPtr->instance.up[i * 3 + 1], objPtr->instance.up[i * 3 + 2]) : (objPtr->instance.up.size() == 3 ? glm::vec3(objPtr->instance.up[0], objPtr->instance.up[1], objPtr->instance.up[2]) : glm::vec3(0.0f, 1.0f, 0.0f));
                    left = (objPtr->instance.left.size() > 3) ? glm::vec3(objPtr->instance.left[i * 3], objPtr->instance.left[i * 3 + 1], objPtr->instance.left[i * 3 + 2]) : (objPtr->instance.left.size() == 3 ? glm::vec3(objPtr->instance.left[0], objPtr->instance.left[1], objPtr->instance.left[2]) : glm::vec3(1.0f, 0.0f, 0.0f));
                    insmatrix = glm::translate(glm::mat4(1.0f), translate);
                    insmatrix = glm::scale(insmatrix, scale);
                    insrotmatrix = glm::mat4(left.x,left.y, left.z, 0.0f,
                                            up.x, up.y, up.z, 0.0f,
                                            front.x, front.y, front.z, 0.0f,
                                            0.0f, 0.0f, 0.0f, 1.0f);
                    insmatrix *= insrotmatrix;
                    objPtr->instance.instanceMatrices.push_back(insmatrix);
                    if (objPtr->type != ObjectType::Joint)
                        objPtr->instance.instanceMatrices.push_back(insrotmatrix);
                }
                glBindVertexArray(0);
                glGenBuffers(1, &objPtr->shader.ibo);
                glBindBuffer(GL_ARRAY_BUFFER, objPtr->shader.ibo);
                if (objPtr->type == ObjectType::Model)
                    glBufferData(GL_ARRAY_BUFFER, objPtr->shader.instanceCount * 2 * sizeof(glm::mat4), &objPtr->instance.instanceMatrices[0], GL_STATIC_DRAW);
                else if (objPtr->type == ObjectType::Joint)
                    glBufferData(GL_ARRAY_BUFFER, objPtr->shader.instanceCount * sizeof(glm::mat4), &objPtr->instance.instanceMatrices[0], GL_STATIC_DRAW);
                glBindVertexArray(objPtr->shader.vao);
            }
        }
        else if (objPtr->type == ObjectType::Text) {
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
            glEnableVertexAttribArray(0);
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        
        if (objPtr->shader.faces.size() > 0) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objPtr->shader.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, objPtr->shader.faces.size() * sizeof(float), &objPtr->shader.faces[0], GL_DYNAMIC_DRAW);
        }
        
        if (objPtr->type == ObjectType::Model) {
            glBindBuffer(GL_ARRAY_BUFFER, objPtr->shader.vbo);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(objPtr->shader.vertices.size() * sizeof(float)));
            glEnableVertexAttribArray(1);
            if (objPtr->material.texture) {
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)((objPtr->shader.vertices.size() + objPtr->shader.normals.size()) * sizeof(float)));
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(float), (void*)((objPtr->shader.vertices.size() + objPtr->shader.normals.size() + objPtr->shader.texCoords.size()) * sizeof(float)));
                glEnableVertexAttribArray(3);
                glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(float), (void*)((objPtr->shader.vertices.size() + objPtr->shader.normals.size() + objPtr->shader.texCoords.size() + objPtr->shader.texOrders.size()) * sizeof(float)));
                glEnableVertexAttribArray(4);
                if (objPtr->material.normMapIndexes.size() > 0) {
                    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)((objPtr->shader.vertices.size() + objPtr->shader.normals.size() + objPtr->shader.texCoords.size() + objPtr->shader.texOrders.size() + objPtr->shader.texQuantities.size()) * sizeof(float)));
                    glEnableVertexAttribArray(5);
                    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)((objPtr->shader.vertices.size() + objPtr->shader.normals.size() + objPtr->shader.texCoords.size() + objPtr->shader.texOrders.size() + objPtr->shader.texQuantities.size() + objPtr->shader.tangents.size()) * sizeof(float)));
                    glEnableVertexAttribArray(6);
                }
            }
            if (objPtr->instance.instanced) {
                glBindBuffer(GL_ARRAY_BUFFER, objPtr->shader.ibo);
                glBindVertexArray(objPtr->shader.vao);
                int attrCount = objPtr->material.texture ? ((objPtr->material.normMapIndexes.size() > 0) ? 6 : 5) : 2;
                glVertexAttribPointer(attrCount, 4, GL_FLOAT, GL_FALSE, 32 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(attrCount);
                glVertexAttribPointer(attrCount + 1, 4, GL_FLOAT, GL_FALSE, 32 * sizeof(float), (void*)(4 * sizeof(float)));
                glEnableVertexAttribArray(attrCount + 1);
                glVertexAttribPointer(attrCount + 2, 4, GL_FLOAT, GL_FALSE, 32 * sizeof(float), (void*)(8 * sizeof(float)));
                glEnableVertexAttribArray(attrCount + 2);
                glVertexAttribPointer(attrCount + 3, 4, GL_FLOAT, GL_FALSE, 32 * sizeof(float), (void*)(12 * sizeof(float)));
                glEnableVertexAttribArray(attrCount + 3);
                glVertexAttribPointer(attrCount + 4, 4, GL_FLOAT, GL_FALSE, 32 * sizeof(float), (void*)(16 * sizeof(float)));
                glEnableVertexAttribArray(attrCount + 4);
                glVertexAttribPointer(attrCount + 5, 4, GL_FLOAT, GL_FALSE, 32 * sizeof(float), (void*)(20 * sizeof(float)));
                glEnableVertexAttribArray(attrCount + 5);
                glVertexAttribPointer(attrCount + 6, 4, GL_FLOAT, GL_FALSE, 32 * sizeof(float), (void*)(24 * sizeof(float)));
                glEnableVertexAttribArray(attrCount + 6);
                glVertexAttribPointer(attrCount + 7, 4, GL_FLOAT, GL_FALSE, 32 * sizeof(float), (void*)(28 * sizeof(float)));
                glEnableVertexAttribArray(attrCount + 7);
                
                glVertexAttribDivisor(attrCount, 1);
                glVertexAttribDivisor(attrCount + 1, 1);
                glVertexAttribDivisor(attrCount + 2, 1);
                glVertexAttribDivisor(attrCount + 3, 1);
                glVertexAttribDivisor(attrCount + 4, 1);
                glVertexAttribDivisor(attrCount + 5, 1);
                glVertexAttribDivisor(attrCount + 6, 1);
                glVertexAttribDivisor(attrCount + 7, 1);
            }
            
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            if (objPtr->material.texture) {
                stbi_set_flip_vertically_on_load(true);
                int width, height, channels;
                for (int i = 0; i < objPtr->material.texturesBase64.size(); i++) {
                    vector<int>::iterator itr;
                    bool isNormalMap = false;
                    itr = find(objPtr->material.normMapIndexes.begin(), objPtr->material.normMapIndexes.end(), i);
                    if (itr != objPtr->material.normMapIndexes.end())
                        isNormalMap = true;
                    objPtr->objectPtr->material.textures.push_back(*new unsigned int());
                    glGenTextures(1, &objPtr->material.textures[i]);
                    glBindTexture(GL_TEXTURE_2D, objPtr->material.textures[i]);
                    vector<unsigned char> decoded = base64Decode(objPtr->objectPtr->material.texturesBase64[i]);
                    unsigned char *data = stbi_load_from_memory(&decoded[0], int(decoded.size()), &width, &height, &channels, 0);
//                    glPixelStorei(GL_PACK_ALIGNMENT, 1);
                    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                    if (channels == 1)
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
                    else if (channels == 3) {
                        if (gammaCorrection)
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                        else
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                    }
                    else if (channels == 4) {
                        if (gammaCorrection)
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                        else
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                    }
                    
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    if (isNormalMap) {
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    }
                    else {
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    }
                    glGenerateMipmap(GL_TEXTURE_2D);
                    stbi_image_free(data);
                    
                    glUseProgram(objPtr->shader.shaderID);
//                    glUniform1i(glGetUniformLocation(objPtr->shader.shaderID, ("textures[" + to_string(i) + "]").c_str()), i);
                    glUniform1i(glGetUniformLocation(objPtr->shader.shaderID, ("texture" + to_string(i)).c_str()), i);
                    
                    itr = find(objPtr->material.specMapIndexes.begin(), objPtr->material.specMapIndexes.end(), i);
                    if (itr != objPtr->material.specMapIndexes.end())
                        glUniform1i(glGetUniformLocation(objPtr->shader.shaderID, ("specMapIndexes[" + to_string(itr - objPtr->material.specMapIndexes.begin()) + "]").c_str()), i);
                    itr = find(objPtr->material.normMapIndexes.begin(), objPtr->material.normMapIndexes.end(), i);
                    if (itr != objPtr->material.normMapIndexes.end())
                        glUniform1i(glGetUniformLocation(objPtr->shader.shaderID, ("normMapIndexes[" + to_string(itr - objPtr->material.normMapIndexes.begin()) + "]").c_str()), i);
                }
            }
            if (shadows) {
                glUseProgram(objPtr->shader.shaderID);
                for (int i = 0; i < shadowFboPtrs.size(); i++)
                    glUniform1i(glGetUniformLocation(objPtr->shader.shaderID, ("shadowDepthMap" + to_string(i)).c_str()), int(objPtr->material.texturesBase64.size()) + i);
            }
        }
        else if (objPtr->type == ObjectType::Light || objPtr->type == ObjectType::Joint) {
            glBindBuffer(GL_ARRAY_BUFFER, objPtr->shader.vbo);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            if (objPtr->type == ObjectType::Joint && objPtr->instance.instanced) {
                glBindBuffer(GL_ARRAY_BUFFER, objPtr->shader.ibo);
                glBindVertexArray(objPtr->shader.vao);
                glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void*)(4 * sizeof(float)));
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void*)(8 * sizeof(float)));
                glEnableVertexAttribArray(3);
                glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void*)(12 * sizeof(float)));
                glEnableVertexAttribArray(4);

                glVertexAttribDivisor(1, 1);
                glVertexAttribDivisor(2, 1);
                glVertexAttribDivisor(3, 1);
                glVertexAttribDivisor(4, 1);
            }
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        else if (objPtr->type == ObjectType::Text) {
            if (FT_Init_FreeType(&ft))
                cout << "ERROR::FREETYPE: Could not init FreeType Library" << endl;
            string path = WORK_DIR + "fonts/" + objPtr->style.font + ".ttf";
            if (path.empty())
                cout << "ERROR::FREETYPE: Failed to load font_name" << endl;
            if (FT_New_Face(ft, path.c_str(), 0, &face))
                cout << "ERROR::FREETYPE: Failed to load font" << endl;
            if (!FT_New_Face(ft, path.c_str(), 0, &face)) {
                FT_Set_Pixel_Sizes(face, 0, 48);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                for (unsigned char c = 0; c < 128; c++) {
                    if (!FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                        unsigned int texture;
                        glGenTextures(1, &texture);
                        glBindTexture(GL_TEXTURE_2D, texture);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        Character character = {texture, glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows), glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top), static_cast<unsigned int>(face->glyph->advance.x)};
                        characters.insert(pair<char, Character>(c, character));
                    }
                }
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            FT_Done_Face(face);
            FT_Done_FreeType(ft);
        }
        
        if (objPtr->type == ObjectType::Joint)
            objPtr->shader.vertexCount = int(objPtr->shader.vertices.size() / 3);
        else if (objPtr->shader.faces.size() > 0)
            objPtr->shader.vertexCount = int(objPtr->shader.faces.size());
        else
            objPtr->shader.vertexCount = int(objPtr->shader.vertices.size() / 3);
    }
    
    for (int i = 0; i < objPtr->subObjects.size(); i++)
        setBuffers(objPtr->subObjects[i]);
}

void drawScene(Object* objPtr)
{
    if (objPtr->hidden)
        return;
    
    if (objPtr->type == ObjectType::Scene || objPtr->type == ObjectType::Framebuffer) {
//        cout << "object " + objPtr->name + " is not drawable, passing draw phase" << endl;
    }
    else if (objPtr->shader.vertices.size() == 0 && objPtr->type != ObjectType::Text) {
//        cout << "object " + objPtr->name + " is not drawable, passing draw phase" << endl;
    }
    else {
//        cout << "object " + objPtr->name + " is drawable, processing draw phase" << endl;
        
        glUseProgram(objPtr->shader.shaderID);
        if (objPtr->type != ObjectType::Text && objPtr->type != ObjectType::Cubemap) {
            glUniformMatrix4fv(glGetUniformLocation(objPtr->shader.shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));
            glUniformMatrix4fv(glGetUniformLocation(objPtr->shader.shaderID, "view"), 1, GL_FALSE, value_ptr(view));
            glm::mat4 model = glm::translate(glm::mat4(1.0f), objPtr->transform.position);
            model = glm::scale(model, objPtr->transform.scale);
            glm::mat4 rotation = glm::mat4(objPtr->transform.left.x, objPtr->transform.left.y, objPtr->transform.left.z, 0,
                              objPtr->transform.up.x, objPtr->transform.up.y, objPtr->transform.up.z, 0,
                              objPtr->transform.front.x, objPtr->transform.front.y, objPtr->transform.front.z, 0,
                              0, 0, 0, 1);
            model *= rotation;
            glUniformMatrix4fv(glGetUniformLocation(objPtr->shader.shaderID, "model"), 1, GL_FALSE,  value_ptr(model));
            glUniformMatrix4fv(glGetUniformLocation(objPtr->shader.shaderID, "rotation"), 1, GL_FALSE,  value_ptr(rotation));
        }
        else if (objPtr->type == ObjectType::Text) {
            glUniformMatrix4fv(glGetUniformLocation(objPtr->shader.shaderID, "projection"), 1, GL_FALSE, value_ptr(textprojection));
            glUniform3fv(glGetUniformLocation(objPtr->shader.shaderID, "textColor"), 1, value_ptr(objPtr->layout.color));
            glActiveTexture(GL_TEXTURE0);
        }
        
        if (objPtr->type == ObjectType::Model) {
            glUniform3fv(glGetUniformLocation(objPtr->shader.shaderID, "cameraPos"), 1, value_ptr(cameraPtr->transform.position));
            int shade = (shading == "phong") ? 0 : (shading == "blinn-phong") ? 1 : 0;
            glUniform1i(glGetUniformLocation(objPtr->shader.shaderID, "shading"), shade);
            glUniform1i(glGetUniformLocation(objPtr->shader.shaderID, "gamma"), gammaCorrection);

            glUniform3fv(glGetUniformLocation(objPtr->shader.shaderID, "modelMaterial.ambient"), 1, value_ptr(objPtr->material.ambient));
            glUniform3fv(glGetUniformLocation(objPtr->shader.shaderID, "modelMaterial.diffuse"), 1, value_ptr(objPtr->material.diffuse));
            glUniform3fv(glGetUniformLocation(objPtr->shader.shaderID, "modelMaterial.specular"), 1, value_ptr(objPtr->material.specular));
            glUniform1f(glGetUniformLocation(objPtr->shader.shaderID, "modelMaterial.shininess"), objPtr->material.shininess);
            
            if (shadows) {
                glUniform1i(glGetUniformLocation(objPtr->shader.shaderID, "shadows"), shadows);
                glUniform1f(glGetUniformLocation(objPtr->shader.shaderID, "farPlane"), shadowFarPlane);
            }
            
            vector<Object>::iterator it = objects.begin();
            while ((it = find_if(it, objects.end(), [] (Object obj) { return obj.type == ObjectType::Light; })) != objects.end()) {
                int index = int(it - objects.begin());
                if (it->objectPtr->hidden) {
                    glUniform1i(glGetUniformLocation(objPtr->shader.shaderID, ("lights[" + to_string(index) + "].lightType").c_str()), -1);
                    it++;
                    continue;
                }
                glUniform1i(glGetUniformLocation(objPtr->shader.shaderID, ("lights[" + to_string(index) + "].lightType").c_str()), static_cast<int>(it->objectPtr->light.lightType));
                glUniform3fv(glGetUniformLocation(objPtr->shader.shaderID, ("lights[" + to_string(index) + "].direction").c_str()), 1, value_ptr(it->objectPtr->transform.front));
                glUniform3fv(glGetUniformLocation(objPtr->shader.shaderID, ("lights[" + to_string(index) + "].position").c_str()), 1, value_ptr(it->objectPtr->transform.position));
                glUniform1f(glGetUniformLocation(objPtr->shader.shaderID, ("lights[" + to_string(index) + "].constant").c_str()), it->objectPtr->light.constant);
                glUniform1f(glGetUniformLocation(objPtr->shader.shaderID, ("lights[" + to_string(index) + "].linear").c_str()), it->objectPtr->light.linear);
                glUniform1f(glGetUniformLocation(objPtr->shader.shaderID, ("lights[" + to_string(index) + "].quadratic").c_str()), it->objectPtr->light.quadratic);
                glUniform1f(glGetUniformLocation(objPtr->shader.shaderID, ("lights[" + to_string(index) + "].cutOff").c_str()), it->objectPtr->light.cutOff);
                glUniform1f(glGetUniformLocation(objPtr->shader.shaderID, ("lights[" + to_string(index) + "].outerCutOff").c_str()), it->objectPtr->light.outerCutOff);
                glUniform3fv(glGetUniformLocation(objPtr->shader.shaderID, ("lights[" + to_string(index) + "].material.ambient").c_str()), 1, value_ptr(it->objectPtr->material.ambient));
                glUniform3fv(glGetUniformLocation(objPtr->shader.shaderID, ("lights[" + to_string(index) + "].material.diffuse").c_str()), 1, value_ptr(it->objectPtr->material.diffuse));
                glUniform3fv(glGetUniformLocation(objPtr->shader.shaderID, ("lights[" + to_string(index) + "].material.specular").c_str()), 1, value_ptr(it->objectPtr->material.specular));
                glUniformMatrix4fv(glGetUniformLocation(objPtr->shader.shaderID, ("lights[" + to_string(index) + "].lightSpace").c_str()), 1, GL_FALSE, value_ptr(it->objectPtr->light.lightSpace));
                it++;
            }
            
            if (objPtr->material.texture) {
                for (int i = 0; i < objPtr->material.textures.size(); i++) {
                    glActiveTexture(GL_TEXTURE0 + i);
                    glBindTexture(GL_TEXTURE_2D, objPtr->material.textures[i]);
                }
            }
            if (shadows) {
                for (int i = 0; i < shadowFboPtrs.size(); i++) {
                    glActiveTexture(GL_TEXTURE0 + int(objPtr->material.textures.size()) + i);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, shadowFboPtrs[i]->material.textures[0]);
                }
            }
        }
        else if (objPtr->type == ObjectType::Light) {
            glUniform3fv(glGetUniformLocation(objPtr->shader.shaderID, "color"), 1, value_ptr(objPtr->material.diffuse / 0.8f));
        }
        
        glBindVertexArray(objPtr->shader.vao);
        
        if (objPtr->type == ObjectType::Joint) {
            if (objPtr->shader.vertexCount > 1 && showJoints) {
                if (objPtr->instance.instanced) {
                    glDrawArraysInstanced(GL_LINES, 0, objPtr->shader.vertexCount, objPtr->shader.instanceCount);
                    glDrawArraysInstanced(GL_POINTS, 0, 2, objPtr->shader.instanceCount);
                }
                else {
                    glDrawArrays(GL_LINES, 0, objPtr->shader.vertexCount);
                    glDrawArrays(GL_POINTS, 0, 2);
                }
            }
        }
        else if (objPtr->type == ObjectType::Text) {
            float xbychar = objPtr->layout.x;
            string::const_iterator c;
            for (c = objPtr->style.text.begin(); c != objPtr->style.text.end(); c++) {
                Character ch = characters[*c];
                float xpos = xbychar + ch.bearing.x * objPtr->layout.size;
                float ypos = objPtr->layout.y - (ch.size.y - ch.bearing.y) * objPtr->layout.size;
                float w = ch.size.x * objPtr->layout.size;
                float h = ch.size.y * objPtr->layout.size;
                float textvertex[6][4] = {{ xpos,     ypos + h,   0.0f, 0.0f },
                                        { xpos,     ypos,       0.0f, 1.0f },
                                        { xpos + w, ypos,       1.0f, 1.0f },
                                        { xpos,     ypos + h,   0.0f, 0.0f },
                                        { xpos + w, ypos,       1.0f, 1.0f },
                                        { xpos + w, ypos + h,   1.0f, 0.0f }};
                glBindTexture(GL_TEXTURE_2D, ch.textureID);
                glBindBuffer(GL_ARRAY_BUFFER, objPtr->shader.vbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(textvertex), textvertex);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                xbychar += (ch.advance >> 6) * objPtr->layout.size;
            }
        }
        else if (objPtr->shader.faces.size() > 0) {
            if (objPtr->instance.instanced)
                glDrawElementsInstanced(GL_TRIANGLES, objPtr->shader.vertexCount, GL_UNSIGNED_INT, 0, objPtr->shader.instanceCount);
            else
                glDrawElements(GL_TRIANGLES, objPtr->shader.vertexCount, GL_UNSIGNED_INT, 0);
        }
        else {
            if (objPtr->instance.instanced)
                glDrawArraysInstanced(GL_TRIANGLES, 0, objPtr->shader.vertexCount, objPtr->shader.instanceCount);
            else
                glDrawArrays(GL_TRIANGLES, 0, objPtr->shader.vertexCount);
        }
            
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    for (int i = 0; i < objPtr->subObjects.size(); i++)
        drawScene(objPtr->subObjects[i]);
}

void deleteScene(Object* objPtr)
{
    if (objPtr->type != ObjectType::Model &&
        objPtr->type != ObjectType::Light &&
        objPtr->type != ObjectType::Joint) {
//        cout << "object " + objPtr->name + " is not drawable, passing delete phase" << endl;
    }
    else if (objPtr->shader.vertices.size() == 0) {
//        cout << "object " + objPtr->name + " is not drawable, passing delete phase" << endl;
    }
    else {
//        cout << "object " + objPtr->name + " is drawable, processing delete phase" << endl;
        
        glDeleteProgram(objPtr->shader.shaderID);
        glDeleteBuffers(1, &objPtr->shader.vbo);
        glDeleteVertexArrays(1, &objPtr->shader.vao);
    }
    for (int i = 0; i < objPtr->subObjects.size(); i++)
        deleteScene(objPtr->subObjects[i]);
}

void drawShadows(Object* objPtr, Object* shadowPtr, bool hidden)
{
    if (objPtr->type == ObjectType::Model && objPtr->shader.vertices.size() > 0 && !objPtr->hidden) {
        glUseProgram(shadowPtr->shader.shaderID);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), objPtr->transform.position);
        model = glm::scale(model, objPtr->transform.scale);
        glm::mat4 rotation = glm::mat4(objPtr->transform.left.x, objPtr->transform.left.y, objPtr->transform.left.z, 0,
                          objPtr->transform.up.x, objPtr->transform.up.y, objPtr->transform.up.z, 0,
                          objPtr->transform.front.x, objPtr->transform.front.y, objPtr->transform.front.z, 0,
                          0, 0, 0, 1);
        model *= rotation;
        glUniformMatrix4fv(glGetUniformLocation(shadowPtr->shader.shaderID, "model"), 1, GL_FALSE,  value_ptr(model));
        if (objPtr->material.texture) {
            for (int i = 0; i < objPtr->material.textures.size(); i++) {
                glUniform1i(glGetUniformLocation(shadowPtr->shader.shaderID, ("textures[" + to_string(i) + "]").c_str()), i);
                vector<int>::iterator itr = find(objPtr->material.specMapIndexes.begin(), objPtr->material.specMapIndexes.end(), i);
                if (itr != objPtr->material.specMapIndexes.end())
                    glUniform1i(glGetUniformLocation(shadowPtr->shader.shaderID, ("specMapIndexes[" + to_string(itr - objPtr->material.specMapIndexes.begin()) + "]").c_str()), i);
                itr = find(objPtr->material.normMapIndexes.begin(), objPtr->material.normMapIndexes.end(), i);
                if (itr != objPtr->material.normMapIndexes.end())
                    glUniform1i(glGetUniformLocation(shadowPtr->shader.shaderID, ("normMapIndexes[" + to_string(itr - objPtr->material.normMapIndexes.begin()) + "]").c_str()), i);
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, objPtr->material.textures[i]);
            }
        }
        int vertexCount = hidden ? 0 : objPtr->shader.vertexCount;
        glBindVertexArray(objPtr->shader.vao);
        if (objPtr->shader.faces.size() > 0) {
            if (objPtr->instance.instanced) {
                for (int i = 0; i < objPtr->instance.instanceMatrices.size(); i++) {
                    glUniformMatrix4fv(glGetUniformLocation(shadowPtr->shader.shaderID, "model"), 1, GL_FALSE,  value_ptr(model * objPtr->instance.instanceMatrices[i]));
                    glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, 0);
                }
            }
            else
                glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, 0);
        }
        else {
            if (objPtr->instance.instanced) {
                for (int i = 0; i < objPtr->instance.instanceMatrices.size(); i++) {
                    glUniformMatrix4fv(glGetUniformLocation(shadowPtr->shader.shaderID, "model"), 1, GL_FALSE,  value_ptr(model * objPtr->instance.instanceMatrices[i]));
                    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
                }
            }
            else
                glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        }
        glBindVertexArray(0);
    }
    for (int i = 0; i < objPtr->subObjects.size(); i++)
        drawShadows(objPtr->subObjects[i], shadowPtr, hidden);
}

void processAnimationFrames()
{
    if (animationStart < 0.0) {
        if (!animationReset) {
            for (int i = 0; i < animationPtrs.size(); i++) {
                Animation* animPtr = animationPtrs[i];
                stringstream ss(animPtr->objPtr);
                void* blankPtr;
                ss >> blankPtr;
                Object* objPtr = reinterpret_cast<Object*>(blankPtr);
                if (animPtr->animAttr == "trns" && animPtr->initValue.size() > 0)
                    objPtr->transform.position = glm::vec3(animPtr->initValue[0], animPtr->initValue[1], animPtr->initValue[2]);
                else if (animPtr->animAttr == "scal" && animPtr->initValue.size() > 0)
                    objPtr->transform.scale = glm::vec3(animPtr->initValue[0], animPtr->initValue[1], animPtr->initValue[2]);
                else if (animPtr->animAttr == "fron" && animPtr->initValue.size() > 0)
                    objPtr->transform.front = glm::vec3(animPtr->initValue[0], animPtr->initValue[1], animPtr->initValue[2]);
                else if (animPtr->animAttr == "left" && animPtr->initValue.size() > 0)
                    objPtr->transform.left = glm::vec3(animPtr->initValue[0], animPtr->initValue[1], animPtr->initValue[2]);
                else if (animPtr->animAttr == "up" && animPtr->initValue.size() > 0)
                    objPtr->transform.up = glm::vec3(animPtr->initValue[0], animPtr->initValue[1], animPtr->initValue[2]);
                else if (animPtr->animAttr == "degr" && animPtr->initValue.size() > 0)
                    objPtr->bone.rotationDegrees = glm::vec3(animPtr->initValue[0], animPtr->initValue[1], animPtr->initValue[2]);
                else if (animPtr->animAttr == "offs" && animPtr->initValue.size() > 0)
                    objPtr->bone.locationOffset = glm::vec3(animPtr->initValue[0], animPtr->initValue[1], animPtr->initValue[2]);
                else if (animPtr->animAttr == "fov" && animPtr->initValue.size() > 0)
                    objPtr->camera.fov = animPtr->initValue[0];
                
                animPtr->initValue.clear();
            }
            for (int i = 0; i < rootJoPtrs.size(); i++)
                setPoseTransformation(rootJoPtrs[i]->name);
            
            animationReset = true;
            vector<Object>::iterator live = find_if(objects.begin(), objects.end(), [] (Object obj) { return obj.name == "textlive"; });
            if (live != objects.end())
                live->objectPtr->hidden = true;
        }
        return;
    }
    for (int i = 0; i < animationPtrs.size(); i++) {
        animationReset = false;
        vector<Object>::iterator live = find_if(objects.begin(), objects.end(), [] (Object obj) { return obj.name == "textlive"; });
        if (live != objects.end())
            live->objectPtr->hidden = false;
        float currentTime = glfwGetTime() - animationStart;
        Animation* animPtr = animationPtrs[i];
        stringstream ss(animPtr->objPtr);
        void* blankPtr;
        ss >> blankPtr;
        Object* objPtr = reinterpret_cast<Object*>(blankPtr);
        
        
        for (int j = 0; j < animPtr->timestamps.size(); j++) {
            if (j == animPtr->timestamps.size() - 1) {
                if (currentTime >= animPtr->timestamps[j]) {
                    if (animationLoop) {
                        animationStart = glfwGetTime();
                        return;
                    }
                    
                    if (animPtr->animAttr == "trns") {
                        if (animPtr->initValue.size() == 0) animPtr->initValue = {objPtr->transform.position.x, objPtr->transform.position.y, objPtr->transform.position.z};
                        objPtr->transform.position = glm::vec3(animPtr->values[j * 3], animPtr->values[j * 3 + 1], animPtr->values[j * 3 + 2]);
                    }
                    else if (animPtr->animAttr == "scal") {
                        if (animPtr->initValue.size() == 0) animPtr->initValue = {objPtr->transform.scale.x, objPtr->transform.scale.y, objPtr->transform.scale.z};
                        objPtr->transform.scale = glm::vec3(animPtr->values[j * 3], animPtr->values[j * 3 + 1], animPtr->values[j * 3 + 2]);
                    }
                    else if (animPtr->animAttr == "fron") {
                        if (animPtr->initValue.size() == 0) animPtr->initValue = {objPtr->transform.front.x, objPtr->transform.front.y, objPtr->transform.front.z};
                        objPtr->transform.front = glm::vec3(animPtr->values[j * 3], animPtr->values[j * 3 + 1], animPtr->values[j * 3 + 2]);
                    }
                    else if (animPtr->animAttr == "up") {
                        if (animPtr->initValue.size() == 0) animPtr->initValue = {objPtr->transform.up.x, objPtr->transform.up.y, objPtr->transform.up.z};
                        objPtr->transform.up = glm::vec3(animPtr->values[j * 3], animPtr->values[j * 3 + 1], animPtr->values[j * 3 + 2]);
                    }
                    else if (animPtr->animAttr == "left") {
                        if (animPtr->initValue.size() == 0) animPtr->initValue = {objPtr->transform.left.x, objPtr->transform.left.y, objPtr->transform.left.z};
                        objPtr->transform.left = glm::vec3(animPtr->values[j * 3], animPtr->values[j * 3 + 1], animPtr->values[j * 3 + 2]);
                    }
                    else if (animPtr->animAttr == "degr") {
                        if (animPtr->initValue.size() == 0) animPtr->initValue = {objPtr->bone.rotationDegrees.x, objPtr->bone.rotationDegrees.y, objPtr->bone.rotationDegrees.z};
                        objPtr->bone.rotationDegrees = glm::vec3(animPtr->values[j * 3], animPtr->values[j * 3 + 1], animPtr->values[j * 3 + 2]);
                    }
                    else if (animPtr->animAttr == "offs") {
                        if (animPtr->initValue.size() == 0) animPtr->initValue = {objPtr->bone.locationOffset.x, objPtr->bone.locationOffset.y, objPtr->bone.locationOffset.z};
                        objPtr->bone.locationOffset = glm::vec3(animPtr->values[j * 3], animPtr->values[j * 3 + 1], animPtr->values[j * 3 + 2]);
                    }
                    else if (animPtr->animAttr == "fov") {
                        if (animPtr->initValue.size() == 0) animPtr->initValue = {objPtr->camera.fov};
                        objPtr->camera.fov = animPtr->values[j];
                    }
                }
            }
            else {
                if (currentTime >= animPtr->timestamps[j] && currentTime < animPtr->timestamps[j + 1]) {
                    if (animPtr->animAttr == "trns" || animPtr->animAttr == "scal" || animPtr->animAttr == "fron" || animPtr->animAttr == "up" || animPtr->animAttr == "left" || animPtr->animAttr == "degr" || animPtr->animAttr == "offs") {
                        glm::vec3 prevValue = glm::vec3(animPtr->values[j * 3], animPtr->values[j * 3 + 1], animPtr->values[j * 3 + 2]);
                        glm::vec3 nextValue = glm::vec3(animPtr->values[(j + 1) * 3], animPtr->values[(j + 1) * 3 + 1], animPtr->values[(j + 1) * 3 + 2]);
                        float diffTime = animPtr->timestamps[j + 1] - animPtr->timestamps[j];
                        glm::vec3 diffValue = nextValue - prevValue;
                        float timeOfst = currentTime - animPtr->timestamps[j];
                        glm::vec3 valueOfst = diffValue * (timeOfst / diffTime);
                        if (animPtr->animAttr == "trns") {
                            if (animPtr->initValue.size() == 0) animPtr->initValue = {objPtr->transform.position.x, objPtr->transform.position.y, objPtr->transform.position.z};
                            objPtr->transform.position = prevValue + valueOfst;
                        }
                        else if (animPtr->animAttr == "scal") {
                            if (animPtr->initValue.size() == 0) animPtr->initValue = {objPtr->transform.scale.x, objPtr->transform.scale.y, objPtr->transform.scale.z};
                            objPtr->transform.scale = prevValue + valueOfst;
                        }
                        else if (animPtr->animAttr == "fron") {
                            if (animPtr->initValue.size() == 0) animPtr->initValue = {objPtr->transform.front.x, objPtr->transform.front.y, objPtr->transform.front.z};
                            objPtr->transform.front = prevValue + valueOfst;
                        }
                        else if (animPtr->animAttr == "up") {
                            if (animPtr->initValue.size() == 0) animPtr->initValue = {objPtr->transform.up.x, objPtr->transform.up.y, objPtr->transform.up.z};
                            objPtr->transform.up = prevValue + valueOfst;
                        }
                        else if (animPtr->animAttr == "left") {
                            if (animPtr->initValue.size() == 0) animPtr->initValue = {objPtr->transform.left.x, objPtr->transform.left.y, objPtr->transform.left.z};
                            objPtr->transform.left = prevValue + valueOfst;
                        }
                        else if (animPtr->animAttr == "degr") {
                            if (animPtr->initValue.size() == 0) animPtr->initValue = {objPtr->bone.rotationDegrees.x, objPtr->bone.rotationDegrees.y, objPtr->bone.rotationDegrees.z};
                            objPtr->bone.rotationDegrees = prevValue + valueOfst;
                        }
                        else if (animPtr->animAttr == "offs") {
                            if (animPtr->initValue.size() == 0) animPtr->initValue = {objPtr->bone.locationOffset.x, objPtr->bone.locationOffset.y, objPtr->bone.locationOffset.z};
                            objPtr->bone.locationOffset = prevValue + valueOfst;
                        }
                    }
                    else if (animPtr->animAttr == "fov") {
                        float prevValue = animPtr->values[j];
                        float nextValue = animPtr->values[j + 1];
                        float diffTime = animPtr->timestamps[j + 1] - animPtr->timestamps[j];
                        float diffValue = nextValue - prevValue;
                        float timeOfst = currentTime - animPtr->timestamps[j];
                        float valueOfst = diffValue * (timeOfst / diffTime);
                        if (animPtr->animAttr == "fov") {
                            if (animPtr->initValue.size() == 0) animPtr->initValue = {objPtr->camera.fov};
                            objPtr->camera.fov = prevValue + valueOfst;
                        }
                    }
                }
            }
        }
    }
    for (int i = 0; i < rootJoPtrs.size(); i++)
        setPoseTransformation(rootJoPtrs[i]->name);
}

void processDiscreteInput(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    
    // sticky keys
    if ((key == GLFW_KEY_LEFT_SUPER || key == GLFW_KEY_RIGHT_SUPER) && action == GLFW_PRESS) {
        keyCommandSticked = true;
    }
    if ((key == GLFW_KEY_LEFT_SUPER || key == GLFW_KEY_RIGHT_SUPER) && action == GLFW_RELEASE) {
        keyCommandSticked = false;
    }
    
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        if (animationStart < 0.0)
            animationStart = glfwGetTime();
        else
            animationStart = -1;
    }
    
    
    if (key == GLFW_KEY_V && action == GLFW_PRESS) {
        if (polygonMode == GL_FILL)
            polygonMode = GL_POINT;
        else if (polygonMode == GL_POINT)
            polygonMode = GL_LINE;
        else if (polygonMode == GL_LINE)
            polygonMode = GL_FILL;
    }
    
    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        captureScreenshot();
    }
    
    if ((key == GLFW_KEY_1 || key == GLFW_KEY_2 || key == GLFW_KEY_3 ||
        key == GLFW_KEY_4 || key == GLFW_KEY_5 || key == GLFW_KEY_6 ||
        key == GLFW_KEY_7 || key == GLFW_KEY_8 || key == GLFW_KEY_9) && action == GLFW_PRESS)  {
        
        if (keyCommandSticked) {
            int fboNo = stoi(glfwGetKeyName(key, 0)) - 2;
            if (fboNo == -1)
                fboPtr = NULL;
            else if (fboNo < fboPtrs.size())
                fboPtr = fboPtrs[fboNo];
        }
        else {
            int camNo = stoi(glfwGetKeyName(key, 0)) - 1;
            if (camNo < cameraPtrs.size())
                cameraPtr = cameraPtrs[camNo];
        }
    }
    
    
    
    if (key == GLFW_KEY_0 && action == GLFW_PRESS) {
        if (keyCommandSticked)
            showJoints = !showJoints;
        
        if (keyZeroIndex == 0) {
            setHumanPose();
            setPoseTransformation("hips");
            
            keyZeroIndex++;
        }
        else if (keyZeroIndex == 1) {
            resetHumanPose();
            setPoseTransformation("hips");
            
            keyZeroIndex = 0;
        }
    }
    
    
    if (key == GLFW_KEY_LEFT_BRACKET && action == GLFW_PRESS) {    // Ğ
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "light1"; });
        it->objectPtr->hidden = !it->objectPtr->hidden;
    }
    if (key == GLFW_KEY_RIGHT_BRACKET && action == GLFW_PRESS) {    // Ü
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "light2"; });
        it->objectPtr->hidden = !it->objectPtr->hidden;
    }
    if (key == GLFW_KEY_APOSTROPHE && action == GLFW_PRESS) {    // İ
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "light3"; });
        it->objectPtr->hidden = !it->objectPtr->hidden;
    }
    if (key == GLFW_KEY_BACKSLASH && action == GLFW_PRESS) {    // ,
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "light4"; });
        it->objectPtr->hidden = !it->objectPtr->hidden;
    }
    
    
    if (key == GLFW_KEY_O && action == GLFW_PRESS) {    // O
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "light1"; });
        it->objectPtr->light.quadratic *= 5.0f;
    }
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {    // P
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "light1"; });
        it->objectPtr->light.quadratic /= 5.0f;
    }
    if (key == GLFW_KEY_L && action == GLFW_PRESS) {    // L
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "light1"; });
        it->objectPtr->light.linear *= 5.0f;
    }
    if (key == GLFW_KEY_SEMICOLON && action == GLFW_PRESS) {    // Ş
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "light1"; });
        it->objectPtr->light.linear /= 5.0f;
    }
}

void processContinuousInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        glm::vec3 offset = cameraPtr->transform.front * cameraPtr->camera.moveSpeed;
        cameraPtr->transform.position = cameraPtr->transform.position + offset;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        glm::vec3 offset = cameraPtr->transform.front * cameraPtr->camera.moveSpeed;
        cameraPtr->transform.position = cameraPtr->transform.position - offset;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        glm::vec3 offset = cameraPtr->transform.left * cameraPtr->camera.moveSpeed;
        cameraPtr->transform.position = cameraPtr->transform.position + offset;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        glm::vec3 offset = cameraPtr->transform.left * cameraPtr->camera.moveSpeed;
        cameraPtr->transform.position = cameraPtr->transform.position - offset;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        glm::vec3 offset = cameraPtr->transform.up * cameraPtr->camera.moveSpeed;
        cameraPtr->transform.position = cameraPtr->transform.position + offset;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        glm::vec3 offset = cameraPtr->transform.up * cameraPtr->camera.moveSpeed;
        cameraPtr->transform.position = cameraPtr->transform.position - offset;
    }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        cameraPtr->transform.up = rotateVectorAroundAxis(cameraPtr->transform.up, cameraPtr->transform.left, cameraPtr->camera.fov * -1.0 * cameraPtr->camera.moveSpeed);
        cameraPtr->transform.front = rotateVectorAroundAxis(cameraPtr->transform.front, cameraPtr->transform.left, cameraPtr->camera.fov * -1.0 * cameraPtr->camera.moveSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        cameraPtr->transform.up = rotateVectorAroundAxis(cameraPtr->transform.up, cameraPtr->transform.left, cameraPtr->camera.fov * cameraPtr->camera.moveSpeed);
        cameraPtr->transform.front = rotateVectorAroundAxis(cameraPtr->transform.front, cameraPtr->transform.left, cameraPtr->camera.fov * cameraPtr->camera.moveSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        cameraPtr->transform.front = rotateVectorAroundAxis(cameraPtr->transform.front, cameraPtr->transform.up, cameraPtr->camera.fov * cameraPtr->camera.moveSpeed);
        cameraPtr->transform.left = rotateVectorAroundAxis(cameraPtr->transform.left, cameraPtr->transform.up, cameraPtr->camera.fov * cameraPtr->camera.moveSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        cameraPtr->transform.front = rotateVectorAroundAxis(cameraPtr->transform.front, cameraPtr->transform.up, cameraPtr->camera.fov * -1.0 * cameraPtr->camera.moveSpeed);
        cameraPtr->transform.left = rotateVectorAroundAxis(cameraPtr->transform.left, cameraPtr->transform.up, cameraPtr->camera.fov * -1.0 * cameraPtr->camera.moveSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS) {
        cameraPtr->transform.left = rotateVectorAroundAxis(cameraPtr->transform.left, cameraPtr->transform.front, cameraPtr->camera.fov * -1.0 * cameraPtr->camera.moveSpeed);
        cameraPtr->transform.up = rotateVectorAroundAxis(cameraPtr->transform.up, cameraPtr->transform.front, cameraPtr->camera.fov * -1.0 * cameraPtr->camera.moveSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_SLASH) == GLFW_PRESS) {
        cameraPtr->transform.left = rotateVectorAroundAxis(cameraPtr->transform.left, cameraPtr->transform.front, cameraPtr->camera.fov * cameraPtr->camera.moveSpeed);
        cameraPtr->transform.up = rotateVectorAroundAxis(cameraPtr->transform.up, cameraPtr->transform.front, cameraPtr->camera.fov * cameraPtr->camera.moveSpeed);
    }
    
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        if (cameraPtr->camera.fov < 180.0f)
            cameraPtr->camera.fov += 0.5f;
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        if (cameraPtr->camera.fov > 0.0f)
            cameraPtr->camera.fov -= 0.5f;
    }
    
    
    
    function<void(Object*, float)> translation = [&translation](Object* obj, float offset) {
        obj->objectPtr->transform.position += glm::vec3(offset, 0.0, 0.0);
        for (int i = 0; i < obj->subObjects.size(); i++)
            translation(obj->subObjects[i], offset);
    };
    function<void(Object*, float)> rotation = [&rotation](Object* obj, float angle) {
        obj->objectPtr->transform.left = rotateVectorAroundAxis(obj->objectPtr->transform.left, obj->objectPtr->transform.up, angle);
        obj->objectPtr->transform.front = rotateVectorAroundAxis(obj->objectPtr->transform.front, obj->objectPtr->transform.up, angle);
        for (int i = 0; i < obj->subObjects.size(); i++)
            rotation(obj->subObjects[i], angle);
    };
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "light1"; });
//        rotation(it->objectPtr, 1.0f);
//        translation(it->objectPtr, 0.01f);
        it->objectPtr->light.cutOff += 0.01f;
//        it->objectPtr->light.outerCutOff += 0.01f;
        
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "light1"; });
//        rotation(it->objectPtr, -1.0f);
//        translation(it->objectPtr, -0.01f);
        it->objectPtr->light.cutOff -= 0.01f;
//        it->objectPtr->light.outerCutOff -= 0.01f;
    }

    
    
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
        float angle = 1.5f;
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
            angle *= -1.0;
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "leftshoulder"; });
        setJointDegrees(it->objectPtr->name, it->objectPtr->bone.rotationDegrees + glm::vec3(angle, 0.0, 0.0));
        setPoseTransformation("hips");
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        float angle = 1.5f;
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
            angle *= -1.0;
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "leftshoulder"; });
        setJointDegrees(it->objectPtr->name, it->objectPtr->bone.rotationDegrees + glm::vec3(0.0, angle, 0.0));
        setPoseTransformation("hips");
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
        float angle = 1.5f;
        if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
            angle *= -1.0;
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "leftshoulder"; });
        setJointDegrees(it->objectPtr->name, it->objectPtr->bone.rotationDegrees + glm::vec3(0.0, 0.0, angle));
        setPoseTransformation("hips");
    }
    
    
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
        float offset = 0.01f;
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
            offset *= -1.0;
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "light1"; });
        it->objectPtr->transform.position += glm::vec3(offset, 0, 0);
    }
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
        float offset = 0.01f;
        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
            offset *= -1.0;
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "light1"; });
        it->objectPtr->transform.position += glm::vec3(0, offset, 0);
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_COMMA) == GLFW_PRESS) {
        float offset = 0.01f;
        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
            offset *= -1.0;
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "light1"; });
        it->objectPtr->transform.position += glm::vec3(0, 0, offset);
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

vector<unsigned char> base64Decode(string const& encoded_string)
{
    string base64_chars =
                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                 "abcdefghijklmnopqrstuvwxyz"
                 "0123456789+/";
    
    long in_len = encoded_string.size();
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

glm::vec3 rotateVectorAroundAxises(glm::vec3 vector, glm::mat3 axises, glm::vec3 degrees)
{
//    cout << "vector:" << glm::to_string(vector) << " axises:" << glm::to_string(axises) << " degrees:" << glm::to_string(degrees);
    glm::vec3 newvector = glm::vec3(vector);
    newvector = rotateVectorAroundAxis(newvector, axises[0], degrees.x);
    newvector = rotateVectorAroundAxis(newvector, axises[1], degrees.y);
    newvector = rotateVectorAroundAxis(newvector, axises[2], degrees.z);
//    cout << " output: " << glm::to_string(newvector) << endl;
    return newvector;
}

void setPoseTransformation(string rootjoint)
{
    vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [rootjoint] (Object obj) { return obj.name == rootjoint; });
    Object* rootJoPtr = it->objectPtr;
    Object* rootPtr = it->objectPtr->superObject;
    
    vector<float> newVertices = { rootPtr->shader.vertices };
    vector<float> newNormals = { rootPtr->shader.normals };
    
    vector<float> verticeLocOffs;
    verticeLocOffs.resize(rootPtr->shader.vertices.size());
    
    vector<Object*> chain;
    function<void(Object*)> setJointsPos = [&newVertices, &newNormals, &verticeLocOffs, &chain, &setJointsPos](Object* objPtr) -> void {
        vector<float> currJoNewPos = { objPtr->shader.vertices };
        glm::vec3 currJoBeg = glm::vec3(objPtr->shader.vertices[0],
                                        objPtr->shader.vertices[1],
                                        objPtr->shader.vertices[2]);
        glm::vec3 currJoEnd = glm::vec3(objPtr->shader.vertices[objPtr->shader.vertices.size() - 3],
                                        objPtr->shader.vertices[objPtr->shader.vertices.size() - 2],
                                        objPtr->shader.vertices[objPtr->shader.vertices.size() - 1]);
        glm::mat3 currJoAx = glm::mat3(glm::vec3(objPtr->bone.rotationXAxis),
                                           glm::vec3(objPtr->bone.rotationYAxis),
                                           glm::vec3(objPtr->bone.rotationZAxis));
        glm::vec3 currJoDegr = glm::vec3(objPtr->bone.rotationDegrees);
        glm::vec3 currJoOffs = glm::vec3(objPtr->bone.locationOffset);
        
        chain.push_back(objPtr);
        vector<Object*> chainCopy = { chain };
        reverse(chain.begin(), chain.end());
        for (int i = 0; i < chain.size(); i++) {
            if (chain[i]->bone.rotationDegrees == glm::vec3(0.0f) && chain[i]->bone.locationOffset == glm::vec3(0.0f))
                continue;
            size_t chainVerLen = chain[i]->shader.vertices.size();
            glm::vec3 chainJoPos = glm::vec3(chain[i]->shader.vertices[chainVerLen - 3],
                                             chain[i]->shader.vertices[chainVerLen - 2],
                                             chain[i]->shader.vertices[chainVerLen - 1]);
            glm::mat3 chainRotAx = glm::mat3(chain[i]->bone.rotationXAxis,
                                             chain[i]->bone.rotationYAxis,
                                             chain[i]->bone.rotationZAxis);
            if (chain[i] != objPtr) {
                currJoAx[0] = rotateVectorAroundAxises(currJoAx[0], chainRotAx, chain[i]->bone.rotationDegrees);
                currJoAx[1] = rotateVectorAroundAxises(currJoAx[1], chainRotAx, chain[i]->bone.rotationDegrees);
                currJoAx[2] = rotateVectorAroundAxises(currJoAx[2], chainRotAx, chain[i]->bone.rotationDegrees);
                currJoBeg = rotateVectorAroundAxises(currJoBeg - chainJoPos, chainRotAx, chain[i]->bone.rotationDegrees) + chainJoPos;
                currJoEnd = rotateVectorAroundAxises(currJoEnd - chainJoPos, chainRotAx, chain[i]->bone.rotationDegrees) + chainJoPos;
                
                glm::vec3 currJoBegOffs = currJoBeg + chainRotAx * chain[i]->bone.locationOffset;
                currJoNewPos[0] = currJoBegOffs.x;
                currJoNewPos[1] = currJoBegOffs.y;
                currJoNewPos[2] = currJoBegOffs.z;
            }
            glm::vec3 currJoEndOffs = currJoEnd + chainRotAx * chain[i]->bone.locationOffset;
            currJoNewPos[currJoNewPos.size() - 3] = currJoEndOffs.x;
            currJoNewPos[currJoNewPos.size() - 2] = currJoEndOffs.y;
            currJoNewPos[currJoNewPos.size() - 1] = currJoEndOffs.z;
        }
        reverse(chain.begin(), chain.end());
        
        glBindBuffer(GL_ARRAY_BUFFER, objPtr->shader.vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, objPtr->shader.vertices.size() * sizeof(float), &currJoNewPos[0]);
        
        vector<float> newVerticesCopy = { newVertices };
        vector<float> newNormalsCopy = { newNormals };
//        cout << "rotating by: " << objPtr->name << endl;
        function<void(Object*)> setSubJointsVertices = [&newVertices, &newNormals, &verticeLocOffs, newVerticesCopy, newNormalsCopy, currJoEnd, currJoAx, currJoDegr, currJoOffs, &setSubJointsVertices](Object* objPtrNested) -> void {
            if (currJoDegr != glm::vec3(0.0f) || currJoOffs != glm::vec3(0.0f)) {
                for (int i = 0; i < objPtrNested->bone.indices.size(); i++) {
                    unsigned int verIdx = objPtrNested->bone.indices[i];
                    glm::vec3 verPos = glm::vec3(newVerticesCopy[verIdx * 3],
                                                 newVerticesCopy[verIdx * 3 + 1],
                                                 newVerticesCopy[verIdx * 3 + 2]);
                    glm::vec3 verNor = glm::vec3(newNormalsCopy[verIdx * 3],
                                                 newNormalsCopy[verIdx * 3 + 1],
                                                 newNormalsCopy[verIdx * 3 + 2]);
//                    if (verIdx == 2848) {
//                        cout << "influenced: " << objPtrNested->name << endl;
//                        cout << "curr: " << glm::to_string(verPos) << endl;
//                    }
                    glm::vec3 verNewPos = rotateVectorAroundAxises(verPos - currJoEnd, currJoAx, currJoDegr) + currJoEnd;
                    verNewPos = (verNewPos - verPos) * objPtrNested->bone.weights[i];
                    newVertices[verIdx * 3] += verNewPos.x;
                    newVertices[verIdx * 3 + 1] += verNewPos.y;
                    newVertices[verIdx * 3 + 2] += verNewPos.z;
                    glm::vec3 verNewNor = rotateVectorAroundAxises(verNor, currJoAx, currJoDegr);
                    verNewNor = (verNewNor - verNor) * objPtrNested->bone.weights[i];
                    newNormals[verIdx * 3] += verNewNor.x;
                    newNormals[verIdx * 3 + 1] += verNewNor.y;
                    newNormals[verIdx * 3 + 2] += verNewNor.z;
                    glm::vec3 locOffs = currJoAx * currJoOffs * objPtrNested->bone.weights[i];
                    verticeLocOffs[verIdx * 3] += locOffs.x;
                    verticeLocOffs[verIdx * 3 + 1] += locOffs.y;
                    verticeLocOffs[verIdx * 3 + 2] += locOffs.z;
                }
            }
            for (Object* ptr : objPtrNested->subObjects)
                setSubJointsVertices(ptr);
        };
        setSubJointsVertices(objPtr);
        
        for (Object* ptr : objPtr->subObjects)
            setJointsPos(ptr);
        chain.pop_back();
    };
    setJointsPos(rootJoPtr);
    
    transform(newVertices.begin(), newVertices.end(), verticeLocOffs.begin(), newVertices.begin(), plus<float>());
    
    glBindBuffer(GL_ARRAY_BUFFER, rootPtr->shader.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, rootPtr->shader.vertices.size() * sizeof(float), &newVertices[0]);
    glBufferSubData(GL_ARRAY_BUFFER, rootPtr->shader.vertices.size() * sizeof(float), rootPtr->shader.normals.size() * sizeof(float), &newNormals[0]);
}

void setJointDegrees(string joint, glm::vec3 degrees)
{
    vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [joint] (Object obj) { return obj.name == joint; });
    it->objectPtr->bone.rotationDegrees = degrees;
}

void setJointOffset(string joint, glm::vec3 offset)
{
    vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [joint] (Object obj) { return obj.name == joint; });
    it->objectPtr->bone.locationOffset = offset;
}

void setHumanPose()
{
    setJointDegrees("hips", glm::vec3(0.92728750, 2.819459614, 0.6830100590));
    setJointOffset("hips", glm::vec3(0.0458009988, -0.4148039817, 0.2045069932));
    setJointDegrees("spine", glm::vec3(-0.21767248593811317, -0.4639044207148562, -0.4288338272456472));
    setJointDegrees("spine1", glm::vec3(-0.5470554104748558, -0.42239642731376315, -0.2626752283373653));
    setJointDegrees("spine2", glm::vec3(-0.5469863081703624, -0.427728163693325, -0.2550662908802653));
    setJointDegrees("neck", glm::vec3(2.823390388009705, -0.7561110124654029, 3.938918441040755));
    setJointDegrees("head", glm::vec3(-9.621363903585033, 2.112767474921235, -4.226407079622116));
    setJointDegrees("leftshoulder", glm::vec3(-15.90240330300782, -5.481039405619881, 3.2497397606251863));
    setJointDegrees("leftarm", glm::vec3(39.930003057757396, -30.41713073272249, -39.505998574461344));
    setJointDegrees("leftforearm", glm::vec3(-64.33596967931629, -48.70662197139994, -16.18998329536933));
    setJointDegrees("lefthand", glm::vec3(-16.117101761830707, 25.955333563073754, 36.430319994974994));
    setJointDegrees("lefthandthumb1", glm::vec3(-4.185785810193934, -3.1340909975985305, 9.335753567025426));
    setJointDegrees("lefthandthumb2", glm::vec3(29.870193089649963, 12.630565105653291, 6.040905316130771));
    setJointDegrees("lefthandthumb3", glm::vec3(-1.590474968643113, -0.23321606215814492, 3.4846169461338197));
    setJointDegrees("lefthandindex1", glm::vec3(-1.6848749064401536, 1.6792290080391976, 19.07705644620953));
    setJointDegrees("lefthandindex2", glm::vec3(-7.2090025604390595, 0.48683123132447337, -11.156290971420843));
    setJointDegrees("lefthandindex3", glm::vec3(-5.359891486537718, 0.4108308359756688, -3.8259176569779485));
    setJointDegrees("lefthandmiddle1", glm::vec3(19.89017339142662, 1.8114897516226764, 7.7877441256081035));
    setJointDegrees("lefthandmiddle2", glm::vec3(-5.924246965767423, 1.4912456602160504, -12.906307526409522));
    setJointDegrees("lefthandmiddle3", glm::vec3(-1.9678191213459284, 0.5114979259034456, -2.885573283987147));
    setJointDegrees("lefthandring1", glm::vec3(-6.744603484617906, 3.1544067549544987, -31.751008621018034));
    setJointDegrees("lefthandring2", glm::vec3(-3.277980671852485, 0.5392973294339263, -3.9590734755078167));
    setJointDegrees("lefthandring3", glm::vec3(-9.943806035243934, -0.9204499529292584, 8.076428047294216));
    setJointDegrees("rightshoulder", glm::vec3(-9.797517897374247, 9.980668566194492, -5.369513942417797));
    setJointDegrees("rightarm", glm::vec3(40.563250386276785, 21.860302183929072, 32.509122052849015));
    setJointDegrees("rightforearm", glm::vec3(-72.41419150504127, 52.924697500895114, 13.240112531526385));
    setJointDegrees("righthand", glm::vec3(-3.2127037001810272, -3.851346451257906, -34.12684211248278));
    setJointDegrees("righthandthumb1", glm::vec3(14.450354604334592, -22.3772194380521, -23.070321421815606));
    setJointDegrees("righthandthumb2", glm::vec3(0.3261853154479057, -0.0863564295539984, -2.8541635918717856));
    setJointDegrees("righthandthumb3", glm::vec3(11.391904323255956, -4.163719176532916, 2.650948602034393));
    setJointDegrees("righthandindex1", glm::vec3(-16.36537060042421, -1.6001962488227452, -7.556985330725577));
    setJointDegrees("righthandindex2", glm::vec3(-21.834265502812983, 0.5320825152362773, 18.320647146576466));
    setJointDegrees("righthandindex3", glm::vec3(2.3847374968215034, 0.5274200573540972, -3.27734354326897));
    setJointDegrees("righthandmiddle1", glm::vec3(5.510058318082326, -1.6135548181788208, -24.557842707943294));
    setJointDegrees("righthandmiddle2", glm::vec3(-7.576708355727635, -1.0587759044957086, 14.153346389597527));
    setJointDegrees("righthandmiddle3", glm::vec3(0.3944095671070772, 0.17444030999862822, -1.1814252912565102));
    setJointDegrees("righthandring1", glm::vec3(27.82154614999634, -7.745547216915765, 11.743600155130022));
    setJointDegrees("righthandring2", glm::vec3(-9.765730196977007, -1.0413890176332066, 10.674903507586574));
    setJointDegrees("righthandring3", glm::vec3(-11.794121356873314, 4.801825330873125, 16.99865891007862));
    setJointDegrees("leftupleg", glm::vec3(2.8178169536469744, -2.859013453069318, -12.232597303473328));
    setJointDegrees("leftleg", glm::vec3(97.12309067666857, -35.69096226260487, 46.546707835072226));
    setJointDegrees("leftfoot", glm::vec3(20.646656115593263, -1.4076701848713051, 8.13622806102497));
    setJointDegrees("lefttoebase", glm::vec3(42.39484074429657, -6.203401919787721, 20.648158757210666));
    setJointDegrees("rightupleg", glm::vec3(-2.051184568373696, -1.4256303807372652, 10.680396687226558));
    setJointDegrees("rightleg", glm::vec3(31.722072524599277, 23.090207517584098, -108.79189513774507));
    setJointDegrees("rightfoot", glm::vec3(21.69824228549218, 0.7983844939816399, -3.2885085546844137));
    setJointDegrees("righttoebase", glm::vec3(-15.374923161333367, 0.45628401067438473, 13.60252066133997));
}

void resetHumanPose()
{
    vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [] (Object obj) { return obj.name == "hips"; });
    
    function<void(Object*)> lambdaFunc = [&lambdaFunc](Object* obj) -> void {
        setJointDegrees(obj->objectPtr->name, glm::vec3(0.0));
        setJointOffset(obj->objectPtr->name, glm::vec3(0.0));
        for (Object* ptr : obj->objectPtr->subObjects)
            lambdaFunc(ptr);
    };
    lambdaFunc(it->objectPtr);
}

void calculateTangents(Object* objPtr)
{
    map<int, glm::vec3> tangentMap;
    for (int i = 0; i < objPtr->shader.faces.size() / 3; i++) {
        glm::vec3 tan;
        
        glm::vec3 v1 = glm::vec3(objPtr->shader.vertices[objPtr->shader.faces[i * 3] * 3], objPtr->shader.vertices[objPtr->shader.faces[i * 3] * 3 + 1], objPtr->shader.vertices[objPtr->shader.faces[i * 3] * 3 + 2]);
        glm::vec3 v2 = glm::vec3(objPtr->shader.vertices[objPtr->shader.faces[i * 3 + 1] * 3], objPtr->shader.vertices[objPtr->shader.faces[i * 3 + 1] * 3 + 1], objPtr->shader.vertices[objPtr->shader.faces[i * 3 + 1] * 3 + 2]);
        glm::vec3 v3 = glm::vec3(objPtr->shader.vertices[objPtr->shader.faces[i * 3 + 2] * 3], objPtr->shader.vertices[objPtr->shader.faces[i * 3 + 2] * 3 + 1], objPtr->shader.vertices[objPtr->shader.faces[i * 3 + 2] * 3 + 2]);
        glm::vec2 t1 = glm::vec2(objPtr->shader.texCoords[objPtr->shader.faces[i * 3] * 2], objPtr->shader.texCoords[objPtr->shader.faces[i * 3] * 2 + 1]);
        glm::vec2 t2 = glm::vec2(objPtr->shader.texCoords[objPtr->shader.faces[i * 3 + 1] * 2], objPtr->shader.texCoords[objPtr->shader.faces[i * 3 + 1] * 2 + 1]);
        glm::vec2 t3 = glm::vec2(objPtr->shader.texCoords[objPtr->shader.faces[i * 3 + 2] * 2], objPtr->shader.texCoords[objPtr->shader.faces[i * 3 + 2] * 2 + 1]);
        glm::vec3 e1 = v2 - v1;
        glm::vec3 e2 = v3 - v1;
        glm::vec2 deltat1 = t2 - t1;
        glm::vec2 deltat2 = t3 - t1;
        
        float c = 1.0f / (deltat1.x * deltat2.y - deltat2.x * deltat1.y);

        tan.x = c * (deltat2.y * e1.x - deltat1.y * e2.x);
        tan.y = c * (deltat2.y * e1.y - deltat1.y * e2.y);
        tan.z = c * (deltat2.y * e1.z - deltat1.y * e2.z);
        
        tangentMap.insert(pair<int, glm::vec3>(objPtr->shader.faces[i * 3], tan));
        tangentMap.insert(pair<int, glm::vec3>(objPtr->shader.faces[i * 3 + 1], tan));
        tangentMap.insert(pair<int, glm::vec3>(objPtr->shader.faces[i * 3 + 2], tan));
    }
    for (map<int, glm::vec3>::iterator it = tangentMap.begin(); it != tangentMap.end(); ++it) {
        objPtr->shader.tangents.push_back(it->second.x);
        objPtr->shader.tangents.push_back(it->second.y);
        objPtr->shader.tangents.push_back(it->second.z);
    }
}

void manipulateHuman()
{
    vector<Object>::iterator rootPtr = find_if(objects.begin(), objects.end(), [] (Object obj) { return obj.name == "human"; });
    
    function<glm::vec3(int vIndice)> findVertexPosition = [rootPtr](int vIndice) {
        return glm::vec3(rootPtr->objectPtr->shader.vertices[vIndice * 3],
                                  rootPtr->objectPtr->shader.vertices[vIndice * 3 + 1],
                                  rootPtr->objectPtr->shader.vertices[vIndice * 3 + 2]);
    };
    
    function<glm::vec3(int vIndice)> findVertexNormal = [rootPtr](int vIndice) {
        return glm::vec3(rootPtr->objectPtr->shader.normals[vIndice * 3],
                                  rootPtr->objectPtr->shader.normals[vIndice * 3 + 1],
                                  rootPtr->objectPtr->shader.normals[vIndice * 3 + 2]);
    };
    
    function<glm::vec3(int tIndice)> findTriangleVertices = [rootPtr](int tIndice) {
        return glm::vec3(rootPtr->objectPtr->shader.faces[tIndice * 3],
                                  rootPtr->objectPtr->shader.faces[tIndice * 3 + 1],
                                  rootPtr->objectPtr->shader.faces[tIndice * 3 + 2]);
    };
    
    function<vector<int>(int vIndice)> findTrianglesIncludesVertex = [rootPtr](int vIndice) {
        vector<int> triangleIndices;
        for (int i = 0; i < rootPtr->objectPtr->shader.faces.size(); i += 3) {
            if (rootPtr->objectPtr->shader.faces[i] == vIndice ||
                rootPtr->objectPtr->shader.faces[i + 1] == vIndice ||
                rootPtr->objectPtr->shader.faces[i + 2] == vIndice) {
                
                int triInd = (int)i / 3;
                triangleIndices.push_back(triInd);
            }
        }
        return triangleIndices;
    };
    
    map<int, glm::vec3> headVertices;
    
    
    ifstream file(WORK_DIR + "head_vertices.txt");
    string line;
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
    long backslashPos = 0;
    string backslash = "\n";
    while ((backslashPos = line.find(backslash)) != string::npos) {
        string row = line.substr(0, backslashPos);
        if (row != "") {
            long commaPos = 0;
            string comma = ",";
            vector<float> values;
            const regex floatRegex("[+-]?([0-9]*[.])?[0-9]+");
            while((commaPos = row.find(comma)) != string::npos) {
                if (regex_match(row.substr(0, commaPos), floatRegex))
                    values.push_back(stof(row.substr(0, commaPos)));
                row.erase(0, commaPos + comma.length());
            }
            if (regex_match(row, floatRegex))
                headVertices.insert(pair<int, glm::vec3>(int(values[0]), glm::vec3(values[1], values[2], stof(row))));
        }
        line.erase(0, backslashPos + backslash.length());
    }
    
    for (int i = 0; i < rootPtr->objectPtr->shader.vertices.size() / 3; i++) {
        if (headVertices.find(i) == headVertices.end())
            continue;
        
        glm::vec3 vrt = findVertexPosition(i);
//        glm::vec3 offset = findVertexNormal(i) * 0.002f;
        glm::vec3 offset = headVertices.at(i);

        
//        if (vrt.y < 1.54)
//            continue;
        
//        headVertices.insert(pair<int, glm::vec3>(i, offset));
        
        vrt += offset;
        rootPtr->objectPtr->shader.vertices[i * 3] = vrt.x;
        rootPtr->objectPtr->shader.vertices[i * 3 + 1] = vrt.y;
        rootPtr->objectPtr->shader.vertices[i * 3 + 2] = vrt.z;
        
        vector<int> triangleIndices = findTrianglesIncludesVertex(i);
        vector<glm::vec3> newTriangleNormals;
        vector<int> adjustedVertIndices;
        for (int j = 0; j < triangleIndices.size(); j++) {
            int triangleIndice = triangleIndices[j];
            glm::vec3 tri = findTriangleVertices(triangleIndice);
            glm::vec3 v1 = findVertexPosition((int)tri[0]);
            glm::vec3 v2 = findVertexPosition((int)tri[1]);
            glm::vec3 v3 = findVertexPosition((int)tri[2]);
            glm::vec3 e1 = v1 - v2;
            glm::vec3 e2 = v2 - v3;
            
            glm::vec3 newTriNormal = glm::normalize(glm::cross(e1, e2));
            newTriangleNormals.push_back(newTriNormal);
        }
        
        for (int j = 0; j < triangleIndices.size(); j++) {
            int triangleIndice = triangleIndices[j];
            glm::vec3 tri = findTriangleVertices(triangleIndice);
            for (int h = 0; h < 3; h++) {
                if (find(adjustedVertIndices.begin(), adjustedVertIndices.end(), tri[h]) != adjustedVertIndices.end())
                    continue;
                vector<int> triangleIndicesOfThis = findTrianglesIncludesVertex(tri[h]);
                glm::vec3 cummulativeNormal = glm::vec3(0.0f);
                for (int g = 0; g < triangleIndicesOfThis.size(); g++) {
                    int triangleIndiceOfThis = triangleIndicesOfThis[g];
                    vector<int>::iterator it = find(triangleIndices.begin(), triangleIndices.end(), triangleIndiceOfThis);
                    if (it != triangleIndices.end()) {
                        cummulativeNormal += newTriangleNormals[it - triangleIndices.begin()];
                    }
                    else {
                        glm::vec3 neighborTriangle = findTriangleVertices(triangleIndiceOfThis);
                        glm::vec3 v1 = findVertexPosition((int)neighborTriangle[0]);
                        glm::vec3 v2 = findVertexPosition((int)neighborTriangle[1]);
                        glm::vec3 v3 = findVertexPosition((int)neighborTriangle[2]);
                        
                        glm::vec3 e1 = v1 - v2;
                        glm::vec3 e2 = v2 - v3;
                        
                        glm::vec3 oneOfTriNormalOfThis = glm::normalize(glm::cross(e1, e2));
                        cummulativeNormal += oneOfTriNormalOfThis;
                    }
                }
                        
                cummulativeNormal /= (int)triangleIndicesOfThis.size();
                
                rootPtr->objectPtr->shader.normals[tri[h] * 3] = cummulativeNormal.x;
                rootPtr->objectPtr->shader.normals[tri[h] * 3 + 1] = cummulativeNormal.y;
                rootPtr->objectPtr->shader.normals[tri[h] * 3 + 2] = cummulativeNormal.z;
                
                adjustedVertIndices.push_back(tri[h]);
            }
        }
    }
    
//    ofstream vertFile;
//    string content = "";
//    for (const auto &entry : headVertices)
//        content += to_string(entry.first) + "," + to_string(entry.second.x) + "," + to_string(entry.second.y) + "," + to_string(entry.second.z) + "\n";
//    vertFile.open("head_vertices.txt");
//    vertFile << content;
//    vertFile.close();
    
    glBindBuffer(GL_ARRAY_BUFFER, rootPtr->objectPtr->shader.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, rootPtr->objectPtr->shader.vertices.size() * sizeof(float), &rootPtr->objectPtr->shader.vertices[0]);
    glBufferSubData(GL_ARRAY_BUFFER, rootPtr->objectPtr->shader.vertices.size() * sizeof(float), rootPtr->objectPtr->shader.normals.size() * sizeof(float), &rootPtr->objectPtr->shader.normals[0]);
}

