#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <cmath>
#include <random>
#include <iostream>

#include <glm/gtx/rotate_vector.hpp>

#define USE_PEGAFOX_UTILS_IMPLEMENTATION
#include <pegafox/utils.hpp>

#define USE_PEGAFOX_COLLIDERS_IMPLEMENTATION
#include <pegafox/colliders.hpp>

//#include <pegafox/raycast_renderer.hpp>
#include "../raycast/raycast_renderer.hpp"

#include "player_data.hpp"

std::default_random_engine ranGen(std::time(nullptr));

pf::RaycastCamera camera;

bool stateChanged = true;

bool shoot = false;

bool mouseGrabbed = false;

#include "client_comm.hpp"

ClientComm comm;

uint8_t playerIndex;
std::vector<PlayerData> players;

int main()
{
  pf::FPS fpsClock;
  
  camera.res.x = 512;
  camera.res.y = 512;
  camera.renderDistance = 64;
  if (!camera.ceilingImg.loadFromFile("assets/blackstone_top.png"))
  {
    std::cout << "failed to load asset file \"player_dark.png\"\n";
  }
  //camera.ceilingScale = 64.0f;
  if (!camera.floorImg.loadFromFile("assets/coal_block.png"))
  {
    std::cout << "failed to load asset file \"player_dark.png\"\n";
  }
  //camera.floorScale = 64.0f;
  camera.right.x = 0.5f;

  sf::Texture playerTex;
  if (!playerTex.loadFromFile("assets/player_dark.png"))
  {
    std::cout << "failed to load asset file \"player_dark.png\"\n";
  }
  sf::RectangleShape playerSprite;
  playerSprite.setTexture(&playerTex);
  playerSprite.setOrigin(sf::Vector2f(0.5f, 0.5f));
  playerSprite.setSize(sf::Vector2f(256.0f, 256.0f));

  sf::RectangleShape healthbar;

  sf::Font font("assets/3270NerdFont-Regular.ttf");

  sf::Text ammo(font);
  ammo.setPosition(sf::Vector2f(4.0f, 36.0f));

  sf::RenderWindow SCREEN(sf::VideoMode(sf::Vector2u(512, 512)), "Sandbox");
  SCREEN.setFramerateLimit(60);

  camera.pos = glm::vec3(1.0f, 1.0f, 0.5f);

  comm.connectToServer();

  while (SCREEN.isOpen())
  {
    while (std::optional<sf::Event> event = SCREEN.pollEvent())
    {
      if (event->is<sf::Event::Closed>())
      {
        SCREEN.close();
        comm.disconnectFromServer();
      } else if (const sf::Event::MouseMoved* move = event->getIf<sf::Event::MouseMoved>())
      {
        if (mouseGrabbed)
        {
          glm::i16vec2 mDelta(glm::vec2(move->position.x - 256, move->position.y - 256) * 0.5f);
          
          camera.front = glm::rotate(camera.front, mDelta.x*0.001f);
          camera.right = glm::rotate(camera.right, mDelta.x*0.001f);

          camera.facing = glm::clamp(camera.facing - mDelta.y*0.003f, -0.5f, 0.5f);
        }
      } else if (event->is<sf::Event::MouseButtonPressed>())
      {
        mouseGrabbed = !mouseGrabbed;
        SCREEN.setMouseCursorGrabbed(mouseGrabbed);
        SCREEN.setMouseCursorVisible(!mouseGrabbed);
      } else if (const sf::Event::KeyReleased* key = event->getIf<sf::Event::KeyReleased>())
      {
        if (key->code == sf::Keyboard::Key::Space)
        {
          shoot = true;
        }
      }
    }
    SCREEN.setTitle(std::to_string(int(fpsClock.get_fps())));
    
    if (mouseGrabbed)
    {
      sf::Mouse::setPosition(sf::Vector2i(256, 256), SCREEN);
    }

    if (SCREEN.hasFocus())
    {
      if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
      {
        camera.pos += glm::vec3(camera.front*0.05f, 0.0f);
        stateChanged = true;
      }

      if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
      {
        camera.pos -= glm::vec3(camera.front*0.05f, 0.0f);
        stateChanged = true;
      }

      if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
      {
        camera.pos -= glm::vec3(camera.right*0.05f, 0.0f);
        stateChanged = true;
      }

      if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
      {
        camera.pos += glm::vec3(camera.right*0.05f, 0.0f);
        stateChanged = true;
      }
    }

    comm.sendKeyInfo(SCREEN);

    comm.getPlayerData(players);
    //std::cout << camera.front.x << ", " << camera.front.y << '\n';
    SCREEN.clear(sf::Color(32, 32, 32));

    for (const PlayerData& player: players)
    {
      if (&player == &players[playerIndex])
      {
        continue;
      }

      uint8_t xPos = glm::round(glm::mod(pf::getAngle(player.pos, camera.pos)-pf::getAngle(glm::vec2(0.0f), player.facing), glm::two_pi<float>()) / M_PI_4) * 32;
      playerSprite.setTextureRect(sf::IntRect(sf::Vector2i(xPos, 0), sf::Vector2i(32, 32)));
      //std::cout << (int)xPos << '\n';
      camera.sprite(glm::vec3(player.pos, 0.5f), playerSprite);
      //std::cout << player.pos.x << ", " << player.pos.y << ", " << player.pos.z << "\n";
    }
    
    camera.update(SCREEN);
    
    healthbar.setSize(sf::Vector2f(players[playerIndex].health * 200.0f, 32.0f));
    healthbar.setFillColor(sf::Color((1.0f-players[playerIndex].health)*255, players[playerIndex].health*255, 0));
    SCREEN.draw(healthbar);

    ammo.setString(std::to_string(players[playerIndex].weaponAmmo[0]));
    SCREEN.draw(ammo);

    SCREEN.display();
  }
  return 0;
}

