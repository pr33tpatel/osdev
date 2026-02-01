#include <net/etherframe.h>

using namespace os;
using namespace os::common;
using namespace os::net;
using namespace os::drivers;

EtherFrameHandler::EtherFrameHandler(EtherFrameProvider* backend, uint16_t etherType) {
  this->etherType_BE = ((etherType & 0x00FF) << 8)
                     | ((etherType & 0xFF00) >> 8); // HACK: convert to big endian 
  this->backend = backend;
  backend->handlers[etherType_BE] = this;

};


EtherFrameHandler::~EtherFrameHandler() {
  backend->handlers[etherType_BE] = 0;
}


bool EtherFrameHandler::OnEtherFrameReceived(uint8_t* etherFramePaylod, uint32_t size) {

  return false;
}


void EtherFrameHandler::Send(uint64_t dstMAC_BE, uint8_t* data, uint32_t size) {
  backend->Send(dstMAC_BE, etherType_BE, data, size);
}

EtherFrameProvider::EtherFrameProvider(amd_am79c973* backend) 
: drivers::RawDataHandler(backend)
{
  for(uint32_t i = 0; i< 65535; i++)
    handlers[i] = 0;

}
EtherFrameProvider::~EtherFrameProvider() {

}

bool EtherFrameProvider::OnRawDataReceived(uint8_t* buffer, uint32_t size) {
  EtherFrameHeader* frame = (EtherFrameHeader*)buffer;
  bool sendBack = false;

  if(frame->dstMac_BE == 0xFFFFFFFFFFFF
  || frame->dstMac_BE == backend->GetMACAddress()) {
    if(handlers[frame->etherType_BE] != 0) {
      sendBack = handlers[frame->etherType_BE]->OnEtherFrameReceived( buffer + sizeof(EtherFrameHeader), size - sizeof(EtherFrameHeader));
    }
  }

  if (sendBack) {
    /* if the sendBack signal is true,
    the sendBack destination is the original source,
    and sendBack source is this computer*/
    frame->dstMac_BE = frame->srcMac_BE;
    frame->srcMac_BE = backend->GetMACAddress();
  }

  return sendBack;
}
void EtherFrameProvider::Send(uint64_t dstMAC_BE, uint16_t etherType_BE, uint8_t* buffer, uint32_t size) {
  uint8_t* buffer2  = (uint8_t*)MemoryManager::activeMemoryManager->malloc(sizeof(EtherFrameHeader) + size);
  EtherFrameHeader* frame = (EtherFrameHeader*)buffer2;

  frame->dstMac_BE = dstMAC_BE;
  frame->srcMac_BE = backend->GetMACAddress();
  frame->etherType_BE = etherType_BE;

  uint8_t* src = buffer;
  uint8_t* dst = buffer2 + sizeof(EtherFrameHeader);
  for(uint32_t i = 0; i < size; i++) 
    dst[i] = src[i];

  backend->Send(buffer2, size+sizeof(EtherFrameHeader));
}


uint32_t EtherFrameProvider::GetIPAddress() {
  return backend->GetIPAddress();
}
