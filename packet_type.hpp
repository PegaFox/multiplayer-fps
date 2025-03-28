#ifndef MULTIPLAYER_FPS_PACKET_TYPE_HPP
#define MULTIPLAYER_FPS_PACKET_TYPE_HPP

#include <cstdint>
#include <SFML/Network/Packet.hpp>

enum class PacketType: uint8_t
{
  // a ping must always be returned
  Ping,

  // contains an array of player objects
  Players,

  // contains a bitfield of key states
  Keys,

  // contains the ip address and port of the sender. Must be returned
  Connect,

  // contains the ip address and port of the sender. Must be returned
  Disconnect
};

sf::Packet& operator <<(sf::Packet& packet, const PacketType& type)
{
  return packet << (const uint8_t&)type;
}

sf::Packet& operator >>(sf::Packet& packet, PacketType& type)
{
  return packet >> (uint8_t&)type;
}

#endif // MULTIPLAYER_FPS_PACKET_TYPE_HPP
