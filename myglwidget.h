#ifndef MYGLWIDGET_H
#define MYGLWIDGET_H

#include <QObject>
#include <QOpenGLWidget>
#include <QWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QFile>

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>

#define GRID_SIZE 3
#define PIXEL_DIM (GRID_SIZE+1)

//
// vertex bits
//

#define V0_BIT 0b0001
#define V1_BIT 0b0010
#define V2_BIT 0b0100
#define V3_BIT 0b1000


struct SrcVertex
{
    glm::vec2 v0;
    glm::vec2 v10;
    glm::vec2 N;
};

struct SrcPolygon
{
    SrcVertex vertices[4];
};

void SrcPolygonInitVertices(SrcPolygon *sp, glm::vec2 *vertices, glm::mat3 &M);
void SrcPolygonInitEdges(SrcPolygon *sp);
float SrcPolygonArea(SrcPolygon *sp);

struct PixelEdge {
    int code;
    int inside[2];
    glm::vec2 v[2];
};

struct Pixel {
    glm::vec2 v;
    int inside;
    PixelEdge xedge;
    PixelEdge yedge;
};

int f2BisectSrcPolygon(SrcPolygon *sp, glm::vec2 v);

struct BisectParams {
    glm::vec2 v[2];
    int inside[2];
    PixelEdge *edge;
};

void PixelEdgeBisectSrcPolygon(BisectParams *bp, SrcPolygon *sp);
void PixelEdgeBorderBisectSrcPolygon(BisectParams *bp, SrcPolygon *sp);

glm::vec2 f2IntersectionDelta(glm::vec2 a0, glm::vec2 a1, glm::vec2 b0, glm::vec2 b10);

glm::ivec2 convert_ivec2_plus(glm::vec2 v);

float f2cross(glm::vec2 &a, glm::vec2 &b);

struct PixelVertices{
    int Nvertices;
    int indices[4];
};

struct Polygon {
    int N;
    glm::vec2 v[10];
};

void PolygonAddVertex(Polygon *p, glm::vec2 &v);
float PolygonArea(Polygon *p);

void PixelVerticesAddVertex(PixelVertices *pv, int index);


class MyGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    MyGLWidget(QWidget *parent);
protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
private:
    float alpha;
    float dalpha;
    float theta;
    QTimer *timer;
    QFile  theta_file;
    bool theta_file_write;
    bool theta_file_open;
    SrcPolygon srcPolygon;
    void InitSrcPolygon(void);
    Pixel pixels[PIXEL_DIM][PIXEL_DIM];
    int  pixelVertices[GRID_SIZE][GRID_SIZE];
    Polygon polygon;
    int Npixelx;
    int Npixely;
    void InitPixels(void);
    void BisectEdges(void);
    void DrawPixelVertices(int flags, SrcPolygon *sp, Polygon *polygon);
    void DrawPolygons(void);
    void DrawSrcPolygon(void);
    void DrawGrid(void);
public slots:
    void timer_func(void);
};

#endif // MYGLWIDGET_H
