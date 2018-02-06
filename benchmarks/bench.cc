#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <vector>

#include <saber/client/saber.h>
#include <saber/util/countdownlatch.h>
#include <saber/util/logging.h>
#include <saber/util/timeops.h>
#include <voyager/core/bg_eventloop.h>

saber::CountDownLatch* g_latch = nullptr;

namespace saber {

class ClientWatcher : public Watcher {
 public:
  virtual void Process(const WatchedEvent& event) {
    if (event.type() == ET_NONE) {
      std::string s;
      switch (event.state()) {
        case SS_CONNECTING:
          s = "Connecting";
          break;
        case SS_CONNECTED:
          s = "Connected";
          break;
        case SS_DISCONNECTED:
          s = "Disconnected";
          break;
        case SS_EXPIRED:
          s = "Expired";
          break;
        case SS_AUTHFAILED:
          s = "AuthFailed";
          break;
        default:
          s = "Unknown";
          assert(false);
          break;
      }
      printf("Session state: %s\n", s.c_str());
    }
  }
};

class Client {
 public:
  Client(voyager::EventLoop* loop, const ClientOptions& options,
         int write_times, int read_times)
      : write_times_(write_times),
        read_times_(read_times),
        finish_(0),
        start_(0),
        end_(0),
        loop_(loop),
        client_(loop, options),
        path_(options.root) {}

  void Start() {
    client_.Connect();
    Create();
  }

  void Stop() { client_.Close(); }

  void Create() {
    CreateRequest request;
    request.set_path(path_);
    request.set_data("lock service");
    client_.Create(request, nullptr,
                   std::bind(&Client::OnCreate, this, std::placeholders::_1,
                             std::placeholders::_2, std::placeholders::_3));
  }

  void OnCreate(const std::string& path, void* context,
                const CreateResponse& response) {
    if (response.code() == RC_OK || response.code() == RC_NODE_EXISTS) {
      printf("create successful, begin to test!\n");
      g_latch->CountDown();
    } else {
      printf("create failed!\n");
      exit(0);
    }
  }

  void GetData() { loop_->RunInLoop(std::bind(&Client::GetDataInLoop, this)); }

  void GetDataInLoop() {
    start_ = NowMicros();
    GetDataRequest request;
    request.set_path(path_);
    request.set_watch(false);
    for (int i = 0; i < read_times_; ++i) {
      client_.GetData(request, nullptr, nullptr,
                      std::bind(&Client::OnGetData, this, std::placeholders::_1,
                                std::placeholders::_2, std::placeholders::_3));
    }
  }

  void OnGetData(const std::string& path, void* context,
                 const GetDataResponse& response) {
    ++finish_;
    // printf("Read finish:%d\n", finish_);
    if (finish_ == read_times_) {
      end_ = NowMicros();
      double time = (double)(end_ - start_) / 1000000;
      printf("Read Time: %f, TPS:%f\n", time, read_times_ / time);
      g_latch->CountDown();
      finish_ = 0;
    }
  }

  void SetData() { loop_->RunInLoop(std::bind(&Client::SetDataInLoop, this)); }

  void SetDataInLoop() {
    start_ = NowMicros();
    SetDataRequest request;
    request.set_path(path_);
    request.set_data("lock service");
    request.set_version(-1);
    for (int i = 0; i < write_times_; ++i) {
      client_.SetData(request, nullptr,
                      std::bind(&Client::OnSetData, this, std::placeholders::_1,
                                std::placeholders::_2, std::placeholders::_3));
    }
  }

  void OnSetData(const std::string& path, void* context,
                 const SetDataResponse& response) {
    ++finish_;
    // printf("Write finish:%d\n", finish_);
    if (finish_ == write_times_) {
      end_ = NowMicros();
      double time = (double)(end_ - start_) / 1000000;
      printf("Write Time: %f, TPS:%f\n", time, write_times_ / time);
      g_latch->CountDown();
      finish_ = 0;
    }
  }

 private:
  int write_times_;
  int read_times_;
  int finish_;
  uint64_t start_;
  uint64_t end_;
  voyager::EventLoop* loop_;
  Saber client_;
  std::string path_;

  Client(const Client&);
  void operator=(const Client&);
};

}  // namespace saber

int main(int argc, char** argv) {
  if (argc != 5) {
    printf(
        "Usage: <%s> <server_ip:server_port,...> <concurrent> <write_times> "
        "<read_times>\n",
        argv[0]);
    return -1;
  }

  // saber::SetLogHandler(nullptr);
  // saber::SetLogLevel(saber::LOGLEVEL_DEBUG);
  saber::ClientWatcher watcher;
  saber::ClientOptions options;
  // options.root = "/ls";
  options.servers = argv[1];
  // options.watcher = &watcher;

  int c = std::stoi(argv[2]);
  int write_times = std::stoi(argv[3]);
  int read_times = std::stoi(argv[4]);

  g_latch = new saber::CountDownLatch(c);
  std::vector<voyager::BGEventLoop> threads(c);
  std::vector<std::unique_ptr<saber::Client>> clients;
  for (int i = 0; i < c; ++i) {
    options.root = "/ls-" + std::to_string(i);
    clients.push_back(std::unique_ptr<saber::Client>(new saber::Client(
        threads[i].Loop(), options, write_times, read_times)));
    clients[i]->Start();
  }
  g_latch->Wait();
  printf("All create successful, begin to test!\n");
  delete g_latch;

  g_latch = new saber::CountDownLatch(c);
  uint64_t start = saber::NowMicros();
  for (int i = 0; i < c; ++i) {
    clients[i]->SetData();
  }
  g_latch->Wait();
  uint64_t end = saber::NowMicros();
  double time = (double)(end - start) / 1000000;
  printf("All Write Time:%f, TPS:%f\n", time, c * write_times / time);
  delete g_latch;

  saber::SleepForMicroseconds(1000000);

  g_latch = new saber::CountDownLatch(c);
  start = saber::NowMicros();
  for (int i = 0; i < c; ++i) {
    clients[i]->GetData();
  }
  g_latch->Wait();
  end = saber::NowMicros();
  time = (double)(end - start) / 1000000;
  printf("All Read Time:%f, TPS:%f\n", time, c * read_times / time);
  delete g_latch;

  for (int i = 0; i < c; ++i) {
    clients[i]->Stop();
  }

  return 0;
}
