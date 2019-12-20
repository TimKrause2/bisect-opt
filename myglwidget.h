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
    int code; // the type of edge
    glm::vec2 v_ends[2]; // the ends of the edge
    int inside_ends[2];  // inside flags for the ends
    glm::vec2 v_edge[2]; // vertices inside the edge
    int inside_edge[2];  // inside flags for the vertices inside the edge
};

struct PixelVertex {
    glm::vec2 v; // the coordinate of the vertex
    int inside;  // the inside flags for the vertex
};

int f2BisectSrcPolygon(SrcPolygon *sp, glm::vec2 v);

void PixelEdgeBisectSrcPolygon(PixelEdge *pe, SrcPolygon *sp);
void PixelEdgeBorderBisectSrcPolygon(PixelEdge *pe, SrcPolygon *sp);

glm::vec2 f2IntersectionDelta(glm::vec2 a0, glm::vec2 a1, glm::vec2 b0, glm::vec2 b10);

glm::ivec2 convert_ivec2_plus(glm::vec2 v);

float f2cross(glm::vec2 &a, glm::vec2 &b);

struct Polygon {
    int N;
    glm::vec2 v[10];
};

void PolygonAddVertex(Polygon *p, glm::vec2 &v);
float PolygonArea(Polygon *p);

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
    PixelVertex pixelVertices[GRID_SIZE+1][GRID_SIZE+1];
    PixelEdge xEdges[GRID_SIZE+1][GRID_SIZE];
    PixelEdge yEdges[GRID_SIZE][GRID_SIZE+1];
    int  pixelVFlags[GRID_SIZE][GRID_SIZE];
    Polygon polygon;
    int Npixelx;
    int Npixely;
    void InitPixels(void);
    //void BisectEdges(void);
    void DrawPixelVertices(int flags, SrcPolygon *sp, Polygon *polygon);
    //void DrawPolygons(void);
    void BisectAndDrawPixels(void);
    void DrawSrcPolygon(void);
    void DrawGrid(void);
public slots:
    void timer_func(void);
};

#endif // MYGLWIDGET_H
