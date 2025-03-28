#ifndef MULTIPLAYER_FPS_MAP_DATA_HPP
#define MULTIPLAYER_FPS_MAP_DATA_HPP

#include <cstdint>
#include <vector>
#include <SFML/Network/Packet.hpp>

struct MapData
{
  uint16_t width;
  uint16_t height;

  std::vector<bool> tiles;
};

sf::Packet& operator <<(sf::Packet& packet, const MapData& mapData)
{
  packet << mapData.width << mapData.height;

  for (uint32_t t = 0; t < mapData.width*mapData.height / 8; t++)
  {
    packet << uint8_t(
      (mapData.tiles[t*8] << 7) |
      (mapData.tiles[t*8 + 1] << 6) |
      (mapData.tiles[t*8 + 2] << 5) |
      (mapData.tiles[t*8 + 3] << 4) |
      (mapData.tiles[t*8 + 4] << 3) |
      (mapData.tiles[t*8 + 5] << 2) |
      (mapData.tiles[t*8 + 6] << 1) |
      (mapData.tiles[t*8 + 7]));
  }

  if (mapData.width*mapData.height % 8)
  {
    uint8_t tile = 0;
    for (uint32_t t = 0; t < mapData.width*mapData.height % 8; t++)
    {
      tile |= mapData.tiles[t*8 + t] << (7 - t);
    }
    packet << tile;
  }

  return packet;
}

sf::Packet& operator >>(sf::Packet& packet, MapData& mapData)
{
  packet >> mapData.width >> mapData.height;

  mapData.tiles.resize(mapData.width*mapData.height);

  for (uint32_t t = 0; t < mapData.width*mapData.height / 8; t++)
  {
    uint8_t tile;
    packet >> tile;

    mapData.tiles[t*8] = tile & 0x80;
    mapData.tiles[t*8 + 1] = tile & 0x40;
    mapData.tiles[t*8 + 2] = tile & 0x20;
    mapData.tiles[t*8 + 3] = tile & 0x10;
    mapData.tiles[t*8 + 4] = tile & 0x08;
    mapData.tiles[t*8 + 5] = tile & 0x04;
    mapData.tiles[t*8 + 6] = tile & 0x02;
    mapData.tiles[t*8 + 7] = tile & 0x01;
  }

  if (mapData.width*mapData.height % 8)
    {
    uint8_t tile;
    packet >> tile;
    for (uint32_t t = 0; t < mapData.width*mapData.height % 8; t++)
    {
      mapData.tiles[t*8 + t] = tile & (0x80 >> t);
    }
  }

  return packet;
}

#endif // MULTIPLAYER_FPS_MAP_DATA_HPP
