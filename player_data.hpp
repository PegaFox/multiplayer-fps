#ifndef MULTIPLAYER_FPS_PLAYER_DATA_HPP
#define MULTIPLAYER_FPS_PLAYER_DATA_HPP

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "client_info.hpp"

struct PlayerData
{
  ClientInfo address;

  glm::vec2 pos;

  glm::vec2 facing;

  float health;

  uint8_t weaponBitfield;
  uint16_t weaponAmmo[8];
};

sf::Packet& operator<<(sf::Packet& packet, const PlayerData& playerData)
{
  packet << playerData.address << playerData.pos.x << playerData.pos.y << playerData.facing.x << playerData.facing.y << playerData.health << playerData.weaponBitfield;

  for (uint8_t w = 0; w < 8; w++)
  {
    packet << playerData.weaponAmmo[w];
  }

  return packet;
}

sf::Packet& operator>>(sf::Packet& packet, PlayerData& playerData)
{
  packet >> playerData.address >> playerData.pos.x >> playerData.pos.y >> playerData.facing.x >> playerData.facing.y >> playerData.health >> playerData.weaponBitfield;

  for (uint8_t w = 0; w < 8; w++)
  {
    packet >> playerData.weaponAmmo[w];
  }

  return packet;
}

#endif // MULTIPLAYER_FPS_PLAYER_DATA_HPP
