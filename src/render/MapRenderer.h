/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2012  <copyright holder> <email>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef MAP_RENDERER_H
#define MAP_RENDERER_H

#include "IRenderer.h"
#include "mechanics/Map.h"

class MapRenderer : public IRenderer
{

public:
  MapRenderer();
  virtual ~MapRenderer();
  
  virtual void update(Time time);
  
  virtual void display(void);
  
  void setMap(MapPtr map);
  
  MapPos getMapPosition(ScreenPos pos);
  
  static inline ScreenPos mapToScreenPos(MapPos mpos)
  {
    ScreenPos spos;

    spos.x = mpos.x - mpos.y;
    spos.y = mpos.z + (mpos.x + mpos.y)/2;
    
    return spos;
  }
  
  static inline MapPos screenToMapPos(ScreenPos spos)
  {
    MapPos mpos;
    
    mpos.x = (spos.x + 2*spos.y)/2;
    mpos.y = spos.y- spos.x/2.0;
    mpos.z = 0;
    
    return mpos;
  }
  
  MapPos cameraPos_;
  bool camChanged_;
  
private:
  MapPtr map_; 
  
  int xOffset_, yOffset_;    //TODO: ScreenPos?
  int rRowBegin_, rRowEnd_;
  int rColBegin_, rColEnd_;
  
  sf::RenderTexture mapTexture_;
  sf::Image mapImage_;
};

typedef boost::shared_ptr<MapRenderer> MapRendererPtr;

#endif // MAP_RENDERER_H
