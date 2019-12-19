#include "myglwidget.h"
#include <math.h>
#include <stdlib.h>

MyGLWidget::MyGLWidget(QWidget *parent) :
    QOpenGLWidget(parent),
    theta_file("/tmp/theta.bin")
{
    alpha = 0.0f;
    dalpha = 0.1/60.0;

    timer = new QTimer(this);

    connect(timer, &QTimer::timeout, this, &MyGLWidget::timer_func);

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

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLdouble aspect = (GLdouble)w/h;
    GLdouble size = 3.0;
    if(aspect>=1.0){
        GLdouble left = -aspect*size/2 + 1.5;
        GLdouble right = aspect*size/2 + 1.5;
        GLdouble top = 0.0;
        GLdouble bottom = -3.0;
        GLdouble near = 1.0;
        GLdouble far = -1.0;
        glOrtho(left,right,bottom,top,near,far);
    }else{
        GLdouble left = 0.0;
        GLdouble right = 3.0;
        GLdouble top = 1.0/aspect*size/2 - 1.5;
        GLdouble bottom = -1.0/aspect*size/2 - 1.5;
        GLdouble near = 1.0;
        GLdouble far = -1.0;
        glOrtho(left,right,bottom,top,near,far);
    }
}

void MyGLWidget::paintGL(){
    glColor3f(0.25f,0.25f,0.25f);
    DrawGrid();

    InitSrcPolygon();
    InitPixels();
    BisectEdges();
    glColor3f(0.5f,0.5f,0.5f);
    DrawPolygons();
    DrawSrcPolygon();

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
    glm::vec2 Tpre(-0.5f,0.5f);
    glm::vec2 Tpost(0.5f,0.0f);
    glm::vec2 S(1.0f,0.5f);
    if(theta_file_open && !theta_file_write){
        theta_file.read((char*)&theta,sizeof(theta));
        if(theta_file.atEnd()){
            theta_file.seek(0);
        }
    }else{
        float offset = (float)(ceil(drand48()*4)*M_PI/2);
        theta = 0.0f;//(drand48()-0.5)*0.1 + offset;
    }
    M = glm::translate(M,Tpost);
    M = glm::rotate(M,theta);
    M = glm::scale(M,S);
    M = glm::translate(M,Tpre);
    SrcPolygonInitVertices(&srcPolygon, vertices, M);

    SrcPolygonInitEdges(&srcPolygon);

    alpha += dalpha;
    if(alpha>=1.0f)alpha-=1.0f;
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

    if(Npixelx==1 && Npixely==1){
        pixels[0][0].v = v0;
        return;
    }

    for(int y=0;y<Npixely+1;y++){
        glm::vec2 v = v0;
        for(int x=0;x<Npixelx+1;x++){
            pixels[y][x].v = v;
            pixels[y][x].inside = f2BisectSrcPolygon(&srcPolygon, v);
            v+=glm::vec2(1.0f,0.0f);
        }
        v0+=glm::vec2(0.0f,-1.0f);
    }
    // initialize the pixel vertex flags to zero
    for(int y=0;y<Npixely;y++){
        for(int x=0;x<Npixelx;x++){
            pixelVertices[y][x] = 0;
        }
    }
    // deposit the vertices into the pixels
    pixelVertices[i2_v0.y-i2_src0.y][i2_src0.x-i2_v0.x] |= V0_BIT;
    pixelVertices[i2_v0.y-i2_src1.y][i2_src1.x-i2_v0.x] |= V1_BIT;
    pixelVertices[i2_v0.y-i2_src2.y][i2_src2.x-i2_v0.x] |= V2_BIT;
    pixelVertices[i2_v0.y-i2_src3.y][i2_src3.x-i2_v0.x] |= V3_BIT;
}

void MyGLWidget::BisectEdges()
{
    if(Npixelx==1 && Npixely==1)
        return;
    Pixel *pixel00 = &pixels[0][0];
    Pixel *pixel10 = &pixels[0][1];
    Pixel *pixel01 = &pixels[1][0];
    BisectParams bp;
    for(int y=0;y<Npixely;y++){
        for(int x=0;x<Npixelx;x++,pixel00++,pixel10++,pixel01++){
            if(x==0){
                // border bisect allong the yaxis edge
                bp.v[0] = pixel00->v;
                bp.inside[0] = pixel00->inside;
                bp.v[1] = pixel01->v;
                bp.inside[1] = pixel01->inside;
                bp.edge = &pixel00->yedge;
                PixelEdgeBorderBisectSrcPolygon(&bp,&srcPolygon);
            }else{
                bp.v[0] = pixel00->v;
                bp.inside[0] = pixel00->inside;
                bp.v[1] = pixel01->v;
                bp.inside[1] = pixel01->inside;
                bp.edge = &pixel00->yedge;
                PixelEdgeBisectSrcPolygon(&bp,&srcPolygon);
            }
            if(y==0){
                bp.v[0] = pixel00->v;
                bp.inside[0] = pixel00->inside;
                bp.v[1] = pixel10->v;
                bp.inside[1] = pixel10->inside;
                bp.edge = &pixel00->xedge;
                PixelEdgeBorderBisectSrcPolygon(&bp,&srcPolygon);
            }else{
                bp.v[0] = pixel00->v;
                bp.inside[0] = pixel00->inside;
                bp.v[1] = pixel10->v;
                bp.inside[1] = pixel10->inside;
                bp.edge = &pixel00->xedge;
                PixelEdgeBisectSrcPolygon(&bp,&srcPolygon);
            }
            // test for the right most edge
            if(x==Npixelx-1){
                pixel10->yedge.code = 0;
            }
            // test for the bottom most edge
            if(y==Npixely-1){
                pixel01->xedge.code = 0;
            }
        }
        // move the pixel pointers to the next line
        int offset = PIXEL_DIM - Npixelx;
        pixel00+=offset;
        pixel10+=offset;
        pixel01+=offset;
    }
}

void MyGLWidget::DrawPixelVertices(int flags, SrcPolygon *sp, Polygon *polygon)
{
    glm::vec2 v;
    glm::vec2 origin = pixels[0][0].v;
    switch(flags){
    case 0b0000:
        return;
    case 0b0001:
        v = sp->vertices[0].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        return;
    case 0b0010:
        v = sp->vertices[1].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        return;
    case 0b0011:
        v = sp->vertices[0].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        v = sp->vertices[1].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        return;
    case 0b0100:
        v = sp->vertices[2].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        return;
    case 0b0101:
        // this case is handled by another part of the program
        return;
    case 0b0110:
        v = sp->vertices[1].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        v = sp->vertices[2].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        return;
    case 0b0111:
        v = sp->vertices[0].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        v = sp->vertices[1].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        v = sp->vertices[2].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        return;
    case 0b1000:
        v = sp->vertices[3].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        return;
    case 0b1001:
        v = sp->vertices[3].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        v = sp->vertices[0].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        return;
    case 0b1010:
        // handled by another part of the program
        // shouldn't happen
        return;
    case 0b1011:
        v = sp->vertices[3].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        v = sp->vertices[0].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        v = sp->vertices[1].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        return;
    case 0b1100:
        v = sp->vertices[2].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        v = sp->vertices[3].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        return;
    case 0b1101:
        v = sp->vertices[2].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        v = sp->vertices[3].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        v = sp->vertices[0].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        return;
    case 0b1110:
        v = sp->vertices[1].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        v = sp->vertices[2].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        v = sp->vertices[3].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        return;
    case 0b1111:
        v = sp->vertices[0].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        v = sp->vertices[1].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        v = sp->vertices[2].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        v = sp->vertices[3].v0 - origin;
        glVertex2f(v.x,v.y);
        PolygonAddVertex(polygon,v);
        return;
    }
}

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

void MyGLWidget::DrawSrcPolygon()
{
    glm::vec2 origin = pixels[0][0].v;
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

void MyGLWidget::DrawGrid(void){
    glBegin(GL_LINES);
    for(int x=0;x<4;x++){
        float x_real = (float)x;
        glVertex2f(x_real,0.0f);
        glVertex2f(x_real,-3.0f);
    }
    glEnd();
    glBegin(GL_LINES);
    for(int y=0;y>-4;y--){
        float y_real = (float)y;
        glVertex2f(0.0f, y_real);
        glVertex2f(3.0,y_real);
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
        if(f_test > -1.1921e-7f){
            r|=inside_bit;
        }
    }
    return r;
}

void PixelEdgeBisectSrcPolygon(BisectParams *bp, SrcPolygon *sp)
{
    // initialize the edge code to no intersections
    bp->edge->code = 0;
    // test for all outside of any edge
    if((~bp->inside[0])&(~bp->inside[1])&0xF){
        return;
    }
    // find the intersecting edges
    int intersecting = bp->inside[0]^bp->inside[1];
    int edge_bit = 1;
    for(int e=0;e<4;e++,edge_bit<<=1){
        if(!(edge_bit&intersecting)) continue;
        switch(bp->edge->code){
        case 0:
            if(edge_bit&bp->inside[0]){
                // v0 is inside
                bp->edge->code = 1;
                bp->edge->v[1] = f2IntersectionDelta(bp->v[0],bp->v[1],sp->vertices[e].v0,sp->vertices[e].v10);
                bp->edge->inside[1] = f2BisectSrcPolygon(sp,bp->edge->v[1]);
            }else{
                // v1 is inside
                bp->edge->code = 2;
                bp->edge->v[0] = f2IntersectionDelta(bp->v[0],bp->v[1],sp->vertices[e].v0,sp->vertices[e].v10);
                bp->edge->inside[0] = f2BisectSrcPolygon(sp,bp->edge->v[0]);
            }
            break;
        case 1:
            // test for all outside
            if((~bp->inside[0])&(~bp->edge->inside[1])&edge_bit){
                bp->edge->code = 0;
                return;
            }
            // test for an intersection
            if((bp->inside[0]^bp->edge->inside[1])&edge_bit){
                if(bp->inside[0]&edge_bit){
                    // v0 is inside
                    bp->edge->code = 1;
                    bp->edge->v[1] = f2IntersectionDelta(bp->v[0],bp->edge->v[1],sp->vertices[e].v0,sp->vertices[e].v10);
                    bp->edge->inside[1] = f2BisectSrcPolygon(sp,bp->edge->v[1]);
                }else{
                    // edge->v[1] is inside
                    bp->edge->code = 3;
                    bp->edge->v[0] = f2IntersectionDelta(bp->v[0],bp->edge->v[1],sp->vertices[e].v0,sp->vertices[e].v10);
                    bp->edge->inside[0] = f2BisectSrcPolygon(sp,bp->edge->v[0]);
                }
            }
            break;
        case 2:
            // test for all outside
            if((~bp->inside[1])&(~bp->edge->inside[0])&edge_bit){
                bp->edge->code = 0;
                return;
            }
            // test for intersection with this edge
            if((bp->inside[1]^bp->edge->inside[0])&edge_bit){
                if(bp->inside[1]&edge_bit){
                    // v1 is inside
                    bp->edge->code = 2;
                    bp->edge->v[0] = f2IntersectionDelta(bp->edge->v[0],bp->v[1],sp->vertices[e].v0,sp->vertices[e].v10);
                    bp->edge->inside[0] = f2BisectSrcPolygon(sp,bp->edge->v[0]);
                }else{
                    // edge->v[0] is inside
                    bp->edge->code = 3;
                    bp->edge->v[1] = f2IntersectionDelta(bp->edge->v[0],bp->v[1],sp->vertices[e].v0,sp->vertices[e].v10);
                    bp->edge->inside[1] = f2BisectSrcPolygon(sp,bp->edge->v[1]);
                }
            }
            break;
        case 3:
            // test for intersection with this edge
            if((bp->edge->inside[0]^bp->edge->inside[1])&edge_bit){
                if(bp->edge->inside[0]&edge_bit){
                    bp->edge->code = 3;
                    bp->edge->v[1] = f2IntersectionDelta(bp->edge->v[0],bp->edge->v[1],sp->vertices[e].v0,sp->vertices[e].v10);
                    bp->edge->inside[1] = f2BisectSrcPolygon(sp,bp->edge->v[1]);
                }else{
                    bp->edge->code = 3;
                    bp->edge->v[0] = f2IntersectionDelta(bp->edge->v[0],bp->edge->v[1],sp->vertices[e].v0,sp->vertices[e].v10);
                    bp->edge->inside[0] = f2BisectSrcPolygon(sp,bp->edge->v[0]);
                }
            }
            break;
        }
    }
}

void PixelEdgeBorderBisectSrcPolygon(BisectParams *bp, SrcPolygon *sp)
{
    bp->edge->code = 0;
    // the only case to bisect a border edge is when there is
    // one intersection and the rest are all inside
    int intersecting = bp->inside[0]^bp->inside[1];
    int all_inside = bp->inside[0]&bp->inside[1];
    switch(intersecting){
    case 1:
        if(all_inside==0b1110){
            if(bp->inside[0]&intersecting){
                // v0 is inside
                bp->edge->code = 1;
                bp->edge->v[1] = f2IntersectionDelta(bp->v[0],bp->v[1],sp->vertices[0].v0,sp->vertices[0].v10);
            }else{
                // v1 is inside
                bp->edge->code = 2;
                bp->edge->v[0] = f2IntersectionDelta(bp->v[0],bp->v[1],sp->vertices[0].v0,sp->vertices[0].v10);
            }
        }
        return;
    case 2:
        if(all_inside==0b1101){
            if(bp->inside[0]&intersecting){
                // v0 is inside
                bp->edge->code = 1;
                bp->edge->v[1] = f2IntersectionDelta(bp->v[0],bp->v[1],sp->vertices[1].v0,sp->vertices[1].v10);
            }else{
                // v1 is inside
                bp->edge->code = 2;
                bp->edge->v[0] = f2IntersectionDelta(bp->v[0],bp->v[1],sp->vertices[1].v0,sp->vertices[1].v10);
            }
        }
        return;
    case 4:
        if(all_inside==0b1011){
            if(bp->inside[0]&intersecting){
                // v0 is inside
                bp->edge->code = 1;
                bp->edge->v[1] = f2IntersectionDelta(bp->v[0],bp->v[1],sp->vertices[2].v0,sp->vertices[2].v10);
            }else{
                // v1 is inside
                bp->edge->code = 2;
                bp->edge->v[0] = f2IntersectionDelta(bp->v[0],bp->v[1],sp->vertices[2].v0,sp->vertices[2].v10);
            }
        }
        return;
    case 8:
        if(all_inside==0b0111){
            if(bp->inside[0]&intersecting){
                // v0 is inside
                bp->edge->code = 1;
                bp->edge->v[1] = f2IntersectionDelta(bp->v[0],bp->v[1],sp->vertices[3].v0,sp->vertices[3].v10);
            }else{
                // v1 is inside
                bp->edge->code = 2;
                bp->edge->v[0] = f2IntersectionDelta(bp->v[0],bp->v[1],sp->vertices[3].v0,sp->vertices[3].v10);
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

void PixelVerticesAddVertex(PixelVertices *pv, int index)
{
    pv->indices[pv->Nvertices] = index;
    pv->Nvertices++;
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