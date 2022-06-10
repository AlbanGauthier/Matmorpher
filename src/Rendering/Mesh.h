#pragma once

#include "Shader.h"

#include "Utils/TangentSpace.h"

#include <QString>
#include <memory>

#define _USE_MATH_DEFINES
#include <math.h>

using glm::vec2;
using glm::vec3;
using glm::vec4;

class Mesh {
public:
    Mesh() {}
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    virtual ~Mesh() {
        //std::cout << "calling Mesh destructor" << std::endl;
        if (m_vao)         glDeleteVertexArrays(1, &m_vao);
        if (m_posVbo)      glDeleteBuffers(1, &m_posVbo);
        if (m_normalVbo)   glDeleteBuffers(1, &m_normalVbo);
        if (m_texCoordVbo) glDeleteBuffers(1, &m_texCoordVbo);
        if (m_tanVbo)      glDeleteBuffers(1, &m_tanVbo);
        if (m_ebo)         glDeleteBuffers(1, &m_ebo);
    }

    static void setRenderMode(bool drawPatches_)
    {
        drawPatches = drawPatches_;
    }

    bool init() {

        // Create a single handle that joins together attributes (vertex positions,
        // normals) and connectivity (triangles indices)
        glCreateVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        if (m_vertexPositions.size() != 0)
        {
            // Generate a GPU buffer to store the positions of the vertices
            glCreateBuffers(1, &m_posVbo);
            size_t vertexBufferSize = sizeof(vec3) * m_vertexPositions.size(); // Gather the size of the buffer from the CPU-side vector
            glNamedBufferStorage(m_posVbo, vertexBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT); // Create a data store on the GPU
            glNamedBufferSubData(m_posVbo, 0, vertexBufferSize, m_vertexPositions.data()); // Fill the data store from a CPU array

            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, m_posVbo);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
        }
        else
            return false;

        if (m_vertexTexCoords.size() != 0)
        {
            // Same for texture coordinates
            glCreateBuffers(1, &m_texCoordVbo);
            size_t texCoordBufferSize = sizeof(vec2) * m_vertexTexCoords.size();
            glNamedBufferStorage(m_texCoordVbo, texCoordBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT);
            glNamedBufferSubData(m_texCoordVbo, 0, texCoordBufferSize, m_vertexTexCoords.data());

            glEnableVertexAttribArray(1);
            glBindBuffer(GL_ARRAY_BUFFER, m_texCoordVbo);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);
        }

        if (m_vertexNormals.size() != 0)
        {
            // Same for normal
            glCreateBuffers(1, &m_normalVbo);
            size_t normalBufferSize = sizeof(vec3) * m_vertexNormals.size(); // Gather the size of the buffer from the CPU-side vector
            glNamedBufferStorage(m_normalVbo, normalBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT);
            glNamedBufferSubData(m_normalVbo, 0, normalBufferSize, m_vertexNormals.data());

            glEnableVertexAttribArray(2);
            glBindBuffer(GL_ARRAY_BUFFER, m_normalVbo);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
        }

        if (m_vertexTangents.size() != 0)
        {
            glCreateBuffers(1, &m_tanVbo);
            size_t tangentsBufferSize = sizeof(vec4) * m_vertexTangents.size(); // Gather the size of the buffer from the CPU-side vector
            glNamedBufferStorage(m_tanVbo, tangentsBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT);
            glNamedBufferSubData(m_tanVbo, 0, tangentsBufferSize, m_vertexTangents.data());

            glEnableVertexAttribArray(3);
            glBindBuffer(GL_ARRAY_BUFFER, m_tanVbo);
            glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), 0);
        }

        glBindVertexArray(0); // deactivate the VAO for now, will be activated at rendering time.

        if (m_triangleIndices.size() != 0)
        {
            // Same for the index buffer that stores the list of indices of the
            // triangles forming the mesh
            glCreateBuffers(1, &m_ebo);
            size_t indexBufferSize = sizeof(unsigned int) * m_triangleIndices.size();
            glNamedBufferStorage(m_ebo, indexBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT);
            glNamedBufferSubData(m_ebo, 0, indexBufferSize, m_triangleIndices.data());
        }
        else
            return false;

        return true;
    }

    void render() {
        // Activate the VAO storing geometry data
        glBindVertexArray(m_vao);

        // Call for rendering: stream the current GPU geometry through the current GPU program
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

        if (drawPatches)
            glDrawElements(GL_PATCHES, static_cast<GLsizei>(m_triangleIndices.size()), GL_UNSIGNED_INT, 0);
        else
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_triangleIndices.size()), GL_UNSIGNED_INT, 0);
    }

    static vec3 polar2Cartesian(
        const float phi, 
        const float theta, 
        const float d) 
    {
        vec3 p;
        p.x = d * std::sin(theta) * std::sin(phi);
        p.y = d * std::cos(theta);
        p.z = d * std::sin(theta) * std::cos(phi);
        return p;
    }

    // the plane spans [0, 1] x [0, 1]
    static std::unique_ptr<Mesh> genPlane(const size_t resolution = 128) 
    {
        drawPatches = true;
        
        std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>();
        const size_t numOfVertices = resolution * resolution;

        mesh->m_vertexPositions.clear();
        mesh->m_vertexTexCoords.clear();
        mesh->m_vertexNormals.clear();
        mesh->m_vertexTangents.clear();

        mesh->m_vertexPositions.resize(numOfVertices);
        mesh->m_vertexTexCoords.resize(numOfVertices);
        mesh->m_vertexNormals.resize(numOfVertices);

        for (unsigned int i = 0; i < resolution; ++i)
        {
            for (unsigned int j = 0; j < resolution; ++j)
            {
                const size_t index = j * resolution + i;

                mesh->m_vertexPositions[index] = vec3( 
                    float(j) / float(resolution - 1), //x
                    float(i) / float(resolution - 1), //y
                    0.0f);                        //z

                mesh->m_vertexTexCoords[index] = vec2( 
                    float(j) / float(resolution - 1),   //u
                    float(i) / float(resolution - 1));  //v

                mesh->m_vertexNormals[index] = vec3(
                    0.0f,   //x
                    0.0f,   //y
                    1.0f);  //z
            }
        }

        mesh->m_triangleIndices.clear();
        mesh->m_triangleIndices.resize(6 * (resolution - 1) * (resolution - 1));

        for (unsigned int i = 0; i < (resolution - 1); ++i)
        {
            for (unsigned int j = 0; j < (resolution - 1); ++j)
            {
                const size_t index = j * (resolution - 1) + i;

                const size_t i0 = j * resolution + i;
                const size_t i1 = j * resolution + i + 1;
                const size_t i2 = (j + static_cast<size_t>(1)) * resolution + i;
                const size_t i3 = (j + static_cast<size_t>(1)) * resolution + i + 1;

                mesh->m_triangleIndices[6 * index] =     i0;
                mesh->m_triangleIndices[6 * index + 1] = i2;
                mesh->m_triangleIndices[6 * index + 2] = i3;

                mesh->m_triangleIndices[6 * index + 3] = i0;
                mesh->m_triangleIndices[6 * index + 4] = i3;
                mesh->m_triangleIndices[6 * index + 5] = i1;
            }
        }

        mesh->m_vertexTangents = genMikkTSpace(
            mesh->m_vertexPositions, 
            mesh->m_vertexNormals, 
            mesh->m_vertexTexCoords, 
            mesh->m_triangleIndices);

        return mesh;
    }

    static std::unique_ptr<Mesh> genSphere(const size_t resolution = 32) {

        drawPatches = true;

        std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>();

        const size_t resolutionX = 2 * resolution;
        const size_t resolutionY = resolution;

        const size_t numOfVertices = resolutionX * resolutionY;
        const float stepPhi =   2.f * M_PI / (resolutionX - 1);
        const float stepTheta = 1.f * M_PI / (resolutionY - 1);

        mesh->m_vertexPositions.clear();
        mesh->m_vertexTexCoords.clear();
        mesh->m_vertexNormals.clear();
        mesh->m_vertexTangents.clear();

        mesh->m_vertexPositions.resize(numOfVertices);
        mesh->m_vertexTexCoords.resize(numOfVertices);
        mesh->m_vertexNormals.resize(numOfVertices);

        for (unsigned int j = 0; j < resolutionY; ++j) 
        {
            for (unsigned int i = 0; i < resolutionX; ++i) 
            {
                const size_t index = j * resolutionX + i;
                const float phi = i * stepPhi + M_PI;
                const float theta = j * stepTheta;
                vec3 p = polar2Cartesian(phi, theta, 1.0);

                mesh->m_vertexPositions[index]  = p;
                mesh->m_vertexNormals[index]    = p;

                mesh->m_vertexTexCoords[index] = vec2( static_cast<float>(i) / (resolutionX - 1),
                                                       static_cast<float>(j) / (resolutionY - 1));
            }
        }

        mesh->m_triangleIndices.clear();
        mesh->m_triangleIndices.reserve(6 * (resolutionX - 1) * (resolutionY - 1));

        for (unsigned int j = 0; j < resolutionY - 1; ++j) 
        {
            for (unsigned int i = 0; i < resolutionX - 1; ++i) 
            {
                const size_t i0 = j * resolutionX + i;
                const size_t i1 = j * resolutionX + i + 1;
                const size_t i2 = (j + static_cast<size_t>(1)) * resolutionX + i;
                const size_t i3 = (j + static_cast<size_t>(1)) * resolutionX + i + 1;
                // triangle 1
                mesh->m_triangleIndices.push_back(i0);
                mesh->m_triangleIndices.push_back(i2);
                mesh->m_triangleIndices.push_back(i3);
                // triangle 2
                mesh->m_triangleIndices.push_back(i0);
                mesh->m_triangleIndices.push_back(i3);
                mesh->m_triangleIndices.push_back(i1);
            }
        }

        mesh->m_vertexTangents = genMikkTSpace(
            mesh->m_vertexPositions,
            mesh->m_vertexNormals,
            mesh->m_vertexTexCoords,
            mesh->m_triangleIndices);

        return mesh;
    }

    static std::unique_ptr<Mesh> genCube() {

        std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>();

        drawPatches = true;
        
        mesh->m_vertexPositions.clear();
        mesh->m_vertexTexCoords.clear();

        mesh->m_vertexPositions.reserve(4 * 6);
        mesh->m_vertexTexCoords.reserve(4 * 6);

        // 0 1 2 3 4 5 6 7
        // sides
        for (unsigned int x = 0; x < 2; ++x) 
        {
            for (unsigned int y = 0; y < 2; ++y)
            {
                for (unsigned int z = 0; z < 2; ++z)
                {
                    mesh->m_vertexPositions.push_back(vec3(x - 0.5f, y - 0.5f, z - 0.5f));
                    mesh->m_vertexTexCoords.push_back(vec2(z, y));
                }
            }
        }

        // 0 1 4 5 2 3 6 7
        // top/bottom
        for (unsigned int y = 0; y < 2; ++y)
        {
            for (unsigned int x = 0; x < 2; ++x)
            {
                for (unsigned int z = 0; z < 2; ++z)
                {
                    mesh->m_vertexPositions.push_back(vec3(x - 0.5f, y - 0.5f, z - 0.5f));
                    mesh->m_vertexTexCoords.push_back(vec2(x, 1.0f - z));
                }
            }
        }

        // 0 2 4 6 1 3 5 7
        // front/back
        for (unsigned int z = 0; z < 2; ++z)
        {
            for (unsigned int x = 0; x < 2; ++x)
            {
                for (unsigned int y = 0; y < 2; ++y)
                {
                    mesh->m_vertexPositions.push_back(vec3(x - 0.5f, y - 0.5f, z - 0.5f));
                    mesh->m_vertexTexCoords.push_back(vec2(x, y));
                }
            }
        }

        mesh->m_triangleIndices.clear();
        mesh->m_triangleIndices.reserve(6 * 2 * 3);

        //back
        mesh->m_triangleIndices.push_back(0);
        mesh->m_triangleIndices.push_back(1);
        mesh->m_triangleIndices.push_back(2);
        mesh->m_triangleIndices.push_back(1);
        mesh->m_triangleIndices.push_back(3);
        mesh->m_triangleIndices.push_back(2);

        //front
        mesh->m_triangleIndices.push_back(5);
        mesh->m_triangleIndices.push_back(6);
        mesh->m_triangleIndices.push_back(7);
        mesh->m_triangleIndices.push_back(4);
        mesh->m_triangleIndices.push_back(6);
        mesh->m_triangleIndices.push_back(5);

        mesh->m_triangleIndices.push_back(0 + 8);
        mesh->m_triangleIndices.push_back(1 + 8);
        mesh->m_triangleIndices.push_back(2 + 8);
        mesh->m_triangleIndices.push_back(1 + 8);
        mesh->m_triangleIndices.push_back(3 + 8);
        mesh->m_triangleIndices.push_back(2 + 8);

        mesh->m_triangleIndices.push_back(5 + 8);
        mesh->m_triangleIndices.push_back(6 + 8);
        mesh->m_triangleIndices.push_back(7 + 8);
        mesh->m_triangleIndices.push_back(4 + 8);
        mesh->m_triangleIndices.push_back(6 + 8);
        mesh->m_triangleIndices.push_back(5 + 8);

        mesh->m_triangleIndices.push_back(0 + 16);
        mesh->m_triangleIndices.push_back(1 + 16);
        mesh->m_triangleIndices.push_back(2 + 16);
        mesh->m_triangleIndices.push_back(1 + 16);
        mesh->m_triangleIndices.push_back(3 + 16);
        mesh->m_triangleIndices.push_back(2 + 16);

        mesh->m_triangleIndices.push_back(5 + 16);
        mesh->m_triangleIndices.push_back(6 + 16);
        mesh->m_triangleIndices.push_back(7 + 16);
        mesh->m_triangleIndices.push_back(4 + 16);
        mesh->m_triangleIndices.push_back(6 + 16);
        mesh->m_triangleIndices.push_back(5 + 16);

        return mesh;
    }

private:
    std::vector<vec3> m_vertexPositions;
    std::vector<vec2> m_vertexTexCoords;
    std::vector<vec3> m_vertexNormals;
    std::vector<unsigned int> m_triangleIndices;

    std::vector<glm::vec4> m_vertexTangents;

    GLuint m_vao = 0;
    GLuint m_posVbo = 0;
    GLuint m_normalVbo = 0;
    GLuint m_texCoordVbo = 0;
    GLuint m_tanVbo = 0;
    GLuint m_ebo = 0;

    static bool drawPatches;
};