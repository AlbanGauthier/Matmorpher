#pragma once

#include <glm/glm.hpp>

class AlignedBox
{
public:
    // Creates an empty axis aligned bounding box
    AlignedBox();

    // Returns the lower bound
    glm::vec3 minPos() { return min_pos_; }

    // Returns the upper bound
    glm::vec3 maxPos() { return max_pos_; }

    // Returns the box diagonal length
    float scale() { return glm::length(max_pos_ - min_pos_); }

    // Returns the center
    glm::vec3 center() { return 0.5f * (max_pos_ + min_pos_); }

    // Extends the box to contains the given point
    void extend(const glm::vec3 &v);

    // Extends the box to contains the given box
    void extend(const AlignedBox &b);

    // Returns a new AlignedBox containing this box transformed by m
    AlignedBox transform(const glm::mat4 &m) const;

private:
    glm::vec3 min_pos_ = glm::vec3(0.);
    glm::vec3 max_pos_ = glm::vec3(0.);
};