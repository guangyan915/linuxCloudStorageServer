#include <iostream>

class Test {
public:
  Test() {
    std::cout << "hello!";
  }
  static Test& getInstance();
};

Test& Test::getInstance() {
  static Test instance;
  return instance;
}

int main() {
  Test::getInstance();
}
