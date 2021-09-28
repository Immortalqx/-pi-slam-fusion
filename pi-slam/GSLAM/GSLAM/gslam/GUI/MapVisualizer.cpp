#include "MapVisualizer.h"
#include <GSLAM/core/Event.h>

#if defined(HAS_GLEW)
#include <GL/glew.h>
#else
#include <QtOpenGL>
#endif

namespace GSLAM {


inline void glVertex(const GSLAM::Point3f& p){glVertex3f(p.x,p.y,p.z);}
inline void glVertex(const GSLAM::Point3d& p){glVertex3d(p.x,p.y,p.z);}
inline void glColor(const GSLAM::Point3ub& c){glColor3ub(c.x,c.y,c.z);}

MapVisualizer::MapVisualizer(MapPtr slam_map,std::string slam_name,GObjectHandle* handle)
    : _map(slam_map),_name(slam_name),_handle(handle),
      _vetexTrajBuffer(0),_mapUpdated(false),_curFrameUpdated(false),_firstUpdate(true){
    if(!_name.empty())
    {
        scommand.Call("AddLayer",_name+".Trajectory");
        scommand.Call("AddLayer",_name+".Connects");
        scommand.Call("AddLayer",_name+".PointCloud");
        scommand.Call("AddLayer",_name+".Frames");
        scommand.Call("AddLayer",_name+".CurrentFrame");
    }
}

void MapVisualizer::draw()
{
    if(!svar.GetInt("SLAM.All",1)) return;

    GSLAM::ReadMutex lock(_mutex);
    glPushMatrix();
    glTranslated(_scenceOrigin.x,_scenceOrigin.y,_scenceOrigin.z);

    double& trajectoryWidth=svar.GetDouble("MainWindow.TrajectoryWidth",2.5);
    double& connectionWidth=svar.GetDouble("MainWindow.ConnectionWidth",1.);
    double& pointCloudSize =svar.GetDouble("MainWindow.PointCloudSize",2.5);

    GSLAM::Point3ub trajectoryColor=svar.get_var("MainWindow.TrajectoryColor",Point3ub(255,255,0));
    GSLAM::Point3ub gpsTrajectoryColor=svar.get_var("MainWindow.GPSTrajectoryColor",Point3ub(255,0,0));
    GSLAM::Point3ub connectionColor=svar.get_var("MainWindow.ConnectionColor",Point3ub(0,255,255));
    GSLAM::Point3ub frameColor  =svar.get_var("MainWindow.FrameColor",Point3ub(0,0,255));
    GSLAM::Point3ub curFrameColor  =svar.get_var("MainWindow.CurrentFrameColor",Point3ub(255,0,0));
#if defined(HAS_GLEW)
    if(!_vetexTrajBuffer)
    {
        glewInit();
        glGenBuffers(7, &_vetexTrajBuffer);
    }

    if(_mapUpdated)
    {
        if(_vetexTraj.size())
        {
            glBindBuffer(GL_ARRAY_BUFFER,_vetexTrajBuffer);
            glBufferData(GL_ARRAY_BUFFER,_vetexTraj.size()*sizeof(Point3f),_vetexTraj.data(), GL_STATIC_DRAW);
        }

        if(_gpsTraj.size())
        {
            glBindBuffer(GL_ARRAY_BUFFER,_gpsTrajBuffer);
            glBufferData(GL_ARRAY_BUFFER,_gpsTraj.size()*sizeof(Point3f),_gpsTraj.data(), GL_STATIC_DRAW);
        }

        if(_vetexConnection.size()){
            glBindBuffer(GL_ARRAY_BUFFER,_vetexConnectionBuffer);
            glBufferData(GL_ARRAY_BUFFER,_vetexConnection.size()*sizeof(Point3d),_vetexConnection.data(), GL_STATIC_DRAW);
        }

        if(_gpsError.size()){
            glBindBuffer(GL_ARRAY_BUFFER,_gpsErrorBuffer);
            glBufferData(GL_ARRAY_BUFFER,_gpsError.size()*sizeof(Point3d),_gpsError.data(), GL_STATIC_DRAW);
        }

        if(_pointCloudVertex.size()){
            glBindBuffer(GL_ARRAY_BUFFER,_pointCloudVertexBuffer);
            glBufferData(GL_ARRAY_BUFFER,_pointCloudVertex.size()*sizeof(Point3f),_pointCloudVertex.data(), GL_STATIC_DRAW);
        }

        if(_pointCloudColors.size()){
            glBindBuffer(GL_ARRAY_BUFFER,_pointCloudColorsBuffer);
            glBufferData(GL_ARRAY_BUFFER,_pointCloudColors.size()*sizeof(Point3ub),_pointCloudColors.data(), GL_STATIC_DRAW);
        }

        _mapUpdated=false;
    }

    if(_curFrameUpdated)
    {
        if(_curConnection.size())
        {
            glBindBuffer(GL_ARRAY_BUFFER,_curConnectionBuffer);
            glBufferData(GL_ARRAY_BUFFER,_curConnection.size()*sizeof(Point3d),_curConnection.data(), GL_STATIC_DRAW);
        }
        _curFrameUpdated=false;
    }

    if(svar.GetInt(_name+".Trajectory",1))
    {
        glDisable(GL_LIGHTING);
        glBindBuffer(GL_ARRAY_BUFFER,_vetexTrajBuffer);
        glVertexPointer(3, GL_FLOAT, 0, 0);
        glLineWidth(trajectoryWidth);
        glColor3f(trajectoryColor.x,trajectoryColor.y,trajectoryColor.z);
        glEnableClientState(GL_VERTEX_ARRAY);
        glDrawArrays(GL_LINE_STRIP,0,_vetexTraj.size());
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    if(svar.GetInt(_name+".GPSOffset",1))
    {
        glDisable(GL_LIGHTING);
        glBindBuffer(GL_ARRAY_BUFFER,_gpsTrajBuffer);
        glVertexPointer(3, GL_FLOAT, 0, 0);
        glLineWidth(trajectoryWidth);
        glColor3ub(gpsTrajectoryColor.x,gpsTrajectoryColor.y,gpsTrajectoryColor.z);
        glEnableClientState(GL_VERTEX_ARRAY);
        glDrawArrays(GL_LINE_STRIP,0,_gpsTraj.size());
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    if(svar.GetInt(_name+".Connects",1))
    {
        glBindBuffer(GL_ARRAY_BUFFER,_vetexConnectionBuffer);
        glVertexPointer(3, GL_DOUBLE, 0, 0);
        glLineWidth(connectionWidth);
        glColor3ub(connectionColor.x,connectionColor.y,connectionColor.z);
        glEnableClientState(GL_VERTEX_ARRAY);
        glDrawArrays(GL_LINES,0,_vetexConnection.size());
        glDisableClientState(GL_VERTEX_ARRAY);
    }


    if(svar.GetInt(_name+".PointCloud",1))
    {
        glBindBuffer(GL_ARRAY_BUFFER,_pointCloudVertexBuffer);
        glVertexPointer(3, GL_FLOAT, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER,_pointCloudColorsBuffer);
        glColorPointer(3, GL_UNSIGNED_BYTE, 0, 0);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glPointSize(pointCloudSize);
        glDrawArrays(GL_POINTS,0,_pointCloudVertex.size());
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    if(svar.GetInt(_name+".Frames",1))
    {
        for(auto sim3:_keyframes)
        {
            drawRect(sim3,frameColor);
        }
    }

    glPopMatrix();
    if(svar.GetInt(_name+".CurrentFrame",1)&&_curFrame)
    {
        GSLAM::SIM3 curPose(_curFrame->getPose(),fabs(_curFrame->getMedianDepth())*0.1);
        drawRect(curPose,curFrameColor);

        glBindBuffer(GL_ARRAY_BUFFER,_curConnectionBuffer);
        glVertexPointer(3, GL_DOUBLE, 0, 0);
        glLineWidth(connectionWidth);
        glEnableClientState(GL_VERTEX_ARRAY);
        glDrawArrays(GL_LINES,0,_curConnection.size());
        glDisableClientState(GL_VERTEX_ARRAY);
    }
#else
    if(!_vetexTrajBuffer){
        _vetexTrajBuffer=glGenLists(1);
    }

    if(_curFrameUpdated)
    {
        _curFrameUpdated=false;
    }

    glDisable(GL_LIGHTING);

    if(svar.GetInt(_name+".Trajectory",1))
    {
        glLineWidth(trajectoryWidth);
        glColor3f(trajectoryColor.x,trajectoryColor.y,trajectoryColor.z);
        glBegin(GL_LINE_STRIP);
        for(auto v:_vetexTraj) glVertex(v);
        glEnd();
    }

    if(svar.GetInt(_name+".GPSOffset",1))
    {
        glLineWidth(trajectoryWidth);
        glColor3ub(gpsTrajectoryColor.x,gpsTrajectoryColor.y,gpsTrajectoryColor.z);
        glBegin(GL_LINE_STRIP);
        for(auto& v:_gpsTraj) glVertex(v);
        glEnd();
    }

    if(svar.GetInt(_name+".Connects",1))
    {
        glLineWidth(connectionWidth);
        glColor3ub(connectionColor.x,connectionColor.y,connectionColor.z);
        glBegin(GL_LINES);
        for(auto& v:_vetexConnection) glVertex(v);
        glEnd();
    }


    if(svar.GetInt(_name+".PointCloud",1))
    {
        if(_mapUpdated)
        {
            glNewList(_vetexTrajBuffer,GL_COMPILE);
            glPointSize(pointCloudSize);
            glBegin(GL_POINTS);
            for(int i=0;i<_pointCloudVertex.size();i++) {
                glColor(_pointCloudColors[i]);
                glVertex(_pointCloudVertex[i]);
            }
            glEnd();
            glEndList();
            _mapUpdated=false;
        }
        glCallList(_vetexTrajBuffer);

    }

    if(svar.GetInt(_name+".Frames",1))
    {
        for(auto sim3:_keyframes)
        {
            drawRect(sim3,frameColor);
        }
    }

    glPopMatrix();
    if(svar.GetInt(_name+".CurrentFrame",1)&&_curFrame)
    {
        GSLAM::SIM3 curPose(_curFrame->getPose(),fabs(_curFrame->getMedianDepth())*0.1);
        drawRect(curPose,curFrameColor);
        glLineWidth(connectionWidth);
        glBegin(GL_LINES);
        for(auto& v:_curConnection) glVertex(v);
        glEnd();
    }
#endif
}


void MapVisualizer::drawRect(GSLAM::SIM3 pose,GSLAM::ColorType color)
{
    if(!_camera.isValid()) _camera=GSLAM::Camera({640.,480.,500.,500.,320.,240.});
    {
        Point3d t=pose.get_translation();
        pi::Point3d tl=_camera.UnProject(pi::Point2d(0,0));
        pi::Point3d tr=_camera.UnProject(pi::Point2d(_camera.width(),0));
        pi::Point3d bl=_camera.UnProject(pi::Point2d(0,_camera.height()));
        pi::Point3d br=_camera.UnProject(pi::Point2d(_camera.width(),_camera.height()));

        GSLAM::Point3Type  W_tl=pose*(pi::Point3d(tl.x,tl.y,1));
        GSLAM::Point3Type  W_tr=pose*(pi::Point3d(tr.x,tr.y,1));
        GSLAM::Point3Type  W_bl=pose*(pi::Point3d(bl.x,bl.y,1));
        GSLAM::Point3Type  W_br=pose*(pi::Point3d(br.x,br.y,1));

        glBegin(GL_LINES);
        glLineWidth(2.5);
        glColor3ub(color.x,color.y,color.z);
        glVertex(t);        glVertex(W_tl);
        glVertex(t);        glVertex(W_tr);
        glVertex(t);        glVertex(W_bl);
        glVertex(t);        glVertex(W_br);
        glVertex(W_tl);     glVertex(W_tr);
        glVertex(W_tr);     glVertex(W_br);
        glVertex(W_br);     glVertex(W_bl);
        glVertex(W_bl);     glVertex(W_tl);
        glEnd();
    }
}

void MapVisualizer::update()
{
    if(!_map) return ;

    GSLAM::FrameArray mapFrames;
    GSLAM::PointArray mapPoints;
    if(!_map->getFrames(mapFrames)) return;
    if(!_map->getPoints(mapPoints)) return;

    std::sort(mapFrames.begin(),mapFrames.end(),compareFr);

    std::vector<GSLAM::SIM3> keyframes;
    std::vector<Point3f> vetexTraj,gpsTraj;
    std::vector<Point3d> vetexConnection,gpsError;
    GSLAM::Point3d       boxMin(1e10,1e10,1e10),boxMax(-1e10,-1e10,-1e10);
    bool                 centerSeted=false;
    Point3d              center;
    double               minRadius=0;
    for(GSLAM::FramePtr& fr:mapFrames)
    {
        if(!_camera.isValid()) _camera=fr->getCamera();
        if(!centerSeted)
        {
            center=fr->getPose().get_translation();
            centerSeted=true;
        }
        Point3d t=fr->getPose().get_translation()-center;
        auto medianDepth=fr->getMedianDepth();
        if(medianDepth>minRadius) minRadius=medianDepth;

        keyframes.push_back(GSLAM::SIM3(fr->getPose().get_rotation(),
                                        t,
                                        fabs(medianDepth)*0.1));

        vetexTraj.push_back(t);
        boxMin.x=std::min(boxMin.x,t.x);boxMax.x=std::max(boxMax.x,t.x);
        boxMin.y=std::min(boxMin.y,t.y);boxMax.y=std::max(boxMax.y,t.y);
        boxMin.z=std::min(boxMin.z,t.z);boxMax.z=std::max(boxMax.z,t.z);

        Point3d ecef;
        if(fr->getGPSECEF(ecef))
        {
            ecef=ecef-center;
            gpsTraj.push_back(ecef);
            gpsError.push_back(t);
            gpsError.push_back(ecef);
        }

        std::map<GSLAM::FrameID,SPtr<GSLAM::FrameConnection> > children;
        if(!fr->getParents(children)) continue;
        for(std::pair<GSLAM::FrameID,SPtr<GSLAM::FrameConnection> > child:children)
        {
            GSLAM::FramePtr ch=_map->getFrame(child.first);
            if(!ch) continue;
            vetexConnection.push_back(t);
            vetexConnection.push_back(ch->getPoseScale().get_translation()-center);
        }
    }

    std::vector<Point3f>   pointCloudVertex;
    std::vector<Point3ub>  pointCloudColors;
    if(mapPoints.size())
    {
        pointCloudVertex.reserve(mapPoints.size());
        pointCloudColors.reserve(mapPoints.size());
        for(GSLAM::PointPtr& pt:mapPoints)
        {
            pointCloudVertex.push_back(pt->getPose()-center);
            auto color=pt->getColor();
            pointCloudColors.push_back(color);
        }
    }
    else
    {
        GSLAM::Point2d idepth;
        GSLAM::Point2f pt;
        GSLAM::ColorType color;
        for(GSLAM::FramePtr& fr:mapFrames)
        {
            GSLAM::Camera cam=fr->getCamera();
            GSLAM::SIM3   sim3=fr->getPoseScale();
            for(int i=0,iend=fr->keyPointNum();i<iend;i++)
            {
                if(!fr->getKeyPointIDepthInfo(i,idepth)) break;
                if(!fr->getKeyPoint(i,pt)) break;
                if(!fr->getKeyPointColor(i,color)) break;
                if(idepth.y>1000)
                {
                    pointCloudVertex.push_back(sim3*(cam.UnProject(pt)/idepth.x)-center);
                    pointCloudColors.push_back(color);//osg::Vec4(color.x/255.,color.y/255.,color.z/255.,1));
                }
            }
        }
    }

    {
        GSLAM::WriteMutex lock(_mutex);
        _keyframes=keyframes;
        _vetexTraj=vetexTraj;
        _gpsTraj=gpsTraj;
        _gpsError=gpsError;
        _vetexConnection=vetexConnection;
        _pointCloudVertex=pointCloudVertex;
        _pointCloudColors=pointCloudColors;
        _mapUpdated=true;
        _scenceCenter=(boxMax+boxMin)/2.+center;
        _scenceRadius=std::max(minRadius,(boxMax-boxMin).norm()/2);
        if(_curFrame)
            _viewPoint.get_rotation()=_curFrame->getPose().get_rotation();
        double r[9];_viewPoint.get_rotation().getMatrix(r);
        _viewPoint.get_translation()=_scenceCenter-Point3d(r[2],r[5],r[8])*double(_scenceRadius*2);
        _scenceOrigin=center;
    }

    if(_handle){
        _handle->handle(new ScenceCenterEvent(_scenceCenter));
        _handle->handle(new ScenceRadiusEvent(_scenceRadius));
        if(_firstUpdate&&_curFrame)
        {
            _handle->handle(new SetViewPoseEvent(_viewPoint));
            _firstUpdate=false;
        }
    }
}

void MapVisualizer::update(const GSLAM::FramePtr& curFrame)
{
    _curFrame=curFrame;
    Point3d t=_curFrame->getPose().get_translation();
    std::map<GSLAM::FrameID,SPtr<GSLAM::FrameConnection> > parents;
    std::vector<Point3d> curConnection;
    if(curFrame->getParents(parents))
    {
        curConnection.reserve(parents.size()*2);
        for(auto parent:parents)
        {
            GSLAM::FramePtr fr=_map->getFrame(parent.first);
            if(!fr) continue;
            curConnection.push_back(t);
            curConnection.push_back(fr->getPose().get_translation());
        }
    }
    {
        GSLAM::WriteMutex lock(_mutex);
        _curConnection=curConnection;
        _curFrameUpdated=true;
    }
}


}

