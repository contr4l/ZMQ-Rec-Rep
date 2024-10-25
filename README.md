# ZMQ-Rec-Rep

## Intro
zeromq recorder and replayer

## Build
Before build recorder/replayer, you need zmq and spdlog firstly.

### zmq && cppzmq
refer to [cppzmq](https://github.com/zeromq/cppzmq?tab=readme-ov-file#build-instructions)

### spdlog
refer to [spdlog](https://github.com/gabime/spdlog?tab=readme-ov-file#install)

### rec/rep
```bash
cmake -DCMAKE_INSTALL_PREFIX="./install" -S . -B build 
cmake --build build -j4
cmake --install build
```

## Data Structure

One DataPacket = Header + Body

### Header
|Field |timestamp |ip | port| topic_len| data_len|
|---|---|---|---|---|---|
|Type|u32|u32|u16|u16|u16|
|Size|4|4|2|2|2|

### Body
|Field |topic|data|
|---|---|---|
|Type|char*|u8[]|
|Size|topic_len|data_len|


## Usage
### Recorder
```cpp
void usage() {
    printf("usage: zmq_recorder [options]\n");
    printf("options:\n");
    printf("  --mode        mode for recorder, [sub: default|rcv]\n");
    printf("  --src         src ip for sender, default 127.0.0.1\n");
    printf("  --port        src port for sender, default 9090\n");
    printf("  --topic       set topic filter string for sub mode, default empty string\n");
}
```
Update:
You can also use --port "8001|8002|8003" to listen several ports.

### Replayer
```cpp
void usage() {
    printf("usage: zmq_replayer [options]\n");
    printf("options:\n");
    printf("  --file        src file for replay, default zmq.bin\n");
    printf("  --mode        mode for replayer, [pub: default|req]\n");
}
```
