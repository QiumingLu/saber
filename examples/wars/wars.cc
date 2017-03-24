#include <saber/client/saber_client.h>

int main() {
  voyager::EventLoop loop;
  saber::SaberClient client(&loop, "127.0.0.1:8888");
  client.Start();
  loop.Loop();
  return 0;
}
