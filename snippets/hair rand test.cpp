srand((unsigned int)time(NULL));
for (int j = 0; j < hairCount; j++) {
    float ran0 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    float ran1 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    float ran2 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    glm::vec3 p0 = currTri[1] + ran0 * (currTri[0] - currTri[1]);
    glm::vec3 p1 = currTri[2] + ran1 * (currTri[1] - currTri[2]);
    glm::vec3 p2 = currTri[0] + ran2 * (currTri[2] - currTri[0]);
    float w0 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    float w1 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    float w2 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    float w0 = max(ran0, 1 - ran0);
    float w1 = max(ran0, 1 - ran1);
    float w2 = max(ran0, 1 - ran2);
    glm::vec3 newrand = ((p1 + (p0 - p1) * w0) + (p2 + (p1 - p2) * w1) + (p0 + (p2 - p0) * w2)) / 3.0f;
    float ranX = minX + static_cast<float>(rand()) / ( static_cast<float>(RAND_MAX/(maxX-minX)));
    float ranY = minY + static_cast<float>(rand()) / ( static_cast<float>(RAND_MAX/(maxY-minY)));
    float ranZ = minZ + static_cast<float>(rand()) / ( static_cast<float>(RAND_MAX/(maxZ-minZ)));
    obj->instance.translate.insert(obj->instance.translate.end(), { newrand.x, newrand.y, newrand.z });
}
