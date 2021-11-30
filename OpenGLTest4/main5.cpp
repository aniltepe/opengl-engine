//
//  main5.cpp
//  OpenGLTest4
//
//  Created by Nazım Anıl Tepe on 9.06.2021.
//
// test of skeletial transform

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
struct Character {
    unsigned int TextureID;
    glm::ivec2   Size;
    glm::ivec2   Bearing;
    unsigned int Advance;
};

int objIndex = 0;
vector<Object> objects;
Object* cameraPtr;
vector<Object*> cameraPtrs;
map<GLchar, Character> characters;

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

bool isLight(Object obj) { return obj.type == ObjectType::Light; }

unsigned int polygonMode = GL_FILL;

glm::mat4 projection;
glm::mat4 view;

float lastFrame = 0.0f;
int frameCount = 0;
int fps = 0;

int main()
{
    Object* scene = createScene("/Users/nazimaniltepe/Documents/Projects/opengl-nscene/OpenGLTest4/scene2.sce");
    
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
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
    
    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }
    
    setShaders(scene);
    setBuffers(scene);
    cameraPtr = cameraPtrs[0];
    
    function<void(Object*)> adjustTransform = [&adjustTransform](Object* obj) {
        if ((obj->type == ObjectType::Model || obj->type == ObjectType::Light || obj->type == ObjectType::Joint) &&
            obj->dictionary.find("trns") == obj->dictionary.end()) {
            obj->dictionary.insert(pair<string, string>("trns", obj->superObject->dictionary.at("trns")));
            obj->transform = obj->superObject->transform;
            if (obj->type == ObjectType::Joint && obj->superObject->type == ObjectType::Joint) {
                obj->transform.position = glm::vec3(obj->shader.vertices[0],
                                                    obj->shader.vertices[1],
                                                    obj->shader.vertices[2]);
                obj->transform.front = glm::vec3(0.0, 0.0, 1.0);
                obj->transform.up = glm::vec3(0.0, 1.0, 0.0);
                obj->transform.right = glm::vec3(-1.0, 0.0, 0.0);
//                obj->shader.vertices[0] -= obj->shader.vertices[3];
//                obj->shader.vertices[1] -= obj->shader.vertices[4];
//                obj->shader.vertices[2] -= obj->shader.vertices[5];
//                obj->shader.vertices[3] = 0.0f;
//                obj->shader.vertices[4] = 0.0f;
//                obj->shader.vertices[5] = 0.0f;
//                obj->shader.vertices[3] = obj->shader.vertices[0] - obj->shader.vertices[3];
//                obj->shader.vertices[4] = obj->shader.vertices[1] - obj->shader.vertices[4];
//                obj->shader.vertices[5] = obj->shader.vertices[2] - obj->shader.vertices[5];
                obj->shader.vertices[3] -= obj->shader.vertices[0];
                obj->shader.vertices[4] -= obj->shader.vertices[1];
                obj->shader.vertices[5] -= obj->shader.vertices[2];
                obj->shader.vertices[0] = 0.0f;
                obj->shader.vertices[1] = 0.0f;
                obj->shader.vertices[2] = 0.0f;
                glBindBuffer(GL_ARRAY_BUFFER, obj->shader.vbo);
                glBufferData(GL_ARRAY_BUFFER, obj->shader.vertices.size() * sizeof(float), &obj->shader.vertices[0], GL_DYNAMIC_DRAW);
            }
        }
        for (int i = 0; i < obj->subObjects.size(); i++)
            adjustTransform(obj->subObjects[i]);
    };
    adjustTransform(scene);
    
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

        projection = glm::perspective(glm::radians(cameraPtr->camera.fov), (float)scene->layout.width / (float)scene->layout.height, cameraPtr->camera.minDistance, cameraPtr->camera.maxDistance);
        view = lookAt(cameraPtr->transform.position, cameraPtr->transform.position + cameraPtr->transform.front, cameraPtr->transform.up);
        
        drawScene(scene);
        
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
                Object* subObjPtr = createObject(newRows, rows[i]);
                subObjPtr->superObject = objPtr;
                objPtr->subObjects.push_back(subObjPtr);
                i = int(itr - rows.begin());
            }
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
    if (objPtr->type == ObjectType::Camera)
        cameraPtrs.push_back(objPtr);
}

void setShaders(Object* objPtr)
{
    if (objPtr->type != ObjectType::Model &&
        objPtr->type != ObjectType::Light &&
        objPtr->type != ObjectType::Joint) {
//        cout << "object " + objPtr->name + " is not drawable, passing shader phase" << endl;
    }
    else if (objPtr->shader.vertices.size() == 0) {
//        cout << "object " + objPtr->name + " has no vertices, passing shader phase" << endl;
    }
    else {
//        cout << "object " + objPtr->name + " is drawable, processing shader phase" << endl;
        
        objPtr->shader.vertexShader = "#version 330 core\nlayout(location = 0) in vec3 vPos;\n";
        objPtr->shader.fragmentShader = "#version 330 core\nout vec4 FragColor;\n";
        
        if (objPtr->type == ObjectType::Model) {
            objPtr->shader.vertexShader += "layout(location = 1) in vec3 vNormal;\n";
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "layout(location = 2) in vec2 vTexCoord;\n" : "";
            objPtr->shader.vertexShader += "out vec3 FragPos;\n";
            objPtr->shader.vertexShader += "out vec3 Normal;\n";
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "out vec2 TexCoord;\n" : "";
        }
        
        objPtr->shader.vertexShader += "uniform mat4 model;\n";
        objPtr->shader.vertexShader += "uniform mat4 view;\n";
        objPtr->shader.vertexShader += "uniform mat4 projection;\n";
        objPtr->shader.vertexShader += "void main() {\n";
        objPtr->shader.vertexShader += "gl_Position = projection * view * model * vec4(vPos, 1.0f);\n";
        
        if (objPtr->type == ObjectType::Model) {
            objPtr->shader.vertexShader += "FragPos = vec3(model * vec4(vPos, 1.0f));\n";
            objPtr->shader.vertexShader += "Normal = vNormal;\n";
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "TexCoord = vTexCoord;\n" : "";
        }
            
        objPtr->shader.vertexShader += "}\0";

        if (objPtr->type == ObjectType::Model) {
            objPtr->shader.fragmentShader += "in vec3 FragPos;\n";
            objPtr->shader.fragmentShader += "in vec3 Normal;\n";
            objPtr->shader.fragmentShader += (objPtr->material.texture) ? "in vec2 TexCoord;\n" : "";
            objPtr->shader.fragmentShader += "struct Material {\n";
            objPtr->shader.fragmentShader += "vec3 ambient;\n";
            objPtr->shader.fragmentShader += "vec3 diffuse;\n";
            objPtr->shader.fragmentShader += "vec3 specular;\n";
            objPtr->shader.fragmentShader += "bool texture;\n";
            objPtr->shader.fragmentShader += "sampler2D diffuseTex;\n";
            objPtr->shader.fragmentShader += "sampler2D specularTex;\n";
            objPtr->shader.fragmentShader += "float shininess;\n";
            objPtr->shader.fragmentShader += "};\n";
            objPtr->shader.fragmentShader += "struct Light {\n";
            objPtr->shader.fragmentShader += "int lightType;\n";
            objPtr->shader.fragmentShader += "vec3 direction;\n";
            objPtr->shader.fragmentShader += "vec3 position;\n";
            objPtr->shader.fragmentShader += "float constant;\n";
            objPtr->shader.fragmentShader += "float linear;\n";
            objPtr->shader.fragmentShader += "float quadratic;\n";
            objPtr->shader.fragmentShader += "float cutOff;\n";
            objPtr->shader.fragmentShader += "float outerCutOff;\n";
            objPtr->shader.fragmentShader += "Material material;\n";
            objPtr->shader.fragmentShader += "};\n";
            objPtr->shader.fragmentShader += "uniform vec3 cameraPos;\n";
            objPtr->shader.fragmentShader += "uniform Material modelMaterial;\n";
            objPtr->shader.fragmentShader += "uniform Light lights[" + to_string(count_if(objects.begin(), objects.end(), isLight)) + "];\n";
            objPtr->shader.fragmentShader += "vec3 CalculateLight(Light light, vec3 normal, vec3 viewDir, vec3 fragPos);\n";
            objPtr->shader.fragmentShader += "void main() {\n";
            objPtr->shader.fragmentShader += "vec3 norm = normalize(Normal);\n";
            objPtr->shader.fragmentShader += "vec3 viewDir = normalize(cameraPos - FragPos);\n";
            objPtr->shader.fragmentShader += "vec3 result = vec3(0.0f);\n";
            objPtr->shader.fragmentShader += "for(int i = 0; i < lights.length(); i++)\n";
            objPtr->shader.fragmentShader += "result += CalculateLight(lights[i], norm, viewDir, FragPos);\n";
            objPtr->shader.fragmentShader += "FragColor = vec4(result, 1.0f);\n";
            objPtr->shader.fragmentShader += "}\n";
            objPtr->shader.fragmentShader += "vec3 CalculateLight(Light light, vec3 normal, vec3 viewDir, vec3 fragPos) {\n";
            objPtr->shader.fragmentShader += "vec3 lightDir = normalize(light.position - fragPos);\n";
            objPtr->shader.fragmentShader += "if (light.lightType == 1)\n";
            objPtr->shader.fragmentShader += "lightDir = normalize(-light.direction);\n";
            objPtr->shader.fragmentShader += "float diffStrength = max(dot(normal, lightDir), 0.0);\n";
            objPtr->shader.fragmentShader += "vec3 reflectDir = reflect(-lightDir, normal);\n";
            objPtr->shader.fragmentShader += "float specStrength = pow(max(dot(viewDir, reflectDir), 0.0), modelMaterial.shininess);\n";
            objPtr->shader.fragmentShader += "vec3 ambient = light.material.ambient * modelMaterial.ambient;\n";
            objPtr->shader.fragmentShader += "vec3 diffuse = light.material.diffuse * diffStrength * modelMaterial.diffuse;\n";
            objPtr->shader.fragmentShader += "vec3 specular = light.material.specular * specStrength * modelMaterial.specular;\n";
            objPtr->shader.fragmentShader += (objPtr->material.texture) ? "ambient = light.material.ambient * vec3(texture(modelMaterial.diffuseTex, TexCoord)) * modelMaterial.diffuse;\n" : "";
            objPtr->shader.fragmentShader += (objPtr->material.texture) ? "diffuse = light.material.diffuse * diffStrength * vec3(texture(modelMaterial.diffuseTex, TexCoord)) * modelMaterial.diffuse;\n" : "";
            objPtr->shader.fragmentShader += (objPtr->material.texture) ? ((objPtr->material.specularTexBase64 != "") ? "specular = light.material.specular * specStrength * vec3(texture(modelMaterial.specularTex, TexCoord));\n" : "") : "";
            objPtr->shader.fragmentShader += "if (light.lightType != 1) {\n";
            objPtr->shader.fragmentShader += "float distance = length(light.position - fragPos);\n";
            objPtr->shader.fragmentShader += "float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));\n";
            objPtr->shader.fragmentShader += "ambient *= attenuation;\n";
            objPtr->shader.fragmentShader += "diffuse *= attenuation;\n";
            objPtr->shader.fragmentShader += "specular *= attenuation;\n";
            objPtr->shader.fragmentShader += "if (light.lightType == 2) {\n";
            objPtr->shader.fragmentShader += "float theta = dot(lightDir, normalize(-light.direction));\n";
            objPtr->shader.fragmentShader += "float epsilon = light.cutOff - light.outerCutOff;\n";
            objPtr->shader.fragmentShader += "float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);\n";
            objPtr->shader.fragmentShader += "ambient *= intensity;\n";
            objPtr->shader.fragmentShader += "diffuse *= intensity;\n";
            objPtr->shader.fragmentShader += "specular *= intensity;\n";
            objPtr->shader.fragmentShader += "}\n";
            objPtr->shader.fragmentShader += "}\n";
            objPtr->shader.fragmentShader += "return (ambient + diffuse + specular);\n";
        }
        else if (objPtr->type == ObjectType::Light) {
            objPtr->shader.fragmentShader += "uniform vec3 color;\n";
            objPtr->shader.fragmentShader += "void main() {\n";
            objPtr->shader.fragmentShader += "FragColor = vec4(color, 1.0f);\n";
        }
        else if (objPtr->type == ObjectType::Joint) {
            objPtr->shader.fragmentShader += "void main() {\n";
            objPtr->shader.fragmentShader += "FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);\n";
        }
            
        objPtr->shader.fragmentShader += "}\0";
        
        int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        const char *vertexShaderSource = objPtr->shader.vertexShader.c_str();
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        const char *fragmentShaderSource = objPtr->shader.fragmentShader.c_str();
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        objPtr->shader.shaderID = glCreateProgram();
        glAttachShader(objPtr->shader.shaderID, vertexShader);
        glAttachShader(objPtr->shader.shaderID, fragmentShader);
        glLinkProgram(objPtr->shader.shaderID);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }
    
    for (int i = 0; i < objPtr->subObjects.size(); i++)
        setShaders(objPtr->subObjects[i]);
}

void setBuffers(Object* objPtr)
{
    if (objPtr->type != ObjectType::Model &&
        objPtr->type != ObjectType::Light &&
        objPtr->type != ObjectType::Joint) {
//        cout << "object " + objPtr->name + " is not drawable, passing buffer phase" << endl;
    }
    else if (objPtr->shader.vertices.size() == 0) {
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
        if (objPtr->type == ObjectType::Joint && objPtr->superObject->type == ObjectType::Joint) {
            objPtr->shader.vertices.insert(objPtr->shader.vertices.begin(),
                                           objPtr->superObject->shader.vertices.end() - 3,
                                           objPtr->superObject->shader.vertices.end());
//            vector<float> newVertices;
//            objPtr->shader.vertices.insert(newVertices.begin(),
//                                           objPtr->superObject->shader.vertices.end() - 3,
//                                           objPtr->superObject->shader.vertices.end());
//            newVertices.push_back(objPtr->superObject->shader.vertices[0] - objPtr->shader.vertices[0]);
//            newVertices.push_back(objPtr->superObject->shader.vertices[1] - objPtr->shader.vertices[1]);
//            newVertices.push_back(objPtr->superObject->shader.vertices[2] - objPtr->shader.vertices[2]);
//            newVertices.push_back(0.0);
//            newVertices.push_back(0.0);
//            newVertices.push_back(0.0);
//            objPtr->transform.position = glm::vec3(objPtr->shader.vertices[0],
//                                                   objPtr->shader.vertices[1],
//                                                   objPtr->shader.vertices[2]);
//            objPtr->shader.vertices = newVertices;
        }
        glBufferData(GL_ARRAY_BUFFER, objPtr->shader.vertices.size() * sizeof(float), &objPtr->shader.vertices[0], GL_DYNAMIC_DRAW);
        if (objPtr->shader.faces.size() > 0) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objPtr->shader.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, objPtr->shader.faces.size() * sizeof(float), &objPtr->shader.faces[0], GL_DYNAMIC_DRAW);
        }
        if (objPtr->type == ObjectType::Model) {
            int attCount = objPtr->material.texture ? 8 : 6;
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, attCount * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, attCount * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            if (objPtr->material.texture) {
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, attCount * sizeof(float), (void*)(6 * sizeof(float)));
                glEnableVertexAttribArray(2);
            }
            glBindVertexArray(0);
            if (objPtr->material.texture) {
                int width, height, nrChannels;
                vector<unsigned char> decoded = base64_decode(objPtr->material.diffuseTexBase64);
                unsigned char *data = stbi_load_from_memory(&decoded[0], decoded.size(), &width, &height, &nrChannels, 0);
                glGenTextures(1, &objPtr->material.diffuseTex);
                glBindTexture(GL_TEXTURE_2D, objPtr->material.diffuseTex);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
                glGenerateMipmap(GL_TEXTURE_2D);
                stbi_image_free(data);

                if (objPtr->material.specularTexBase64 != "") {
                    vector<unsigned char> decoded_ = base64_decode(objPtr->material.specularTexBase64);
                    unsigned char *data_ = stbi_load_from_memory(&decoded_[0], decoded_.size(), &width, &height, &nrChannels, 0);
                    glGenTextures(1, &objPtr->material.specularTex);
                    glBindTexture(GL_TEXTURE_2D, objPtr->material.specularTex);
                    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_);
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
                    glGenerateMipmap(GL_TEXTURE_2D);
                    stbi_image_free(data_);
                }

                glUseProgram(objPtr->shader.shaderID);
                glUniform1i(glGetUniformLocation(objPtr->shader.shaderID, "modelMaterial.diffuseTex"), 0);
                if (objPtr->material.specularTexBase64 != "")
                    glUniform1i(glGetUniformLocation(objPtr->shader.shaderID, "modelMaterial.specularTex"), 1);
            }
        }
        else if (objPtr->type == ObjectType::Light || objPtr->type == ObjectType::Joint) {
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glBindVertexArray(0);
        }
        if (objPtr->type == ObjectType::Joint)
            objPtr->shader.vertexCount = objPtr->shader.vertices.size() / 3;
        else if (objPtr->shader.faces.size() > 0)
            objPtr->shader.vertexCount = objPtr->shader.faces.size();
        else if (objPtr->type == ObjectType::Model)
            objPtr->shader.vertexCount = objPtr->material.texture ? objPtr->shader.vertices.size() / 8 : objPtr->shader.vertices.size() / 6;
        else
            objPtr->shader.vertexCount = objPtr->shader.vertices.size() / 3;
    }
    
    for (int i = 0; i < objPtr->subObjects.size(); i++)
        setBuffers(objPtr->subObjects[i]);
}

void drawScene(Object* objPtr)
{
    if (objPtr->type != ObjectType::Model &&
        objPtr->type != ObjectType::Light &&
        objPtr->type != ObjectType::Joint) {
//        cout << "object " + objPtr->name + " is not drawable, passing draw phase" << endl;
    }
    else if (objPtr->shader.vertices.size() == 0) {
//        cout << "object " + objPtr->name + " is not drawable, passing draw phase" << endl;
    }
    else {
//        cout << "object " + objPtr->name + " is drawable, processing draw phase" << endl;
        
        glUseProgram(objPtr->shader.shaderID);
        glUniformMatrix4fv(glGetUniformLocation(objPtr->shader.shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(objPtr->shader.shaderID, "view"), 1, GL_FALSE, value_ptr(view));
        glm::mat4 model = glm::translate(glm::mat4(1.0f), objPtr->transform.position);
        model = glm::scale(model, objPtr->transform.scale);
        glm::mat4 rotation = glm::mat4(objPtr->transform.right.x, objPtr->transform.right.y, objPtr->transform.right.z, 0,
                          objPtr->transform.up.x, objPtr->transform.up.y, objPtr->transform.up.z, 0,
                          objPtr->transform.front.x, objPtr->transform.front.y, objPtr->transform.front.z, 0,
                          0, 0, 0, 1);
        model *= rotation;
        glUniformMatrix4fv(glGetUniformLocation(objPtr->shader.shaderID, "model"), 1, GL_FALSE,  value_ptr(model));
        
        if (objPtr->type == ObjectType::Model) {
            glUniform3fv(glGetUniformLocation(objPtr->shader.shaderID, "cameraPos"), 1, value_ptr(cameraPtr->transform.position));

            glUniform3fv(glGetUniformLocation(objPtr->shader.shaderID, "modelMaterial.ambient"), 1, value_ptr(objPtr->material.ambient));
            glUniform3fv(glGetUniformLocation(objPtr->shader.shaderID, "modelMaterial.diffuse"), 1, value_ptr(objPtr->material.diffuse));
            glUniform3fv(glGetUniformLocation(objPtr->shader.shaderID, "modelMaterial.specular"), 1, value_ptr(objPtr->material.specular));
            glUniform1i(glGetUniformLocation(objPtr->shader.shaderID, "modelMaterial.texture"), (int)objPtr->material.texture);
            glUniform1f(glGetUniformLocation(objPtr->shader.shaderID, "modelMaterial.shininess"), objPtr->material.shininess);

            if (objPtr->material.texture) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, objPtr->material.diffuseTex);
                if (objPtr->material.specularTexBase64 != "") {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, objPtr->material.specularTex);
                }
            }
            vector<Object>::iterator it = objects.begin();
            int index = 0;
            while ((it = find_if(it, objects.end(), isLight)) != objects.end()) {
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
        }
        else if (objPtr->type == ObjectType::Light) {
            glUniform3fv(glGetUniformLocation(objPtr->shader.shaderID, "color"), 1, value_ptr(objPtr->material.diffuse / 0.8f));
        }
        
        glBindVertexArray(objPtr->shader.vao);
        
        if (objPtr->type == ObjectType::Joint)
            glDrawArrays(GL_LINES, 0, objPtr->shader.vertexCount);
        else if (objPtr->shader.faces.size() > 0)
            glDrawElements(GL_TRIANGLES, objPtr->shader.vertexCount, GL_UNSIGNED_INT, 0);
        else
            glDrawArrays(GL_TRIANGLES, 0, objPtr->shader.vertexCount);
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
    
    if ((key == GLFW_KEY_1 && action == GLFW_PRESS) ||
        (key == GLFW_KEY_2 && action == GLFW_PRESS) ||
        (key == GLFW_KEY_3 && action == GLFW_PRESS) ||
        (key == GLFW_KEY_4 && action == GLFW_PRESS) ||
        (key == GLFW_KEY_5 && action == GLFW_PRESS) ||
        (key == GLFW_KEY_6 && action == GLFW_PRESS) ||
        (key == GLFW_KEY_7 && action == GLFW_PRESS) ||
        (key == GLFW_KEY_8 && action == GLFW_PRESS) ||
        (key == GLFW_KEY_9 && action == GLFW_PRESS)) {
        int camNo = stoi(glfwGetKeyName(key, 0)) - 1;
        if (camNo < cameraPtrs.size())
            cameraPtr = cameraPtrs[camNo];
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
        glm::vec3 offset = cameraPtr->transform.right * cameraPtr->camera.moveSpeed;
        cameraPtr->transform.position = cameraPtr->transform.position - offset;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        glm::vec3 offset = cameraPtr->transform.right * cameraPtr->camera.moveSpeed;
        cameraPtr->transform.position = cameraPtr->transform.position + offset;
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
        cameraPtr->transform.up = rotateVectorAroundAxis(cameraPtr->transform.up, cameraPtr->transform.right, 0.5f);
        cameraPtr->transform.front = rotateVectorAroundAxis(cameraPtr->transform.front, cameraPtr->transform.right, 0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        cameraPtr->transform.up = rotateVectorAroundAxis(cameraPtr->transform.up, cameraPtr->transform.right, -0.5f);
        cameraPtr->transform.front = rotateVectorAroundAxis(cameraPtr->transform.front, cameraPtr->transform.right, -0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        cameraPtr->transform.front = rotateVectorAroundAxis(cameraPtr->transform.front, cameraPtr->transform.up, 0.5f);
        cameraPtr->transform.right = rotateVectorAroundAxis(cameraPtr->transform.right, cameraPtr->transform.up, 0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        cameraPtr->transform.front = rotateVectorAroundAxis(cameraPtr->transform.front, cameraPtr->transform.up, -0.5f);
        cameraPtr->transform.right = rotateVectorAroundAxis(cameraPtr->transform.right, cameraPtr->transform.up, -0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        cameraPtr->transform.right = rotateVectorAroundAxis(cameraPtr->transform.right, cameraPtr->transform.front, -0.5f);
        cameraPtr->transform.up = rotateVectorAroundAxis(cameraPtr->transform.up, cameraPtr->transform.front, -0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        cameraPtr->transform.right = rotateVectorAroundAxis(cameraPtr->transform.right, cameraPtr->transform.front, 0.5f);
        cameraPtr->transform.up = rotateVectorAroundAxis(cameraPtr->transform.up, cameraPtr->transform.front, 0.5f);
    }

    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        float move = 1.0f;
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
            move *= -1.0;
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [] (Object obj) { return obj.name ==     "leftforearm"; });

        function<void(Object*)> lambdaFunc = [move, &lambdaFunc](Object* obj) -> void {
//            obj->objectPtr->transform.position.x += move;
            obj->objectPtr->transform.up = rotateVectorAroundAxis(obj->objectPtr->transform.up, obj->objectPtr->transform.front, move);
            obj->objectPtr->transform.right = rotateVectorAroundAxis(obj->objectPtr->transform.right, obj->objectPtr->transform.front, move);
            for (Object* ptr : obj->objectPtr->subObjects)
                lambdaFunc(ptr);
        };

        lambdaFunc(it->objectPtr);
    }
    
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
        float angle = 1.0f;
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
            angle *= -1.0;
        vector<Object>::iterator it = find_if(objects.begin(), objects.end(), [] (Object obj) { return obj.name ==     "leftforearm"; });
        
        glm::vec3 ax = it->objectPtr->transform.front;
        glm::vec3 jo = glm::vec3(it->objectPtr->shader.vertices[0],
                                it->objectPtr->shader.vertices[1],
                                it->objectPtr->shader.vertices[2]);
        function<void(Object*)> lambdaFunc = [it, jo, ax, angle, &lambdaFunc](Object* obj) -> void {
            glm::vec3 beg = glm::vec3(obj->objectPtr->shader.vertices[0],
                                    obj->objectPtr->shader.vertices[1],
                                    obj->objectPtr->shader.vertices[2]);
            if (jo != beg) {
                beg = rotateVectorAroundAxis(beg - jo, ax, angle);
                beg += jo;
            }
            glm::vec3 end = glm::vec3(obj->objectPtr->shader.vertices[3],
                                    obj->objectPtr->shader.vertices[4],
                                    obj->objectPtr->shader.vertices[5]);
            end = rotateVectorAroundAxis(end - jo, ax, angle);
            end += jo;
            obj->objectPtr->shader.vertices.clear();
            obj->objectPtr->shader.vertices.push_back(beg.x);
            obj->objectPtr->shader.vertices.push_back(beg.y);
            obj->objectPtr->shader.vertices.push_back(beg.z);
            obj->objectPtr->shader.vertices.push_back(end.x);
            obj->objectPtr->shader.vertices.push_back(end.y);
            obj->objectPtr->shader.vertices.push_back(end.z);
            glBindBuffer(GL_ARRAY_BUFFER, obj->objectPtr->shader.vbo);
            glBufferData(GL_ARRAY_BUFFER, obj->objectPtr->shader.vertices.size() * sizeof(float), &obj->objectPtr->shader.vertices[0], GL_DYNAMIC_DRAW);
            for (Object* ptr : obj->objectPtr->subObjects)
                lambdaFunc(ptr);
        };
        
        lambdaFunc(it->objectPtr);
    }
    
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        if (cameraPtr->camera.fov < 180.0f)
            cameraPtr->camera.fov += 0.5f;
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        if (cameraPtr->camera.fov > 0.0f)
            cameraPtr->camera.fov -= 0.5f;
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

