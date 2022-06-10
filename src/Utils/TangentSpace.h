#pragma once

#include <glm/glm.hpp>
#include <vector>

std::vector<glm::vec4> genMikkTSpace(
    const std::vector<glm::vec3> &pos, 
    const std::vector<glm::vec3> &nor, 
    const std::vector<glm::vec2> &tex, 
    const std::vector<unsigned int> &faces);

std::vector<glm::vec4> genMikkTSpace(
    const std::vector<float>& pos,
    const std::vector<float>& nor,
    const std::vector<float>& tex,
    const std::vector<unsigned int>& faces);