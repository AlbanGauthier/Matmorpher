#include "NormalReorientation.h"

void printWorkGroupsCapabilities() {

    /*
    Taille maximale des workgroups:
        x:2147483647
        y:65535
        z:65535
    Nombre maximal d'invocation locale:
        x:1536
        y:1024
        z:64
    Nombre maximum d'invocation de workgroups:
        1536
    */

    int workgroup_count[3];
    int workgroup_size[3];
    int workgroup_invocations;

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &workgroup_count[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &workgroup_count[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &workgroup_count[2]);

    printf("Taille maximale des workgroups:\n\tx:%u\n\ty:%u\n\tz:%u\n",
        workgroup_count[0], workgroup_count[1], workgroup_count[2]);

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &workgroup_size[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &workgroup_size[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &workgroup_size[2]);

    printf("Nombre maximal d'invocation locale:\n\tx:%u\n\ty:%u\n\tz:%u\n",
        workgroup_size[0], workgroup_size[1], workgroup_size[2]);

    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &workgroup_invocations);
    printf("Nombre maximum d'invocation de workgroups:\n\t%u\n", workgroup_invocations);
}

void normalizeTable(
    vector<float>& inputVec, 
    float& hf, 
    float depth,
    float max_radius,
    bool print_histo)
{
    // take care of NaNs
    int tab_max_index = std::distance(inputVec.begin(), std::max_element(inputVec.begin(), inputVec.end()));
    float max = inputVec[tab_max_index];
    for (int i = 0; i < inputVec.size(); i++)
    {
        if (inputVec[i] <= 0.00001f) // i.e. sum = 0
            inputVec[i] = max;
    }

    int tab_min_index = std::distance(inputVec.begin(), std::min_element(inputVec.begin(), inputVec.end()));
    hf = max_radius * tab_min_index / depth;
    
    // optionnal : normalize and show inputVec
    float min = inputVec[tab_min_index];
    
    for (int i = 0; i < inputVec.size(); i++)
    {
        inputVec[i] -= min;
        inputVec[i] /= max;
    }

    if (print_histo)
    {
        int histo_height = 20;
        for (int k = histo_height; k > 0; k--)
        {
            for (int i = 0; i < inputVec.size(); i++)
            {
                float hist_val = inputVec[i];
                float bin_val = float(k) / histo_height;
                if (hist_val > bin_val)
                    cout << "|";
                else
                    cout << " ";
            }
            cout << endl;
        }
    }
}

void processDiffTable(
    const string& mat_path,
    const vector<float>& diffTable,
    vector<float>& res,
    unsigned int width,
    unsigned int height,
    unsigned int depth)
{
//#pragma omp parallel for
    for (unsigned int d = 0; d < depth; d++)
    {
        float sum = 0;
        for (unsigned int j = 0; j < height; j++)
        {
            for (unsigned int i = 0; i < width; i++)
            {
                size_t index_vec = i + width * (j + height * d);
                float val1 = 0;
                if (std::isnan(diffTable[index_vec]))
                {
                    sum = 0;
                    i = width;
                    j = height;
                }
                else
                {
                    val1 = diffTable[index_vec];
                }
                //float error = abs(val1);
                sum += val1;
            }
        }
        res[d] = sum;
    }
}

void computeHeightFactor(
    const Shader& shader,
    int mat_id,
    const string& mat_name, 
    float& computed_heightF)
{
    // Parameters of the search
    const GLsizei computeTableWidth = 512;
    const GLsizei computeTableHeight = 512;
    const GLsizei depth = 128;

    // Init gradient texture
    GLuint gradTexID;
    glCreateTextures(GL_TEXTURE_2D, 1, &gradTexID);
    glTextureStorage2D(gradTexID, 1, GL_RGBA32F, computeTableWidth, computeTableHeight);
    
    // Init table of textures
    GLuint normalTextureID;
    glCreateTextures(GL_TEXTURE_3D, 1, &normalTextureID);
    glTextureStorage3D(normalTextureID, 1, GL_RGBA32F, computeTableWidth, computeTableHeight, depth);

    // Init difference table
    GLuint diffTextureID;
    glCreateTextures(GL_TEXTURE_3D, 1, &diffTextureID);
    glTextureStorage3D(diffTextureID, 1, GL_R32F, computeTableWidth, computeTableHeight, depth);
    
    ////////////////////
    // COMPUTE SHADER //
    ////////////////////

    auto startTime = std::chrono::system_clock::now();

    float height_factor_max_radius = 0.2f;

    shader.use();

    shader.setInt("mat_idx", mat_id);
    shader.setFloat("depth", depth);
    shader.setFloat("max_radius", height_factor_max_radius);

    glBindImageTexture(0, gradTexID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(1, normalTextureID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(2, diffTextureID, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);

    // Step 0 : compute gradients
    shader.setInt("step_", 0);
    glDispatchCompute(computeTableWidth, computeTableHeight, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Step 1 : compute normal table
    shader.setInt("step_", 1);
    glDispatchCompute(computeTableWidth, computeTableHeight, depth);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Step 2 : compute diff table
    shader.setInt("step_", 2);
    glDispatchCompute(computeTableWidth, computeTableHeight, depth);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Fetch diff table from GPU
    vector<GLfloat> diff_table(computeTableWidth * computeTableHeight * depth);
    glBindTexture(GL_TEXTURE_3D, diffTextureID);
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RED, GL_FLOAT, diff_table.data());
    vector<float> vec_norms(depth, 0);
    processDiffTable(mat_name, diff_table, vec_norms, computeTableWidth, computeTableHeight, depth);
    normalizeTable(vec_norms, computed_heightF, depth, height_factor_max_radius);

    // Delete normal table & gradient textures on the GPU
    glDeleteTextures(1, &gradTexID);
    glDeleteTextures(1, &normalTextureID);
    glDeleteTextures(1, &diffTextureID);

    cout << "fitted height factor: " + std::to_string(computed_heightF) << endl;
    shader.setFloat("computed_height_factor", computed_heightF);

    auto endTime = std::chrono::system_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    cout << "compute shader duration : " << time << " ms" << endl;
}