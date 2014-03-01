#ifndef _propagation_
#define _propagation_

#include "Track.h"
#include "Matrix.h"

TrackState propagateLineToR(TrackState& inputState, float r) {

  bool dump = false;

  SVector6& par = inputState.parameters;
  SMatrix66& err = inputState.errors;

  //straight line for now
  float r0 = sqrt(par.At(0)*par.At(0)+par.At(1)*par.At(1));
  float dr = r-r0;
  float pt = sqrt(par.At(3)*par.At(3)+par.At(4)*par.At(4));
  float path = dr/pt;//this works only if direction is along radius, i.e. origin is at (0,0)

  TrackState result;

  SMatrixSym66 propMatrix = ROOT::Math::SMatrixIdentity();
  propMatrix(0,3)=path;
  propMatrix(1,4)=path;
  propMatrix(2,5)=path;
  result.parameters=propMatrix*par;
  if (dump) {
    //test R of propagation
    std::cout << "initial R=" << r0 << std::endl;
    std::cout << "target R=" << r << std::endl;
    std::cout << "arrived at R=" << sqrt(result.parameters[0]*result.parameters[0]+result.parameters[1]*result.parameters[1]) << std::endl;
  }

  SMatrixSym66 errorProp = ROOT::Math::SMatrixIdentity();
  errorProp(0,3)=path*path;
  errorProp(1,4)=path*path;
  errorProp(2,5)=path*path;
  result.errors=err*errorProp;

  return result;
}

#include "Math/Vector2D.h"
#include "Math/Point2D.h"
typedef ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<float> > Vector2D;
typedef ROOT::Math::PositionVector2D<ROOT::Math::Cartesian2D<float> > Point2D;

TrackState propagateHelixToR(TrackState& inputState, float r) {

  bool dump = false;

  float xin = inputState.parameters.At(0);
  float yin = inputState.parameters.At(1);
  float pxin = inputState.parameters.At(3);
  float pyin = inputState.parameters.At(4);
  float pzin = inputState.parameters.At(5);

  float pt2 = pxin*pxin+pyin*pyin;
  float pt = sqrt(pt2);
  //p=0.3Br => r=p/(0.3*B)
  float k=100./(0.3*3.8);//fixme 0.3 more precise
  float curvature = pt*k;//in cm
  if (dump) std::cout << "curvature=" << curvature << std::endl;
  float ctgTheta=pzin/pt;

  float totalDistance = 0;
  float r0in = sqrt(xin*xin+yin*yin);
  float dTDdx = -xin/r0in;
  float dTDdy = -yin/r0in;
  float dTDdpx = 0.;
  float dTDdpy = 0.;

  //make a copy for now...
  SVector6 par = inputState.parameters;
  SMatrix66 err = inputState.errors;

  //5 iterations is a good starting point
  unsigned int Niter = 5;
  for (unsigned int i=0;i<Niter;++i) {

    if (dump) std::cout << "propagation iteration #" << i << std::endl;

    float x = par.At(0);
    float y = par.At(1);
    float z = par.At(2);
    float px = par.At(3);
    float py = par.At(4);
    float pz = par.At(5);

    float r0 = sqrt(x*x+y*y);
    float r0inv = 1./r0;
    if (dump) std::cout << "r0=" << r0 << " pt=" << pt << std::endl;
    
    float distance = r-r0;//fixme compute real distance between two points
    totalDistance+=distance;
    if (dump) std::cout << "distance=" << distance << std::endl;  
    float angPath = distance/curvature;
    if (dump) std::cout << "angPath=" << angPath << std::endl;
    float cosAP=cos(angPath);
    float sinAP=sin(angPath);

    par.At(0) = x + k*(px*sinAP-py*(1-cosAP));
    par.At(1) = y + k*(py*sinAP+px*(1-cosAP));
    par.At(2) = z + distance*ctgTheta;

    par.At(3) = px*cosAP-py*sinAP;
    par.At(4) = py*cosAP-px*sinAP;
    par.At(5) = pz;
    
    if (i+1!=Niter) {
      //update derivative on D
      float dAPdx = -x/(r0*curvature);
      float dAPdy = -y/(r0*curvature);
      float dAPdpx = -angPath*px/pt2;
      float dAPdpy = -angPath*py/pt2;

      float dxdx = 1 + k*dAPdx*(px*sinAP + py*cosAP);
      float dxdy = k*dAPdy*(px*sinAP + py*cosAP);
      float dydx = k*dAPdx*(py*sinAP - px*cosAP);
      float dydy = 1 + k*dAPdy*(py*sinAP - px*cosAP);

      float dxdpx = k*(sinAP + px*cosAP*dAPdpx - py*sinAP*dAPdpx);
      float dxdpy = k*(px*cosAP*dAPdpy - 1. + cosAP - py*sinAP*dAPdpy);
      float dydpx = k*(py*cosAP*dAPdpy - 1. + cosAP - px*sinAP*dAPdpx);
      float dydpy = k*(sinAP + py*cosAP*dAPdpy - px*sinAP*dAPdpy);

      dTDdx -= r0inv*(x*dxdx + y*dydx);
      dTDdy -= r0inv*(x*dxdy + y*dydy);
      dTDdpx -= r0inv*(x*dxdpx + y*dydpx);
      dTDdpy -= r0inv*(x*dxdpy + y*dydpy);
    }

    if (dump) std::cout << par.At(0) << " " << par.At(1) << " " << par.At(2) << std::endl;
    if (dump) std::cout << par.At(3) << " " << par.At(4) << " " << par.At(5) << std::endl;

  }

  float totalAngPath=totalDistance/curvature;

  float TD=totalDistance;
  float TP=totalAngPath;
  float C=curvature;

  float dCdpx = k*pxin/pt;
  float dCdpy = k*pyin/pt;

  float dTPdx = dTDdx/C;
  float dTPdy = dTDdy/C;
  float dTPdpx = (dTDdpx*C - TD*dCdpx)/(C*C);
  float dTPdpy = (dTDdpy*C - TD*dCdpy)/(C*C);

  float cosTP = cos(TP);
  float sinTP = sin(TP);

  //par.At(0) = xin + k*(pxin*sinTP-pyin*(1-cosTP));
  //par.At(1) = yin + k*(pyin*sinTP+pxin*(1-cosTP));
  //par.At(2) = zin + TD*ctgTheta;

  float dxdx = 1 + k*dTPdx*(pxin*sinTP + pyin*cosTP);
  float dxdy = k*dTPdy*(pxin*sinTP + pyin*cosTP);
  float dydx = k*dTPdx*(pyin*sinTP - pxin*cosTP);
  float dydy = 1 + k*dTPdy*(pyin*sinTP - pxin*cosTP);

  float dxdpx = k*(sinTP + pxin*cosTP*dTPdpx - pyin*sinTP*dTPdpx);
  float dxdpy = k*(pxin*cosTP*dTPdpy - 1. + cosTP - pyin*sinTP*dTPdpy);
  float dydpx = k*(pyin*cosTP*dTPdpy - 1. + cosTP - pxin*sinTP*dTPdpx);
  float dydpy = k*(sinTP + pyin*cosTP*dTPdpy - pxin*sinTP*dTPdpy);

  float dzdx = dTDdx*ctgTheta;
  float dzdy = dTDdy*ctgTheta;

  float dzdpx = dTDdpx*ctgTheta + TD*pzin*pxin/pt;
  float dzdpy = dTDdpy*ctgTheta + TD*pzin*pyin/pt;
  float dzdpz = TD/pt;

  //par.At(3) = pxin*cosTP-pyin*sinTP;
  //par.At(4) = pyin*cosTP-pxin*sinTP;
  //par.At(5) = pzin;

  float dpxdx = -dTPdx*(pxin*sinTP - pyin*cosTP);
  float dpxdy = -dTPdy*(pxin*sinTP - pyin*cosTP);
  float dpydx = -dTPdx*(pyin*sinTP - pxin*cosTP);
  float dpydy = -dTPdy*(pyin*sinTP - pxin*cosTP);
  
  float dpxdpx = cosTP - dTPdpx*(pxin*sinTP + pyin*cosTP);
  float dpxdpy = -sinTP - dTPdpy*(pxin*sinTP + pyin*cosTP);
  float dpydpx = -sinTP - dTPdpx*(pyin*sinTP + pxin*cosTP);
  float dpydpy = cosTP - dTPdpy*(pyin*sinTP + pxin*cosTP);

  SMatrixSym66 errorProp = ROOT::Math::SMatrixIdentity();
  errorProp(0,0)=dxdx*dxdx;
  errorProp(0,1)=dxdy*dxdy;
  errorProp(0,3)=dxdpx*dxdpx;
  errorProp(0,4)=dxdpy*dxdpy;

  errorProp(1,0)=dydx*dydx;
  errorProp(1,1)=dydy*dydy;
  errorProp(1,3)=dydpx*dydpx;
  errorProp(1,4)=dydpy*dydpy;

  errorProp(2,0)=dzdx*dzdx;
  errorProp(2,1)=dzdy*dzdy;
  errorProp(2,3)=dzdpx*dzdpx;
  errorProp(2,4)=dzdpy*dzdpy;
  errorProp(2,5)=dzdpz*dzdpz;

  errorProp(3,0)=dpxdx*dpxdx;
  errorProp(3,1)=dpxdy*dpxdy;
  errorProp(3,3)=dpxdpx*dpxdpx;
  errorProp(3,4)=dpxdpy*dpxdpy;

  errorProp(4,0)=dpydx*dpydx;
  errorProp(4,1)=dpydy*dpydy;
  errorProp(4,3)=dpydpx*dpydpx;
  errorProp(4,4)=dpydpy*dpydpy;

  TrackState result;
  result.parameters=par;
  result.errors=err*errorProp;
  if (dump) dumpMatrix(result.errors);
  return result;
}

#endif