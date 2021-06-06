//
//  test3.cpp
//  OpenGLTest4
//
//  Created by Nazım Anıl Tepe on 12.04.2021.
//

#include <iostream>
#include <cmath>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <map>
#include <time.h>
#include <typeinfo>
using namespace std;
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

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
struct Layout {
    float x;
    float y;
    int width;
    int height;
};
struct Shader {
    vector<float> vertices {};
    vector<unsigned int> faces {};
    int vertexCount;
    unsigned int vao;
    unsigned int vbo;
    unsigned int ebo;
    int shader;
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
    vector<Object> subObjects {};
    Shader shader;
    Light light;
    Camera camera;
    Material material;
    Transform transform;
    Layout layout;
    string additionalInfo;
};

int objIndex = 0;
Object sceneObject;

void createScene(string path);
Object processObject(vector<string> rows, string name);
Object processObjectTypes(Object object);
vector<unsigned char> base64_decode(string const& encoded_string);

template <class T>
vector<T> processAttributeArray(string s) {
    vector<T> values {};
    long space = 0;
    string delimiter = " ";
    while ((space = s.find(delimiter)) != string::npos) {
        string next = s.substr(0, space);
        if (next != "")
            values.push_back(typeid(T).name() == "unsigned int" ? stoi(next) : stof(next));
        s.erase(0, space + delimiter.length());
    }
    if (s != "")
        values.push_back(typeid(T).name() == "unsigned int" ? stoi(s) : stof(s));
    return values;
}

int main() {
    createScene("/Users/nazimaniltepe/Documents/Projects/opengl-nscene/OpenGLTest4/scene2.sce");
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
    
    sceneObject = processObject(rows, "scene");
    sceneObject = processObjectTypes(sceneObject);
}

Object processObject(vector<string> rows, string name)
{
    Object object;
    object.index = objIndex++;
    object.name = name;
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
            object.dictionary.insert(pair<string, string>(pairKey, pairValue));
        }
        else {
            vector<string>::const_iterator itr;
            itr = find(rows.begin(), rows.end(), "/" + rows[i]);
            if (itr != rows.end()) {
                vector<string>::const_iterator first = rows.begin() + i + 1;
                vector<string>::const_iterator last = rows.begin() + (itr - rows.begin()) - 1;
                vector<string> newRows(first, last);
                Object subObject = processObject(newRows, rows[i]);
                object.subObjects.push_back(subObject);
                i = itr - rows.begin();
            }
        }
    }
    return object;
}

Object processObjectTypes(Object object)
{
    for (const auto &entry : object.dictionary) {
        if (entry.first == "type")
            object.type = static_cast<ObjectType>(stoi(entry.second));
        else if (entry.first == "ltyp")
            object.light.lightType = static_cast<LightType>(stoi(entry.second));
        else if (entry.first == "wd")
            object.layout.width = stoi(object.dictionary.at("wd"));
        else if (entry.first == "hg")
            object.layout.height = stoi(object.dictionary.at("hg"));
        else if (entry.first == "cnst")
            object.light.constant = stof(object.dictionary.at("cnst"));
        else if (entry.first == "lnr")
            object.light.linear = stof(object.dictionary.at("lnr"));
        else if (entry.first == "quad")
            object.light.quadratic = stof(object.dictionary.at("quad"));
        else if (entry.first == "cut")
            object.light.cutOff = stof(object.dictionary.at("cut"));
        else if (entry.first == "ocut")
            object.light.outerCutOff = stof(object.dictionary.at("ocut"));
        else if (entry.first == "fov")
            object.camera.fov = stof(object.dictionary.at("fov"));
        else if (entry.first == "mind")
            object.camera.minDistance = stof(object.dictionary.at("mind"));
        else if (entry.first == "maxd")
            object.camera.maxDistance = stof(object.dictionary.at("maxd"));
        else if (entry.first == "mvsp")
            object.camera.moveSpeed = stof(object.dictionary.at("mvsp"));
        else if (entry.first == "mtdf")
            object.material.diffuseTexBase64 = object.dictionary.at("mtdf");
        else if (entry.first == "mtsp")
            object.material.specularTexBase64 = object.dictionary.at("mtsp");
        else if (entry.first == "v")
            object.shader.vertices = processAttributeArray<float>(object.dictionary.at("v"));
        else if (entry.first == "f")
            object.shader.faces = processAttributeArray<unsigned int>(object.dictionary.at("f"));
        else if (entry.first == "mtrl") {
            vector<float> sequence = processAttributeArray<float>(object.dictionary.at("mtrl"));
            istringstream is_bool(sequence[0]);
            is_bool >> boolalpha >> object.material.texture;
            object.material.texture = sequence[0] == 0.0 ? false : true;
            object.material.ambient = glm::vec3(sequence[1], sequence[2], sequence[3]);
            object.material.diffuse = glm::vec3(sequence[4], sequence[5], sequence[6]);
            object.material.specular = glm::vec3(sequence[7], sequence[8], sequence[9]);
            object.material.shininess = sequence[10];
        }
        else if (entry.first == "trns") {
            vector<float> sequence = processAttributeArray<float>(object.dictionary.at("trns"));
            object.transform.position = glm::vec3(sequence[0], sequence[1], sequence[2]);
            object.transform.scale = glm::vec3(sequence[3], sequence[4], sequence[5]);
            object.transform.front = glm::vec3(sequence[6], sequence[7], sequence[8]);
            object.transform.up = glm::vec3(sequence[9], sequence[10], sequence[11]);
            object.transform.right = glm::vec3(sequence[12], sequence[13], sequence[14]);
        }
    }
    for (int i = 0; i < object.subObjects.size(); i++) {
        object.subObjects[i] = processObjectTypes(object.subObjects[i]);
    }
    return object;
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
