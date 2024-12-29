#include "stlasciimodel.h"

// ################################################
// ######### STLTriangle class desciption #########
// ################################################

STLTriangle::STLTriangle() {
    reset();
}

STLTriangle::~STLTriangle() {}

void STLTriangle::setVertex(const int& index, const QVector3D& point3D) {
    if (!checkVertexIndex(index)) {
        return;
    }
    v_[index] = point3D;
}

QVector3D STLTriangle::getVertex(const int& index) {
    if (!checkVertexIndex(index)) {
        return QVector3D();
    }
    return v_[index];
}

void STLTriangle::setNormal(const float& nx, const float& ny, const float& nz) {
    n_ = QVector3D(nx, ny, nz);
}

QVector3D STLTriangle::getNormal() {
    return n_;
}

void STLTriangle::reset() {
    n_ = QVector3D(0.f, 0.f, 0.f);
    for (int i = 0; i < 3; ++i) {
        v_[i] = QVector3D(0.f, 0.f, 0.f);
    }
}

float STLTriangle::getArea() {
    QVector3D AB = v_[0] - v_[1];
    QVector3D AC = v_[0] - v_[2];
    float area = 0.5 * QVector3D::crossProduct(AB, AC).length();
    return area;
}

float STLTriangle::getTheta() {
    if (fabs(n_.z()) < 0.0001) {
        return 0.;
    }
    float theta = qAtan(n_.z() / sqrt(n_.x() * n_.x() + n_.y() * n_.y()));
    return qRadiansToDegrees(theta);
}

bool STLTriangle::checkVertexIndex(const int& index) {
    if (index < 0 || index > 2) {
        qDebug() << "CRITICAL: invalid index provided to STLTriangle::SetVertex()!";
        return false;
    }
    return true;
}

// ################################################
// ######## STLASCIIModel class desciption ########
// ################################################

STLASCIIModel::STLASCIIModel() {
    reset();
}

STLASCIIModel::~STLASCIIModel() {}

bool STLASCIIModel::isInitialized() {
    return initialized_;
}

void STLASCIIModel::deInitialize() {
    initialized_ = false;
}

void STLASCIIModel::reset() {
    name_ = "";
    triangles_.clear();
    initialized_ = false;

    const float MAXIMUM_POSSIBLE = std::numeric_limits<float>::max();
    const float MINIMUM_POSSIBLE = std::numeric_limits<float>::min();
    x_min_ = MAXIMUM_POSSIBLE;
    x_max_ = MINIMUM_POSSIBLE;
    y_min_ = MAXIMUM_POSSIBLE;
    y_max_ = MINIMUM_POSSIBLE;
    z_min_ = MAXIMUM_POSSIBLE;
    z_max_ = MINIMUM_POSSIBLE;
}

void STLASCIIModel::setName(const QString& newName) {
    name_ = newName;
}

QString STLASCIIModel::getName() {
    return name_;
}

void STLASCIIModel::addTriangle(STLTriangle t) {
    if (!initialized_) {
        initialized_ = true;
    }

    QVector3D v;
    for (int i = 0; i < 3; ++i) {
        v = t.getVertex(i);
        if (v.x() < x_min_) x_min_ = v.x();
        if (v.x() > x_max_) x_max_ = v.x();
        if (v.y() < y_min_) y_min_ = v.y();
        if (v.y() > y_max_) y_max_ = v.y();
        if (v.z() < z_min_) z_min_ = v.z();
        if (v.z() > z_max_) z_max_ = v.z();
    }
    triangles_.push_back(t);
}

STLTriangle STLASCIIModel::getTriangle(const int& index) {
    return triangles_.at(index);
}

int STLASCIIModel::getNTriangles() {
    return triangles_.size();
}

QVector3D STLASCIIModel::getCenter() {
    return QVector3D((x_min_ + x_max_) / 2., (y_min_ + y_max_) / 2., (z_min_ + z_max_) / 2.);
}

float STLASCIIModel::xMin() {
    return x_min_;
}

float STLASCIIModel::xMax() {
    return x_max_;
}

float STLASCIIModel::yMin() {
    return y_min_;
}

float STLASCIIModel::yMax() {
    return y_max_;
}

float STLASCIIModel::zMin() {
    return z_min_;
}

float STLASCIIModel::zMax() {
    return z_max_;
}

// ################################################
// ########## STLParser class desciption ##########
// ################################################

STLParser::STLParser() {}

STLParser::~STLParser() {}

STLASCIIModel STLParser::parse(QFile& file) {
    STLASCIIModel m;
    STLTriangle t;
    parser_status_ status = PARSE_OK;
    int vertexCounter = 0;
    int linesCounter = 0;

    while (!file.atEnd() && status != PARSE_FAILED) {
        QString line = file.readLine().trimmed();
        QStringList parts = line.split(' ', QString::SkipEmptyParts);
        // skip blank liness
        if (parts.length() == 0) {
            continue;
        } else {
            ++linesCounter;
        }
        // check if first non-blank line starts properly
        if (linesCounter == 0 && !line.startsWith("solid")) {
            status = PARSE_FAILED;
            break;
        }
        // set model name
        if (parts[0] == "solid") {
            if (parts.length() > 2) {
                QStringList nameParts = parts;
                nameParts.removeFirst();
                m.setName(nameParts.join(' '));
            }
        }
        // start facet
        else if (line.startsWith("facet normal")) {
            if (parts.length() != 5) {
                status = PARSE_FAILED;
                break;
            }
            bool ok = true;
            float nx = parts[2].toFloat(&ok);
            float ny = parts[3].toFloat(&ok);
            float nz = parts[4].toFloat(&ok);
            if (!ok) {
                status = PARSE_FAILED;
                break;
            }
            t.reset();
            t.setNormal(nx, ny, nz);
        }
        // start outer loop
        else if (line.startsWith("outer loop")) {
            // TODO: set outer loop status
            vertexCounter = 0;
        }
        // new vertex
        else if (parts[0] == "vertex") {
            // TODO: check outer loop status
            if (parts.length() != 4) {
                status = PARSE_FAILED;
                continue;
            }
            bool ok = true;
            float vx = parts[1].toFloat(&ok);
            float vy = parts[2].toFloat(&ok);
            float vz = parts[3].toFloat(&ok);
            if (!ok) {
                status = PARSE_FAILED;
                break;
            }
            if (vertexCounter > 2) {
                status = PARSE_FAILED;
                break;
            }
            t.setVertex(vertexCounter++, QVector3D(vx, vy, vz));
        }
        // end outer loop
        else if (parts[0] == "endloop") {
            // TODO: check outer loop status
            if (parts.length() != 1) {
                status = PARSE_FAILED;
                break;
            }
        }
        // end outer loop
        else if (parts[0] == "endfacet") {
            // TODO: check outer loop status
            if (parts.length() != 1) {
                status = PARSE_FAILED;
                break;
            }
            m.addTriangle(t);
            t.reset();
        }
    }

    if (status == PARSE_FAILED) {
        m.deInitialize();
        qDebug() << "Something wrong with the model";
    }
    return m;
}
