#include "WarpTextureDesign.h"

bool resize_img(const unsigned char* img_in, int oldX, int oldY, unsigned char* output, int newX, int newY)
{
    if (stbir_resize_uint8_generic(
        img_in, oldX, oldY, 0,
        output, newX, newY, 0,
        1, STBIR_ALPHA_CHANNEL_NONE, 0,
        STBIR_EDGE_WRAP, STBIR_FILTER_TRIANGLE, STBIR_COLORSPACE_LINEAR, NULL) == 0)
    {
        cout << "failed to resample image" << endl;
        return false;
    }
    else
        return true;
}

void write_warpgrid(const char* fname_in, const vector<vector<vec2>>& warp_in)
{
    size_t height = warp_in.size();
    size_t width = warp_in[0].size();
    size_t nb_pts = height * width;

    std::string filename = fname_in;

    std::ofstream myfile;
    myfile.open(filename);

    if (myfile.is_open())
    {
        myfile << nb_pts << "\n";
        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                myfile << warp_in[i][j][0] << " " << warp_in[i][j][1] << "\n";
            }
        }
        myfile.close();
    }

    else
    {
        std::cerr << "could not open file to write in" << std::endl;
    }
}

void write_warpgrid_image(const char* fname_in, const vector<vector<vec2>>& warp_in)
{
    size_t size = warp_in.size();
    vector<unsigned char> data_out(warp_in.size() * warp_in[0].size() * 3, 0);
    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            data_out[3 * (y * size + x) + 0] = (unsigned char)(255.0f * clamp(warp_in[y][x][0], 0.0f, 1.0f));
            data_out[3 * (y * size + x) + 1] = (unsigned char)(255.0f * clamp(warp_in[y][x][1], 0.0f, 1.0f));
        }
    }

    cout << "writing:" << fname_in << endl;
    if (stbi_write_png(fname_in, size, size, 3, data_out.data(), 0) == 0)
        cout << "error writting:" << fname_in << endl;
}

void saveGridImage(
    vector<vector<vec2>> const& G,
    std::string const& filename,
    int N)
{
    int width = 4096, height = 4096;

    QImage image(1.1f * width, 1.1f * height, QImage::Format_RGB32);
    QPainter painter(&image);

    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.fillRect(0, 0, 1.1f * width, 1.1f * height, QColor(255, 255, 255));

    QPen pen;
    pen.setWidth(1);
    pen.setColor(Qt::red);
    painter.setPen(pen);

    painter.drawRect(0.05f * width, 0.05f * height, width, height);

    int jump = 1;

    for (int k = 0; k < N - jump; k += jump) {
        for (int l = 0; l < N - jump; l += jump) {
            vec2 p1(G[k][l].x, G[k][l].y);
            vec2 p2(G[k + jump][l].x, G[k + jump][l].y);
            vec2 p3(G[k][l + jump].x, G[k][l + jump].y);
            vec2 p4(G[k + jump][l + jump].x, G[k + jump][l + jump].y);
            //painter.drawLine(width * p1[0], height * p1[1], width * p2[0], height * p2[1]);
            //painter.drawLine(width * p1[0], height * p1[1], width * p3[0], height * p3[1]);

            painter.drawLine(width * p1[0] + 0.05f * width, height * p1[1] + 0.05f * height, width * p2[0] + 0.05f * width, height * p2[1] + 0.05f * height);
            painter.drawLine(width * p1[0] + 0.05f * width, height * p1[1] + 0.05f * height, width * p3[0] + 0.05f * width, height * p3[1] + 0.05f * height);
            painter.drawLine(width * p1[0] + 0.05f * width, height * p1[1] + 0.05f * height, width * p4[0] + 0.05f * width, height * p4[1] + 0.05f * height);
            painter.drawLine(width * p3[0] + 0.05f * width, height * p3[1] + 0.05f * height, width * p4[0] + 0.05f * width, height * p4[1] + 0.05f * height);
            painter.drawLine(width * p2[0] + 0.05f * width, height * p2[1] + 0.05f * height, width * p4[0] + 0.05f * width, height * p4[1] + 0.05f * height);
        }
    }

    painter.end();

    image.mirrored(false, true).save(QString::fromStdString(filename));
}

void saveGridImage_test(
    std::vector<vec2> const& neigh,
    vector<vector<vec2>> const& G,
    std::string const& filename,
    int N)
{
    int width = 1024, height = 1024;

    QImage image(width, height, QImage::Format_RGB32);
    QPainter painter(&image);

    painter.fillRect(0, 0, width, height, QColor(255, 255, 255));

    QPen pen;
    pen.setWidth(1);
    pen.setColor(Qt::red);
    painter.setPen(pen);

    for (int k = 0; k < N - 1; k++) {
        for (int l = 0; l < N - 1; l++) {
            vec2 p1(G[k][l].x, G[k][l].y);
            vec2 p2(G[k + 1][l].x, G[k + 1][l].y);
            vec2 p3(G[k][l + 1].x, G[k][l + 1].y);
            vec2 p4(G[k + 1][l + 1].x, G[k + 1][l + 1].y);

            painter.drawLine(width * p1[0], height * p1[1], width * p2[0], height * p2[1]);
            painter.drawLine(width * p1[0], height * p1[1], width * p3[0], height * p3[1]);
            painter.drawLine(width * p1[0], height * p1[1], width * p4[0], height * p4[1]);

            painter.drawLine(width * p3[0], height * p3[1], width * p4[0], height * p4[1]);
            painter.drawLine(width * p2[0], height * p2[1], width * p4[0], height * p4[1]);
        }
    }

    for (auto val : neigh)
    {
        pen.setWidth(5);
        painter.setPen(pen);
        painter.drawPoint(width * val.x, height * val.y);
    }

    painter.end();

    image.mirrored(false, true).save(QString::fromStdString(filename));
}

vector<vector<vec2>> get_padded_warpgrid(const vector<vector<vec2>>& warpgrid)
{
    size_t height = warpgrid.size();
    size_t width = warpgrid[0].size();

    size_t padding = 1;
    vector<vector<vec2>> warp_out(height + 2 * padding, vector<vec2>(width + 2 * padding, vec2(0, 0)));

    // corners
    warp_out[0][0] = warpgrid[height - 1][width - 1] - vec2(1.0, 1.0);
    warp_out[0][width + padding] = warpgrid[height - 1][0] + vec2(1.0, -1.0);
    warp_out[height + padding][0] = warpgrid[0][width - 1] + vec2(-1.0, 1.0);
    warp_out[height + padding][width + padding] = warpgrid[0][0] + vec2(1.0, 1.0);

    //center
    for (int y = padding; y < height + 1; y++)
        for (int x = padding; x < width + 1; x++)
            warp_out[y][x] = warpgrid[y - padding][x - padding];

    //lines
    for (int x = padding; x < width + 1; x++)
    {
        warp_out[0][x] = warpgrid[height - 1][x - padding] - vec2(0.0, 1.0);
        warp_out[height + padding][x] = warpgrid[0][x - padding] + vec2(0.0, 1.0);
    }

    //columns
    for (int y = padding; y < height + 1; y++)
    {
        warp_out[y][0] = warpgrid[y - padding][width - 1] - vec2(1.0, 0.0);
        warp_out[y][width + padding] = warpgrid[y - padding][0] + vec2(1.0, 0.0);
    }

    return warp_out;
}

std::vector<vec2> get_positions(vec2 coords, int scale)
{
    std::vector<vec2> res;
    float delta = 1.0f / (4.0f * scale);
    res.push_back(coords + vec2(0.0f, 0.0f) * delta);
    res.push_back(coords + vec2(0.0f, -1.0f) * delta);
    res.push_back(coords + vec2(1.0f, -1.0f) * delta);
    res.push_back(coords + vec2(-1.0f, 0.0f) * delta);
    res.push_back(coords + vec2(1.0f, 0.0f) * delta);
    res.push_back(coords + vec2(-1.0f, 1.0f) * delta);
    res.push_back(coords + vec2(0.0f, 1.0f) * delta);
    res.push_back(coords + vec2(-1.0f, -1.0f) * delta);
    res.push_back(coords + vec2(1.0f, 1.0f) * delta);
    return res;
}

std::vector<vec2> get_positions(const vector<vector<vec2>>& warpgrid, size_t i, size_t j)
{
    std::vector<vec2> res;
    vec2 coords = warpgrid[i][j];
    float alpha = 0.5f;
    res.push_back(coords);
    res.push_back(alpha * warpgrid[i + 0][j - 1] + (1 - alpha) * coords);
    res.push_back(alpha * warpgrid[i + 1][j - 1] + (1 - alpha) * coords);
    res.push_back(alpha * warpgrid[i - 1][j + 0] + (1 - alpha) * coords);
    res.push_back(alpha * warpgrid[i + 1][j + 0] + (1 - alpha) * coords);
    res.push_back(alpha * warpgrid[i - 1][j + 1] + (1 - alpha) * coords);
    res.push_back(alpha * warpgrid[i + 0][j + 1] + (1 - alpha) * coords);
    res.push_back(alpha * warpgrid[i - 1][j - 1] + (1 - alpha) * coords);
    res.push_back(alpha * warpgrid[i + 1][j + 1] + (1 - alpha) * coords);
    return res;
}

float compute_frobenius_norm(const vector<vector<vec2>>& warp_pad_in, float x, float y, int i, int j)
{
    int scale = warp_pad_in.size() - 2;

    int xb = j + 1;    int yb = i;
    int xc = j + 1;    int yc = i - 1;
    int xd = j;        int yd = i + 1;
    int xe = j;        int ye = i - 1;
    int xf = j - 1;    int yf = i;
    int xg = j - 1;    int yg = i + 1;

    float x_ba = warp_pad_in[yb][xb][0] - x;
    float y_ba = warp_pad_in[yb][xb][1] - y;

    float x_bc = warp_pad_in[yb][xb][0] - warp_pad_in[yc][xc][0];
    float y_bc = warp_pad_in[yb][xb][1] - warp_pad_in[yc][xc][1];

    float x_ce = warp_pad_in[yc][xc][0] - warp_pad_in[ye][xe][0];
    float y_ce = warp_pad_in[yc][xc][1] - warp_pad_in[ye][xe][1];

    float x_da = warp_pad_in[yd][xd][0] - x;
    float y_da = warp_pad_in[yd][xd][1] - y;

    float x_ae = x - warp_pad_in[ye][xe][0];
    float y_ae = y - warp_pad_in[ye][xe][1];

    float x_af = x - warp_pad_in[yf][xf][0];
    float y_af = y - warp_pad_in[yf][xf][1];

    float x_dg = warp_pad_in[yd][xd][0] - warp_pad_in[yg][xg][0];
    float y_dg = warp_pad_in[yd][xd][1] - warp_pad_in[yg][xg][1];

    float x_gf = warp_pad_in[yg][xg][0] - warp_pad_in[yf][xf][0];
    float y_gf = warp_pad_in[yg][xg][1] - warp_pad_in[yf][xf][1];

    float sum = 0;

    sum += glm::length(glm::vec4(x_ba - 1.0f / scale, x_bc, y_ba, y_bc - 1.0f / scale));
    sum += glm::length(glm::vec4(x_ce - 1.0f / scale, x_ae, y_ce, y_ae - 1.0f / scale));
    sum += glm::length(glm::vec4(x_ba - 1.0f / scale, x_da, y_ba, y_da - 1.0f / scale));
    sum += glm::length(glm::vec4(x_af - 1.0f / scale, x_ae, y_af, y_ae - 1.0f / scale));
    sum += glm::length(glm::vec4(x_dg - 1.0f / scale, x_da, y_dg, y_da - 1.0f / scale));
    sum += glm::length(glm::vec4(x_af - 1.0f / scale, x_gf, y_af, y_gf - 1.0f / scale));

    sum /= warp_pad_in.size() - 2;

    return sum;
}

vector<vector<vec2>> upsample_warpgrid(const vector<vector<vec2>>& warp_in)
{
    size_t height = warp_in.size();
    size_t width = warp_in[0].size();

    int offset = 0;

    if (height == 8) offset = 1;

    vector<vector<vec2>> warp_out(2 * height, vector<vec2>(2 * width, vec2(0.0f, 0.0f)));

    vector<vector<vec2>> padded_warp_grid = get_padded_warpgrid(warp_in);

    size_t padd_height = height + 2;
    size_t padd_width = width + 2;

    TextureSampler<vec2> samplerWarp(padded_warp_grid);

    for (int i = 0; i < 2 * height; i++)
        for (int j = 0; j < 2 * width; j++)
            warp_out[i][j] = samplerWarp.sampleTexture(
                (j + 3.0f - offset) / (2 * padd_width),
                (i + 3.0f - offset) / (2 * padd_height));

    return warp_out;
}

vector<vector<vec2>> resize_final_warpgrid(const vector<vector<vec2>>& warp_in)
{
    return vector<vector<vec2>>();
}

bool computeWarpgridTextureDesign(string fname_in, string fname_in_2, float alpha, int output_size)
{
    stbi_set_flip_vertically_on_load(true);

    int F0_size_x, F0_size_y, F1_size_x, F1_size_y, nbChannels;

    unsigned char* F0 = stbi_load(fname_in.c_str(), &F0_size_x, &F0_size_y, &nbChannels, 1);
    if (F0 == nullptr)
    {
        cerr << "could not open input file: " << fname_in << endl;
        exit(EXIT_FAILURE);
    }

    unsigned char* F1 = stbi_load(fname_in_2.c_str(), &F1_size_x, &F1_size_y, &nbChannels, 1);
    if (F1 == nullptr)
    {
        cerr << "could not open input file: " << fname_in_2 << endl;
        exit(EXIT_FAILURE);
    }

    stbi_set_flip_vertically_on_load(false);

    size_t grid_scale = 8;
    vector<float> range_vec(grid_scale);
    for (int i = 0; i < grid_scale; i++)
        range_vec[i] = (i + 0.5f) / float(grid_scale);

    vector<vector<vec2>> warp_grid(grid_scale, vector<vec2>(grid_scale, vec2(0, 0)));

    for (int y = 0; y < grid_scale; y++)
        for (int x = 0; x < grid_scale; x++)
            warp_grid[y][x] = vec2(range_vec[x], range_vec[y]);

    while (grid_scale <= output_size)
    {
        vector<unsigned char> F0_resized(grid_scale * grid_scale);
        if (!resize_img(F0, F0_size_x, F0_size_y, F0_resized.data(), grid_scale, grid_scale))
            exit(EXIT_FAILURE);

        size_t feature_scale = 2 * grid_scale;

        vector<unsigned char> F1_resized(feature_scale * feature_scale);
        if (!resize_img(F1, F1_size_x, F1_size_y, F1_resized.data(), feature_scale, feature_scale))
            exit(EXIT_FAILURE);

        TextureSampler<float> samplerF0(F0_resized.data(), grid_scale, grid_scale);
        TextureSampler<float> samplerF1(F1_resized.data(), feature_scale, feature_scale);

        //if (stbi_write_png(("F0_resized_" + std::to_string(it) + ".png").c_str(), feature_scale, feature_scale, 1, F0_resized.data(), 0) == 0)
        //    cout << "error writting:" << fname_in << endl;

        //if (stbi_write_png(("F1_resized_" + std::to_string(it) + ".png").c_str(), grid_scale, grid_scale, 1, F1_resized.data(), 0) == 0)
        //    cout << "error writting:" << fname_in << endl;

        vector<vector<vec2>> padded_warp_grid = get_padded_warpgrid(warp_grid);

        //saveGridImage(warp_grid, "warpgrid_" + std::to_string(grid_scale) + ".png", grid_scale);
        //saveGridImage(padded_warp_grid, "padded_warpgrid_" + std::to_string(grid_scale) + ".png", grid_scale + 2);

        vector<vector<vec2>> warpgrid_out = warp_grid; // copy the warpgrid to fix the other vertices during optimization

        for (int i = 0; i < grid_scale; i++)
        {
            for (int j = 0; j < grid_scale; j++)
            {
                float min_L2 = 10e9;
                vec2 coords = vec2(-1.0, -1.0);

                // neighborhood positions around (i,j) to compare feature map values
                //std::vector<vec2> F1_neighbors = get_positions(warp_grid[i][j], grid_scale);
                std::vector<vec2> F1_neighbors = get_positions(padded_warp_grid, i + 1, j + 1);

                // half-pixel offset
                float F0_val = samplerF0.sampleTexture((j + 0.5f) / float(grid_scale), (i + 0.5f) / float(grid_scale));

                // one ring neighborhood + current position
                for (auto n_coords : F1_neighbors)
                {
                    float F1_val = samplerF1.sampleTexture(n_coords[0], n_coords[1]);

                    // L2 deformation error
                    float matrix_norm = compute_frobenius_norm(padded_warp_grid, n_coords[0], n_coords[1], i + 1, j + 1);

                    // total L2 error : deformation + feature value errors
                    float L2_err = powf((F1_val - F0_val), 2.0f) + alpha * matrix_norm;

                    if (L2_err < min_L2)
                    {
                        min_L2 = L2_err;
                        coords = n_coords;
                    }
                }

                //if (i == 2 && j == 2)
                //{
                //    saveGridImage_test(F1_neighbors, warp_grid, "coucou.png", grid_scale);
                //}

                warpgrid_out[i][j] = coords;
            }
        }

        if (grid_scale < output_size)
        {
            warp_grid = upsample_warpgrid(warpgrid_out);
        }
        else // last pass consists only in grid warping
        {
            warp_grid = warpgrid_out;
            break;
        }

        grid_scale = 2 * grid_scale;

    }

    std::stringstream stream_tmp;
    stream_tmp << std::fixed << std::setprecision(2) << alpha;
    std::string alpha_str = stream_tmp.str();

    std::filesystem::path path1(fname_in);
    std::filesystem::path path2(fname_in_2);

    std::string name = "warp_TD_" + path1.stem().string() + "_" + path2.stem().string() + "_" + alpha_str + ".txt";
    write_warpgrid(name.c_str(), warp_grid);

    std::string namepng = "warp_TD_" + path1.stem().string() + "_" + path2.stem().string() + "_" + alpha_str + ".png";
    saveGridImage(warp_grid, namepng, grid_scale);

    stbi_image_free(F0);
    stbi_image_free(F1);

    return true;
}