#ifndef IPCCOMMON_H
#define IPCCOMMON_H

#include <cstdint>

typedef enum class IpcProtReqId : uint8_t {
    kIpcProtReqIdStore = 156,
    kIpcProtReqIdGet,
} IpcProtReqId;

const uint32_t kIpcMaxPayloadLength = 1024u;
const uint32_t kIpcProtAck = 0xdeadbeefu;

#endif // IPCCOMMON_H
