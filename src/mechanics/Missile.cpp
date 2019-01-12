#include "Missile.h"

#include "resource/LanguageManager.h"
#include "mechanics/UnitManager.h"
#include "mechanics/Map.h"
#include "core/Constants.h"

#include <genie/dat/Unit.h>

Missile::Missile(const genie::Unit &data, const Unit::Ptr &sourceUnit, const MapPos &target) :
    Entity(Type::Missile, LanguageManager::getString(data.LanguageDLLName) + " (" + std::to_string(data.ID) + ")", sourceUnit->map()),
    m_sourceUnit(sourceUnit),
    m_data(data),
    m_targetPosition(target)
{
    m_attacks = sourceUnit->data()->Combat.Attacks;
    sourceUnit->activeMissiles++;
    DBG << sourceUnit->activeMissiles;
    DBG << "Firing at" << target;
    defaultGraphics = AssetManager::Inst()->getGraphic(data.StandingGraphic.first);
    m_renderer.setGraphic(defaultGraphics);
}

Missile::~Missile()
{
    Unit::Ptr sourceUnit = m_sourceUnit.lock();
    if (sourceUnit) {
        sourceUnit->activeMissiles--;
        DBG << sourceUnit->activeMissiles;
    }
}

void Missile::setBlastType(const BlastType type, const float radius)
{
    m_blastType = type;

    switch(type) {
    case DamageNearby:
        m_blastRadius = radius;
        break;
    case DamageResources:
        break;
    case DamageTrees:
        break;
    case DamageTargetOnly:
        break;
    default:
        m_blastType = DamageTargetOnly;
        break;
    }
}


bool Missile::update(Time time)
{
    if (isExploding()) {
        m_renderer.setCurrentFrame(m_renderer.currentFrame() + 1);
        return true;
    }

    if (!m_isFlying) {
        return false;
    }

    MapPtr map = m_map.lock();
    if (!map) {
        WARN << "No map!";
        return false;
    }
    if (position().z <= map->elevationAt(position())) {
        DBG << "we hit the ground";
        m_renderer.setGraphic(AssetManager::Inst()->getGraphic(m_data.DyingGraphic));
        m_isFlying = false;
        return false;
    }

    if (m_previousUpdateTime == 0) {
        const float distance = position().distance(m_targetPosition);
        m_angle = std::atan2(m_targetPosition.y - position().y, m_targetPosition.x - position().x);
        float flightTime = distance / m_data.Speed;
        float timeToApex = flightTime / 2;
        m_zVelocity = m_data.Missile.ProjectileArc * distance / timeToApex;
        m_zAcceleration = m_zVelocity / timeToApex;
        m_previousUpdateTime = time;
        m_startingElevation = position().z;
        return false;
    }

    const float elapsed = time - m_previousUpdateTime;
    m_previousUpdateTime = time;

    const float movement = elapsed * m_data.Speed * 0.15;

    MapPos newPos = position();
    newPos.x += movement * cos(m_angle);
    newPos.y += movement * sin(m_angle);

    m_zVelocity -= m_zAcceleration * elapsed * 0.15;
    newPos.z += m_zVelocity * elapsed * 0.15;

    int tileX = newPos.x / Constants::TILE_SIZE;
    int tileY = newPos.y / Constants::TILE_SIZE;

    if (!map->isValidTile(tileX, tileY)) {
        DBG << "out of bounds";
        m_isFlying = false;
        return false;
    }

    setPosition(newPos);

    std::vector<Unit::Ptr> hitUnits;

    for (int dx = tileX-1; dx<=tileX+1; dx++) {
        for (int dy = tileY-1; dy<=tileY+1; dy++) {
            const std::vector<std::weak_ptr<Entity>> &entities = map->entitiesAt(dx, dy);
            if (entities.empty()) {
                continue;
            }

            for (const std::weak_ptr<Entity> &entity : entities) {
                Unit::Ptr otherUnit = Entity::asUnit(entity);
                if (IS_UNLIKELY(!otherUnit)) {
                    continue;
                }

                if (newPos.z > otherUnit->data()->Size.z) {
                    continue;
                }

                const float xSize = (otherUnit->data()->Size.x + m_data.Size.x + m_blastRadius) * Constants::TILE_SIZE;
                const float ySize = (otherUnit->data()->Size.y + m_data.Size.y + m_blastRadius) * Constants::TILE_SIZE;
                const float xDistance = std::abs(otherUnit->position().x - newPos.x);
                const float yDistance = std::abs(otherUnit->position().y - newPos.y);

                if (IS_UNLIKELY(xDistance < xSize && yDistance < ySize)) {
                    hitUnits.push_back(otherUnit);
                    break;
                }
            }
        }
    }

    if (hitUnits.empty()) {
        return true;
    }

    m_isFlying = false;
    m_renderer.setGraphic(AssetManager::Inst()->getGraphic(m_data.DyingGraphic));

    for (Unit::Ptr &hitUnit : hitUnits) {
        DBG << "hit a unit" << hitUnit->debugName;
        for (const genie::unit::AttackOrArmor &attack : m_attacks) {
            hitUnit->takeDamage(attack);
        }
    }

    return true;
}