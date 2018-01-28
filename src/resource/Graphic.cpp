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

#include "Graphic.h"
#include <resource/DataManager.h>
#include "ResourceManager.h"

#include <genie/resource/SlpFile.h>
#include <genie/resource/SlpFrame.h>
#include <genie/resource/Color.h>

namespace res {

Logger &Graphic::log = Logger::getLogger("freeaoe.resource.Graphic");

//------------------------------------------------------------------------------
Graphic::Graphic(uint32_t id) :
    Resource(id, TYPE_GRAPHIC)
{
}

//------------------------------------------------------------------------------
Graphic::~Graphic()
{
}

//------------------------------------------------------------------------------
sf::Image Graphic::getImage(uint32_t frame_num, float angle)
{
    if (!data_) {
        log.error("Failed to load data");
        return sf::Image();
    }
    if (!slp_) {
        log.error("Failed to load slp");
        return sf::Image();
    }

    bool mirrored = false;
    if (data_->AngleCount > 1) {
        int lookupAngle = angleToOrientation(angle);
        if (lookupAngle > data_->AngleCount/2) {
            mirrored = true;
            lookupAngle = data_->AngleCount - lookupAngle;
        }
        frame_num += lookupAngle * data_->FrameCount;
    }

    std::unordered_map<int, sf::Image> &cache = mirrored ? m_flippedImages : m_images;

    if (cache.find(frame_num) != cache.end()) {
        return cache[frame_num];
    }
    sf::Image img = convertFrameToImage(slp_->getFrame(frame_num));

    if (mirrored) {
        img.flipHorizontally();
    }
    cache[frame_num] = img;

    return cache[frame_num];
}

sf::Image Graphic::overlayImage(uint32_t frame_num, float angle, uint8_t playerId)
{
    if (!data_) {
        log.error("Failed to load data");
        return sf::Image();
    }
    if (!slp_) {
        log.error("Failed to load slp");
        return sf::Image();
    }

    genie::PlayerColour pc  = DataManager::Inst().getPlayerColor(playerId);
    genie::PalFilePtr palette = ResourceManager::Inst()->getPalette(50500);

    bool mirrored = false;
    if (data_->AngleCount > 1) {
        int lookupAngle = angleToOrientation(angle);
        if (lookupAngle > data_->AngleCount/2) {
            mirrored = true;
            lookupAngle = data_->AngleCount - lookupAngle;
        }
        frame_num += lookupAngle * data_->FrameCount;
    }

    const genie::SlpFramePtr frame = slp_->getFrame(frame_num);
    const genie::SlpFrameData &frameData = frame->img_data;

    const int width = frame->getWidth();
    const int height = frame->getHeight();
    sf::Image img;
    img.create(width, height, sf::Color::Transparent);

    genie::Color outlineColor = (*palette)[pc.UnitOutlineColor];
    const sf::Color outline(outlineColor.r, outlineColor.g, outlineColor.b);
    for (const genie::XY pos : frameData.outline_pc_mask) {
        img.setPixel(pos.x, pos.y, outline);
    }

    if (mirrored) {
        img.flipHorizontally();
    }

    return img;
}

std::vector<GraphicPtr> Graphic::getDeltas()
{
    return m_deltas;
}

//------------------------------------------------------------------------------
ScreenPos Graphic::getHotspot(uint32_t frame_num, bool mirrored) const
{
    genie::SlpFramePtr frame = slp_->getFrame(frame_num);

    int32_t hot_spot_x = frame->hotspot_x;

    if (mirrored)
        hot_spot_x = frame->getWidth() - hot_spot_x;

    return ScreenPos(hot_spot_x, frame->hotspot_y);
}

//------------------------------------------------------------------------------
float Graphic::getFrameRate(void) const
{
    return data_->FrameDuration;
}

//------------------------------------------------------------------------------
float Graphic::getReplayDelay(void) const
{
    return data_->ReplayDelay;
}

//------------------------------------------------------------------------------
uint32_t Graphic::getFrameCount(void) const
{
    return data_->FrameCount;
}

//------------------------------------------------------------------------------
uint32_t Graphic::getAngleCount(void) const
{
    return data_->AngleCount;
}

//------------------------------------------------------------------------------
bool Graphic::load(void)
{
    if (!isLoaded()) {
        data_ = std::make_unique<genie::Graphic>(DataManager::Inst().getGraphic(getId()));

        for (const genie::GraphicDelta &delta : data_->Deltas) {
            if (delta.GraphicID < 0) {
                continue;
            }

            GraphicPtr deltaPtr = ResourceManager::Inst()->getGraphic(delta.GraphicID);

            if (!deltaPtr) {
                log.error("Failed to load delta graphic %d", delta.GraphicID);
                continue;
            }

            deltaPtr->offset_ = ScreenPos(delta.OffsetX, delta.OffsetY);
            m_deltas.push_back(deltaPtr);
        }

        if (!m_deltas.empty()) {
            std::reverse(m_deltas.begin(), m_deltas.end());
        }

        slp_ = ResourceManager::Inst()->getSlp(data_->SLP);
        if (!slp_) {
            log.warn("Failed to get slp for id %d", data_->SLP);
            slp_ = ResourceManager::Inst()->getSlp(15000); // TODO Loading grass if -1
            return false;
        }

        if (slp_->getFrameCount() != data_->FrameCount * (data_->AngleCount / 2 + 1)) {
            log.warn("Graphic [%d]: Framecount between data and slp differs (%d vs %d, %d angles, %d frames)",
                     getId(), data_->FrameCount * (data_->AngleCount / 2 + 1), slp_->getFrameCount(), data_->AngleCount, data_->FrameCount);
        }

        return Resource::load();
    }
    return true;
}

//------------------------------------------------------------------------------
void Graphic::unload(void)
{
    if (isLoaded()) {
        slp_->unload();
        Resource::unload();
    }
}

int Graphic::angleToOrientation(float angle) const
{
    // The graphics start pointing south, and goes clock-wise
    angle = - angle - M_PI_2;

    int lookupAngle = std::round(data_->AngleCount * angle / (M_PI * 2.));

    // The angle we get in isn't normalized
    while (lookupAngle > data_->AngleCount) {
        lookupAngle -= data_->AngleCount;
    }
    while (lookupAngle < 0) {
        lookupAngle += data_->AngleCount;
    }

    return lookupAngle;
}

}
