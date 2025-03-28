#ifndef MULTIPLAYER_FPS_SERVER_COMM_HPP
#define MULTIPLAYER_FPS_SERVER_COMM_HPP

#include <iostream>
#include <random>
#include <fstream>
#include <map>

#include <glm/mat2x2.hpp>
#include <glm/fwd.hpp>
#include <glm/common.hpp>
#include <glm/trigonometric.hpp>

#include <SFML/Network/Socket.hpp>
#include <SFML/Network/UdpSocket.hpp>

#include "../raycast/raycast_renderer.hpp"

#include <pegafox/colliders.hpp>

#include "client_info.hpp"
#include "packet_type.hpp"
#include "key_data.hpp"
#include "weapon.hpp"
#include "player_data.hpp"
#include "map_data.hpp"

extern std::default_random_engine ranGen;

extern bool stateChanged;

class ServerComm
{
  public:

    ServerComm()
    {
      if (socket.bind(serverData.port) != sf::Socket::Status::Done)
      {
        std::cout << "failed to bind socket to port " << serverData.port << "\n";
      }
      socket.setBlocking(false);

      world.width = 32;
      world.height = 32;
      world.tiles.resize(world.width * world.height);

      for (uint16_t i = 0; i < 32; i++)
      {
        MapData updated = world;
        for (uint16_t y = 1; y < world.height-1; y++)
        {
          for (uint16_t x = 1; x < world.width-1; x++)
          {
            if (i == 0) // randomize on first pass
            {
              updated.tiles[y*world.width + x] = ranGen() % 2;
            } else
            {
              uint8_t total = 
                world.tiles[(y-1)*world.width + x - 1] +
                world.tiles[(y-1)*world.width + x] +
                world.tiles[(y-1)*world.width + x + 1] +
                world.tiles[y*world.width + x + 1] +
                world.tiles[(y+1)*world.width + x + 1] +
                world.tiles[(y+1)*world.width + x] +
                world.tiles[(y+1)*world.width + x - 1] +
                world.tiles[y*world.width + x - 1];
              if (total > 0 && total < 5)
              {
                updated.tiles[y*world.width + x] = 0;
              } else
              {
                updated.tiles[y*world.width + x] = 1;
              }
            }
          }
        }
        world = updated;
      }

      // add border
      for (uint16_t y = 0; y < world.height; y++)
      {
        for (uint16_t x = 0; x < world.width; x++)
        {
          if (x == 0 || x == world.width-1 || y == 0 || y == world.height-1)
          {
            world.tiles[y*world.width + x] = 1;
          }
        }
      }

      // flood fill
      uint8_t searchTiles[world.width*world.height];
      for (uint16_t t = 0; t < world.width*world.height; t++)
      {
        searchTiles[t] = world.tiles[t] * 2;
      }

      glm::u16vec2 startPos = {world.width/2, world.height/2};
      while (world.tiles[startPos.y*world.width + startPos.x])
      {
        startPos.x = ranGen() % world.width;
        startPos.y = ranGen() % world.height;
      }

      uint16_t nextTile = startPos.y*world.width + startPos.x;
      searchTiles[nextTile] = 1;

      while (nextTile < world.width*world.height)
      {
        searchTiles[nextTile] = 2;
        searchTiles[nextTile - world.width] = glm::max(searchTiles[nextTile - world.width], uint8_t(1));
        searchTiles[nextTile + world.width] = glm::max(searchTiles[nextTile + world.width], uint8_t(1));
        searchTiles[nextTile - 1] = glm::max(searchTiles[nextTile - 1], uint8_t(1));
        searchTiles[nextTile + 1] = glm::max(searchTiles[nextTile + 1], uint8_t(1));

        for (nextTile = 0; nextTile < world.width*world.height; nextTile++)
        {
          if (searchTiles[nextTile] == 1)
          {
            break;
          }
        }
      }

      // remove inaccessible tiles
      for (uint16_t t = 0; t < world.width*world.height; t++)
      {
        world.tiles[t] = (searchTiles[t] == 0) | world.tiles[t];
      }
    
      renderer.wallMap.resize(world.height);
      for (uint16_t y = 0; y < world.height; y++)
      {
        renderer.wallMap[y].resize(world.width);
        for (uint16_t x = 0; x < world.width; x++)
        {
          renderer.wallMap[y][x].fillState = pf::Wall::FillState(world.tiles[y*world.width + x] * int(pf::Wall::Filled));
        }
      }

      std::ofstream outFile("map.pbm");
      outFile << "P1\n" << world.width << " " << world.height << "\n";

      for (uint16_t y = 0; y < world.height; y++)
      {
        for (uint16_t x = 0; x < world.width; x++)
        {
          outFile << world.tiles[y*world.width + x] << ' ';
        }
        outFile << '\n';
      }
      outFile.close();
    }

    void receiveData()
    {
      sf::Packet receiveData;
      ClientInfo receiveAddress;

      while (socket.receive(receiveData, receiveAddress.address, receiveAddress.port) == sf::Socket::Status::Done)
      {
        PacketType type;
        receiveData >> type;

        switch (type)
        {
          case PacketType::Ping:
            break;
          case PacketType::Keys:

            updatePlayerPos(receiveData, receiveAddress);

            break;
          case PacketType::Connect:

            addClient(receiveData, receiveAddress);

            break;
          case PacketType::Disconnect:

            removeClient(receiveData, receiveAddress);

            break;
        }
      }
    }

    void sendData()
    {
      if (stateChanged)
      {
        sf::Packet sendData;
        sendData << PacketType::Players;
        for (std::pair<const ClientInfo, PlayerData>& client : clients)
        {
          sendData << client.second;
        }

        for (std::pair<const ClientInfo, PlayerData>& client : clients)
        {
          //std::cout << client.first.address->toString() << ":" << client.first.port << '\n';
          if (socket.send(sendData, *client.first.address, client.first.port) != sf::Socket::Status::Done)
          {
            std::cout << "failed to send player data\n";
          }
        }

        /*PacketType packetType;
        sendData >> packetType;
        for (uint p = 0; p < clients.size(); p++)
        {
          PlayerData playerData;
          sendData >> playerData;
          std::cout << playerData.address.address->toString() << ":" << playerData.address.port << '\n';
        }*/
      }

      stateChanged = false;
    }

  private:

    void addClient(sf::Packet& receiveData, ClientInfo& receiveAddress)
    {
      ClientInfo clientAddress;
      receiveData >> clientAddress;

      if (clientAddress == receiveAddress)
      {
        std::cout << "received connect request from " << clientAddress.address->toString() << ':' << clientAddress.port << '\n';

        glm::vec3 startPos(4.5f, 4.5f, 0.5f);
        /*while (world.tiles[glm::floor(startPos.y) * world.width + glm::floor(startPos.x)] == 1)
        {
          startPos.x = (ranGen() % world.width) + 0.5f;
          startPos.y = (ranGen() % world.height) + 0.5f;
        }*/

        clients[clientAddress] = PlayerData{clientAddress, startPos, glm::vec2(1.0f, 0.0f), 1.0f, 1, {255, 0, 0, 0, 0, 0, 0, 0}};

        sf::Packet sendData;
        sendData << PacketType::Connect << serverData << world;
        if (socket.send(sendData, *clientAddress.address, clientAddress.port) == sf::Socket::Status::Done)
        {
          std::cout << "pinged client\n";
        }

        stateChanged = true;
      }
    }

    void removeClient(sf::Packet& receiveData, ClientInfo& receiveAddress)
    {
      ClientInfo clientAddress;
      receiveData >> clientAddress;

      if (clientAddress == receiveAddress)
      {
        std::cout << "received disconnect request from " << clientAddress.address->toString() << ':' << clientAddress.port << '\n';

        std::map<ClientInfo, PlayerData>::iterator pos = clients.find(clientAddress);

        if (pos != clients.end())
        {
          clients.erase(pos);

          sf::Packet sendData;
          sendData << PacketType::Disconnect << serverData;
          if (socket.send(sendData, *clientAddress.address, clientAddress.port) == sf::Socket::Status::Done)
          {
            std::cout << "pinged client\n";
          }

          stateChanged = true;
        }

      }
    }

    void updatePlayerPos(sf::Packet& receiveData, ClientInfo& receiveAddress)
    {
      KeyData keyData;
      receiveData >> keyData;

      std::map<ClientInfo, PlayerData>::iterator client = clients.find(receiveAddress);
      if (keyData.pos != client->second.pos || keyData.facing != client->second.facing || keyData.shoot)
      {
        stateChanged = true;

        client->second.pos = keyData.pos;
        client->second.facing = keyData.facing;

        if (keyData.shoot && client->second.weaponAmmo[(uint8_t)Weapon::Pistol] > 0)
        {
          client->second.weaponAmmo[(uint8_t)Weapon::Pistol]--;
          pf::RayCastData hit = renderer.castRay(glm::vec2(client->second.pos.x, client->second.pos.y), client->second.facing)[0];
          //std::cout << "fired shot from (" << client.second.pos.x << ", " << client.second.pos.y << "), to (" << hit.hitPos.x << ", " << hit.hitPos.y << ")\n";

          float lowDis = INFINITY;
          PlayerData* closest = nullptr;
          for (std::pair<const ClientInfo, PlayerData>& target: clients)
          {
            if (&target == &(*client))
            {
              continue;
            }

            glm::vec2 collidePoint = pf::lineClosestPoint(glm::vec2(client->second.pos.x, client->second.pos.y), hit.hitPos, glm::vec2(target.second.pos.x, target.second.pos.y));
                
            if (glm::distance(glm::vec2(target.second.pos.x, target.second.pos.y), collidePoint) < 0.33f)
            {
              float playerDis = glm::distance(glm::vec2(target.second.pos.x, target.second.pos.y), glm::vec2(client->second.pos.x, client->second.pos.y));

              if (playerDis < lowDis)
              {
                lowDis = playerDis;
                closest = &target.second;
              }
            }
          }

          if (closest)
          {
            closest->health -= 0.05f;
          }
        
          /*glm::vec3 normVel = glm::vec3((vel.x > 0)*2.0f - 1.0f, (vel.y > 0)*2.0f - 1.0f, 0.0f) * 0.2f;

          if (world.tiles[glm::floor(client.second.pos.y) * world.width + glm::floor(client.second.pos.x+normVel.x)] == 1)
          {
            vel.x = 0.0f;
          }
          if (world.tiles[glm::floor(client.second.pos.y+normVel.y) * world.width + glm::floor(client.second.pos.x)] == 1)
          {
            vel.y = 0.0f;
          }

          client.second.pos += vel;*/

          //std::cout << "to (" << client.second.pos.x << ", " << client.second.pos.y << ", " << client.second.pos.z << ")\n";

          /*float ang = -keyData.mouseDelta*0.008f;
          client.second.facing = glm::mat2(glm::cos(ang), -glm::sin(ang), glm::sin(ang), glm::cos(ang)) * client.second.facing;*/

        }
      }
    }

    MapData world;

    pf::RaycastCamera renderer;

    std::map<ClientInfo, PlayerData> clients;

    ClientInfo serverData = {sf::IpAddress::getLocalAddress(), 2007};

    sf::UdpSocket socket;

};

#endif // MULTIPLAYER_FPS_SERVER_COMM_HPP
