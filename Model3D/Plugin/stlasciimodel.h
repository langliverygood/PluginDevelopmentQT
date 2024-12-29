#ifndef MODEL_H
#define MODEL_H

#include <QVector3D>
#include <QtCore>
#include <limits>

class STLTriangle {
public:
    STLTriangle();
    ~STLTriangle();

    void setVertex(const int& index, const QVector3D& point3D);
    QVector3D getVertex(const int& index);
    void setNormal(const float& nx, const float& ny, const float& nz);
    QVector3D getNormal();

    void reset();
    float getArea();
    float getTheta();

private:
    QVector3D v_[3];
    QVector3D n_;

    bool checkVertexIndex(const int& index);
};

class STLASCIIModel {
public:
    STLASCIIModel();
    STLASCIIModel(const QVector<STLTriangle>& stl_model);
    ~STLASCIIModel();

    bool isInitialized();
    void deInitialize();
    void reset();

    void setName(const QString& newName);
    QString getName();

    void addTriangle(STLTriangle t);
    STLTriangle getTriangle(const int& index);
    int getNTriangles();

    QVector3D getCenter();
    float xMin();
    float xMax();
    float yMin();
    float yMax();
    float zMin();
    float zMax();

private:
    QString name_;
    bool initialized_;
    QVector<STLTriangle> triangles_;

    float x_min_;
    float x_max_;
    float y_min_;
    float y_max_;
    float z_min_;
    float z_max_;
};

class STLParser {
public:
    STLParser();
    ~STLParser();

    STLASCIIModel parse(QFile& file);

private:
    enum parser_status_ { PARSE_OK, PARSE_FAILED };
};

#endif  // MODEL_H
