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


#ifndef COMMANDMOVE_H
#define COMMANDMOVE_H
#include <communication/ICommand.h>
#include <SFML/Config.hpp>
#include <global/Types.h>

class CommandMove : public ICommand
{

public:
  CommandMove(sf::Uint32 unit_id, MapPos target);
  CommandMove(const CommandMove& other);
  virtual ~CommandMove();
  
  virtual void execute(GameServer *gs);
  
private:
  sf::Uint32 unit_id_;
  MapPos target_;
};

#endif // COMMANDMOVE_H
