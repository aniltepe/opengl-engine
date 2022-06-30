//
//  test7.cpp
//  OpenGLTest4
//
//  Created by Nazım Anıl Tepe on 2.06.2022.
//

#include <stdio.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/string_cast.hpp>
#include <iostream>
#include <cmath>
#include <vector>
using namespace std;

glm::vec3 rotateVectorAroundAxis(glm::vec3 vector, glm::vec3 axis, float angle);
glm::vec3 rotateVectorAroundAxises(glm::vec3 vector, glm::mat3 axises, glm::vec3 degrees);

void test1();
void test2();
void test3();
void test4();

int main() {
//    test1();
    test2();
//    test3();
}

glm::vec3 rotateVectorAroundAxis(glm::vec3 vector, glm::vec3 axis, float angle)
{
    return vector * cos(glm::radians(angle)) + cross(axis, vector) * sin(glm::radians(angle)) + axis * dot(axis, vector) * (1.0f - cos(glm::radians(angle)));
}

glm::vec3 rotateVectorAroundAxises(glm::vec3 vector, glm::mat3 axises, glm::vec3 degrees)
{
    glm::vec3 newvector = glm::vec3(vector);
    newvector = rotateVectorAroundAxis(newvector, axises[0], degrees.x);
    newvector = rotateVectorAroundAxis(newvector, axises[1], degrees.y);
    newvector = rotateVectorAroundAxis(newvector, axises[2], degrees.z);
    return newvector;
}

void test1() {
    glm::vec3 vertice = glm::vec3(-0.3, 0.5, 0.1);
    glm::vec3 joint = glm::vec3(0, 0.75, 0);
    glm::vec3 degrees = glm::vec3(-45, 0, 0);
    glm::mat3 axises = glm::mat3(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
    float weight = 0.333333;
    glm::vec3 newvector = vertice - joint;
    newvector = rotateVectorAroundAxises(newvector, axises, degrees);
    newvector += joint;
    newvector = (newvector - vertice) * weight;
//    newvector = (newvector - vertice);
    
    cout << to_string(vertice + newvector) << endl;
    
    glm::vec3 vertice1 = glm::vec3(-0.3, 0.5, 0.1);
//    glm::vec3 vertice1 = vertice + newvector;
    glm::vec3 joint1 = glm::vec3(0, 0.5, 0);
    glm::vec3 degrees1 = glm::vec3(0, 45, 0);
    glm::mat3 axises1 = glm::mat3(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
    float weight1 = 0.333333;
    glm::vec3 newvector1 = vertice1 - joint1;
    newvector1 = rotateVectorAroundAxises(newvector1, axises1, degrees1);
    newvector1 += joint1;
    newvector1 = (newvector1 - vertice1) * weight1;
//    newvector1 = (newvector1 - vertice1);
    
    
//    glm::vec3 vertice2 = glm::vec3(-0.3, 0.5, 0.1);
    glm::vec3 vertice2 = vertice + newvector;
    glm::vec3 joint2 = glm::vec3(0, 0.5, 0);
    glm::vec3 degrees2 = glm::vec3(0, 45, 0);
    glm::mat3 axises2 = glm::mat3(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
    float weight2 = 0.333333;
    glm::vec3 newvector2 = vertice2 - joint2;
    newvector2 = rotateVectorAroundAxises(newvector2, axises2, degrees2);
    newvector2 += joint2;
    newvector2 = (newvector2 - vertice2) * weight2;
//    newvector2 = (newvector2 - vertice2);
    
    newvector = vertice + newvector + newvector1 + newvector2;
//    newvector = vertice + (newvector + newvector1 + newvector2) * weight;
    cout << to_string(newvector) << endl;
}

void test2() {
        glm::vec3 vertice1 = glm::vec3(-0.3, 0.5, 0.1);
        glm::vec3 joint1 = glm::vec3(0, 0.5, 0);
        glm::vec3 degrees1 = glm::vec3(0, 45, 0);
        glm::mat3 axises1 = glm::mat3(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
        float weight1 = 0.333333;
        glm::vec3 newvector1 = vertice1 - joint1;
        newvector1 = rotateVectorAroundAxises(newvector1, axises1, degrees1);
        newvector1 += joint1;
        newvector1 = (newvector1 - vertice1) * weight1;
    //    newvector1 = (newvector1 - vertice1);
        
        
        glm::vec3 vertice2 = glm::vec3(-0.3, 0.5, 0.1);
        glm::vec3 joint2 = glm::vec3(0, 0.75, 0);
        glm::vec3 degrees2 = glm::vec3(0, 45, 0);
        glm::mat3 axises2 = glm::mat3(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
        float weight2 = 0.333333;
        glm::vec3 newvector2 = vertice2 - joint2;
        newvector2 = rotateVectorAroundAxises(newvector2, axises2, degrees2);
        newvector2 += joint2;
        newvector2 = (newvector2 - vertice2) * weight2;
    //    newvector2 = (newvector2 - vertice2);
        
        glm::vec3 newvector = vertice1 + newvector1 + newvector2;
    //    newvector = vertice + (newvector + newvector1 + newvector2) * weight;
        cout << to_string(newvector) << endl;
}

void test3() {
    glm::vec3 vertice = glm::vec3(-0.3, 0.5, 0.1);
    glm::vec3 joint = glm::vec3(0, 0.75, 0);
    glm::vec3 degrees = glm::vec3(-45, 45, 0);
    glm::mat3 axises = glm::mat3(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
    float weight = 0.333333;
    glm::vec3 newvector = vertice - joint;
    newvector = rotateVectorAroundAxises(newvector, axises, degrees);
    newvector += joint;
    newvector = (newvector - vertice) * weight;
//    newvector = (newvector - vertice);
    
    cout << to_string(vertice + newvector) << endl;
    
//    glm::vec3 vertice1 = glm::vec3(-0.3, 0.5, 0.1);
    glm::vec3 vertice1 = vertice + newvector;
    glm::vec3 joint1 = glm::vec3(0, 0.5, 0);
    glm::vec3 degrees1 = glm::vec3(0, 45, 0);
    glm::mat3 axises1 = glm::mat3(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
    float weight1 = 0.333333;
    glm::vec3 newvector1 = vertice1 - joint1;
    newvector1 = rotateVectorAroundAxises(newvector1, axises1, degrees1);
    newvector1 += joint1;
    newvector1 = (newvector1 - vertice1) * weight1;
//    newvector1 = (newvector1 - vertice1);
    
    
    
    newvector = vertice + newvector + newvector1;
//    newvector = vertice + (newvector + newvector1 + newvector2) * weight;
    cout << to_string(newvector) << endl;
}


void test4() {
        glm::vec3 vertice1 = glm::vec3(-0.3, 0.5, 0.1);
        glm::vec3 joint1 = glm::vec3(0, 0.5, 0);
        glm::vec3 degrees1 = glm::vec3(0, 45, 0);
        glm::mat3 axises1 = glm::mat3(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
        float weight1 = 0.333333;
        glm::vec3 newvector1 = vertice1 - joint1;
        newvector1 = rotateVectorAroundAxises(newvector1, axises1, degrees1);
        newvector1 += joint1;
        newvector1 = (newvector1 - vertice1) * weight1;
    //    newvector1 = (newvector1 - vertice1);
        
        
        glm::vec3 vertice2 = glm::vec3(-0.3, 0.5, 0.1);
        glm::vec3 joint2 = glm::vec3(0, 0.75, 0);
        glm::vec3 degrees2 = glm::vec3(0, 45, 0);
        glm::mat3 axises2 = glm::mat3(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
        float weight2 = 0.333333;
        glm::vec3 newvector2 = vertice2 - joint2;
        newvector2 = rotateVectorAroundAxises(newvector2, axises2, degrees2);
        newvector2 += joint2;
        newvector2 = (newvector2 - vertice2) * weight2;
    //    newvector2 = (newvector2 - vertice2);
        
        glm::vec3 newvector = vertice1 + newvector1 + newvector2;
    //    newvector = vertice + (newvector + newvector1 + newvector2) * weight;
        cout << to_string(newvector) << endl;
}
