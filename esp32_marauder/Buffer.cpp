#include "Buffer.h"
#include "PcapHeader.h"
#include "lang_var.h"

Buffer::Buffer(){
  bufA = (uint8_t*)malloc(BUF_SIZE);
  bufB = (uint8_t*)malloc(BUF_SIZE);
}

void Buffer::createFile(const char* name, bool is_pcap, bool is_gpx){
  int i=0;
  if (is_pcap) {
    do{
      fileName = "/"+String(name)+"_"+(String)i+".pcap";
      i++;
    } while(fs->exists(fileName));
  }
  else if ((!is_pcap) && (!is_gpx)) {
    do{
      fileName = "/"+String(name)+"_"+(String)i+".log";
      i++;
    } while(fs->exists(fileName));
  }
  else {
    do{
      fileName = "/"+String(name)+"_"+(String)i+".gpx";
      i++;
    } while(fs->exists(fileName));
  }

  Serial.println(fileName);
  
  file = fs->open(fileName, FILE_WRITE);
  file.close();
}

void Buffer::open(bool is_pcap){
  bufSizeA = 0;
  bufSizeB = 0;

  writing = true;

  if (is_pcap) {
    uint8_t header[marauder::kPcapGlobalHeaderSize];
    marauder::makePcapGlobalHeader(SNAP_LEN, header, true);
    write(header, sizeof(header));
  }
}

String Buffer::getFileName() {
  return this->fileName;
}

void Buffer::openFile(const char* file_name, fs::FS* fs, bool serial, bool is_pcap, bool is_gpx) {
  bool save_pcap = settings_obj.loadSetting<bool>("SavePCAP");
  if (!save_pcap) {
    this->fs = NULL;
    this->serial = false;
    writing = false;
    return;
  }
  this->fs = fs;
  this->serial = serial;
  if (this->fs) {
    createFile(file_name, is_pcap, is_gpx);
  }
  if (this->fs || this->serial) {
    open(is_pcap);
  } else {
    writing = false;
  }
}

void Buffer::pcapOpen(const char* file_name, fs::FS* fs, bool serial) {
  openFile(file_name, fs, serial, true);
}

void Buffer::logOpen(const char* file_name, fs::FS* fs, bool serial) {
  openFile(file_name, fs, serial, false);
}

void Buffer::gpxOpen(const char* file_name, fs::FS* fs, bool serial) {
  openFile(file_name, fs, serial, false, true);
}

void Buffer::addPacket(const uint8_t* rt, uint32_t rt_len, const uint8_t* payload, uint32_t payload_len) {
  if (!writing || payload == nullptr || payload_len == 0) return;

  uint32_t total_wire_len = rt_len + payload_len;
  uint32_t total_pcap_rec_len = 16 + total_wire_len; // 16-byte PCAP packet header

  // Ping-pong buffer switch without blocking
  if (useA) {
    if (bufSizeA + total_pcap_rec_len >= BUF_SIZE) {
      if (bufSizeB == 0) {
        useA = false;
      } else {
        return; // Buffers saturated, drop single packet rather than stalling WiFi driver
      }
    }
  } else {
    if (bufSizeB + total_pcap_rec_len >= BUF_SIZE) {
      if (bufSizeA == 0) {
        useA = true;
      } else {
        return;
      }
    }
  }

  uint32_t microSeconds = micros();
  uint32_t seconds = (microSeconds / 1000) / 1000;
  microSeconds -= seconds * 1000 * 1000;

  write(seconds);
  write(microSeconds);
  write(total_wire_len);
  write(total_wire_len);

  if (rt_len > 0 && rt != nullptr) {
    write(rt, rt_len);
  }
  write(payload, payload_len);
}

void Buffer::add(const uint8_t* buf, uint32_t len, bool is_pcap){
  if (!writing || buf == nullptr || len == 0) return;

  if((useA && bufSizeA + len >= BUF_SIZE && bufSizeB > 0) || (!useA && bufSizeB + len >= BUF_SIZE && bufSizeA > 0)){
    return;
  }
  
  if(useA && bufSizeA + len + 16 >= BUF_SIZE && bufSizeB == 0){
    useA = false;
  }
  else if(!useA && bufSizeB + len + 16 >= BUF_SIZE && bufSizeA == 0){
    useA = true;
  }

  uint32_t microSeconds = micros();
  uint32_t seconds = (microSeconds/1000)/1000;
  microSeconds -= seconds*1000*1000;
  
  if (is_pcap) {
    write(seconds); // ts_sec
    write(microSeconds); // ts_usec
    write(len); // incl_len
    write(len); // orig_len
  }
  
  write(buf, len); // packet payload
}

void Buffer::append(wifi_promiscuous_pkt_t *packet, int len) {
  bool save_packet = settings_obj.loadSetting<bool>(text_table4[7]);
  if (save_packet && packet != nullptr) {
    uint8_t rt_header[marauder::kRadiotapHeaderSize];
    marauder::makeRadiotapHeader(packet->rx_ctrl.rssi, packet->rx_ctrl.channel,
                                 packet->rx_ctrl.rate, rt_header);
    addPacket(rt_header, sizeof(rt_header), packet->payload, len);
  }
}

void Buffer::append(String log) {
  bool save_packet = settings_obj.loadSetting<bool>(text_table4[7]);
  if (save_packet) {
    add((const uint8_t*)log.c_str(), log.length(), false);
  }
}

void Buffer::write(int32_t n){
  uint8_t buf[4];
  buf[0] = n;
  buf[1] = n >> 8;
  buf[2] = n >> 16;
  buf[3] = n >> 24;
  write(buf,4);
}

void Buffer::write(uint32_t n){
  uint8_t buf[4];
  buf[0] = n;
  buf[1] = n >> 8;
  buf[2] = n >> 16;
  buf[3] = n >> 24;
  write(buf,4);
}

void Buffer::write(uint16_t n){
  uint8_t buf[2];
  buf[0] = n;
  buf[1] = n >> 8;
  write(buf,2);
}

void Buffer::write(const uint8_t* buf, uint32_t len){
  if(!writing || buf == nullptr || len == 0) return;
  
  if(useA){
    if (bufSizeA + len <= BUF_SIZE) {
      memcpy(&bufA[bufSizeA], buf, len);
      bufSizeA += len;
    }
  }else{
    if (bufSizeB + len <= BUF_SIZE) {
      memcpy(&bufB[bufSizeB], buf, len);
      bufSizeB += len;
    }
  }
}

void Buffer::saveFs(){
  file = fs->open(fileName, FILE_APPEND);
  if (!file) {
    Serial.println(text02+fileName+"'");
    return;
  }

  if(useA){
    if(bufSizeB > 0){
      file.write(bufB, bufSizeB);
    }
    if(bufSizeA > 0){
      file.write(bufA, bufSizeA);
    }
  } else {
    if(bufSizeA > 0){
      file.write(bufA, bufSizeA);
    }
    if(bufSizeB > 0){
      file.write(bufB, bufSizeB);
    }
  }

  file.close();
}

void Buffer::saveSerialBuffer(const uint8_t* data, uint32_t len) {
  if (data == nullptr || len == 0) return;

  const char* mark_begin = "[BUF/BEGIN]";
  const size_t mark_begin_len = strlen(mark_begin);
  const char* mark_close = "[BUF/CLOSE]";
  const size_t mark_close_len = strlen(mark_close);

  Serial.write((const uint8_t*)mark_begin, mark_begin_len);
  Serial.write(data, len);
  Serial.write((const uint8_t*)mark_close, mark_close_len);
}

void Buffer::saveSerial() {
  if(useA){
    if(bufSizeB > 0) saveSerialBuffer(bufB, bufSizeB);
    if(bufSizeA > 0) saveSerialBuffer(bufA, bufSizeA);
  } else {
    if(bufSizeA > 0) saveSerialBuffer(bufA, bufSizeA);
    if(bufSizeB > 0) saveSerialBuffer(bufB, bufSizeB);
  }
}

void Buffer::save() {
  if((bufSizeA + bufSizeB) == 0){
    return;
  }

  saving = true;

  if(this->fs) saveFs();
  if(this->serial) saveSerial();

  bufSizeA = 0;
  bufSizeB = 0;

  saving = false;
}
