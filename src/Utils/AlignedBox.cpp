#include "AlignedBox.h"

AlignedBox::AlignedBox()
    : min_pos_(
        std::numeric_limits<float>::max(), 
        std::numeric_limits<float>::max(), 
        std::numeric_limits<float>::max()), 
    max_pos_(
        -std::numeric_limits<float>::max(), 
        -std::numeric_limits<float>::max(), 
        -std::numeric_limits<float>::max())
{
}

void AlignedBox::extend(const glm::vec3 &v)
{
    for (int i = 0; i < 3; i++)
    {
        min_pos_[i] = glm::min(v[i], min_pos_[i]);
        max_pos_[i] = glm::max(v[i], max_pos_[i]);
    }
}

void AlignedBox::extend(const AlignedBox &b)
{
    for (int i = 0; i < 3; i++)
    {
        min_pos_[i] = glm::min(b.min_pos_[i], min_pos_[i]);
        max_pos_[i] = glm::max(b.max_pos_[i], max_pos_[i]);
    }
}

AlignedBox AlignedBox::transform(const glm::mat4 &m) const
{
    AlignedBox aabb;
    glm::vec3 corner;
    for (int i = 0; i < 2; i++)
    {
        corner[0] = i == 0 ? min_pos_[0] : max_pos_[0];
        for (int j = 0; j < 2; j++)
        {
            corner[1] = j == 0 ? min_pos_[1] : max_pos_[1];
            for (int k = 0; k < 2; k++)
            {
                corner[2] = k == 0 ? min_pos_[2] : max_pos_[2];
                aabb.extend(m * glm::vec4(corner, 1.f));
            }
        }
    }

    return aabb;
}
