/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  <copyright holder> <email>

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


#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <mechanics/IState.h>

class GameClient;
class GameServer;
namespace sf {
class RenderTarget;
}

class RenderGame;
class GameManager;
//------------------------------------------------------------------------------
/// State where the game is processed
//
class GameState : public IState
{

public:
  GameState(sf::RenderTarget *render_target);
  virtual ~GameState();
  
  virtual void init();
  
  virtual void draw();
  virtual void update();
  virtual void handleEvent(sf::Event event);
    
private:
  
  GameState(const GameState& other);
  
  GameManager *game_manager_;
  
  GameServer *game_server_;
  GameClient *game_client_;
  RenderGame *game_renderer_;
  
  sf::RenderTarget *render_target_;
};

#endif // GAMESTATE_H