#ifndef MULTIPLAYER_FPS_CLIENT_COMM_HPP
#define MULTIPLAYER_FPS_CLIENT_COMM_HPP

#include <random>
#include <iostream>
#include <SFML/Network/Socket.hpp>
#include <SFML/Network/UdpSocket.hpp>

#include "../raycast/raycast_renderer.hpp"

#include "packet_type.hpp"
#include "client_info.hpp"
#include "key_data.hpp"
#include "player_data.hpp"
#include "map_data.hpp"

extern std::default_random_engine ranGen;

extern pf::RaycastCamera camera;

extern bool stateChanged;

extern bool shoot;

extern uint8_t playerIndex;

class ClientComm
{
  public:

    ClientComm()
    {
      socket.setBlocking(false);
      if (socket.bind(sf::Socket::AnyPort) != sf::Socket::Status::Done)
      {
        std::cout << "failed to bind socket to port\n";
      }

      connect.address = sf::IpAddress::getLocalAddress();
      connect.port = socket.getLocalPort();

      std::cout << "selected address " << connect.address->toString() << ":" << connect.port << '\n';
    }

    void connectToServer()
    {

      sf::Packet sendData;
      sendData << PacketType::Connect << connect;
      sf::Packet receiveData = sendRequest(PacketType::Connect, sendData);

      if (!receiveData.endOfPacket())
      {
        MapData world;

        receiveData >> serverData >> world;

        camera.wallMap.resize(world.height);
        for (uint16_t y = 0; y < world.height; y++)
        {
          camera.wallMap[y].resize(world.width);
          for (uint16_t x = 0; x < world.width; x++)
          {
            if (world.tiles[y*world.width + x] == 0)
            {
              camera.wallMap[y][x].fillState = pf::Wall::Empty;
            } else
            {
              camera.wallMap[y][x].fillState = pf::Wall::Filled;
              camera.wallMap[y][x].up.textured = true;
              camera.wallMap[y][x].down.textured = true;
              camera.wallMap[y][x].left.textured = true;
              camera.wallMap[y][x].right.textured = true;
              if (
                !camera.wallMap[y][x].up.texture.loadFromFile("assets/blackstone.png") ||
                !camera.wallMap[y][x].down.texture.loadFromFile("assets/blackstone.png") ||
                !camera.wallMap[y][x].left.texture.loadFromFile("assets/blackstone.png") ||
                !camera.wallMap[y][x].right.texture.loadFromFile("assets/blackstone.png"))
              {
                std::cout << "failed to load asset file \"blackstone.png\"\n";
              }
            }
            camera.wallMap[y][x].fogColor = glm::vec3(0.0f);
            camera.wallMap[y][x].fogMaxDistance = 4;
            camera.wallMap[y][x].fogMaxStrength = 0.95f;
            camera.wallMap[y][x].fogMinStrength = 0.0f;
          }
        } 

        camera.pos = glm::vec3(4.5f, 4.5f, 0.5f);
        while (world.tiles[glm::floor(camera.pos.y) * world.width + glm::floor(camera.pos.x)] == 1)
        {
          camera.pos.x = (ranGen() % world.width) + 0.5f;
          camera.pos.y = (ranGen() % world.height) + 0.5f;
        }
      }
    }

    void disconnectFromServer()
    {
      sf::Packet sendData;
      sendData << PacketType::Disconnect << connect;
      sendRequest(PacketType::Disconnect, sendData, -1);
    }

    void sendKeyInfo(sf::Window& SCREEN)
    {
      if (SCREEN.hasFocus())
      {
        KeyData keyData;

        if (SCREEN.hasFocus())
        {
          keyData.pos = glm::vec2(camera.pos);
          keyData.facing = camera.front;
          keyData.shoot = shoot;
          shoot = false;
        }

        if (stateChanged)
        {
          sf::Packet sendData;
          sendData << PacketType::Keys << keyData;

          if (socket.send(sendData, *serverData.address, serverData.port) != sf::Socket::Status::Done)
          {
            std::cout << "failed to send key data\n";
          }
          stateChanged = false;
        }
      }
    }

    void getPlayerData(std::vector<PlayerData>& players)
    {  
      //std::cout << serverData.address->toString() << ":" << serverData.port << '\n';

      sf::Packet receiveData;
      std::optional<sf::IpAddress> address;
      uint16_t port;
      if (socket.receive(receiveData, address, port) == sf::Socket::Status::Done)
      {
        PacketType type;
        receiveData >> type;

        if (type == PacketType::Players)
        {
          players.resize((receiveData.getDataSize()-sizeof(PacketType)) / 47 /*number of bytes in compressed playerData struct*/);
          //std::cout << receiveData.getDataSize() << ", " << sizeof(PacketType) << ", " << sizeof(PlayerData) << ", " << players.size() << "\n";
          for (std::size_t p = 0; p < players.size(); p++)
          {
            receiveData >> players[p];
            
            //std::cout << players[p].address.address->toString() << ":" << players[p].address.port << ", " << players[p].pos.x << ", " << players[p].pos.y << ", " << players[p].pos.z << '\n';
            if (players[p].address == connect)
            {
              playerIndex = p;
            }
          }
        }
      }

    }
  private:

    sf::Packet sendRequest(PacketType requestType, sf::Packet& sendData, uint_fast32_t timeout = 60)
    {
      for (uint_fast32_t i = 0; i < timeout; i++)
      {
        if (socket.send(sendData, *serverData.address, serverData.port) == sf::Socket::Status::Done)
        {
          std::cout << "request sent\n";

          sf::sleep(sf::seconds(0.1f));

          // if server replies, exit loop
          sf::Packet receiveData;
          std::optional<sf::IpAddress> address;
          uint16_t port;
          if (socket.receive(receiveData, address, port) == sf::Socket::Status::Done)
          {
            PacketType type;
            receiveData >> type;

            if (type == requestType)
            {
              std::cout << "received reply from server\n";

              return receiveData;
            }
          }
          
          sf::sleep(sf::seconds(0.9f));
        } else
        {
          std::cout << "failed to send request\n";
        }

      }

      std::cout << "request timed out\n";

      return sf::Packet();
    }

    ClientInfo connect;

    ClientInfo serverData = {sf::IpAddress::Broadcast, 2007};
    sf::UdpSocket socket;
};

#endif // MULTIPLAYER_FPS_CLIENT_COMM_HPP
