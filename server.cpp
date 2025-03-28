#include <ctime>
#include <random>

std::default_random_engine ranGen(std::time(nullptr));

bool stateChanged;

#define USE_PEGAFOX_COLLIDERS_IMPLEMENTATION
#include "server_comm.hpp"

ServerComm comm;

int main()
{
  while (true)
  {
    comm.receiveData();

    comm.sendData();
  }
  return 0;
}
