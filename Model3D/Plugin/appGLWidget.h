#ifndef AppGLWidget_H
#define AppGLWidget_H

#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QTimer>
#include <QtOpenGL>
#include <QtWidgets>

#include "stlasciimodel.h"

class AppGLWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit AppGLWidget(QWidget *parent = 0);
    ~AppGLWidget();

    void loadFile();
    void loadFile(const QString &filename);
    void setModel(STLASCIIModel model);

    QString statusText() const { return status_text_; }

protected:
    // opengl stuff
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);

    // user event handlers
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

private:
    void draw();
    void drawModel();

    STLASCIIModel model_;

    QPoint mouse_last_pos_;
    int x_rot_, y_rot_, z_rot_;
    float scale_, s0_;
    QString status_text_;
};

#endif  // AppGLWidget_H
