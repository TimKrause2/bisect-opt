#include "myglwidget.h"
#include <math.h>
#include <stdlib.h>

MyGLWidget::MyGLWidget(QWidget *parent) :
    QOpenGLWidget(parent),
    theta_file("/tmp/theta.bin")
{
    setFocusPolicy(Qt::ClickFocus);
    grabKeyboard();

    alpha = 0.0f;
    dalpha = 0.1/60.0;
    advance_i_fail = false;
    timer = new QTimer(this);

    connect(timer, &QTimer::timeout, this, &MyGLWidget::timer_func);

    //
    // glitch transform
    //
    float theta_glitch = 45.0;
    theta_glitch *= M_PI/180.0;
    glm::vec2 A(127.0f/2.0f,-127.0f/2.0f);
    glm::mat3 M = glm::translate(glm::mat3(1.0f),A);
    M = glm::rotate(M, theta_glitch);
    M = glm::scale(M,glm::vec2(1.0f/3.0f,3.0f));
    M = glm::rotate(M,-theta_glitch);
    M = glm::translate(M,-A);

    M_inv = glm::inverse(M);

    EmulateTransform(128,128,M_inv);

    i_fail = 0;

    theta_file_open = false;
    if(theta_file.open(QIODevice::ReadOnly)){
        // check the size
        if(theta_file.size()>0){
            theta_file_write = false;
            theta_file_open = true;
            qDebug("theta_file open for reading.\n");
        }else{
            theta_file.close();
        }
    }
    if(!theta_file_open){
        // open for writing
        if(theta_file.open(QIODevice::WriteOnly)){
            theta_file_write = true;
            theta_file_open = true;
            qDebug("theta_file open for writing.\n");
        }
    }
    if(!theta_file_open){
        qDebug("theta_file not open\n");
    }

    timer->start(1000/60);

}

void MyGLWidget::initializeGL(){
    initializeOpenGLFunctions();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void MyGLWidget::resizeGL(int w, int h){
    glViewport(0,0,w,h);

    width = w;
    height = h;

}

void MyGLWidget::paintGL(){
    InitSrcPolygon();
    InitPixels();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLdouble aspect = (GLdouble)width/height;
    GLdouble size = (float)grid_size;
    if(aspect>=1.0){
        GLdouble left = -aspect*size/2 + size/2;
        GLdouble right = aspect*size/2 + size/2;
        GLdouble top = 0.0;
        GLdouble bottom = -size;
        GLdouble near = 1.0;
        GLdouble far = -1.0;
        glOrtho(left,right,bottom,top,near,far);
    }else{
        GLdouble left = 0.0;
        GLdouble right = 3.0;
        GLdouble top = 1.0/aspect*size/2 - size/2;
        GLdouble bottom = -1.0/aspect*size/2 - size/2;
        GLdouble near = 1.0;
        GLdouble far = -1.0;
        glOrtho(left,right,bottom,top,near,far);
    }
    glColor3f(0.25f,0.25f,0.25f);
    DrawGrid();

    glColor3f(0.5f,0.5f,0.5f);
    BisectAndDrawPixels();
    DrawSrcPolygon();
    alpha += dalpha;
    if(alpha>=1.0f)alpha-=1.0f;

}

void MyGLWidget::keyPressEvent(QKeyEvent *event)
{
    switch(event->key()){
    case Qt::Key_Space:
        advance_i_fail = true;
        break;
    default:
        QOpenGLWidget::keyPressEvent(event);
        break;
    }
}

void MyGLWidget::InitSrcPolygon()
{
    glm::vec2 vertices[4] = {
        glm::vec2(0.0f,0.0f),
        glm::vec2(0.0f,-1.0f),
        glm::vec2(1.0f,-1.0f),
        glm::vec2(1.0f,0.0f)
    };

    glm::mat3 M(1.0f);
    if(theta_file_open && !theta_file_write){
        theta_file.read((char*)&alpha,sizeof(alpha));
        if(theta_file.atEnd()){
            theta_file.seek(0);
        }
    }

    theta = 2.0*M_PI*alpha;

    //
    // shear and scaling test
    //
//    M = glm::translate(M,glm::vec2(0.0,alpha));
//    M = glm::shearX(M,1.0f);
//    M = glm::scale(M,glm::vec2(0.75f,1.1f));

    //
    // another shear and scaling test along the axis
    //
//    M = glm::translate(M,glm::vec2(alpha,0.0f));
//    M = glm::shearY(M,1.0f);
//    M = glm::scale(M,glm::vec2(1.1f,0.75f));

    //
    // interleaved vertices and edges test
    //
//    M = glm::translate(M,glm::vec2(0.5,-0.5));
//    M = glm::rotate(M,theta);
//    M = glm::scale(M,glm::vec2(3.0,1.0));
//    M = glm::rotate(M,(float)M_PI/4.0f);
//    M = glm::scale(M,glm::vec2(0.5,0.5));
//    M = glm::translate(M,glm::vec2(-0.5,0.5));

    //
    // rotate about the center of the pixel
    //
//    M = glm::translate(M,glm::vec2(0.5f,-0.5f));
//    M = glm::rotate(M,2.0f*(float)M_PI*alpha);
//    M = glm::translate(M,glm::vec2(-0.5f,0.5f));

    //
    // three vertices in one pixel test
    //
//    M = glm::translate(M, glm::vec2(0.25f,-0.5f));
//    M = glm::rotate(M,theta);
//    M = glm::scale(M,glm::vec2(0.5f,0.5f));
//    M = glm::translate(M, glm::vec2(-0.5f,0.5f));

    //
    // glitch transform
    //
    float theta_glitch = 90.0;
    theta_glitch *= M_PI/180.0;
    M = glm::translate(M,glm::vec2(alpha,0.0));
    M = glm::rotate(M, theta_glitch);
    M = glm::scale(M,glm::vec2(1.0f/3.0f,3.0f));
    M = glm::rotate(M,-theta_glitch);
    M = glm::inverse(M);

    if(fail_vector.empty()){
        SrcPolygonInitVertices(&srcPolygon, vertices, M);
    }else{
        glm::vec2 v2_src00 = fail_vector[i_fail];
        srcPolygon.vertices[0].v0 = v2_src00;
        srcPolygon.vertices[1].v0 = v2_src00 + v2_dsrcy;
        srcPolygon.vertices[2].v0 = v2_src00 + v2_dsrcy + v2_dsrcx;
        srcPolygon.vertices[3].v0 = v2_src00 + v2_dsrcx;
        if(advance_i_fail){
            i_fail++;
            if(i_fail==fail_vector.size())i_fail=0;
            advance_i_fail = false;
        }
    }

    SrcPolygonInitEdges(&srcPolygon);

}

void MyGLWidget::InitPixels()
{
    glm::ivec2 i2_src0 = convert_ivec2_plus(srcPolygon.vertices[0].v0);
    glm::ivec2 i2_src1 = convert_ivec2_plus(srcPolygon.vertices[1].v0);
    glm::ivec2 i2_src2 = convert_ivec2_plus(srcPolygon.vertices[2].v0);
    glm::ivec2 i2_src3 = convert_ivec2_plus(srcPolygon.vertices[3].v0);

    glm::ivec2 i2_min = glm::min(glm::min(i2_src0,i2_src1),glm::min(i2_src2,i2_src3));
    glm::ivec2 i2_max = glm::max(glm::max(i2_src0,i2_src1),glm::max(i2_src2,i2_src3));

    Npixelx = i2_max.x - i2_min.x + 1;
    Npixely = i2_max.y - i2_min.y + 1;

    glm::ivec2 i2_v0(i2_min.x,i2_max.y);
    glm::vec2 v0 = i2_v0;

    grid_size = (Npixelx>Npixely)?Npixelx:Npixely;
    if(grid_size<3) grid_size = 3;

    if(Npixelx==1 && Npixely==1){
        pixelVertices[0][0].v = v0;
        return;
    }

    PixelVertex *pixelVertex = &pixelVertices[0][0];
    int x;
    int y;
    for(y=0;y<Npixely+1;y++){
        glm::vec2 v = v0;
        for(x=0;x<Npixelx+1;x++,pixelVertex++){
            pixelVertex->v = v;
            pixelVertex->inside = f2BisectSrcPolygon(&srcPolygon, v);
            v+=glm::vec2(1.0f,0.0f);
        }
        // move the pointer to the next line
        pixelVertex+=(GRID_SIZE+1) - x;
        v0+=glm::vec2(0.0f,-1.0f);
    }
    // initialize the pixel vertex flags to zero
    int *pixelVFlag = &pixelVFlags[0][0];
    for(int y=0;y<Npixely;y++){
        int x;
        for(x=0;x<Npixelx;x++,pixelVFlag++){
            *pixelVFlag = 0;
        }
        pixelVFlag += GRID_SIZE - x;
    }
    // deposit the vertices into the pixels
    pixelVFlags[i2_v0.y-i2_src0.y][i2_src0.x-i2_v0.x] |= V0_BIT;
    pixelVFlags[i2_v0.y-i2_src1.y][i2_src1.x-i2_v0.x] |= V1_BIT;
    pixelVFlags[i2_v0.y-i2_src2.y][i2_src2.x-i2_v0.x] |= V2_BIT;
    pixelVFlags[i2_v0.y-i2_src3.y][i2_src3.x-i2_v0.x] |= V3_BIT;
}

void MyGLWidget::BisectAndDrawPixels(void){
    PixelVertex *pixel00 = &pixelVertices[0][0];
    PixelVertex *pixel10 = &pixelVertices[0][1];
    PixelVertex *pixel01 = &pixelVertices[1][0];
    PixelVertex *pixel11 = &pixelVertices[1][1];
    PixelEdge *xEdgeTop = &xEdges[0][0];
    PixelEdge *xEdgeBottom = &xEdges[1][0];
    PixelEdge *yEdgeLeft = &yEdges[0][0];
    PixelEdge *yEdgeRight = &yEdges[0][1];
    glm::vec2 origin = pixelVertices[0][0].v;
    glm::vec2 v;
    int x;
    int y;
    GLfloat even_colors[3]={0.5f,0.5f,0.25f};
    GLfloat odd_colors[3]={0.25f,0.25f,0.5f};
    if(Npixelx==1 && Npixely==1){
        // source polygon is completely within pixel (0,0)
        // draw the vertices of the source polygon
        glColor3fv(even_colors);
        glBegin(GL_POLYGON);
        v = srcPolygon.vertices[0].v0 - origin;
        glVertex2f(v.x,v.y);
        v = srcPolygon.vertices[1].v0 - origin;
        glVertex2f(v.x,v.y);
        v = srcPolygon.vertices[2].v0 - origin;
        glVertex2f(v.x,v.y);
        v = srcPolygon.vertices[3].v0 - origin;
        glVertex2f(v.x,v.y);
        glEnd();
        return;
    }
    float src_area = SrcPolygonArea(&srcPolygon);
    float total_area = 0.0f;
    for(y=0;y<Npixely;y++){
        for(x=0;x<Npixelx;x++,pixel00++,pixel01++,pixel10++,pixel11++,xEdgeTop++,
            xEdgeBottom++,yEdgeLeft++,yEdgeRight++){
            //
            // bisect the new edges
            //
            // test for the top of the grid
            //
            if(y==0){
                xEdgeTop->code = 0;
                xEdgeTop->v_ends[0] = pixel00->v;
                xEdgeTop->inside_ends[0] = pixel00->inside;
                xEdgeTop->v_ends[1] = pixel10->v;
                xEdgeTop->inside_ends[1] = pixel10->inside;
                PixelEdgeBorderBisectSrcPolygon(xEdgeTop,&srcPolygon);
            }
            //
            // test for the left most edge
            //
            if(x==0){
                yEdgeLeft->code = 0;
                yEdgeLeft->v_ends[0] = pixel00->v;
                yEdgeLeft->inside_ends[0] = pixel00->inside;
                yEdgeLeft->v_ends[1] = pixel01->v;
                yEdgeLeft->inside_ends[1] = pixel01->inside;
                PixelEdgeBorderBisectSrcPolygon(yEdgeLeft,&srcPolygon);
            }
            //
            // bisect the fresh bottom edge
            //
            xEdgeBottom->code = 0;
            xEdgeBottom->v_ends[0] = pixel01->v;
            xEdgeBottom->inside_ends[0] = pixel01->inside;
            xEdgeBottom->v_ends[1] = pixel11->v;
            xEdgeBottom->inside_ends[1] = pixel11->inside;
            if(y<(Npixely-1)){
                PixelEdgeBisectSrcPolygon(xEdgeBottom,&srcPolygon);
            }else{
                PixelEdgeBorderBisectSrcPolygon(xEdgeBottom,&srcPolygon);
            }
            //
            // initialize the right edge
            //
            yEdgeRight->code = 0;
            yEdgeRight->v_ends[0] = pixel10->v;
            yEdgeRight->inside_ends[0] = pixel10->inside;
            yEdgeRight->v_ends[1] = pixel11->v;
            yEdgeRight->inside_ends[1] = pixel11->inside;
            if(x<(Npixelx-1)){
                PixelEdgeBisectSrcPolygon(yEdgeRight,&srcPolygon);
            }else{
                PixelEdgeBorderBisectSrcPolygon(yEdgeRight,&srcPolygon);
            }
            //
            // now draw the pixel
            //
            if(y&1){
                if(x&1){
                    glColor3fv(even_colors);
                }else{
                    glColor3fv(odd_colors);
                }
            }else{
                if(x&1){
                    glColor3fv(odd_colors);
                }else{
                    glColor3fv(even_colors);
                }
            }
            polygon.N = 0;
            int pixelVFlag = pixelVFlags[y][x];
            if(pixelVFlag==0b0001 || pixelVFlag==0b0010
                    || pixelVFlag==0b0100 || pixelVFlag==0b1000
                    || pixelVFlag==0b0101 || pixelVFlag==0b1010){
                //
                // single vertex in the pixel
                //
                PolygonAddEdgeSingleVertexForward(&polygon,yEdgeLeft,pixelVFlag,&srcPolygon);
                PolygonAddEdgeSingleVertexForward(&polygon,xEdgeBottom,pixelVFlag,&srcPolygon);
                PolygonAddEdgeSingleVertexReverse(&polygon,yEdgeRight,pixelVFlag,&srcPolygon);
                PolygonAddEdgeSingleVertexReverse(&polygon,xEdgeTop,pixelVFlag,&srcPolygon);
            }else{
                // vertices in the pixel are drawn all at once
                bool iv_drawn = false;
                //
                // draw the left edge
                //
                switch(yEdgeLeft->code){
                case 0:
                    if(yEdgeLeft->inside_ends[0]==0b1111){
                        PolygonAddVertex(&polygon,yEdgeLeft->v_ends[0]);
                    }else{
                        PolygonAddMultiVFlag(&polygon, pixelVFlag, &srcPolygon);
                        iv_drawn = true;
                    }
                    break;
                case 1:
                    PolygonAddVertex(&polygon,yEdgeLeft->v_ends[0]);
                    PolygonAddVertex(&polygon,yEdgeLeft->v_edge[1]);
                    break;
                case 2:
                    PolygonAddVertex(&polygon,yEdgeLeft->v_edge[0]);
                    break;
                case 3:
                    PolygonAddVertex(&polygon,yEdgeLeft->v_edge[0]);
                    PolygonAddVertex(&polygon,yEdgeLeft->v_edge[1]);
                    break;
                }
                //
                // draw the bottom edge
                //
                switch(xEdgeBottom->code){
                case 0:
                    if(xEdgeBottom->inside_ends[0]==0b1111){
                        PolygonAddVertex(&polygon,xEdgeBottom->v_ends[0]);
                    }else{
                        if(!iv_drawn){
                            PolygonAddMultiVFlag(&polygon, pixelVFlag, &srcPolygon);
                            iv_drawn = true;
                        }
                    }
                    break;
                case 1:
                    PolygonAddVertex(&polygon,xEdgeBottom->v_ends[0]);
                    PolygonAddVertex(&polygon,xEdgeBottom->v_edge[1]);
                    break;
                case 2:
                    PolygonAddVertex(&polygon,xEdgeBottom->v_edge[0]);
                    break;
                case 3:
                    PolygonAddVertex(&polygon,xEdgeBottom->v_edge[0]);
                    PolygonAddVertex(&polygon,xEdgeBottom->v_edge[1]);
                    break;
                }
                //
                // draw the right edge - it's in the reverse direction to
                // the drawing order
                //
                switch(yEdgeRight->code){
                case 0:
                    if(yEdgeRight->inside_ends[1]==0b1111){
                        PolygonAddVertex(&polygon,yEdgeRight->v_ends[1]);
                    }else{
                        if(!iv_drawn){
                            PolygonAddMultiVFlag(&polygon, pixelVFlag, &srcPolygon);
                            iv_drawn = true;
                        }
                    }
                    break;
                case 1:
                    PolygonAddVertex(&polygon,yEdgeRight->v_edge[1]);
                    break;
                case 2:
                    PolygonAddVertex(&polygon,yEdgeRight->v_ends[1]);
                    PolygonAddVertex(&polygon,yEdgeRight->v_edge[0]);
                    break;
                case 3:
                    PolygonAddVertex(&polygon,yEdgeRight->v_edge[1]);
                    PolygonAddVertex(&polygon,yEdgeRight->v_edge[0]);
                    break;
                }
                //
                // draw the top edge - it's in the reverse direction
                // as well
                //
                switch(xEdgeTop->code){
                case 0:
                    if(xEdgeTop->inside_ends[1]==0b1111){
                        PolygonAddVertex(&polygon,xEdgeTop->v_ends[1]);
                    }else{
                        if(!iv_drawn){
                            PolygonAddMultiVFlag(&polygon, pixelVFlag, &srcPolygon);
                            iv_drawn = true;
                        }
                    }
                    break;
                case 1:
                    PolygonAddVertex(&polygon,xEdgeTop->v_edge[1]);
                    break;
                case 2:
                    PolygonAddVertex(&polygon,xEdgeTop->v_ends[1]);
                    PolygonAddVertex(&polygon,xEdgeTop->v_edge[0]);
                    break;
                case 3:
                    PolygonAddVertex(&polygon,xEdgeTop->v_edge[1]);
                    PolygonAddVertex(&polygon,xEdgeTop->v_edge[0]);
                    break;
                }

            }
            DrawPolygon();
            total_area += PolygonArea(&polygon);
        }
        //
        // advance the pointers to the next line
        //
        int offset = (GRID_SIZE+1) - x;
        pixel00 += offset;
        pixel01 += offset;
        pixel10 += offset;
        pixel11 += offset;
        yEdgeLeft += offset;
        yEdgeRight += offset;
        offset = GRID_SIZE - x;
        xEdgeTop += offset;
        xEdgeBottom += offset;
    }
    float area_error = (total_area - src_area)/src_area;
    if(fabsf(area_error)>0.05f){
        qDebug("area_error:%f",area_error);
//        if(theta_file.openMode()==QIODevice::WriteOnly){
//            theta_file.write((char*)&alpha,sizeof(alpha));
//        }
    }
}

bool MyGLWidget::BisectAndVerifyPixels()
{
    PixelVertex *pixel00 = &pixelVertices[0][0];
    PixelVertex *pixel10 = &pixelVertices[0][1];
    PixelVertex *pixel01 = &pixelVertices[1][0];
    PixelVertex *pixel11 = &pixelVertices[1][1];
    PixelEdge *xEdgeTop = &xEdges[0][0];
    PixelEdge *xEdgeBottom = &xEdges[1][0];
    PixelEdge *yEdgeLeft = &yEdges[0][0];
    PixelEdge *yEdgeRight = &yEdges[0][1];
    int x;
    int y;
    if(Npixelx==1 && Npixely==1){
        return true;
    }
    float src_area = SrcPolygonArea(&srcPolygon);
    float total_area = 0.0f;
    for(y=0;y<Npixely;y++){
        for(x=0;x<Npixelx;x++,pixel00++,pixel01++,pixel10++,pixel11++,xEdgeTop++,
            xEdgeBottom++,yEdgeLeft++,yEdgeRight++){
            //
            // bisect the new edges
            //
            // test for the top of the grid
            //
            if(y==0){
                xEdgeTop->code = 0;
                xEdgeTop->v_ends[0] = pixel00->v;
                xEdgeTop->inside_ends[0] = pixel00->inside;
                xEdgeTop->v_ends[1] = pixel10->v;
                xEdgeTop->inside_ends[1] = pixel10->inside;
                PixelEdgeBorderBisectSrcPolygon(xEdgeTop,&srcPolygon);
            }
            //
            // test for the left most edge
            //
            if(x==0){
                yEdgeLeft->code = 0;
                yEdgeLeft->v_ends[0] = pixel00->v;
                yEdgeLeft->inside_ends[0] = pixel00->inside;
                yEdgeLeft->v_ends[1] = pixel01->v;
                yEdgeLeft->inside_ends[1] = pixel01->inside;
                PixelEdgeBorderBisectSrcPolygon(yEdgeLeft,&srcPolygon);
            }
            //
            // bisect the fresh bottom edge
            //
            xEdgeBottom->code = 0;
            xEdgeBottom->v_ends[0] = pixel01->v;
            xEdgeBottom->inside_ends[0] = pixel01->inside;
            xEdgeBottom->v_ends[1] = pixel11->v;
            xEdgeBottom->inside_ends[1] = pixel11->inside;
            if(y<(Npixely-1)){
                PixelEdgeBisectSrcPolygon(xEdgeBottom,&srcPolygon);
            }else{
                PixelEdgeBorderBisectSrcPolygon(xEdgeBottom,&srcPolygon);
            }
            //
            // initialize the right edge
            //
            yEdgeRight->code = 0;
            yEdgeRight->v_ends[0] = pixel10->v;
            yEdgeRight->inside_ends[0] = pixel10->inside;
            yEdgeRight->v_ends[1] = pixel11->v;
            yEdgeRight->inside_ends[1] = pixel11->inside;
            if(x<(Npixelx-1)){
                PixelEdgeBisectSrcPolygon(yEdgeRight,&srcPolygon);
            }else{
                PixelEdgeBorderBisectSrcPolygon(yEdgeRight,&srcPolygon);
            }
            //
            // now create the polygon for this pixel
            //
            polygon.N = 0;
            int pixelVFlag = pixelVFlags[y][x];
            if(pixelVFlag==0b0001 || pixelVFlag==0b0010
                    || pixelVFlag==0b0100 || pixelVFlag==0b1000
                    || pixelVFlag==0b0101 || pixelVFlag==0b1010){
                //
                // single vertex in the pixel
                //
                PolygonAddEdgeSingleVertexForward(&polygon,yEdgeLeft,pixelVFlag,&srcPolygon);
                PolygonAddEdgeSingleVertexForward(&polygon,xEdgeBottom,pixelVFlag,&srcPolygon);
                PolygonAddEdgeSingleVertexReverse(&polygon,yEdgeRight,pixelVFlag,&srcPolygon);
                PolygonAddEdgeSingleVertexReverse(&polygon,xEdgeTop,pixelVFlag,&srcPolygon);
            }else{
                // vertices in the pixel are drawn all at once
                bool iv_drawn = false;
                //
                // draw the left edge
                //
                switch(yEdgeLeft->code){
                case 0:
                    if(yEdgeLeft->inside_ends[0]==0b1111){
                        PolygonAddVertex(&polygon,yEdgeLeft->v_ends[0]);
                    }else{
                        PolygonAddMultiVFlag(&polygon, pixelVFlag, &srcPolygon);
                        iv_drawn = true;
                    }
                    break;
                case 1:
                    PolygonAddVertex(&polygon,yEdgeLeft->v_ends[0]);
                    PolygonAddVertex(&polygon,yEdgeLeft->v_edge[1]);
                    break;
                case 2:
                    PolygonAddVertex(&polygon,yEdgeLeft->v_edge[0]);
                    break;
                case 3:
                    PolygonAddVertex(&polygon,yEdgeLeft->v_edge[0]);
                    PolygonAddVertex(&polygon,yEdgeLeft->v_edge[1]);
                    break;
                }
                //
                // draw the bottom edge
                //
                switch(xEdgeBottom->code){
                case 0:
                    if(xEdgeBottom->inside_ends[0]==0b1111){
                        PolygonAddVertex(&polygon,xEdgeBottom->v_ends[0]);
                    }else{
                        if(!iv_drawn){
                            PolygonAddMultiVFlag(&polygon, pixelVFlag, &srcPolygon);
                            iv_drawn = true;
                        }
                    }
                    break;
                case 1:
                    PolygonAddVertex(&polygon,xEdgeBottom->v_ends[0]);
                    PolygonAddVertex(&polygon,xEdgeBottom->v_edge[1]);
                    break;
                case 2:
                    PolygonAddVertex(&polygon,xEdgeBottom->v_edge[0]);
                    break;
                case 3:
                    PolygonAddVertex(&polygon,xEdgeBottom->v_edge[0]);
                    PolygonAddVertex(&polygon,xEdgeBottom->v_edge[1]);
                    break;
                }
                //
                // draw the right edge - it's in the reverse direction to
                // the drawing order
                //
                switch(yEdgeRight->code){
                case 0:
                    if(yEdgeRight->inside_ends[1]==0b1111){
                        PolygonAddVertex(&polygon,yEdgeRight->v_ends[1]);
                    }else{
                        if(!iv_drawn){
                            PolygonAddMultiVFlag(&polygon, pixelVFlag, &srcPolygon);
                            iv_drawn = true;
                        }
                    }
                    break;
                case 1:
                    PolygonAddVertex(&polygon,yEdgeRight->v_edge[1]);
                    break;
                case 2:
                    PolygonAddVertex(&polygon,yEdgeRight->v_ends[1]);
                    PolygonAddVertex(&polygon,yEdgeRight->v_edge[0]);
                    break;
                case 3:
                    PolygonAddVertex(&polygon,yEdgeRight->v_edge[1]);
                    PolygonAddVertex(&polygon,yEdgeRight->v_edge[0]);
                    break;
                }
                //
                // draw the top edge - it's in the reverse direction
                // as well
                //
                switch(xEdgeTop->code){
                case 0:
                    if(xEdgeTop->inside_ends[1]==0b1111){
                        PolygonAddVertex(&polygon,xEdgeTop->v_ends[1]);
                    }else{
                        if(!iv_drawn){
                            PolygonAddMultiVFlag(&polygon, pixelVFlag, &srcPolygon);
                            iv_drawn = true;
                        }
                    }
                    break;
                case 1:
                    PolygonAddVertex(&polygon,xEdgeTop->v_edge[1]);
                    break;
                case 2:
                    PolygonAddVertex(&polygon,xEdgeTop->v_ends[1]);
                    PolygonAddVertex(&polygon,xEdgeTop->v_edge[0]);
                    break;
                case 3:
                    PolygonAddVertex(&polygon,xEdgeTop->v_edge[1]);
                    PolygonAddVertex(&polygon,xEdgeTop->v_edge[0]);
                    break;
                }

            }
            total_area += PolygonArea(&polygon);
        }
        //
        // advance the pointers to the next line
        //
        int offset = (GRID_SIZE+1) - x;
        pixel00 += offset;
        pixel01 += offset;
        pixel10 += offset;
        pixel11 += offset;
        yEdgeLeft += offset;
        yEdgeRight += offset;
        offset = GRID_SIZE - x;
        xEdgeTop += offset;
        xEdgeBottom += offset;
    }
    float area_error = (total_area - src_area)/src_area;
    if(fabsf(area_error)>0.05f){
        qDebug("verify area_error:%f",area_error);
        return false;
    }
    return true;
}

glm::vec2 v2conform_axis(glm::vec2 v){
    glm::vec2 v_abs = glm::abs(v);
    if(v_abs.x>=v_abs.y){
        float tan_theta = v_abs.y/v_abs.x;
        if(tan_theta < 1e-5){
            v_abs.y=0;
        }
    }else{
        float tan_theta = v_abs.x/v_abs.y;
        if(tan_theta < 1e-5){
            v_abs.x=0;
        }
    }
    return v_abs * glm::sign(v);
}


void MyGLWidget::EmulateTransform(int width, int height, glm::mat3 &M_inv)
{
    glm::vec3 v3_dx(1.0f,0.0f,0.0f);
    glm::vec3 v3_dy(0.0f,-1.0f,0.0f);
    v2_dsrcx = v2conform_axis(glm::vec2(M_inv*v3_dx));
    v2_dsrcy = v2conform_axis(glm::vec2(M_inv*v3_dy));

    for(int y=0;y<height;y++){
        glm::vec3 v3_y(0.0f,-(float)y,1.0f);
        glm::vec2 v2_src00(M_inv*v3_y);
        for(int x=0;x<width;x++,v2_src00+=v2_dsrcx){
            srcPolygon.vertices[0].v0 = v2_src00;
            srcPolygon.vertices[1].v0 = v2_src00 + v2_dsrcy;
            srcPolygon.vertices[2].v0 = v2_src00 + v2_dsrcy + v2_dsrcx;
            srcPolygon.vertices[3].v0 = v2_src00 + v2_dsrcx;

            SrcPolygonInitEdges(&srcPolygon);
            InitPixels();
            if(!BisectAndVerifyPixels()){
                qDebug("pixel failed x:%d y:%d",x,y);
                fail_vector.push_back(v2_src00);
            }
        }
    }
}


void MyGLWidget::PolygonAddSingleVFlag(Polygon *polygon, int flags, SrcPolygon *sp)
{
    switch(flags){
    case 0b0001:
        PolygonAddVertex(polygon,sp->vertices[0].v0);
        return;
    case 0b0010:
        PolygonAddVertex(polygon,sp->vertices[1].v0);
        return;
    case 0b0100:
        PolygonAddVertex(polygon,sp->vertices[2].v0);
        return;
    case 0b1000:
        PolygonAddVertex(polygon,sp->vertices[3].v0);
        return;
    }
}

void MyGLWidget::PolygonAddMultiVFlag(Polygon *polygon, int vflag, SrcPolygon *sp){
    switch(vflag){
    case 0b0000:
        return;
    case 0b0011:
        PolygonAddVertex(polygon,sp->vertices[0].v0);
        PolygonAddVertex(polygon,sp->vertices[1].v0);
        return;
    case 0b0110:
        PolygonAddVertex(polygon,sp->vertices[1].v0);
        PolygonAddVertex(polygon,sp->vertices[2].v0);
        return;
    case 0b0111:
        PolygonAddVertex(polygon,sp->vertices[0].v0);
        PolygonAddVertex(polygon,sp->vertices[1].v0);
        PolygonAddVertex(polygon,sp->vertices[2].v0);
        return;
    case 0b1001:
        PolygonAddVertex(polygon,sp->vertices[3].v0);
        PolygonAddVertex(polygon,sp->vertices[0].v0);
        return;
    case 0b1011:
        PolygonAddVertex(polygon,sp->vertices[3].v0);
        PolygonAddVertex(polygon,sp->vertices[0].v0);
        PolygonAddVertex(polygon,sp->vertices[1].v0);
        return;
    case 0b1100:
        PolygonAddVertex(polygon,sp->vertices[2].v0);
        PolygonAddVertex(polygon,sp->vertices[3].v0);
        return;
    case 0b1101:
        PolygonAddVertex(polygon,sp->vertices[2].v0);
        PolygonAddVertex(polygon,sp->vertices[3].v0);
        PolygonAddVertex(polygon,sp->vertices[0].v0);
        return;
    case 0b1110:
        PolygonAddVertex(polygon,sp->vertices[1].v0);
        PolygonAddVertex(polygon,sp->vertices[2].v0);
        PolygonAddVertex(polygon,sp->vertices[3].v0);
        return;
    }
}


void MyGLWidget::PolygonAddEdgeForward(Polygon *polygon, PixelEdge *edge)
{
    switch(edge->code){
    case 0:
        if(edge->inside_ends[0]==0b1111){
            PolygonAddVertex(polygon,edge->v_ends[0]);
        }
        break;
    case 1:
        PolygonAddVertex(polygon,edge->v_ends[0]);
        PolygonAddVertex(polygon,edge->v_edge[1]);
        break;
    case 2:
        PolygonAddVertex(polygon,edge->v_edge[0]);
        break;
    case 3:
        PolygonAddVertex(polygon,edge->v_edge[0]);
        PolygonAddVertex(polygon,edge->v_edge[1]);
        break;
    }
}

void MyGLWidget::PolygonAddEdgeReverse(Polygon *polygon, PixelEdge *edge)
{
    switch(edge->code){
    case 0:
        if(edge->inside_ends[1]==0b1111){
            PolygonAddVertex(polygon,edge->v_ends[1]);
        }
        break;
    case 1:
        PolygonAddVertex(polygon,edge->v_edge[1]);
        break;
    case 2:
        PolygonAddVertex(polygon,edge->v_ends[1]);
        PolygonAddVertex(polygon,edge->v_edge[0]);
        break;
    case 3:
        PolygonAddVertex(polygon,edge->v_edge[1]);
        PolygonAddVertex(polygon,edge->v_edge[0]);
        break;
    }
}

void MyGLWidget::PolygonAddEdgeSingleVertexForward(Polygon *polygon, PixelEdge *edge, int vflag, SrcPolygon *sp)
{
    switch(edge->code){
    case 0:
        if(edge->inside_ends[0]==0b1111){
            PolygonAddVertex(polygon,edge->v_ends[0]);
        }
        break;
    case 1:
        PolygonAddVertex(polygon,edge->v_ends[0]);
        PolygonAddVertex(polygon,edge->v_edge[1]);
        break;
    case 2:
        vflag &= edge->vflag_edge[0];
        if(vflag){
            PolygonAddSingleVFlag(polygon, vflag, sp);
        }
        PolygonAddVertex(polygon,edge->v_edge[0]);
        break;
    case 3:
        vflag &= edge->vflag_edge[0];
        if(vflag){
            PolygonAddSingleVFlag(polygon, vflag, sp);
        }
        PolygonAddVertex(polygon,edge->v_edge[0]);
        PolygonAddVertex(polygon,edge->v_edge[1]);
        break;
    }
}

void MyGLWidget::PolygonAddEdgeSingleVertexReverse(Polygon *polygon, PixelEdge *edge, int vflag, SrcPolygon *sp)
{
    switch(edge->code){
    case 0:
        if(edge->inside_ends[1]==0b1111){
            PolygonAddVertex(polygon,edge->v_ends[1]);
        }
        break;
    case 1:
        vflag &= edge->vflag_edge[1];
        if(vflag){
            PolygonAddSingleVFlag(polygon, vflag, sp);
        }
        PolygonAddVertex(polygon,edge->v_edge[1]);
        break;
    case 2:
        PolygonAddVertex(polygon,edge->v_ends[1]);
        PolygonAddVertex(polygon,edge->v_edge[0]);
        break;
    case 3:
        vflag &= edge->vflag_edge[1];
        if(vflag){
            PolygonAddSingleVFlag(polygon, vflag, sp);
        }
        PolygonAddVertex(polygon,edge->v_edge[1]);
        PolygonAddVertex(polygon,edge->v_edge[0]);
        break;
    }
}

/*
void MyGLWidget::DrawPolygons()
{
    glm::vec2 origin = pixels[0][0].v;
    glm::vec2 v;
    if(Npixelx==1 && Npixely==1){
        // source is completely inside this pixel
        // just render the vertices in the pixel
        glBegin(GL_POLYGON);
        for(int i=0;i<4;i++){
            v = srcPolygon.vertices[i].v0 - origin;
            glVertex2f(v.x, v.y);
        }
        glEnd();
        return;
    }

    Pixel *pixel00 = &pixels[0][0];
    Pixel *pixel10 = &pixels[0][1];
    Pixel *pixel01 = &pixels[1][0];
    Pixel *pixel11 = &pixels[1][1];
    float total_area = 0.0f;
    float src_area = SrcPolygonArea(&srcPolygon);
    for(int y=0;y<Npixely;y++){
        for(int x=0;x<Npixelx;x++,pixel00++,pixel10++,pixel01++,pixel11++){
            // first test for the trivial case
            // test for all outside
            if(
                    (~pixel00->inside)&
                    (~pixel10->inside)&
                    (~pixel11->inside)&
                    (~pixel01->inside)&
                    0b1111){
                // this pixel is completely outside of
                continue;
            }

            // test for vertices in the pixel
            int v_flags = pixelVertices[y][x];
            if(v_flags){
                if(v_flags==0b0101){
                    // inside vertices need to be drawn interleaved with the
                    // pixel edges
                    //
                    // not handled for now
                    continue;
                }else if(v_flags==0b1010){
                    // inside vertices are interleaved with the pixel edges
                    //
                    // not handled for now
                    continue;
                }
            }

            // the rest of the cases are handled
            bool iv_drawn = false; // interior vertices drawn
            polygon.N = 0;
            glBegin(GL_POLYGON);
            // draw the left yaxis
            switch(pixel00->yedge.code){
            case 0:
                if(pixel00->inside==0b1111){
                    v = pixel00->v - origin;
                    glVertex2f(v.x,v.y);
                    PolygonAddVertex(&polygon,v);
                }else{
                    if(!iv_drawn){
                        // draw the vertices in the pixel
                        DrawPixelVertices(v_flags,&srcPolygon,&polygon);
                        iv_drawn = true;
                    }
                }
                break;
            case 1:
                v = pixel00->v - origin;
                glVertex2f(v.x,v.y);
                PolygonAddVertex(&polygon,v);
                v = pixel00->yedge.v[1] - origin;
                glVertex2f(v.x,v.y);
                PolygonAddVertex(&polygon,v);
                break;
            case 2:
                v = pixel00->yedge.v[0] - origin;
                glVertex2f(v.x,v.y);
                PolygonAddVertex(&polygon,v);
                break;
            case 3:
                v = pixel00->yedge.v[0] - origin;
                glVertex2f(v.x,v.y);
                PolygonAddVertex(&polygon,v);
                v = pixel00->yedge.v[1] - origin;
                glVertex2f(v.x,v.y);
                PolygonAddVertex(&polygon,v);
                break;
            }
            // draw the bottom xaxis
            switch(pixel01->xedge.code){
            case 0:
                if(pixel01->inside==0b1111){
                    v = pixel01->v - origin;
                    glVertex2f(v.x,v.y);
                    PolygonAddVertex(&polygon,v);
                }else{
                    if(!iv_drawn){
                        // draw the vertices in the pixel
                        DrawPixelVertices(v_flags,&srcPolygon,&polygon);
                        iv_drawn = true;
                    }
                }
                break;
            case 1:
                v = pixel01->v - origin;
                glVertex2f(v.x,v.y);
                PolygonAddVertex(&polygon,v);
                v = pixel01->xedge.v[1] - origin;
                glVertex2f(v.x,v.y);
                PolygonAddVertex(&polygon,v);
                break;
            case 2:
                v = pixel01->xedge.v[0] - origin;
                glVertex2f(v.x,v.y);
                PolygonAddVertex(&polygon,v);
                break;
            case 3:
                v = pixel01->xedge.v[0] - origin;
                glVertex2f(v.x,v.y);
                PolygonAddVertex(&polygon,v);
                v = pixel01->xedge.v[1] - origin;
                glVertex2f(v.x,v.y);
                PolygonAddVertex(&polygon,v);
                break;
            }
            // draw the right yaxis
            switch(pixel10->yedge.code){
            case 0:
                if(pixel11->inside==0b1111){
                    v = pixel11->v - origin;
                    glVertex2f(v.x,v.y);
                    PolygonAddVertex(&polygon,v);
                }else{
                    if(!iv_drawn){
                        // draw the vertices in the pixel
                        DrawPixelVertices(v_flags,&srcPolygon,&polygon);
                        iv_drawn = true;
                    }
                }
                break;
            case 1:
                v = pixel10->yedge.v[1] - origin;
                glVertex2f(v.x,v.y);
                PolygonAddVertex(&polygon,v);
                break;
            case 2:
                v = pixel11->v - origin;
                glVertex2f(v.x,v.y);
                PolygonAddVertex(&polygon,v);
                v = pixel10->yedge.v[0] - origin;
                glVertex2f(v.x,v.y);
                PolygonAddVertex(&polygon,v);
                break;
            case 3:
                v = pixel10->yedge.v[1] - origin;
                glVertex2f(v.x,v.y);
                PolygonAddVertex(&polygon,v);
                v = pixel10->yedge.v[0] - origin;
                glVertex2f(v.x,v.y);
                PolygonAddVertex(&polygon,v);
                break;
            }
            // draw the top xaxis
            switch(pixel00->xedge.code){
            case 0:
                if(pixel10->inside==0b1111){
                    v = pixel10->v - origin;
                    glVertex2f(v.x,v.y);
                    PolygonAddVertex(&polygon,v);
                }else{
                    if(!iv_drawn){
                        // draw the vertices in the pixel
                        DrawPixelVertices(v_flags,&srcPolygon,&polygon);
                        iv_drawn = true;
                    }
                }
                break;
            case 1:
                v = pixel00->xedge.v[1] - origin;
                PolygonAddVertex(&polygon,v);
                glVertex2f(v.x,v.y);
                break;
            case 2:
                v = pixel10->v - origin;
                glVertex2f(v.x,v.y);
                PolygonAddVertex(&polygon,v);
                v = pixel00->xedge.v[0] - origin;
                glVertex2f(v.x,v.y);
                PolygonAddVertex(&polygon,v);
                break;
            case 3:
                v = pixel00->xedge.v[1] - origin;
                glVertex2f(v.x,v.y);
                PolygonAddVertex(&polygon,v);
                v = pixel00->xedge.v[0] - origin;
                glVertex2f(v.x,v.y);
                PolygonAddVertex(&polygon,v);
                break;
            }
            glEnd();
            float pixel_area = PolygonArea(&polygon);
            total_area += pixel_area;
        }

        // move the pointers to the next line
        int offset = PIXEL_DIM - Npixelx;
        pixel00+=offset;
        pixel01+=offset;
        pixel10+=offset;
        pixel11+=offset;
    }
    if(total_area/src_area<0.95f){
        qDebug("Total area was less than source area\n");
        if(theta_file_open && theta_file_write){
            theta_file.write((char*)&theta,sizeof(theta));
        }
    }
}
*/

void MyGLWidget::DrawSrcPolygon()
{
    glm::vec2 origin = pixelVertices[0][0].v;
    glm::vec2 v[4];
    for(int i=0;i<4;i++){
        v[i] = srcPolygon.vertices[i].v0 - origin;
    }

    glColor3f(1.0f,0.0f,0.0f);
    glBegin(GL_LINES);
    glVertex2f(v[0].x, v[0].y);
    glVertex2f(v[1].x, v[1].y);
    glEnd();

    glColor3f(0.0f,1.0f,0.0f);
    glBegin(GL_LINES);
    glVertex2f(v[1].x, v[1].y);
    glVertex2f(v[2].x, v[2].y);
    glEnd();

    glColor3f(0.0f,0.0f,1.0f);
    glBegin(GL_LINES);
    glVertex2f(v[2].x, v[2].y);
    glVertex2f(v[3].x, v[3].y);
    glEnd();

    glColor3f(1.0f,1.0f,0.0f);
    glBegin(GL_LINES);
    glVertex2f(v[3].x, v[3].y);
    glVertex2f(v[0].x, v[0].y);
    glEnd();
}

void MyGLWidget::DrawPolygon()
{
    glm::vec2 origin = pixelVertices[0][0].v;
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(-origin.x,-origin.y,0.0f);

    glBegin(GL_POLYGON);
    for(int i=0;i<polygon.N;i++){
        glm::vec2 v = polygon.v[i];
        glVertex2f(v.x,v.y);
    }
    glEnd();
    glPopMatrix();

}

void MyGLWidget::DrawGrid(void){
    glBegin(GL_LINES);
    float f_grid_size = (float)grid_size;
    for(int x=0;x<=grid_size;x++){
        float x_real = (float)x;
        glVertex2f(x_real,0.0f);
        glVertex2f(x_real,-f_grid_size);
    }
    glEnd();
    glBegin(GL_LINES);
    for(int y=0;y>=-grid_size;y--){
        float y_real = (float)y;
        glVertex2f(0.0f, y_real);
        glVertex2f(f_grid_size,y_real);
    }
    glEnd();
}

void MyGLWidget::timer_func()
{
    repaint(0,0,-1,-1);
}

void SrcPolygonInitVertices(SrcPolygon *sp, glm::vec2 *vertices, glm::mat3 &M)
{
    for(int v=0;v<4;v++){
        glm::vec3 vw(vertices[v],1.0f);
        glm::vec3 vwp = M*vw;
        sp->vertices[v].v0 = glm::vec2(vwp);
    }

}

void SrcPolygonInitEdges(SrcPolygon *sp)
{
    for(int i_v0=0;i_v0<4;i_v0++){
        int i_v1 = i_v0 + 1;
        if(i_v1==4)i_v1 = 0;
        glm::vec2 v10 = sp->vertices[i_v1].v0 - sp->vertices[i_v0].v0;
        sp->vertices[i_v0].v10 = v10;
        sp->vertices[i_v0].N = glm::normalize(glm::vec2(-v10.y,v10.x));
    }
}

int f2BisectSrcPolygon(SrcPolygon *sp, glm::vec2 v)
{
    int r=0;
    int inside_bit = 1;
    float f_test;
    for(int e=0;e<4;e++,inside_bit<<=1){
        glm::vec2 vv0 = v - sp->vertices[e].v0;
        f_test = glm::dot(vv0,sp->vertices[e].N);
        if(f_test > -1e-5f){
            r|=inside_bit;
        }
    }
    return r;
}

void PixelEdgeBisectSrcPolygon(PixelEdge *pe, SrcPolygon *sp)
{
    // test for all outside of any edge
    if((~pe->inside_ends[0])&(~pe->inside_ends[1])&0b1111){
        return;
    }
    // find the intersecting edges
    int intersecting = pe->inside_ends[0]^pe->inside_ends[1];
    int edge_bit = 1;
    for(int e=0;e<4;e++,edge_bit<<=1){
        if(!(edge_bit&intersecting)) continue;
        switch(pe->code){
        case 0:
            if(edge_bit&pe->inside_ends[0]){
                // v0 is inside
                pe->code = 1;
                pe->v_edge[1] = f2IntersectionDelta(pe->v_ends[0],pe->v_ends[1],sp->vertices[e].v0,sp->vertices[e].v10);
                pe->inside_edge[1] = f2BisectSrcPolygon(sp,pe->v_edge[1]);
                pe->vflag_edge[1] = edge_bit;
            }else{
                // v1 is inside
                pe->code = 2;
                pe->v_edge[0] = f2IntersectionDelta(pe->v_ends[0],pe->v_ends[1],sp->vertices[e].v0,sp->vertices[e].v10);
                pe->inside_edge[0] = f2BisectSrcPolygon(sp,pe->v_edge[0]);
                pe->vflag_edge[0] = edge_bit;
            }
            break;
        case 1:
            // test for all outside
            if((~pe->inside_ends[0])&(~pe->inside_edge[1])&edge_bit){
                pe->code = 0;
                return;
            }
            // test for an intersection
            if((pe->inside_ends[0]^pe->inside_edge[1])&edge_bit){
                if(pe->inside_ends[0]&edge_bit){
                    // v0 is inside
                    pe->code = 1;
                    pe->v_edge[1] = f2IntersectionDelta(pe->v_ends[0],pe->v_edge[1],sp->vertices[e].v0,sp->vertices[e].v10);
                    pe->inside_edge[1] = f2BisectSrcPolygon(sp,pe->v_edge[1]);
                    pe->vflag_edge[1] = edge_bit;
                }else{
                    // v_edge[1] is inside
                    pe->code = 3;
                    pe->v_edge[0] = f2IntersectionDelta(pe->v_ends[0],pe->v_edge[1],sp->vertices[e].v0,sp->vertices[e].v10);
                    pe->inside_edge[0] = f2BisectSrcPolygon(sp,pe->v_edge[0]);
                    pe->vflag_edge[0] = edge_bit;
                }
            }
            break;
        case 2:
            // test for all outside
            if((~pe->inside_ends[1])&(~pe->inside_edge[0])&edge_bit){
                pe->code = 0;
                return;
            }
            // test for intersection with this edge
            if((pe->inside_ends[1]^pe->inside_edge[0])&edge_bit){
                if(pe->inside_ends[1]&edge_bit){
                    // v1 is inside
                    pe->code = 2;
                    pe->v_edge[0] = f2IntersectionDelta(pe->v_edge[0],pe->v_ends[1],sp->vertices[e].v0,sp->vertices[e].v10);
                    pe->inside_edge[0] = f2BisectSrcPolygon(sp,pe->v_edge[0]);
                    pe->vflag_edge[0] = edge_bit;
                }else{
                    // edge->v[0] is inside
                    pe->code = 3;
                    pe->v_edge[1] = f2IntersectionDelta(pe->v_edge[0],pe->v_ends[1],sp->vertices[e].v0,sp->vertices[e].v10);
                    pe->inside_edge[1] = f2BisectSrcPolygon(sp,pe->v_edge[1]);
                    pe->vflag_edge[1] = edge_bit;
                }
            }
            break;
        case 3:
            // test for all outside
            if((~pe->inside_edge[0])&(~pe->inside_edge[1])&0b1111){
                pe->code = 0;
                return;
            }
            // test for intersection with this edge
            if((pe->inside_edge[0]^pe->inside_edge[1])&edge_bit){
                if(pe->inside_edge[0]&edge_bit){
                    pe->code = 3;
                    pe->v_edge[1] = f2IntersectionDelta(pe->v_edge[0],pe->v_edge[1],sp->vertices[e].v0,sp->vertices[e].v10);
                    pe->inside_edge[1] = f2BisectSrcPolygon(sp,pe->v_edge[1]);
                    pe->vflag_edge[1] = edge_bit;
                }else{
                    pe->code = 3;
                    pe->v_edge[0] = f2IntersectionDelta(pe->v_edge[0],pe->v_edge[1],sp->vertices[e].v0,sp->vertices[e].v10);
                    pe->inside_edge[0] = f2BisectSrcPolygon(sp,pe->v_edge[0]);
                    pe->vflag_edge[0] = edge_bit;
                }
            }
            break;
        }
    }
}

void PixelEdgeBorderBisectSrcPolygon(PixelEdge *pe, SrcPolygon *sp)
{
    // the only case to bisect a border edge is when there is
    // one intersection and the rest are all inside
    int intersecting = pe->inside_ends[0]^pe->inside_ends[1];
    int all_inside = pe->inside_ends[0]&pe->inside_ends[1];
    switch(intersecting){
    case 1:
        if(all_inside==0b1110){
            if(pe->inside_ends[0]&intersecting){
                // v0 is inside
                pe->code = 1;
                pe->v_edge[1] = f2IntersectionDelta(pe->v_ends[0],pe->v_ends[1],sp->vertices[0].v0,sp->vertices[0].v10);
                pe->vflag_edge[1] = 0b0001;
            }else{
                // v1 is inside
                pe->code = 2;
                pe->v_edge[0] = f2IntersectionDelta(pe->v_ends[0],pe->v_ends[1],sp->vertices[0].v0,sp->vertices[0].v10);
                pe->vflag_edge[0] = 0b0001;
            }
        }
        return;
    case 2:
        if(all_inside==0b1101){
            if(pe->inside_ends[0]&intersecting){
                // v0 is inside
                pe->code = 1;
                pe->v_edge[1] = f2IntersectionDelta(pe->v_ends[0],pe->v_ends[1],sp->vertices[1].v0,sp->vertices[1].v10);
                pe->vflag_edge[1] = 0b0010;
            }else{
                // v1 is inside
                pe->code = 2;
                pe->v_edge[0] = f2IntersectionDelta(pe->v_ends[0],pe->v_ends[1],sp->vertices[1].v0,sp->vertices[1].v10);
                pe->vflag_edge[0] = 0b0010;
            }
        }
        return;
    case 4:
        if(all_inside==0b1011){
            if(pe->inside_ends[0]&intersecting){
                // v0 is inside
                pe->code = 1;
                pe->v_edge[1] = f2IntersectionDelta(pe->v_ends[0],pe->v_ends[1],sp->vertices[2].v0,sp->vertices[2].v10);
                pe->vflag_edge[1] = 0b0100;
            }else{
                // v1 is inside
                pe->code = 2;
                pe->v_edge[0] = f2IntersectionDelta(pe->v_ends[0],pe->v_ends[1],sp->vertices[2].v0,sp->vertices[2].v10);
                pe->vflag_edge[0] = 0b0100;
            }
        }
        return;
    case 8:
        if(all_inside==0b0111){
            if(pe->inside_ends[0]&intersecting){
                // v0 is inside
                pe->code = 1;
                pe->v_edge[1] = f2IntersectionDelta(pe->v_ends[0],pe->v_ends[1],sp->vertices[3].v0,sp->vertices[3].v10);
                pe->vflag_edge[1] = 0b1000;
            }else{
                // v1 is inside
                pe->code = 2;
                pe->v_edge[0] = f2IntersectionDelta(pe->v_ends[0],pe->v_ends[1],sp->vertices[3].v0,sp->vertices[3].v10);
                pe->vflag_edge[0] = 0b1000;
            }
        }
        return;
    default:
        return;
    }
}

glm::vec2 f2IntersectionDelta(glm::vec2 a0, glm::vec2 a1, glm::vec2 b0, glm::vec2 b10)
{
    glm::vec2 d_a = a1-a0;
    glm::vec2 r_a0b0;
    bool swap_a;
    float d_a_dot_d_b = glm::dot(d_a,b10);
    if(d_a_dot_d_b<0.0f){
        // swap a0 and a1 and start over
        d_a *= -1.0f;
        d_a_dot_d_b *= -1.0f;
        r_a0b0 = a1 - b0;
        swap_a = true;
    }else{
        r_a0b0 = a0 - b0;
        swap_a = false;
    }
    float d_a_dot_d_a = glm::dot(d_a,d_a);
    float d_b_dot_d_b = glm::dot(b10,b10);
    float d_ab_dot_d_a = glm::dot(r_a0b0,d_a);
    float d_ab_dot_d_b = glm::dot(r_a0b0,b10);
    float t_num_p = d_a_dot_d_b*d_ab_dot_d_b;
    float t_num_m = d_b_dot_d_b*d_ab_dot_d_a;
    float t_det = d_a_dot_d_a*d_b_dot_d_b - d_a_dot_d_b*d_a_dot_d_b;
    float t = (t_num_p - t_num_m) / t_det;
    if(!isfinite(t)){
        qDebug("infinite result t_det:%f",t_det);
        t=0.5f;
    }
    if(t<0.0f)t=0.0f;
    if(t>1.0f)t=1.0f;
    if(swap_a){
        return a1 + d_a*t;
    }else{
        return a0 + d_a*t;
    }
}

glm::ivec2 convert_ivec2_plus(glm::vec2 v)
{
    glm::ivec2 r = v;
    if(v.x<0.0f) r.x--;
    if(v.y>0.0f) r.y++;
    return r;
}

float f2cross(glm::vec2 &a, glm::vec2 &b)
{
    return a.x*b.y - a.y*b.x;
}

float SrcPolygonArea(SrcPolygon *sp)
{
    // since the source polygon is allways a paralellogram
    // the area is just the cross product of two of the sides
    return f2cross(sp->vertices[0].v10,sp->vertices[1].v10);
}

void PolygonAddVertex(Polygon *p, glm::vec2 &v)
{
    p->v[p->N] = v;
    p->N++;
}

float PolygonArea(Polygon *p)
{
    if(p->N<3)return 0.0f;
    int Ntri = p->N - 2;
    glm::vec2 v0 = p->v[0];
    float area = 0.0f;
    for(int t=0;t<Ntri;t++){
        glm::vec2 v10 = p->v[t+1] - v0;
        glm::vec2 v21 = p->v[t+2] - p->v[t+1];
        area += f2cross(v10,v21);
    }
    return area/2.0f;
}
