#pragma once

#include <iostream>
#include <fstream>

#include <QImage>
#include <QPainter>
#include <QPen>

#include "Eigen/SVD"
#include "Eigen/Geometry"

#include "glm/glm.hpp"

#include "WarpUtils.h"

void writeIntoFile(
    Eigen::VectorXd& X,
    std::string material_names);

void saveFeatureSetImage(
    std::vector<pointFeature> const& P,
    std::string const& filename,
    int width = 1024, int height = 1024,
    QImage::Format format = QImage::Format_RGB32);

void saveGridImage(
    Eigen::VectorXd const& G,
    std::string const& filename,
    int N, int width = 1024, int height = 1024,
    QImage::Format format = QImage::Format_RGB32);

template< class point_t >
void savePointSetImage(
    std::vector<point_t> const& P,
    std::string const& filename,
    QColor pointColor = QColor(255, 0, 0),
    int width = 1024,
    int height = 1024,
    QImage::Format format = QImage::Format_RGB32)
{
    QImage image(width, height, format);
    QPainter painter(&image);

    painter.fillRect(0, 0, width, height, QColor(255, 255, 255));

    QPen pen;
    pen.setWidth(1);
    pen.setColor(pointColor);
    painter.setPen(pen);

    for (const auto& p : P) {
        painter.drawPoint(width * p[0], height * p[1]);
    }

    painter.end();

    image.mirrored(false, true).save(QString::fromStdString(filename));
}

template< class point_t >
void savePointSetImageandNormals(
    std::vector<point_t> const& P,
    std::vector<point_t> const& P_normals,
    std::string const& filename,
    QColor pointColor = QColor(255, 0, 0),
    int width = 4096,
    int height = 4096,
    QImage::Format format = QImage::Format_RGB32)
{
    QImage image(width, height, format);
    QPainter painter(&image);

    painter.fillRect(0, 0, width, height, QColor(255, 255, 255));

    QPen pen;
    pen.setWidth(5);
    pen.setColor(pointColor);
    painter.setPen(pen);

    for (const auto& p : P) {
        painter.drawPoint(width * p[0], height * p[1]);
    }

    pen.setWidth(1);
    pen.setColor(QColor(0, 255, 0));
    painter.setPen(pen);
    int normal_idx = 0;
    for (const auto& p : P) {
        painter.drawLine(
            width * p[0], 
            height * p[1], 
            width * (p[0] + 0.01 * P_normals[normal_idx][0]), 
            height * (p[1] + 0.01 * P_normals[normal_idx][1]));
        normal_idx++;
    }

    painter.end();

    image.mirrored(false, true).save(QString::fromStdString(filename));
}
