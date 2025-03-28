#ifndef MULTIPLAYER_FPS_KEY_DATA_HPP
#define MULTIPLAYER_FPS_KEY_DATA_HPP

#include <glm/vec2.hpp>
#include <SFML/Network/Packet.hpp>

struct KeyData
{
  glm::vec2 pos;
  glm::vec2 facing;

  bool shoot;
};

sf::Packet& operator <<(sf::Packet& packet, const KeyData& keyData)
{
  return packet << keyData.pos.x << keyData.pos.y << keyData.facing.x << keyData.facing.y << keyData.shoot;
}

sf::Packet& operator >>(sf::Packet& packet, KeyData& keyData)
{
  return packet >> keyData.pos.x >> keyData.pos.y >> keyData.facing.x >> keyData.facing.y >> keyData.shoot;
}

#endif // MULTIPLAYER_FPS_KEY_DATA_HPP
