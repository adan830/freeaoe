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


#include "SlpFrame.h"

#include <SFML/Graphics/Image.hpp>

#include <iostream>
#include "ColorPalette.h"

//Debug
#include <assert.h>

using sf::Uint8;
using sf::Uint16;
using sf::Uint32;

using std::auto_ptr;

SlpFrame::SlpFrame(std::istream* istr, std::streampos pos, 
                   std::streampos file_pos, ColorPalette *palette) 
                   : FileIO(istr, pos), file_pos_(file_pos), palette_(palette)
{
  palette_ = new ColorPalette("../aoe2/50500.pal"); //TODO: Warning
  
  left_edges_   = 0;
  right_edges_  = 0;
  image_        = 0;
}


SlpFrame::~SlpFrame()
{
  delete palette_; //TODO
  
  delete [] left_edges_;
  delete [] right_edges_;
  
  delete image_;
}

sf::Image* SlpFrame::getImage()
{
  return image_;
}

void SlpFrame::loadHeader()
{
  cmd_table_offset_     = readUInt32();
  outline_table_offset_ = readUInt32();
  palette_offset_       = readUInt32();
  properties_           = readUInt32();
  
  width_     = readInt32();
  height_    = readInt32();
  hotspot_x_ = readInt32();
  hotspot_y_ = readInt32();
  
 // std::cout << std::hex << (unsigned int)(tellg() - file_pos_) << std::endl;
}

void SlpFrame::load()
{
  assert(!image_); //TODO: Not implemented
  image_ = new sf::Image(width_, height_);
  
  readEdges();
  
  // Skipping command offsets. They are not needed now but
  // they can be used for checking file integrity.
  for (Uint32 i=0; i < height_; i++)
  {
    readUInt32();
  }
  
  // Each row has it's commands, 0x0F signals the end of a rows commands.
  for (Uint32 row = 0; row < height_; row++)
  {
    //std::cout << row << ": " << std::hex << (int)(tellg() - file_pos_) << std::endl;
    Uint8 data = 0;
    Uint32 pix_pos = left_edges_[row]; //pos where to start putting pixels
    
    while (data != 0x0F)
    {
      data = readUInt8();
     
      //if ( (tellg() - file_pos_) == 0x1630)
      //  std::cout << row << ": " << std::hex << (int)(tellg() - file_pos_) << " cmd: " << (int)data<< std::endl;
      if (data == 0x0F)
        break;
      
      /*
       * Command description and code snippets borrowed from  Bryce Schroeders 
       * SLPLib (bryce@lanset.com). TODO: Ask him before releasing
       */
      Uint8 cmd = data & 0xF;
      
      Uint32 pix_cnt = 0;
      //Uint32 pix_pos = left_edges_[row];
      Uint8 color_index = 0;
      
      switch (cmd) //0x00
      {
        case 0: // Lesser block copy
        case 4:
        case 8:
        case 0xC:
          pix_cnt = data >> 2;
          readPixelsToImage(row, pix_pos, pix_cnt);
          break;
        
        case 1: // lesser skip (making pixels transparent)
        case 5:
        case 9:
        case 0xD:
          pix_cnt = (data & 0xFC) >> 2;
          
          setPixelsToColor(row, pix_pos, pix_cnt, sf::Color(0,0,0,0));
          break;
          
        case 2: // greater block copy
          pix_cnt = ((data & 0xF0) << 4) + readUInt8();
          
          readPixelsToImage(row, pix_pos, pix_cnt);
          break;
          
        case 3: // greater skip
          pix_cnt = ((data & 0xF0) << 4) + readUInt8();
         
          setPixelsToColor(row, pix_pos, pix_cnt, sf::Color(0,0,0,0));
          break;
          
        case 6: // copy and transform (player color)
          pix_cnt = getPixelCountFromData(data);
 
          // TODO: player color
          readPixelsToImage(row, pix_pos, pix_cnt);
          
          break;
          
        case 7: // Run of plain color
          pix_cnt = getPixelCountFromData(data);
          
          color_index = readUInt8();
          setPixelsToColor(row, pix_pos, pix_cnt, 
                           palette_->getColorAt(color_index));
        break;
        
        case 0xA: // Transform block (player color)
          pix_cnt = getPixelCountFromData(data);
          
          // TODO: readUint8() | player_color
          color_index = readUInt8();
          setPixelsToColor(row, pix_pos, pix_cnt, 
                           palette_->getColorAt(color_index));
        break;
        
        case 0x0B: // Shadow pixels
          //TODO: incomplete
          pix_cnt = getPixelCountFromData(data);
          pix_pos += pix_cnt; //skip until implemented
          
        break;
        
        case 0x0E: // extended commands.. TODO
        
          switch (data)
          {
            /*case 0x0E: //xflip?? skip?? TODO
            case 0x1E:
              //row-= 1;   
            break;
            */
            
            case 0x4E: //Outline pixels TODO
            case 0x6E: 
              pix_pos += 1;
            break;
            
            case 0x5E: //Outline run TODO
            case 0x7E: 
              pix_cnt = readUInt8();
              pix_pos += pix_cnt;
            break;
          }

        break;
        default:
          std::cerr << "SlpFrame: Unknown cmd at " << std::hex << 
                  (int)(tellg() - file_pos_)<< ": " << (int) data << std::endl;
          break;
      }
      
    }
  }
  
}

//------------------------------------------------------------------------------
void SlpFrame::readEdges()
{
  std::streampos cmd_table_pos = file_pos_ + std::streampos(cmd_table_offset_);
  
  assert(left_edges_ == 0 && right_edges_ == 0);
  
  left_edges_ = new sf::Int16[height_];
  right_edges_= new sf::Int16[height_];
  
  sf::Uint32 row_cnt = 0;
  
  while (tellg() < cmd_table_pos)
  {
    left_edges_[row_cnt] = readInt16();
    right_edges_[row_cnt] = readInt16();
    
    if (left_edges_[row_cnt] >= 0) // if first edge is -1 skip
    {
      assert((left_edges_[row_cnt] + right_edges_[row_cnt]) < width_);
      
      // Set edges transparent
      for (sf::Uint32 i=0; i < left_edges_[row_cnt]; i++)
        image_->SetPixel(i, row_cnt, sf::Color(0,0,0,0));
      
      for (sf::Uint32 i=width_-1; i >= (width_ - right_edges_[row_cnt]); i--)
        image_->SetPixel(i, row_cnt, sf::Color(0,0,0,0));
    }
    
    row_cnt ++;
  }
 
}

//------------------------------------------------------------------------------
void SlpFrame::readPixelsToImage(Uint32 row, Uint32 &col, Uint32 count)
{
  Uint32 to_pos = col + count;
  while (col < to_pos)
  {
    Uint8 pixel_index = readUInt8();
    image_->SetPixel(col, row, palette_->getColorAt(pixel_index));
    col ++;
  }
 
}

//------------------------------------------------------------------------------
void SlpFrame::setPixelsToColor(Uint32 row, Uint32 &col, Uint32 count, 
                                sf::Color color)
{
  Uint32 to_pos = col + count;
  
  while (col < to_pos)
  {
    image_->SetPixel(col, row, color);
    col ++;
  }
}

//------------------------------------------------------------------------------
Uint8 SlpFrame::getPixelCountFromData(Uint8 data)
{
  Uint8 pix_cnt;
  
  data = (data & 0xF0) >> 4;
          
  if (data == 0)
    pix_cnt = readUInt8();
  else
    pix_cnt = data;
          
  return pix_cnt;
}

