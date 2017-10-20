#include <iostream>

#include "saber/util/runloop.h"

using namespace std;
using namespace saber;

int main() {
  RunLoop loop;
  loop.RunEvery(10 * 1000 * 1000, []() {
    static int i = 0;
    cout << "test " << i++ << endl;
  });
  loop.Loop();
  return 0;
}
