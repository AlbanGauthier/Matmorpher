#include "WarpIO.h"

void writeIntoFile(
    Eigen::VectorXd& X, 
    std::string material_names)
{
    int nb_pts = int(X.size() / 2); //xy
    std::ofstream myfile;
    std::string filename = "warp_" + material_names + ".txt";

    myfile.open(filename);
    if (myfile.is_open())
    {
        myfile << nb_pts << "\n";
        for (int i = 0; i < nb_pts; i++)
        {
            myfile << X[2 * i] << " " << X[2 * i + 1] << "\n";
        }
        myfile.close();
    }
    else
    {
        std::cerr << "could not open file to write in" << std::endl;
    }
}

void saveFeatureSetImage(
    std::vector<pointFeature> const& P, 
    std::string const& filename, 
    int width, int height,
    QImage::Format format) 
{
    QImage image(width, height, format);
    QPainter painter(&image);

    painter.fillRect(0, 0, width, height, QColor(255, 255, 255));


    for (pointFeature const& p : P) {
        QPen pen;
        pen.setWidth(1 + (p.cov_sigma[1] * p.cov_sigma[1]));
        pen.setColor(Qt::red);
        painter.setPen(pen);

        painter.drawPoint(width * p[0], height * p[1]);
    }

    painter.end();

    image.mirrored(false, true).save(QString::fromStdString(filename));
}

void saveGridImage(
    Eigen::VectorXd const& G, 
    std::string const& filename, 
    int N, int width, int height, 
    QImage::Format format) 
{
    QImage image(width, height, format);
    QPainter painter(&image);

    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.fillRect(0, 0, width, height, QColor(255, 255, 255));

    QPen pen;
    pen.setWidth(1);
    pen.setColor(Qt::red);
    painter.setPen(pen);

    for (int k = 0; k < N - 1; ++k) {
        for (int l = 0; l < N - 1; ++l) {
            vec2 p1(G[2 * (k + l * N)], G[2 * (k + l * N) + 1]);
            vec2 p2(G[2 * (k + 1 + l * N)], G[2 * (k + 1 + l * N) + 1]);
            vec2 p3(G[2 * (k + (l + 1) * N)], G[2 * (k + (l + 1) * N) + 1]);
            painter.drawLine(width * p1[0], height * p1[1], width * p2[0], height * p2[1]);
            painter.drawLine(width * p1[0], height * p1[1], width * p3[0], height * p3[1]);
        }
    }

    painter.end();

    image.mirrored(false, true).save(QString::fromStdString(filename));
}
