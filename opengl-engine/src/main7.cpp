//
//  main7.cpp
//  OpenGLTest4
//
//  Created by Nazım Anıl Tepe on 02.09.2021.
//

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
};
struct Layout {
    float x;
    float y;
    int width;
    int height;
};
struct Shader {
    vector<float> vertices;
    vector<float> normals;
    vector<float> texCoords;
    vector<float> texOrders;
    vector<unsigned int> faces;
    int vertexCount;
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
    Bone bone;
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
void rotateJoint(string joint, glm::vec3 degrees);
void locateJoint(string joint, glm::vec3 offset);
void resetPose(string joint);

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

float lastFrame = 0.0f;

int main()
{
    Object* scene = createScene("/Users/nazimaniltepe/Documents/Projects/opengl-nscene/OpenGLTest4/scene7.sce");
    
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
    
    function<void(Object*)> adjustBoneTransform = [&adjustBoneTransform](Object* obj) {
        if ((obj->type == ObjectType::Model || obj->type == ObjectType::Light || obj->type == ObjectType::Joint) &&
            obj->dictionary.find("trns") == obj->dictionary.end()) {
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
        for (int i = 0; i < obj->subObjects.size(); i++)
            adjustBoneTransform(obj->subObjects[i]);
    };
    adjustBoneTransform(scene);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(10.0);
    
    
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        float timeDelta = currentFrame - lastFrame;
        lastFrame = currentFrame;
        float fps = 1 / timeDelta;
//        cout << fps << endl;

        glClearColor(0.28f, 0.28f, 0.28f, 1.0f);
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
        else if (entry.first.rfind("tex", 0) == 0) {
            objPtr->material.texturesBase64.push_back(entry.second);
            objPtr->material.texture = true;
        }
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
        else if (entry.first == "roll")
            objPtr->bone.rollDegree = stof(entry.second);
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
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "layout(location = 3) in float vTexOrder;\n" : "";
            objPtr->shader.vertexShader += "out vec3 FragPos;\n";
            objPtr->shader.vertexShader += "out vec3 Normal;\n";
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "out vec2 TexCoord;\n" : "";
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "out float TexOrder;\n" : "";
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
            objPtr->shader.vertexShader += (objPtr->material.texture) ? "TexOrder = vTexOrder;\n" : "";
        }
            
        objPtr->shader.vertexShader += "}\0";

        if (objPtr->type == ObjectType::Model) {
            objPtr->shader.fragmentShader += "in vec3 FragPos;\n";
            objPtr->shader.fragmentShader += "in vec3 Normal;\n";
            objPtr->shader.fragmentShader += (objPtr->material.texture) ? "in vec2 TexCoord;\n" : "";
            objPtr->shader.fragmentShader += (objPtr->material.texture) ? "in float TexOrder;\n" : "";
            objPtr->shader.fragmentShader += "struct Material {\n";
            objPtr->shader.fragmentShader += "vec3 ambient;\n";
            objPtr->shader.fragmentShader += "vec3 diffuse;\n";
            objPtr->shader.fragmentShader += "vec3 specular;\n";
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
            objPtr->shader.fragmentShader += (objPtr->material.texture) ? "uniform sampler2D textures[" + to_string(objPtr->material.texturesBase64.size()) + "];\n" : "";
            objPtr->shader.fragmentShader += "uniform vec3 cameraPos;\n";
            objPtr->shader.fragmentShader += "uniform Material modelMaterial;\n";
            objPtr->shader.fragmentShader += "uniform Light lights[" + to_string(count_if(objects.begin(), objects.end(), [] (Object obj) { return obj.type == ObjectType::Light; })) + "];\n";
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
            objPtr->shader.fragmentShader += (objPtr->material.texture) ? "int order = int(TexOrder);\n" : "";
            objPtr->shader.fragmentShader += (objPtr->material.texture) ? "ambient = light.material.ambient * vec3(texture(textures[order], TexCoord)) * modelMaterial.diffuse;\n" : "";
            objPtr->shader.fragmentShader += (objPtr->material.texture) ? "diffuse = light.material.diffuse * diffStrength * vec3(texture(textures[order], TexCoord)) * modelMaterial.diffuse;\n" : "";
            objPtr->shader.fragmentShader += (objPtr->material.texture) ? "specular = light.material.specular * specStrength * vec3(texture(textures[order], TexCoord));\n" : "";
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
        int success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        const char *fragmentShaderSource = objPtr->shader.fragmentShader.c_str();
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        objPtr->shader.shaderID = glCreateProgram();
        glAttachShader(objPtr->shader.shaderID, vertexShader);
        glAttachShader(objPtr->shader.shaderID, fragmentShader);
        glLinkProgram(objPtr->shader.shaderID);
        glGetProgramiv(objPtr->shader.shaderID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(objPtr->shader.shaderID, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }
    
//    if (objPtr->name == "cubesmooth1")
//        cout << objPtr->shader.fragmentShader << endl;
    
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
        if (objPtr->type == ObjectType::Joint && objPtr->superObject->type == ObjectType::Joint)
            objPtr->shader.vertices.insert(objPtr->shader.vertices.begin(),
                                           objPtr->superObject->shader.vertices.end() - 3,
                                           objPtr->superObject->shader.vertices.end());
        
        int attCount = objPtr->material.texture ? 9 : 6;
        glBufferData(GL_ARRAY_BUFFER, objPtr->shader.vertices.size() / 3 * attCount * sizeof(float), NULL, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, objPtr->shader.vertices.size() * sizeof(float), &objPtr->shader.vertices[0]);
        glBufferSubData(GL_ARRAY_BUFFER, objPtr->shader.vertices.size() * sizeof(float), objPtr->shader.normals.size() * sizeof(float), &objPtr->shader.normals[0]);
        if (objPtr->material.texture) {
            glBufferSubData(GL_ARRAY_BUFFER, (objPtr->shader.vertices.size() + objPtr->shader.normals.size()) * sizeof(float), objPtr->shader.texCoords.size() * sizeof(float), &objPtr->shader.texCoords[0]);
            glBufferSubData(GL_ARRAY_BUFFER, (objPtr->shader.vertices.size() + objPtr->shader.normals.size() + objPtr->shader.texCoords.size()) * sizeof(float), objPtr->shader.texOrders.size() * sizeof(float), &objPtr->shader.texOrders[0]);
        }
        if (objPtr->shader.faces.size() > 0) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objPtr->shader.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, objPtr->shader.faces.size() * sizeof(float), &objPtr->shader.faces[0], GL_DYNAMIC_DRAW);
        }
        if (objPtr->type == ObjectType::Model) {
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(objPtr->shader.vertices.size() * sizeof(float)));
            glEnableVertexAttribArray(1);
            if (objPtr->material.texture) {
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)((objPtr->shader.vertices.size() + objPtr->shader.normals.size()) * sizeof(float)));
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(float), (void*)((objPtr->shader.vertices.size() + objPtr->shader.normals.size() + objPtr->shader.texCoords.size()) * sizeof(float)));
                glEnableVertexAttribArray(3);
            }
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            if (objPtr->material.texture) {
                stbi_set_flip_vertically_on_load(true);
                int width, height, nrChannels;
                for (int i = 0; i < objPtr->material.texturesBase64.size(); i++) {
                    objPtr->objectPtr->material.textures.push_back(*new unsigned int());
                    glGenTextures(1, &objPtr->material.textures[i]);
                    glBindTexture(GL_TEXTURE_2D, objPtr->material.textures[i]);
                    vector<unsigned char> decoded = base64_decode(objPtr->objectPtr->material.texturesBase64[i]);
                    unsigned char *data = stbi_load_from_memory(&decoded[0], int(decoded.size()), &width, &height, &nrChannels, 0);
                    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                    if (nrChannels == 3)
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                    else if (nrChannels == 4)
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    stbi_image_free(data);
                    
                    glUseProgram(objPtr->shader.shaderID);
                    glUniform1i(glGetUniformLocation(objPtr->shader.shaderID, ("textures[" + to_string(i) + "]").c_str()), i);
                }
            }
        }
        else if (objPtr->type == ObjectType::Light || objPtr->type == ObjectType::Joint) {
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
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
        glm::mat4 rotation = glm::mat4(objPtr->transform.left.x, objPtr->transform.left.y, objPtr->transform.left.z, 0,
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
            glUniform1f(glGetUniformLocation(objPtr->shader.shaderID, "modelMaterial.shininess"), objPtr->material.shininess);
            
            vector<Object>::iterator it = objects.begin();
            int index = 0;
            while ((it = find_if(it, objects.end(), [] (Object obj) { return obj.type == ObjectType::Light; })) != objects.end()) {
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
        else if (objPtr->shader.faces.size() > 0)
            glDrawElements(GL_TRIANGLES, objPtr->shader.vertexCount, GL_UNSIGNED_INT, 0);
        else
            glDrawArrays(GL_TRIANGLES, 0, objPtr->shader.vertexCount);
        
        glBindVertexArray(0);
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
    
    if (key == GLFW_KEY_RIGHT_BRACKET && action == GLFW_PRESS) {
        resetPose("hips");
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
        cameraPtr->transform.up = rotateVectorAroundAxis(cameraPtr->transform.up, cameraPtr->transform.left, -0.5f);
        cameraPtr->transform.front = rotateVectorAroundAxis(cameraPtr->transform.front, cameraPtr->transform.left, -0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        cameraPtr->transform.up = rotateVectorAroundAxis(cameraPtr->transform.up, cameraPtr->transform.left, 0.5f);
        cameraPtr->transform.front = rotateVectorAroundAxis(cameraPtr->transform.front, cameraPtr->transform.left, 0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        cameraPtr->transform.front = rotateVectorAroundAxis(cameraPtr->transform.front, cameraPtr->transform.up, 0.5f);
        cameraPtr->transform.left = rotateVectorAroundAxis(cameraPtr->transform.left, cameraPtr->transform.up, 0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        cameraPtr->transform.front = rotateVectorAroundAxis(cameraPtr->transform.front, cameraPtr->transform.up, -0.5f);
        cameraPtr->transform.left = rotateVectorAroundAxis(cameraPtr->transform.left, cameraPtr->transform.up, -0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS) {
        cameraPtr->transform.left = rotateVectorAroundAxis(cameraPtr->transform.left, cameraPtr->transform.front, -0.5f);
        cameraPtr->transform.up = rotateVectorAroundAxis(cameraPtr->transform.up, cameraPtr->transform.front, -0.5f);
    }
    if (glfwGetKey(window, GLFW_KEY_SLASH) == GLFW_PRESS) {
        cameraPtr->transform.left = rotateVectorAroundAxis(cameraPtr->transform.left, cameraPtr->transform.front, 0.5f);
        cameraPtr->transform.up = rotateVectorAroundAxis(cameraPtr->transform.up, cameraPtr->transform.front, 0.5f);
    }
    
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        if (cameraPtr->camera.fov < 180.0f)
            cameraPtr->camera.fov += 0.5f;
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        if (cameraPtr->camera.fov > 0.0f)
            cameraPtr->camera.fov -= 0.5f;
    }

    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
        float angle = 0.5f;
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
            angle *= -1.0;
        rotateJoint("head", glm::vec3(angle, 0.0, 0.0));
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        float angle = 0.5f;
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
            angle *= -1.0;
        rotateJoint("head", glm::vec3(0.0, angle, 0.0));
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
        float angle = 0.5f;
        if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
            angle *= -1.0;
        rotateJoint("head", glm::vec3(0.0, 0.0, angle));
    }
    
    
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
        float offset = 0.01f;
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
            offset *= -1.0;
        locateJoint("head", glm::vec3(offset, 0.0, 0.0));
    }
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
        float offset = 0.01f;
        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
            offset *= -1.0;
        locateJoint("head", glm::vec3(0.0, offset, 0.0));
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_COMMA) == GLFW_PRESS) {
        float offset = 0.01f;
        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
            offset *= -1.0;
        locateJoint("head", glm::vec3(0.0, 0.0, offset));
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

void rotateJoint(string joint, glm::vec3 degrees)
{
    degrees *= -1.0f;
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
            }
            glBindBuffer(GL_ARRAY_BUFFER, obj->objectPtr->shader.vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, obj->objectPtr->shader.vertices.size() * sizeof(float), &obj->objectPtr->shader.vertices[0]);
        }
        
        
        for (int i = 0; i < obj->objectPtr->bone.indices.size(); i++) {
            unsigned int indice = obj->objectPtr->bone.indices[i];
            glm::vec3 pnt = glm::vec3(rootPtr->shader.vertices[indice * 3],
                                      rootPtr->shader.vertices[indice * 3 + 1],
                                      rootPtr->shader.vertices[indice * 3 + 2]);
            pnt = rotateVectorAroundAxis(pnt - jo, axis, angle * obj->objectPtr->bone.weights[i]);
            pnt += jo;
            rootPtr->shader.vertices[indice * 3] = pnt.x;
            rootPtr->shader.vertices[indice * 3 + 1] = pnt.y;
            rootPtr->shader.vertices[indice * 3 + 2] = pnt.z;
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
    
    degrees *= -1.0;
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
