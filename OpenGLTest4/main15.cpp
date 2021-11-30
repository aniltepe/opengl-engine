//
//  main15.cpp
//  OpenGLTest4
//
//  Created by Nazım Anıl Tepe on 02.11.2021.
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

#include <unistd.h>
#define GetCurrentDir getcwd

enum struct ObjectType : int {
    Scene, Model, Light, Camera, Joint, Text, Cubemap, Framebuffer
};
enum struct LightType : int {
    point, directional, spotlight
};
enum struct FboType : int {
    inverse, graysc, kernel, custom
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
    vector<float> bitangents;
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
struct Bone {
    vector<unsigned int> indices;
    vector<float> weights;
    float rollDegree = 0.0f;
    glm::vec3 locationOffset = glm::vec3(0.0f);
    glm::vec3 rotationDegrees = glm::vec3(0.0f);
    glm::vec3 rotationXAxis;
    glm::vec3 rotationYAxis;
    glm::vec3 rotationZAxis;
    glm::vec3 referenceXAxis;
    glm::vec3 referenceYAxis;
    glm::vec3 referenceZAxis;
};
struct Style {
    string text;
    string font;
    vector<float> kernel;
    FboType fboType;
};
struct Instanced {
    vector<float> translate;
    vector<float> scale;
    vector<float> front;
    vector<float> up;
    vector<float> left;
    bool isInstanced = false;
    vector<glm::mat4> instanceMatrices;
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
    Instanced instanced;
};
struct Character {
    unsigned int textureID;
    glm::ivec2   size;
    glm::ivec2   bearing;
    unsigned int advance;
};

string WORK_DIR = "/Users/nazimaniltepe/Documents/Projects/opengl-nscene/OpenGLTest4/";
int objIndex = 0;
vector<Object> objects;
Object* cameraPtr;
vector<Object*> cameraPtrs;
Object* fboPtr;
vector<Object*> fboPtrs;
map<GLchar, Character> characters;
string shading = "phong";
bool gammaCorrection = false;
bool multiSampling = false;
bool shadows = false;
unsigned int polygonMode = GL_FILL;
float lastFrame = 0.0f;
bool commandKeySticked = false;

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
void processDiscreteInput(GLFWwindow* window, int key, int scancode, int action, int mods);
void processContinuousInput(GLFWwindow* window);
void resizeFramebuffer(GLFWwindow* window, int width, int height);
int captureScreenshot();
vector<unsigned char> base64Decode(string const& encoded_string);
void renderText(std::string text, float x, float y, float scale, glm::vec3 color);
glm::vec3 rotateVectorAroundAxis(glm::vec3 vector, glm::vec3 axis, float angle);
void rotateJoint(string joint, glm::vec3 degrees);
void locateJoint(string joint, glm::vec3 offset);
void setPose();
void resetPose(string joint);
void calculateTangentsBitangents(Object* objPtr);

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
    Object* scene = createScene(WORK_DIR + "scene15.sce");
    
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
            if (obj->dictionary.find("trns") == obj->dictionary.end()) {
                obj->dictionary.insert(pair<string, string>("trns", obj->superObject->dictionary.at("trns")));
                obj->transform = obj->superObject->transform;
                if (obj->type == ObjectType::Joint) {
                    obj->bone.rotationXAxis = glm::vec3(obj->transform.left);
                    obj->bone.rotationYAxis = glm::vec3(obj->transform.up);
                    obj->bone.rotationZAxis = glm::vec3(obj->transform.front);
                    
                    obj->bone.rotationXAxis = rotateVectorAroundAxis(obj->bone.rotationXAxis, obj->bone.rotationYAxis, obj->bone.rollDegree * -1.0f);
                    obj->bone.rotationZAxis = rotateVectorAroundAxis(obj->bone.rotationZAxis, obj->bone.rotationYAxis, obj->bone.rollDegree * -1.0f);
                    
                    obj->bone.referenceXAxis = glm::vec3(obj->bone.rotationXAxis);
                    obj->bone.referenceYAxis = glm::vec3(obj->bone.rotationYAxis);
                    obj->bone.referenceZAxis = glm::vec3(obj->bone.rotationZAxis);
                }
            }
            if (obj->superObject->instanced.isInstanced && !obj->instanced.isInstanced) {
                obj->instanced.isInstanced = true;
                obj->instanced.translate = obj->superObject->instanced.translate;
                obj->instanced.scale = obj->superObject->instanced.scale;
                obj->instanced.front = obj->superObject->instanced.front;
                obj->instanced.up = obj->superObject->instanced.up;
                obj->instanced.left = obj->superObject->instanced.left;
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
    
    setShaders(scene);
    setBuffers(scene);
    
    cameraPtr = cameraPtrs[0];
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glPointSize(10.0);
    if (multiSampling)
        glEnable(GL_MULTISAMPLE);
    
//    setPose();
    
//
//    // shadow
//    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
//    unsigned int depthMapFBO;
//    glGenFramebuffers(1, &depthMapFBO);
//    // create depth texture
//    unsigned int depthMap;
//    glGenTextures(1, &depthMap);
//    glBindTexture(GL_TEXTURE_2D, depthMap);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//    // attach depth texture as FBO's depth buffer
//    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
//    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
//    glDrawBuffer(GL_NONE);
//    glReadBuffer(GL_NONE);
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
//
//
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        float timeDelta = currentFrame - lastFrame;
        lastFrame = currentFrame;
        float fps = 1 / timeDelta;
//        cout << fps << endl;
        
        processContinuousInput(window);
        glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
        
        if (fboPtr != NULL && fboPtr->hidden == false) {
            glBindFramebuffer(GL_FRAMEBUFFER, fboPtr->shader.fbo);
        }
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.28f, 0.28f, 0.28f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        
        projection = glm::perspective(glm::radians(cameraPtr->camera.fov), scene->layout.width / scene->layout.height, cameraPtr->camera.minDistance, cameraPtr->camera.maxDistance);
        view = lookAt(cameraPtr->transform.position, cameraPtr->transform.position + cameraPtr->transform.front, cameraPtr->transform.up);
        textprojection = glm::ortho(0.0f, scene->layout.width, 0.0f, scene->layout.height);
        
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
        else if (entry.first == "tgn")
            objPtr->shader.tangents = processAttributeArray<float>(entry.second);
        else if (entry.first == "btgn")
            objPtr->shader.bitangents = processAttributeArray<float>(entry.second);
        else if (entry.first == "kern")
            objPtr->style.kernel = processAttributeArray<float>(entry.second);
        else if (entry.first == "instrns") {
            objPtr->instanced.translate = processAttributeArray<float>(entry.second);
            objPtr->instanced.isInstanced = true;
        }
        else if (entry.first == "insscal") {
            objPtr->instanced.scale = processAttributeArray<float>(entry.second);
            objPtr->instanced.isInstanced = true;
        }
        else if (entry.first == "insfron") {
            objPtr->instanced.front = processAttributeArray<float>(entry.second);
            objPtr->instanced.isInstanced = true;
        }
        else if (entry.first == "insup") {
            objPtr->instanced.up = processAttributeArray<float>(entry.second);
            objPtr->instanced.isInstanced = true;
        }
        else if (entry.first == "insleft") {
            objPtr->instanced.left = processAttributeArray<float>(entry.second);
            objPtr->instanced.isInstanced = true;
        }
        else if (entry.first == "roll")
            objPtr->bone.rollDegree = stof(entry.second);
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
        else if (entry.first == "trns") {
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
        else if (entry.first.rfind("tex", 0) == 0) {
            objPtr->material.texturesBase64.push_back(entry.second);
            objPtr->material.texture = true;
            if (0 == entry.first.compare(entry.first.length() - 2, 2, "nr"))
                objPtr->material.normMapIndexes.push_back((int)objPtr->material.texturesBase64.size() - 1);
            else if (0 == entry.first.compare(entry.first.length() - 2, 2, "sp"))
                objPtr->material.specMapIndexes.push_back((int)objPtr->material.texturesBase64.size() - 1);
        }
        else if (entry.first == "colo") {
            vector<float> sequence = processAttributeArray<float>(entry.second);
            objPtr->layout.color = glm::vec3(sequence[0], sequence[1], sequence[2]);
        }
    }
    if (objPtr->type == ObjectType::Camera)
        cameraPtrs.push_back(objPtr);
    else if (objPtr->type == ObjectType::Framebuffer)
        fboPtrs.push_back(objPtr);
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
        objPtr->shader.fragmentShader = "#version 330 core\nout vec4 FragColor;\n";
        
        objPtr->shader.vertexShader += (objPtr->type != ObjectType::Text && objPtr->type != ObjectType::Framebuffer) ? "layout(location = 0) in vec3 vPos;\n" : "";
        objPtr->shader.vertexShader += (objPtr->type == ObjectType::Text) ? "layout(location = 0) in vec4 vPos;\n" : "";
        objPtr->shader.vertexShader += (objPtr->type == ObjectType::Text) ? "out vec2 TexCoord;\n" : "";
        
        objPtr->shader.vertexShader += (objPtr->type == ObjectType::Cubemap) ? "out vec3 TexCoord;\n" : "";
        
        objPtr->shader.vertexShader += (objPtr->type == ObjectType::Framebuffer) ? "layout(location = 0) in vec2 vPos;\n" : "";
        objPtr->shader.vertexShader += (objPtr->type == ObjectType::Framebuffer) ? "layout(location = 1) in vec2 vTexCoord;\n" : "";
        objPtr->shader.vertexShader += (objPtr->type == ObjectType::Framebuffer) ? "out vec2 TexCoord;\n" : "";
        objPtr->shader.vertexShader += (objPtr->type == ObjectType::Framebuffer) ? "void main() {\n" : "";
        objPtr->shader.vertexShader += (objPtr->type == ObjectType::Framebuffer) ? "\tTexCoord = vTexCoord;\n" : "";
        objPtr->shader.vertexShader += (objPtr->type == ObjectType::Framebuffer) ? "\tgl_Position = vec4(vPos.x, vPos.y, 0.0, 1.0);\n" : "";
        
        if (objPtr->type == ObjectType::Model) {
            objPtr->shader.vertexShader += "layout(location = 1) in vec3 vNormal;\n";
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "layout(location = 2) in vec2 vTexCoord;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "layout(location = 3) in float vTexOrder;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "layout(location = 4) in float vTexQty;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.normMapIndexes.size() > 0) ? "layout(location = 5) in vec3 vTangent;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.normMapIndexes.size() > 0) ? "layout(location = 6) in vec3 vBitangent;\n" : "";
            if (objPtr->instanced.isInstanced) {
                objPtr->shader.vertexShader += (objPtr->material.texture) ? ((objPtr->material.normMapIndexes.size() > 0) ? "layout(location = 7) in mat4 instanceMatrix;\n" : "layout(location = 5) in mat4 instanceMatrix;\n") : "layout(location = 2) in mat4 instanceMatrix;\n";
                objPtr->shader.vertexShader += (objPtr->material.texture) ? ((objPtr->material.normMapIndexes.size() > 0) ? "layout(location = 11) in mat4 instanceRotationMatrix;\n" : "layout(location = 9) in mat4 instanceRotationMatrix;\n") : "layout(location = 6) in mat4 instanceRotationMatrix;\n";
            }
            objPtr->shader.vertexShader += "out vec3 FragPos;\n";
            objPtr->shader.vertexShader += "out vec3 Normal;\n";
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "out vec2 TexCoord;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "out float TexOrder;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "out float TexQty;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.normMapIndexes.size() > 0) ? "out mat3 NormalMatrix;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.normMapIndexes.size() > 0) ? "out vec3 Tangent;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.normMapIndexes.size() > 0) ? "out vec3 Bitangent;\n" : "";
        }
        
        if (objPtr->type != ObjectType::Framebuffer) {
            objPtr->shader.vertexShader += (objPtr->type != ObjectType::Text && objPtr->type != ObjectType::Cubemap) ? "uniform mat4 model;\n" : "";
            objPtr->shader.vertexShader += (objPtr->type != ObjectType::Text) ? "uniform mat4 view;\n" : "";
            objPtr->shader.vertexShader += "uniform mat4 projection;\n";
            objPtr->shader.vertexShader += "uniform mat4 rotation;\n";
            objPtr->shader.vertexShader += "void main() {\n";
            if (objPtr->instanced.translate.size() > 0)
                objPtr->shader.vertexShader += "\tgl_Position = projection * view * model * instanceMatrix * vec4(vPos, 1.0f);\n";
            else
                objPtr->shader.vertexShader += (objPtr->type != ObjectType::Text && objPtr->type != ObjectType::Cubemap) ? "\tgl_Position = projection * view * model * vec4(vPos, 1.0f);\n" : "";
            objPtr->shader.vertexShader += (objPtr->type == ObjectType::Text) ? "\tgl_Position = projection * vec4(vPos.xy, 0.0, 1.0);\n" : "";
            objPtr->shader.vertexShader += (objPtr->type == ObjectType::Text) ? "\tTexCoord = vPos.zw;\n" : "";
        }
        
        if (objPtr->type == ObjectType::Model) {
            if (objPtr->instanced.isInstanced) {
                objPtr->shader.vertexShader += "\tFragPos = vec3(model * instanceMatrix * vec4(vPos, 1.0f));\n";
                objPtr->shader.vertexShader += "\tNormal = vec3(rotation * instanceRotationMatrix * vec4(vNormal, 1.0f));\n";
            }
            else {
                objPtr->shader.vertexShader += "\tFragPos = vec3(model * vec4(vPos, 1.0f));\n";
                objPtr->shader.vertexShader += "\tNormal = vec3(rotation * vec4(vNormal, 1.0f));\n";
            }
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "\tTexCoord = vTexCoord;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "\tTexOrder = vTexOrder;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "\tTexQty = vTexQty;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.normMapIndexes.size() > 0) ? "\tNormalMatrix = transpose(inverse(mat3(model)));\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.normMapIndexes.size() > 0) ? "\tTangent = vTangent;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.normMapIndexes.size() > 0) ? "\tBitangent = vBitangent;\n" : "";
        }
            
        objPtr->shader.vertexShader += "}\0";

        if (objPtr->type == ObjectType::Model) {
            objPtr->shader.fragmentShader += "in vec3 FragPos;\n";
            objPtr->shader.fragmentShader += "in vec3 Normal;\n";
            objPtr->shader.fragmentShader += (objPtr->material.texture) ? "in vec2 TexCoord;\n" : "";
            objPtr->shader.fragmentShader += (objPtr->material.texture) ? "in float TexOrder;\n" : "";
            objPtr->shader.fragmentShader += (objPtr->material.texture) ? "in float TexQty;\n" : "";
            objPtr->shader.fragmentShader += (objPtr->material.normMapIndexes.size() > 0) ? "in mat3 NormalMatrix;\n" : "";
            objPtr->shader.fragmentShader += (objPtr->material.normMapIndexes.size() > 0) ? "in vec3 Tangent;\n" : "";
            objPtr->shader.fragmentShader += (objPtr->material.normMapIndexes.size() > 0) ? "in vec3 Bitangent;\n" : "";
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
            objPtr->shader.fragmentShader += "};\n";
            objPtr->shader.fragmentShader += (objPtr->material.texture) ? "uniform sampler2D textures[" + to_string(objPtr->material.texturesBase64.size()) + "];\n" : "";
            objPtr->shader.fragmentShader += (objPtr->material.specMapIndexes.size() > 0) ? "uniform int specMapIndexes[" + to_string(objPtr->material.specMapIndexes.size()) + "];\n" : "";
            objPtr->shader.fragmentShader += (objPtr->material.normMapIndexes.size() > 0) ? "uniform int normMapIndexes[" + to_string(objPtr->material.normMapIndexes.size()) + "];\n" : "";
            objPtr->shader.fragmentShader += "uniform vec3 cameraPos;\n";
            objPtr->shader.fragmentShader += "uniform Material modelMaterial;\n";
            objPtr->shader.fragmentShader += "uniform Light lights[" + to_string(count_if(objects.begin(), objects.end(), [] (Object obj) { return obj.type == ObjectType::Light; })) + "];\n";
            objPtr->shader.fragmentShader += "uniform int shading;\n";
            objPtr->shader.fragmentShader += "uniform bool gamma;\n";
            objPtr->shader.fragmentShader += "mat3 TBN;\n";
            objPtr->shader.fragmentShader += "vec4 CalculateLight(Light light, vec3 normal, vec3 viewDir, vec3 fragPos);\n";
            objPtr->shader.fragmentShader += "void main() {\n";
            if (objPtr->material.normMapIndexes.size() > 0) {
                objPtr->shader.fragmentShader += "\tvec3 T = normalize(NormalMatrix * Tangent);\n";
                objPtr->shader.fragmentShader += "\tvec3 N = normalize(NormalMatrix * Normal);\n";
                objPtr->shader.fragmentShader += "\tT = normalize(T - dot(T, N) * N);\n";
                objPtr->shader.fragmentShader += "\tvec3 B = cross(N, T);\n";
                objPtr->shader.fragmentShader += "\tTBN = transpose(mat3(T, B, N));\n";
                objPtr->shader.fragmentShader += "\tvec3 tangentCameraPos = TBN * cameraPos;\n";
                objPtr->shader.fragmentShader += "\tvec3 tangentFragPos = TBN * FragPos;\n";
                objPtr->shader.fragmentShader += "\tvec3 norm = vec3(0.0f);\n";
                objPtr->shader.fragmentShader += "\tfor(int i = 0; i < normMapIndexes.length(); i++) {\n";
                objPtr->shader.fragmentShader += "\t\tnorm += texture(textures[normMapIndexes[i]], TexCoord).rgb;\n";
                objPtr->shader.fragmentShader += "\t}\n";
                objPtr->shader.fragmentShader += "\tnorm /= normMapIndexes.length();\n";
                objPtr->shader.fragmentShader += "\tnorm = normalize(norm * 2.0 - 1.0);\n";
            }
            else
                objPtr->shader.fragmentShader += "\tvec3 norm = normalize(Normal);\n";
            objPtr->shader.fragmentShader += (objPtr->material.normMapIndexes.size() > 0) ? "\tvec3 viewDir = normalize(tangentCameraPos - tangentFragPos);\n" : "\tvec3 viewDir = normalize(cameraPos - FragPos);\n";
            objPtr->shader.fragmentShader += "\tvec4 result = vec4(0.0f);\n";
            objPtr->shader.fragmentShader += "\tfor (int i = 0; i < lights.length(); i++)\n";
            objPtr->shader.fragmentShader += "\t\tif (lights[i].lightType != -1)\n";
            objPtr->shader.fragmentShader += (objPtr->material.normMapIndexes.size() > 0) ? "\t\t\tresult += CalculateLight(lights[i], norm, viewDir, tangentFragPos);\n" : "\t\t\tresult += CalculateLight(lights[i], norm, viewDir, FragPos);\n";
            objPtr->shader.fragmentShader += "\tif (gamma)\n";
            objPtr->shader.fragmentShader += "\t\tresult.xyz = pow(result.xyz, vec3(1.0/2.2));\n";
            objPtr->shader.fragmentShader += "\tFragColor = result;\n";
            objPtr->shader.fragmentShader += "}\n";
            objPtr->shader.fragmentShader += "vec4 CalculateLight(Light light, vec3 normal, vec3 viewDir, vec3 fragPos) {\n";
            objPtr->shader.fragmentShader += (objPtr->material.normMapIndexes.size() > 0) ? "\tvec3 lightPos = TBN * light.position;\n" : "\tvec3 lightPos = light.position;\n";
            objPtr->shader.fragmentShader += "\tvec3 lightDir = normalize(lightPos - fragPos);\n";
            objPtr->shader.fragmentShader += "\tif (light.lightType == 1)\n";
            objPtr->shader.fragmentShader += "\t\tlightDir = normalize(-light.direction);\n";
            objPtr->shader.fragmentShader += "\tfloat diffStrength = max(dot(normal, lightDir), 0.0);\n";
            objPtr->shader.fragmentShader += "\tvec3 reflectDir = reflect(-lightDir, normal);\n";
            objPtr->shader.fragmentShader += "\tvec3 halfwayDir = normalize(lightDir + viewDir);\n";
            objPtr->shader.fragmentShader += "\tfloat specStrength = 0.0f;\n";
            objPtr->shader.fragmentShader += "\tif (shading == 0)\n";
            objPtr->shader.fragmentShader += "\t\tspecStrength = pow(max(dot(viewDir, reflectDir), 0.0), modelMaterial.shininess);\n";
            objPtr->shader.fragmentShader += "\telse if (shading == 1)\n";
            objPtr->shader.fragmentShader += "\t\tspecStrength = pow(max(dot(normal, halfwayDir), 0.9), modelMaterial.shininess);\n";
            
            if (objPtr->material.texture) {
                objPtr->shader.fragmentShader += "\tint complexOrder = int(TexOrder);\n";
                objPtr->shader.fragmentShader += "\tint texQuantity = int(TexQty);\n";
                objPtr->shader.fragmentShader += "\tvec4 ambient = vec4(0.0f);\n";
                objPtr->shader.fragmentShader += "\tvec4 diffuse = vec4(0.0f);\n";
                objPtr->shader.fragmentShader += "\tvec4 specular = vec4(0.0f);\n";
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
                objPtr->shader.fragmentShader += "\t\tvec4 texv4 = texture(textures[order], TexCoord);\n";
                objPtr->shader.fragmentShader += "\t\tif (specMap)\n";
                objPtr->shader.fragmentShader += "\t\t\tspecular += vec4(light.material.specular, 1.0f) * specStrength * texture(textures[order], TexCoord);\n";
                objPtr->shader.fragmentShader += "\t\telse {\n";
                objPtr->shader.fragmentShader += "\t\t\tambient += vec4(light.material.ambient, 1.0f) * texture(textures[order], TexCoord) * vec4(modelMaterial.diffuse, 1.0f);\n";
                objPtr->shader.fragmentShader += "\t\t\tdiffuse += vec4(light.material.diffuse, 1.0f) * diffStrength * texture(textures[order], TexCoord) * vec4(modelMaterial.diffuse, 1.0f);\n";
                objPtr->shader.fragmentShader += "\t\t}\n";
                objPtr->shader.fragmentShader += "\t}\n";
            }
            else {
                objPtr->shader.fragmentShader += "\tvec4 ambient = vec4(light.material.ambient, 1.0f) * vec4(modelMaterial.ambient, 1.0f);\n";
                objPtr->shader.fragmentShader += "\tvec4 diffuse = vec4(light.material.diffuse, 1.0f) * diffStrength * vec4(modelMaterial.diffuse, 1.0f);\n";
                objPtr->shader.fragmentShader += "\tvec4 specular = vec4(light.material.specular, 1.0f) * specStrength * vec4(modelMaterial.specular, 1.0f);\n";
            }
            
            objPtr->shader.fragmentShader += "\tif (light.lightType != 1) {\n";
            objPtr->shader.fragmentShader += "\t\tfloat distance = length(lightPos - fragPos);\n";
            objPtr->shader.fragmentShader += "\t\tfloat attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));\n";
            objPtr->shader.fragmentShader += "\t\tambient.xyz *= attenuation;\n";
            objPtr->shader.fragmentShader += "\t\tdiffuse.xyz *= attenuation;\n";
            objPtr->shader.fragmentShader += "\t\tspecular.xyz *= attenuation;\n";
            
            objPtr->shader.fragmentShader += "\t\tif (gamma) {\n";
            objPtr->shader.fragmentShader += "\t\tambient.xyz *= 1.0 / (distance * distance);\n";
            objPtr->shader.fragmentShader += "\t\tdiffuse.xyz *= 1.0 / (distance * distance);\n";
            objPtr->shader.fragmentShader += "\t\tspecular.xyz *= 1.0 / (distance * distance);\n";
            objPtr->shader.fragmentShader += "\t\t}\n";
            objPtr->shader.fragmentShader += "\t\tif (light.lightType == 2) {\n";
            objPtr->shader.fragmentShader += "\t\t\tfloat theta = dot(lightDir, normalize(-light.direction));\n";
            objPtr->shader.fragmentShader += "\t\t\tfloat epsilon = light.cutOff - light.outerCutOff;\n";
            objPtr->shader.fragmentShader += "\t\t\tfloat intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);\n";
            objPtr->shader.fragmentShader += "\t\t\tambient.xyz *= intensity;\n";
            objPtr->shader.fragmentShader += "\t\t\tdiffuse.xyz *= intensity;\n";
            objPtr->shader.fragmentShader += "\t\t\tspecular.xyz *= intensity;\n";
            objPtr->shader.fragmentShader += "\t\t}\n";
            objPtr->shader.fragmentShader += "\t}\n";
            objPtr->shader.fragmentShader += "\treturn (ambient + diffuse + specular);\n";
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
        else if (objPtr->type == ObjectType::Framebuffer) {
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
        objPtr->shader.shaderID = glCreateProgram();
        glAttachShader(objPtr->shader.shaderID, vertexShader);
        glAttachShader(objPtr->shader.shaderID, fragmentShader);
        glLinkProgram(objPtr->shader.shaderID);
        glGetProgramiv(objPtr->shader.shaderID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(objPtr->shader.shaderID, 512, NULL, infoLog);
            cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }
    
//    if (objPtr->name == "radiators") {
//        cout << objPtr->shader.vertexShader << endl;
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
        
        glGenVertexArrays(1, &objPtr->shader.vao);
        glGenBuffers(1, &objPtr->shader.vbo);
        if (objPtr->shader.faces.size() > 0)
            glGenBuffers(1, &objPtr->shader.ebo);
        glBindVertexArray(objPtr->shader.vao);
        glBindBuffer(GL_ARRAY_BUFFER, objPtr->shader.vbo);
        if (objPtr->type == ObjectType::Framebuffer) {
            glBufferData(GL_ARRAY_BUFFER, objPtr->shader.vertices.size() * 2 * sizeof(float), NULL, GL_STATIC_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, objPtr->shader.vertices.size() * sizeof(float), &objPtr->shader.vertices[0]);
            glBufferSubData(GL_ARRAY_BUFFER, objPtr->shader.vertices.size() * sizeof(float), objPtr->shader.texCoords.size() * sizeof(float), &objPtr->shader.texCoords[0]);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(objPtr->shader.vertices.size() * sizeof(float)));
            glEnableVertexAttribArray(1);
            glGenFramebuffers(1, &objPtr->shader.fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, objPtr->shader.fbo);
            objPtr->objectPtr->material.textures.push_back(*new unsigned int());
            glGenTextures(1, &objPtr->material.textures[0]);
            GLenum textype = multiSampling ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
            glBindTexture(textype, objPtr->material.textures[0]);
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
            
            return;
        }
        if (objPtr->type == ObjectType::Joint && objPtr->superObject->type == ObjectType::Joint)
            objPtr->shader.vertices.insert(objPtr->shader.vertices.begin(),
                                           objPtr->superObject->shader.vertices.end() - 3,
                                           objPtr->superObject->shader.vertices.end());
        if (objPtr->type != ObjectType::Text && objPtr->type != ObjectType::Cubemap) {
            int attrCount = objPtr->material.texture ? (objPtr->material.normMapIndexes.size() > 0 ? 16 : 10) : 6;
            glBufferData(GL_ARRAY_BUFFER, objPtr->shader.vertices.size() / 3 * attrCount * sizeof(float), NULL, GL_DYNAMIC_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, objPtr->shader.vertices.size() * sizeof(float), &objPtr->shader.vertices[0]);
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
                    if (objPtr->shader.tangents.size() == 0 || objPtr->shader.bitangents.size() == 0)
                        calculateTangentsBitangents(objPtr);
                    glBufferSubData(GL_ARRAY_BUFFER, (objPtr->shader.vertices.size() + objPtr->shader.normals.size() + objPtr->shader.texCoords.size() + objPtr->shader.texOrders.size() + objPtr->shader.texQuantities.size()) * sizeof(float), objPtr->shader.tangents.size() * sizeof(float), &objPtr->shader.tangents[0]);
                    glBufferSubData(GL_ARRAY_BUFFER, (objPtr->shader.vertices.size() + objPtr->shader.normals.size() + objPtr->shader.texCoords.size() + objPtr->shader.texOrders.size() + objPtr->shader.texQuantities.size() + objPtr->shader.tangents.size()) * sizeof(float), objPtr->shader.bitangents.size() * sizeof(float), &objPtr->shader.bitangents[0]);
                }
            }
            
            if (objPtr->instanced.isInstanced) {
                if (objPtr->instanced.translate.size() >= 3)
                    objPtr->shader.instanceCount = int(objPtr->instanced.translate.size() / 3);
                else if (objPtr->instanced.scale.size() >= 3)
                    objPtr->shader.instanceCount = int(objPtr->instanced.scale.size() / 3);
                else if (objPtr->instanced.front.size() >= 3)
                    objPtr->shader.instanceCount = int(objPtr->instanced.front.size() / 3);
                else if (objPtr->instanced.up.size() >= 3)
                    objPtr->shader.instanceCount = int(objPtr->instanced.up.size() / 3);
                else if (objPtr->instanced.left.size() >= 3)
                    objPtr->shader.instanceCount = int(objPtr->instanced.left.size() / 3);
                
                for (int i = 0; i < objPtr->shader.instanceCount; i++) {
                    glm::mat4 insmatrix, insrotmatrix;
                    glm::vec3 translate, scale, front, up, left;
                    translate = (objPtr->instanced.translate.size() > 3) ? glm::vec3(objPtr->instanced.translate[i * 3], objPtr->instanced.translate[i * 3 + 1], objPtr->instanced.translate[i * 3 + 2]) : (objPtr->instanced.translate.size() == 3 ? glm::vec3(objPtr->instanced.translate[0], objPtr->instanced.translate[1], objPtr->instanced.translate[2]) : glm::vec3(0.0f));
                    scale = (objPtr->instanced.scale.size() > 3) ? glm::vec3(objPtr->instanced.scale[i * 3], objPtr->instanced.scale[i * 3 + 1], objPtr->instanced.scale[i * 3 + 2]) : (objPtr->instanced.scale.size() == 3 ? glm::vec3(objPtr->instanced.scale[0], objPtr->instanced.scale[1], objPtr->instanced.scale[2]) : glm::vec3(1.0, 1.0, 1.0));
                    front = (objPtr->instanced.front.size() > 3) ? glm::vec3(objPtr->instanced.front[i * 3], objPtr->instanced.front[i * 3 + 1], objPtr->instanced.front[i * 3 + 2]) : (objPtr->instanced.front.size() == 3 ? glm::vec3(objPtr->instanced.front[0], objPtr->instanced.front[1], objPtr->instanced.front[2]) : glm::vec3(0.0, 0.0, 1.0));
                    up = (objPtr->instanced.up.size() > 3) ? glm::vec3(objPtr->instanced.up[i * 3], objPtr->instanced.up[i * 3 + 1], objPtr->instanced.up[i * 3 + 2]) : (objPtr->instanced.up.size() == 3 ? glm::vec3(objPtr->instanced.up[0], objPtr->instanced.up[1], objPtr->instanced.up[2]) : glm::vec3(0.0, 1.0, 0.0));
                    left = (objPtr->instanced.left.size() > 3) ? glm::vec3(objPtr->instanced.left[i * 3], objPtr->instanced.left[i * 3 + 1], objPtr->instanced.left[i * 3 + 2]) : (objPtr->instanced.left.size() == 3 ? glm::vec3(objPtr->instanced.left[0], objPtr->instanced.left[1], objPtr->instanced.left[2]) : glm::vec3(1.0, 0.0, 0.0));
                    insmatrix = glm::translate(glm::mat4(1.0f), translate);
                    insmatrix = glm::scale(insmatrix, scale);
                    insrotmatrix = glm::mat4(left.x,left.y, left.z, 0,
                                            up.x, up.y, up.z, 0,
                                            front.x, front.y, front.z, 0,
                                            0, 0, 0, 1);
                    insmatrix *= insrotmatrix;
                    objPtr->instanced.instanceMatrices.push_back(insmatrix);
                    objPtr->instanced.instanceMatrices.push_back(insrotmatrix);
                    
                }
                glGenBuffers(1, &objPtr->shader.ibo);
                glBindBuffer(GL_ARRAY_BUFFER, objPtr->shader.ibo);
                glBufferData(GL_ARRAY_BUFFER, objPtr->shader.instanceCount * 2 * sizeof(glm::mat4), &objPtr->instanced.instanceMatrices[0], GL_STATIC_DRAW);
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
            if (objPtr->instanced.isInstanced) {
                glBindBuffer(GL_ARRAY_BUFFER, objPtr->shader.ibo);
                glBindVertexArray(objPtr->shader.vao);
                int attrCount = objPtr->material.texture ? ((objPtr->material.normMapIndexes.size() > 0) ? 7 : 5) : 2;
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
                int width, height, nrChannels;
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
                    unsigned char *data = stbi_load_from_memory(&decoded[0], int(decoded.size()), &width, &height, &nrChannels, 0);
                    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                    if (nrChannels == 1)
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
                    else if (nrChannels == 3) {
                        if (gammaCorrection)
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                        else
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                    }
                    else if (nrChannels == 4) {
                        if (gammaCorrection)
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                        else
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
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
                    glUniform1i(glGetUniformLocation(objPtr->shader.shaderID, ("textures[" + to_string(i) + "]").c_str()), i);
                    
                    itr = find(objPtr->material.specMapIndexes.begin(), objPtr->material.specMapIndexes.end(), i);
                    if (itr != objPtr->material.specMapIndexes.end())
                        glUniform1i(glGetUniformLocation(objPtr->shader.shaderID, ("specMapIndexes[" + to_string(itr - objPtr->material.specMapIndexes.begin()) + "]").c_str()), i);
                    itr = find(objPtr->material.normMapIndexes.begin(), objPtr->material.normMapIndexes.end(), i);
                    if (itr != objPtr->material.normMapIndexes.end())
                        glUniform1i(glGetUniformLocation(objPtr->shader.shaderID, ("normMapIndexes[" + to_string(itr - objPtr->material.normMapIndexes.begin()) + "]").c_str()), i);
                }
            }
        }
        else if (objPtr->type == ObjectType::Light || objPtr->type == ObjectType::Joint) {
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
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
            
            vector<Object>::iterator it = objects.begin();
            int index = 0;
            while ((it = find_if(it, objects.end(), [] (Object obj) { return obj.type == ObjectType::Light; })) != objects.end()) {
                if (it->objectPtr->hidden) {
                    glUniform1i(glGetUniformLocation(objPtr->shader.shaderID, ("lights[" + to_string(index) + "].lightType").c_str()), -1);
                    index++;
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
                index++;
                it++;
            }
            if (objPtr->material.texture) {
                for (int i = 0; i < objPtr->material.textures.size(); i++) {
                    glActiveTexture(GL_TEXTURE0 + i);
                    glBindTexture(GL_TEXTURE_2D, objPtr->material.textures[i]);
                }
            }
        }
        else if (objPtr->type == ObjectType::Light) {
            glUniform3fv(glGetUniformLocation(objPtr->shader.shaderID, "color"), 1, value_ptr(objPtr->material.diffuse / 0.8f));
        }
        
        glBindVertexArray(objPtr->shader.vao);
        
        if (objPtr->type == ObjectType::Joint) {
            glDrawArrays(GL_LINES, 0, objPtr->shader.vertexCount);
            glDrawArrays(GL_POINTS, 0, 1);
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
            if (objPtr->instanced.isInstanced)
                glDrawElementsInstanced(GL_TRIANGLES, objPtr->shader.vertexCount, GL_UNSIGNED_INT, 0, objPtr->shader.instanceCount);
            else
                glDrawElements(GL_TRIANGLES, objPtr->shader.vertexCount, GL_UNSIGNED_INT, 0);
        }
        else {
            if (objPtr->instanced.isInstanced)
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

void processDiscreteInput(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    
    // sticky keys
    if ((key == GLFW_KEY_LEFT_SUPER || key == GLFW_KEY_RIGHT_SUPER) && action == GLFW_PRESS) {
        commandKeySticked = true;
    }
    if ((key == GLFW_KEY_LEFT_SUPER || key == GLFW_KEY_RIGHT_SUPER) && action == GLFW_RELEASE) {
        commandKeySticked = false;
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
        
        if (commandKeySticked) {
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
//        resetPose("hips");
//
//        shading = shading == "blinn-phong" ? "phong" : "blinn-phong";

//        if (fboPtr != NULL)
//            fboPtr->hidden = !fboPtr->hidden;
        
        
//        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "baseflat"; });
//        it->objectPtr->hidden = !it->objectPtr->hidden;
//        it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "basesmooth"; });
//        it->objectPtr->hidden = !it->objectPtr->hidden;
        
        
        cout << "pos: " << cameraPtr->transform.position.x << " " << cameraPtr->transform.position.y << " " << cameraPtr->transform.position.z << endl;
        cout << "fro: " << cameraPtr->transform.front.x << " " << cameraPtr->transform.front.y << " " << cameraPtr->transform.front.z << endl;
        cout << "up: " << cameraPtr->transform.up.x << " " << cameraPtr->transform.up.y << " " << cameraPtr->transform.up.z << endl;
        cout << "lef: " << cameraPtr->transform.left.x << " " << cameraPtr->transform.left.y << " " << cameraPtr->transform.left.z << endl;
        cout << "fov: " << cameraPtr->camera.fov << endl;
        
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
        //option 0
    }
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {    // P
        //option 1
    }
    if (key == GLFW_KEY_L && action == GLFW_PRESS) {    // L
        //option 2
    }
    if (key == GLFW_KEY_SEMICOLON && action == GLFW_PRESS) {    // Ş
        //option 3
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
        cameraPtr->transform.up = rotateVectorAroundAxis(cameraPtr->transform.up, cameraPtr->transform.left, cameraPtr->camera.fov * -0.01);
        cameraPtr->transform.front = rotateVectorAroundAxis(cameraPtr->transform.front, cameraPtr->transform.left, cameraPtr->camera.fov * -0.01);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        cameraPtr->transform.up = rotateVectorAroundAxis(cameraPtr->transform.up, cameraPtr->transform.left, cameraPtr->camera.fov * 0.01);
        cameraPtr->transform.front = rotateVectorAroundAxis(cameraPtr->transform.front, cameraPtr->transform.left, cameraPtr->camera.fov * 0.01);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        cameraPtr->transform.front = rotateVectorAroundAxis(cameraPtr->transform.front, cameraPtr->transform.up, cameraPtr->camera.fov * 0.01);
        cameraPtr->transform.left = rotateVectorAroundAxis(cameraPtr->transform.left, cameraPtr->transform.up, cameraPtr->camera.fov * 0.01);
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        cameraPtr->transform.front = rotateVectorAroundAxis(cameraPtr->transform.front, cameraPtr->transform.up, cameraPtr->camera.fov * -0.01);
        cameraPtr->transform.left = rotateVectorAroundAxis(cameraPtr->transform.left, cameraPtr->transform.up, cameraPtr->camera.fov * -0.01);
    }
    if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS) {
        cameraPtr->transform.left = rotateVectorAroundAxis(cameraPtr->transform.left, cameraPtr->transform.front, cameraPtr->camera.fov * -0.01);
        cameraPtr->transform.up = rotateVectorAroundAxis(cameraPtr->transform.up, cameraPtr->transform.front, cameraPtr->camera.fov * -0.01);
    }
    if (glfwGetKey(window, GLFW_KEY_SLASH) == GLFW_PRESS) {
        cameraPtr->transform.left = rotateVectorAroundAxis(cameraPtr->transform.left, cameraPtr->transform.front, cameraPtr->camera.fov * 0.01);
        cameraPtr->transform.up = rotateVectorAroundAxis(cameraPtr->transform.up, cameraPtr->transform.front, cameraPtr->camera.fov * 0.01);
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
        obj->objectPtr->transform.position += glm::vec3(0.0, 0.0, offset);
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
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "human"; });
        rotation(it->objectPtr, 1.0f);
        translation(it->objectPtr, 0.01f);
//        locateJoint("head", glm::vec3(0.01, 0.0, 0.0));
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "human"; });
        rotation(it->objectPtr, -1.0f);
        translation(it->objectPtr, -0.01f);
//        locateJoint("head", glm::vec3(-0.01, 0.0, 0.0));
    }

    
    
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
//        float angle = 0.5f;
//        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
//            angle *= -1.0;
//        rotateJoint("leftshoulder", glm::vec3(angle, 0.0, 0.0));
        float offset = 0.01f;
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
            offset *= -1.0;
        locateJoint("leftarm", glm::vec3(offset, 0.0, 0.0));
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
//        float angle = 0.5f;
//        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
//            angle *= -1.0;
//        rotateJoint("leftshoulder", glm::vec3(0.0, angle, 0.0));
        float offset = 0.01f;
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
            offset *= -1.0;
        locateJoint("leftarm", glm::vec3(0.0, offset, 0.0));
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
//        float angle = 0.5f;
//        if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
//            angle *= -1.0;
//        rotateJoint("leftshoulder", glm::vec3(0.0, 0.0, angle));
        float offset = 0.01f;
        if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
            offset *= -1.0;
        locateJoint("leftarm", glm::vec3(0.0, 0.0, offset));
    }
    
    
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
        float offset = 0.01f;
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
            offset *= -1.0;
//        locateJoint("head", glm::vec3(offset, 0.0, 0.0));
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "light1"; });
        it->objectPtr->transform.position += glm::vec3(offset, 0, 0);
    }
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
        float offset = 0.01f;
        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
            offset *= -1.0;
//        locateJoint("head", glm::vec3(0.0, offset, 0.0));
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [](Object obj) { return obj.name == "light1"; });
        it->objectPtr->transform.position += glm::vec3(0, offset, 0);
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_COMMA) == GLFW_PRESS) {
        float offset = 0.01f;
        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
            offset *= -1.0;
//        locateJoint("head", glm::vec3(0.0, 0.0, offset));
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

void rotateJoint(string joint, glm::vec3 degrees)
{
    vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [joint] (Object obj) { return obj.name == joint; });
    
    size_t vertLeng = it->objectPtr->shader.vertices.size();
    glm::vec3 jo = glm::vec3(it->objectPtr->shader.vertices[vertLeng - 3],
                            it->objectPtr->shader.vertices[vertLeng - 2],
                            it->objectPtr->shader.vertices[vertLeng - 1]);
    glm::vec3 axis;
    float angle;
    
    Object* rootPtr = it->objectPtr;
    while (rootPtr->type != ObjectType::Model)
        rootPtr = rootPtr->superObject;
    
    function<void(Object*)> lambdaFunc = [rootPtr, jo, &axis, &angle, &lambdaFunc](Object* obj) -> void {
        size_t vertLeng = obj->objectPtr->shader.vertices.size();
        glm::vec3 end = glm::vec3(obj->objectPtr->shader.vertices[vertLeng - 3],
                                obj->objectPtr->shader.vertices[vertLeng - 2],
                                obj->objectPtr->shader.vertices[vertLeng - 1]);
        if (obj->objectPtr->type == ObjectType::Joint) {
            if (jo != end) {
                glm::vec3 beg = glm::vec3(obj->objectPtr->shader.vertices[0],
                                          obj->objectPtr->shader.vertices[1],
                                          obj->objectPtr->shader.vertices[2]);
                beg = rotateVectorAroundAxis(beg - jo, axis, angle);
                beg += jo;
                end = rotateVectorAroundAxis(end - jo, axis, angle);
                end += jo;
                obj->objectPtr->bone.rotationXAxis = rotateVectorAroundAxis(obj->objectPtr->bone.rotationXAxis, axis, angle);
                obj->objectPtr->bone.rotationYAxis = rotateVectorAroundAxis(obj->objectPtr->bone.rotationYAxis, axis, angle);
                obj->objectPtr->bone.rotationZAxis = rotateVectorAroundAxis(obj->objectPtr->bone.rotationZAxis, axis, angle);
                obj->objectPtr->bone.referenceXAxis = rotateVectorAroundAxis(obj->objectPtr->bone.referenceXAxis, axis, angle);
                obj->objectPtr->bone.referenceYAxis = rotateVectorAroundAxis(obj->objectPtr->bone.referenceYAxis, axis, angle);
                obj->objectPtr->bone.referenceZAxis = rotateVectorAroundAxis(obj->objectPtr->bone.referenceZAxis, axis, angle);
                obj->objectPtr->shader.vertices[0] = beg.x;
                obj->objectPtr->shader.vertices[1] = beg.y;
                obj->objectPtr->shader.vertices[2] = beg.z;
                obj->objectPtr->shader.vertices[3] = end.x;
                obj->objectPtr->shader.vertices[4] = end.y;
                obj->objectPtr->shader.vertices[5] = end.z;
                glBindBuffer(GL_ARRAY_BUFFER, obj->objectPtr->shader.vbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, obj->objectPtr->shader.vertices.size() * sizeof(float), &obj->objectPtr->shader.vertices[0]);
            }
        }
        else {
            for (int i = 0; i < obj->objectPtr->shader.vertices.size(); i += 3) {
                glm::vec3 vert = glm::vec3(obj->objectPtr->shader.vertices[i],
                                          obj->objectPtr->shader.vertices[i + 1],
                                          obj->objectPtr->shader.vertices[i + 2]);
                vert = rotateVectorAroundAxis(vert - jo, axis, angle);
                vert += jo;
                obj->objectPtr->shader.vertices[i] = vert.x;
                obj->objectPtr->shader.vertices[i + 1] = vert.y;
                obj->objectPtr->shader.vertices[i + 2] = vert.z;
                glm::vec3 nor = glm::vec3(obj->objectPtr->shader.normals[i],
                                          obj->objectPtr->shader.normals[i + 1],
                                          obj->objectPtr->shader.normals[i + 2]);
                nor = rotateVectorAroundAxis(nor - jo, axis, angle);
                nor += jo;
                obj->objectPtr->shader.normals[i] = nor.x;
                obj->objectPtr->shader.normals[i + 1] = nor.y;
                obj->objectPtr->shader.normals[i + 2] = nor.z;
            }
            glBindBuffer(GL_ARRAY_BUFFER, obj->objectPtr->shader.vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, obj->objectPtr->shader.vertices.size() * sizeof(float), &obj->objectPtr->shader.vertices[0]);
            glBufferSubData(GL_ARRAY_BUFFER, obj->objectPtr->shader.vertices.size() * sizeof(float), obj->objectPtr->shader.normals.size() * sizeof(float), &obj->objectPtr->shader.normals[0]);
        }
        
        
        for (int i = 0; i < obj->objectPtr->bone.indices.size(); i++) {
            unsigned int indice = obj->objectPtr->bone.indices[i];
            glm::vec3 pos = glm::vec3(rootPtr->shader.vertices[indice * 3],
                                      rootPtr->shader.vertices[indice * 3 + 1],
                                      rootPtr->shader.vertices[indice * 3 + 2]);
            pos = rotateVectorAroundAxis(pos - jo, axis, angle * obj->objectPtr->bone.weights[i]);
            pos += jo;
            rootPtr->shader.vertices[indice * 3] = pos.x;
            rootPtr->shader.vertices[indice * 3 + 1] = pos.y;
            rootPtr->shader.vertices[indice * 3 + 2] = pos.z;
            glm::vec3 nor = glm::vec3(rootPtr->shader.normals[indice * 3],
                                      rootPtr->shader.normals[indice * 3 + 1],
                                      rootPtr->shader.normals[indice * 3 + 2]);
            nor = rotateVectorAroundAxis(nor - jo, axis, angle * obj->objectPtr->bone.weights[i]);
            nor += jo;
            rootPtr->shader.normals[indice * 3] = nor.x;
            rootPtr->shader.normals[indice * 3 + 1] = nor.y;
            rootPtr->shader.normals[indice * 3 + 2] = nor.z;
        }
        
        for (Object* ptr : obj->objectPtr->subObjects)
            lambdaFunc(ptr);
    };
    
    if (degrees.x != 0.0) {
        axis = it->objectPtr->bone.rotationXAxis;
        angle = degrees.x;
        lambdaFunc(it->objectPtr);
    }
    
    if (degrees.y != 0.0) {
        axis = it->objectPtr->bone.rotationYAxis;
        angle = degrees.y;
        lambdaFunc(it->objectPtr);
        it->objectPtr->bone.rotationXAxis = rotateVectorAroundAxis(it->objectPtr->bone.rotationXAxis, it->objectPtr->bone.rotationYAxis, angle);
    }
    
    if (degrees.z != 0.0) {
        axis = it->objectPtr->bone.rotationZAxis;
        angle = degrees.z;
        lambdaFunc(it->objectPtr);
        it->objectPtr->bone.rotationXAxis = rotateVectorAroundAxis(it->objectPtr->bone.rotationXAxis, it->objectPtr->bone.rotationZAxis, angle);
        it->objectPtr->bone.rotationYAxis = rotateVectorAroundAxis(it->objectPtr->bone.rotationYAxis, it->objectPtr->bone.rotationZAxis, angle);
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, rootPtr->shader.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, rootPtr->shader.vertices.size() * sizeof(float), &rootPtr->shader.vertices[0]);
    glBufferSubData(GL_ARRAY_BUFFER, rootPtr->shader.vertices.size() * sizeof(float), rootPtr->shader.normals.size() * sizeof(float), &rootPtr->shader.normals[0]);
    
    it->objectPtr->bone.rotationDegrees += degrees;
}

void locateJoint(string joint, glm::vec3 offset)
{
    vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [joint] (Object obj) { return obj.name == joint; });
    
    size_t vertLeng = it->objectPtr->shader.vertices.size();
    glm::vec3 jo = glm::vec3(it->objectPtr->shader.vertices[vertLeng - 3],
                            it->objectPtr->shader.vertices[vertLeng - 2],
                            it->objectPtr->shader.vertices[vertLeng - 1]);
    
    glm::vec3 adjustedOffset = it->objectPtr->bone.referenceXAxis * offset.x + it->objectPtr->bone.referenceYAxis * offset.y + it->objectPtr->bone.referenceZAxis * offset.z;
    
    Object* rootPtr = it->objectPtr;
    while (rootPtr->type != ObjectType::Model)
        rootPtr = rootPtr->superObject;
    
    function<void(Object*)> lambdaFunc = [rootPtr, jo, adjustedOffset, &lambdaFunc](Object* obj) -> void {
        size_t vertLeng = obj->objectPtr->shader.vertices.size();
        glm::vec3 end = glm::vec3(obj->objectPtr->shader.vertices[vertLeng - 3],
                                obj->objectPtr->shader.vertices[vertLeng - 2],
                                obj->objectPtr->shader.vertices[vertLeng - 1]);
        if (obj->objectPtr->type == ObjectType::Joint) {
            if (jo != end) {
                glm::vec3 beg = glm::vec3(obj->objectPtr->shader.vertices[0],
                                          obj->objectPtr->shader.vertices[1],
                                          obj->objectPtr->shader.vertices[2]);
                beg += adjustedOffset;
                obj->objectPtr->shader.vertices[0] = beg.x;
                obj->objectPtr->shader.vertices[1] = beg.y;
                obj->objectPtr->shader.vertices[2] = beg.z;
            }
            end += adjustedOffset;
            obj->objectPtr->shader.vertices[3] = end.x;
            obj->objectPtr->shader.vertices[4] = end.y;
            obj->objectPtr->shader.vertices[5] = end.z;
            glBindBuffer(GL_ARRAY_BUFFER, obj->objectPtr->shader.vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, obj->objectPtr->shader.vertices.size() * sizeof(float), &obj->objectPtr->shader.vertices[0]);
        }
        else {
            for (int i = 0; i < obj->objectPtr->shader.vertices.size(); i += 3) {
                glm::vec3 vert = glm::vec3(obj->objectPtr->shader.vertices[i],
                                          obj->objectPtr->shader.vertices[i + 1],
                                          obj->objectPtr->shader.vertices[i + 2]);
                vert += adjustedOffset;
                obj->objectPtr->shader.vertices[i] = vert.x;
                obj->objectPtr->shader.vertices[i + 1] = vert.y;
                obj->objectPtr->shader.vertices[i + 2] = vert.z;
            }
            glBindBuffer(GL_ARRAY_BUFFER, obj->objectPtr->shader.vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, obj->objectPtr->shader.vertices.size() * sizeof(float), &obj->objectPtr->shader.vertices[0]);
        }
        
        
        for (int i = 0; i < obj->objectPtr->bone.indices.size(); i++) {
            unsigned int indice = obj->objectPtr->bone.indices[i];
            glm::vec3 pnt = glm::vec3(rootPtr->shader.vertices[indice * 3],
                                      rootPtr->shader.vertices[indice * 3 + 1],
                                      rootPtr->shader.vertices[indice * 3 + 2]);
            pnt += adjustedOffset * obj->objectPtr->bone.weights[i];
            rootPtr->shader.vertices[indice * 3] = pnt.x;
            rootPtr->shader.vertices[indice * 3 + 1] = pnt.y;
            rootPtr->shader.vertices[indice * 3 + 2] = pnt.z;
        }
        
        for (Object* ptr : obj->objectPtr->subObjects)
            lambdaFunc(ptr);
    };
    lambdaFunc(it->objectPtr);
    
    glBindBuffer(GL_ARRAY_BUFFER, rootPtr->shader.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, rootPtr->shader.vertices.size() * sizeof(float), &rootPtr->shader.vertices[0]);
    
    it->objectPtr->bone.locationOffset += offset;
}

void setPose()
{
    rotateJoint("hips", glm::vec3(1.87, -45.2, -8.11));
    locateJoint("hips", glm::vec3(0.0963, -0.05, 0.00774));
    rotateJoint("spine", glm::vec3(3.65, -5.08, 0.181));
    rotateJoint("spine1", glm::vec3(7.33, -10.3, -0.07));
    rotateJoint("spine2", glm::vec3(7.33, -10.3, -0.07));
    rotateJoint("neck", glm::vec3(-7.03, -4.75, -3.26));
    rotateJoint("head", glm::vec3(-0.305, 7.06, 0.318));
    rotateJoint("leftshoulder", glm::vec3(-0.226, 11.7, -0.399));
    rotateJoint("leftarm", glm::vec3(-21.4, -39.4, 7.1));
    rotateJoint("leftforearm", glm::vec3(-46.4, -35.8, -19.1));
    rotateJoint("lefthand", glm::vec3(0.695, -27.9, 26.5));
    rotateJoint("lefthandthumb1", glm::vec3(-25.9, 30.4, 7.61));
    rotateJoint("lefthandthumb2", glm::vec3(1.62, 25.3, -3.55));
    rotateJoint("lefthandindex1", glm::vec3(-8.66, 2.1, -16.3));
    rotateJoint("lefthandindex2", glm::vec3(5.77, -1.78, 11.6));
    rotateJoint("lefthandmiddle1", glm::vec3(-24.2, -3.83, -2.44));
    rotateJoint("lefthandmiddle2", glm::vec3(5.63, -2.51, 11.7));
    rotateJoint("lefthandring1", glm::vec3(8.57, -2.35, 18.0));
    rotateJoint("lefthandring2", glm::vec3(18.4, -6.67, 20.1));
    rotateJoint("rightshoulder", glm::vec3(0.57, 9.85, 0.155));
    rotateJoint("rightarm", glm::vec3(1.48, 31.0, 2.19));
    rotateJoint("rightforearm", glm::vec3(-44.4, 33.2, 17.5));
    rotateJoint("righthand", glm::vec3(-0.027, 20.5, -23.9));
    rotateJoint("righthandthumb1", glm::vec3(-22.8, 20.3, -6.63));
    rotateJoint("righthandthumb2", glm::vec3(-2.99, -2.33, 3.38));
    rotateJoint("righthandindex1", glm::vec3(17.4, -6.99, 23.8));
    rotateJoint("righthandindex2", glm::vec3(-9.22, -0.589, 5.0));
    rotateJoint("righthandmiddle1", glm::vec3(-19.1, -2.69, 28.4));
    rotateJoint("righthandmiddle2", glm::vec3(6.66, 2.99, -14.8));
    rotateJoint("righthandring1", glm::vec3(-22.2, -12.3, -18.2));
    rotateJoint("righthandring2", glm::vec3(19.5, 7.82, -21.3));
    rotateJoint("leftupleg", glm::vec3(0.231, 33.8, -21.0));
    rotateJoint("leftleg", glm::vec3(5.31, -0.467, 51.3));
    rotateJoint("leftfoot", glm::vec3(33.3, 12.0, 7.71));
    rotateJoint("lefttoebase", glm::vec3(0.031, -0.026, -0.026));
    rotateJoint("rightupleg", glm::vec3(27.1, 23.0, 19.5));
    rotateJoint("rightleg", glm::vec3(0.168, -1.19, -41.4));
    rotateJoint("rightfoot", glm::vec3(13.8, 4.71, -9.73));
    rotateJoint("righttoebase", glm::vec3(0.004, -0.11, -0.262));
}

void resetPose(string joint)
{
    vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [joint] (Object obj) { return obj.name == joint; });
    
    function<void(Object*)> lambdaFunc = [&lambdaFunc](Object* obj) -> void {
        rotateJoint(obj->objectPtr->name, -1.0f * obj->objectPtr->bone.rotationDegrees);
        locateJoint(obj->objectPtr->name, -1.0f * obj->objectPtr->bone.locationOffset);
        for (Object* ptr : obj->objectPtr->subObjects)
            lambdaFunc(ptr);
    };
    lambdaFunc(it->objectPtr);
}

void calculateTangentsBitangents(Object* objPtr)
{
    map<int, glm::vec3> tangentMap;
    map<int, glm::vec3> bitangentMap;
    for (int i = 0; i < objPtr->shader.faces.size() / 3; i++) {
        glm::vec3 tan, bitan;
        
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

        bitan.x = c * (-deltat2.x * e1.x + deltat1.x * e2.x);
        bitan.y = c * (-deltat2.x * e1.y + deltat1.x * e2.y);
        bitan.z = c * (-deltat2.x * e1.z + deltat1.x * e2.z);
        
        tangentMap.insert(pair<int, glm::vec3>(objPtr->shader.faces[i * 3], tan));
        tangentMap.insert(pair<int, glm::vec3>(objPtr->shader.faces[i * 3 + 1], tan));
        tangentMap.insert(pair<int, glm::vec3>(objPtr->shader.faces[i * 3 + 2], tan));
        bitangentMap.insert(pair<int, glm::vec3>(objPtr->shader.faces[i * 3], bitan));
        bitangentMap.insert(pair<int, glm::vec3>(objPtr->shader.faces[i * 3 + 1], bitan));
        bitangentMap.insert(pair<int, glm::vec3>(objPtr->shader.faces[i * 3 + 2], bitan));
    }
    for (map<int, glm::vec3>::iterator it = tangentMap.begin(); it != tangentMap.end(); ++it) {
        objPtr->shader.tangents.push_back(it->second.x);
        objPtr->shader.tangents.push_back(it->second.y);
        objPtr->shader.tangents.push_back(it->second.z);
    }
    for (map<int, glm::vec3>::iterator it = bitangentMap.begin(); it != bitangentMap.end(); ++it) {
        objPtr->shader.bitangents.push_back(it->second.x);
        objPtr->shader.bitangents.push_back(it->second.y);
        objPtr->shader.bitangents.push_back(it->second.z);
    }
}



