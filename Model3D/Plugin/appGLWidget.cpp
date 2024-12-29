#include "appGLWidget.h"

AppGLWidget::AppGLWidget(QWidget *parent) : QOpenGLWidget(parent) {
    x_rot_ = -180;
    y_rot_ = 0;
    z_rot_ = 0;
    scale_ = 1.0f;
    s0_ = 1.001f;
}

AppGLWidget::~AppGLWidget() {}

void AppGLWidget::loadFile() {
    QString filename =
        QFileDialog::getOpenFileName(this, "Load ASCII .stl model", QString(), "STL Files (*.stl)");
    loadFile(filename);
}

void AppGLWidget::loadFile(const QString &filename) {
    if (!filename.isNull()) {
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Cannot open file";
            return;
        }
        STLParser parser;
        STLASCIIModel m = parser.parse(file);
        if (m.isInitialized()) {
            this->setModel(m);
            this->show();
            // FIXME: what if filename is veery long?
            status_text_ = filename + ": " + m.getName();
        } else {
            qDebug() << "Cannot read ASCII .stl file";
        }
    } else {
        qDebug() << "file does not exists";
        return;
    }
    QTimer *timer = new QTimer;
    connect(timer, &QTimer::timeout, [=] {
        z_rot_ -= 10;
        update();
    });
    timer->start(200);
}

void AppGLWidget::setModel(STLASCIIModel model) {
    model_ = model;
    resize(width(), height());
    update();
}

void AppGLWidget::initializeGL() {
    // Set up the rendering context, define display lists etc.:
    initializeOpenGLFunctions();

    //    float red = 248.0f / 255.0f, green = 251.0f / 255.0f, blue = 254.0f / 255.0f;
    //    float red = 20.0f / 255.0f, green = 20.0f / 255.0f, blue = 30.0f / 255.0f;
    //    glClearColor(red, green, blue, 1.0f);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    static GLfloat lightPosition[4] = {0.f, 0.f, 1.f, 0.f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    update();
}

void AppGLWidget::resizeGL(int w, int h) {
    // setup viewport, projection etc.:
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    int l = qMin(w, h);
    glViewport(0, 0, l, l);
    glMatrixMode(GL_MODELVIEW);
}

void AppGLWidget::paintGL() {
    // draw the scene:
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (model_.isInitialized()) {
        float wf = model_.xMax() - model_.xMin();
        float hf = model_.yMax() - model_.yMin();
        float df = model_.zMax() - model_.zMin();
        float diag = sqrt(wf * wf + hf * hf + df * df);
        glOrtho(-0.5 * diag, +0.5 * diag, -0.5 * diag, +0.5 * diag, 2.0, 2.0 + 2 * diag);
        glTranslatef(0.0f, 0.0f, -2.0f - diag);
    }

    glMatrixMode(GL_MODELVIEW);
    glScalef(scale_, scale_, scale_);
    glRotatef(x_rot_ / 2.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(y_rot_ / 2.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(z_rot_ / 2.0f, 0.0f, 0.0f, 1.0f);

    draw();
}

void AppGLWidget::draw() {
    // check if model has been initialized
    if (model_.isInitialized()) {
        drawModel();
    }
}

void AppGLWidget::drawModel() {
    glColor3b(1, 0, 0);
    QVector3D n;
    QVector3D v;
    STLTriangle t;
    QVector3D center = model_.getCenter();
    for (int i = 0; i < model_.getNTriangles(); ++i) {
        t = model_.getTriangle(i);
        glBegin(GL_TRIANGLES);
        n = t.getNormal();
        glNormal3f(n.x(), n.y(), n.z());
        for (size_t j = 0; j < 3; ++j) {
            v = t.getVertex(j);
            glVertex3f(v.x() - center.x(), v.y() - center.y(), v.z() - center.z());
        }
        glEnd();
    }
}

void AppGLWidget::mousePressEvent(QMouseEvent *event) {
    //    m_mouseLastPos = event->pos();
}

void AppGLWidget::mouseMoveEvent(QMouseEvent *event) {
    //    int dx = event->x() - m_mouseLastPos.x();
    //    int dy = event->y() - m_mouseLastPos.y();
    //    m_mouseLastPos = event->pos();

    //    m_xRot += dy;
    //    m_yRot += dx;
    //    update();
}

void AppGLWidget::wheelEvent(QWheelEvent *event) {
    scale_ *= 1 + event->delta() * (s0_ - 1);
    update();
}
