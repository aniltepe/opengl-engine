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
using namespace std;
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

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
index = 0;

void createScene(string path);
vector<unsigned char> base64_decode(string const& encoded_string);

int main() {
    createScene("/Users/nazimaniltepe/Documents/Projects/opengl-nscene/OpenGLTest4/scene2.nsce");
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

processObject(vector<string> rows) {
    map<string, string> dictionary;
    for (int i = 0; i < rows.size(); i++) {
        if (!regex_match(rows[i], regex("[\\s]*"))) {
            if (
            if (rows[i].find(":") != string::npos) {
                string pairKey = rows[j].substr(0, rows[j].find(":"));
                pairKey.erase(pairKey.begin(), find_if(pairKey.begin(), pairKey.end(), [](unsigned char c) {
                    return !isspace(c);
                }));
                string pairValue = rows[j].substr(rows[j].find(":") + 1);
                pairValue.erase(pairValue.begin(), find_if(pairValue.begin(), pairValue.end(), [](unsigned char c) {
                    return !isspace(c);
                }));
                dictionary.insert(pair<string, string>(pairKey, pairValue));
            }
        }
        
        if (rows[i].find("/") == string::npos && rows[i].find(":") == string::npos && !regex_match(rows[i], regex("[\\s]*"))) {
            int j = i + 1;
            while (rows[j].rfind("/" + rows[i], 0) != 0) {
                if (regex_match(rows[j], regex("[\\s]*"))) {
                    j++;
                    continue;
                }
                if (rows[j].find(":") != string::npos) {
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
                else {
                    
                }
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
