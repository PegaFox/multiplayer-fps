#ifndef MULTIPLAYER_FPS_CLIENT_INFO_HPP
#define MULTIPLAYER_FPS_CLIENT_INFO_HPP

#include <iostream>

#include <SFML/Network/IpAddress.hpp>
#include <SFML/Network/Packet.hpp>

struct ClientInfo
{
  std::optional<sf::IpAddress> address;
  uint16_t port;

  bool operator==(const ClientInfo& other) const
  {
    return address == other.address && port == other.port;
  }

  bool operator<(const ClientInfo& other) const
  {
    if (address < other.address)
    {
      return true;
    } else if (address == other.address)
    {
      return port < other.port;
    } else
    {
      return false;
    }
  }
};

sf::Packet& operator <<(sf::Packet& packet, const ClientInfo& client)
{
  if (client.address.has_value())
  {
    return packet << client.address->toInteger() << client.port;
  } else
  {
    std::cout << "note: sending an invalid address\n";
    return packet << uint32_t(0) << uint16_t(0);
  }
}

sf::Packet& operator >>(sf::Packet& packet, ClientInfo& client)
{
  uint32_t ipValue;

  packet >> ipValue >> client.port;

  client.address = sf::IpAddress(ipValue);

  return packet;
}

#endif // MULTIPLAYER_FPS_CLIENT_INFO_HPP
