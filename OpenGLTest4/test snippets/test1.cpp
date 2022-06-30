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

glm::vec3 rotateVectorAroundAxis(glm::vec3 vector, glm::vec3 axis, float angle);

int main() {
    glm::vec3 res = rotateVectorAroundAxis(glm::vec3(0.0, 1.0, 0.0), glm::vec3(-1.0, 0.0, 0.0), -30);
//    cout << res.x << " " << res.y << " " << res.z << endl;
    glm::vec3 ress = rotateVectorAroundAxis(res, glm::vec3(0.0, 0.0, 1.0), -60);
    cout << ress.x << " " << ress.y << " " << ress.z << endl;
    
    glm::vec3 asd = rotateVectorAroundAxis(glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0), -60);
    glm::vec3 asdd = rotateVectorAroundAxis(glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, 0.0, 1.0), -60);
    glm::vec3 asddd = rotateVectorAroundAxis(asd, asdd, -30);
    asddd.z *= -1.0;
    cout << asddd.x << " " << asddd.y << " " << asddd.z << endl;
    
}

glm::vec3 rotateVectorAroundAxis(glm::vec3 vector, glm::vec3 axis, float angle)
{
    return vector * cos(glm::radians(angle)) + cross(axis, vector) * sin(glm::radians(angle)) + axis * dot(axis, vector) * (1.0f - cos(glm::radians(angle)));
}
