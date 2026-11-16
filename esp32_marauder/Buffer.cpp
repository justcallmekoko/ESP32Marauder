#include "Buffer.h"
#include "PcapHeader.h"
#include "lang_var.h"

Buffer::Buffer(){
  bufA = (uint8_t*)malloc(BUF_SIZE);
  bufB = (uint8_t*)malloc(BUF_SIZE);
}

void Buffer::createFile(const char* name, bool is_pcap, bool is_gpx){
  int i=0;
  String prefix = directory ? String(directory) + "/" : "/";
  if (is_pcap) {
    do{
      fileName = prefix+String(name)+"_"+(String)i+".pcap";
      i++;
    } while(fs->exists(fileName));
  }
  else if ((!is_pcap) && (!is_gpx)) {
    do{
      fileName = prefix+String(name)+"_"+(String)i+".log";
      i++;
    } while(fs->exists(fileName));
  }
  else {
    do{
      fileName = prefix+String(name)+"_"+(String)i+".gpx";
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

  bufSizeB = 0;

  writing = true;

  if (is_pcap) {
    uint8_t header[marauder::kPcapGlobalHeaderSize];
    marauder::makePcapGlobalHeader(SNAP_LEN, header);
    write(header, sizeof(header));
  }
}

String Buffer::getFileName() {
  return this->fileName;
}

void Buffer::setDirectory(const char* path) {
  directory = path;
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

void Buffer::add(const uint8_t* buf, uint32_t len, bool is_pcap){
  constexpr uint32_t pcap_record_header_len = 16;
  const uint32_t record_header_len = is_pcap ? pcap_record_header_len : 0;
  if (!writing || !buf || len > BUF_SIZE || record_header_len + len > BUF_SIZE) return;

  portENTER_CRITICAL(&bufferMux);

  // buffer is full -> drop packet
  if((useA && bufSizeA + record_header_len + len > BUF_SIZE && bufSizeB > 0) ||
     (!useA && bufSizeB + record_header_len + len > BUF_SIZE && bufSizeA > 0)){
    //Serial.print(";"); 
    portEXIT_CRITICAL(&bufferMux);
    return;
  }
  
  if(useA && bufSizeA + record_header_len + len > BUF_SIZE && bufSizeB == 0){
    useA = false;
    //Serial.println("\nswitched to buffer B");
  }
  else if(!useA && bufSizeB + record_header_len + len > BUF_SIZE && bufSizeA == 0){
    useA = true;
    //Serial.println("\nswitched to buffer A");
  }

  uint32_t microSeconds = micros(); // e.g. 45200400 => 45s 200ms 400us
  uint32_t seconds = (microSeconds/1000)/1000; // e.g. 45200400/1000/1000 = 45200 / 1000 = 45s

  microSeconds -= seconds*1000*1000; // e.g. 45200400 - 45*1000*1000 = 45200400 - 45000000 = 400us (because we only need the offset)
  
  if (is_pcap) {
    uint8_t header[pcap_record_header_len];
    const uint32_t values[] = {seconds, microSeconds, len, len};
    for (size_t i = 0; i < 4; i++) {
      header[i * 4] = values[i];
      header[i * 4 + 1] = values[i] >> 8;
      header[i * 4 + 2] = values[i] >> 16;
      header[i * 4 + 3] = values[i] >> 24;
    }
    writeUnlocked(header, sizeof(header));
  }
  
  writeUnlocked(buf, len); // packet payload
  portEXIT_CRITICAL(&bufferMux);
}

void Buffer::append(wifi_promiscuous_pkt_t *packet, int len) {
  bool save_packet = settings_obj.loadSetting<bool>(text_table4[7]);
  if (save_packet) {
    add(packet->payload, len, true);
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
  if(!writing || !buf || len > BUF_SIZE) return;
  portENTER_CRITICAL(&bufferMux);
  if (!saving) writeUnlocked(buf, len);
  portEXIT_CRITICAL(&bufferMux);
}

bool Buffer::writeUnlocked(const uint8_t* buf, uint32_t len){
  if(useA){
    if (len > BUF_SIZE - bufSizeA) return false;
    memcpy(&bufA[bufSizeA], buf, len);
    bufSizeA += len;
  }else{
    if (len > BUF_SIZE - bufSizeB) return false;
    memcpy(&bufB[bufSizeB], buf, len);
    bufSizeB += len;
  }
  return true;
}

bool Buffer::saveSerial(const uint8_t* data, uint32_t len) {
  // Saves to main console UART, user-facing app will ignore these markers
  // Uses / and ] in markers as they are illegal characters for SSIDs
  const char* mark_begin = "[BUF/BEGIN]";
  const size_t mark_begin_len = strlen(mark_begin);
  const char* mark_close = "[BUF/CLOSE]";
  const size_t mark_close_len = strlen(mark_close);

  // Additional buffer and memcpy's so that a single Serial.write() is called
  // This is necessary so that other console output isn't mixed into buffer stream
  uint8_t* buf = (uint8_t*)malloc(mark_begin_len + len + mark_close_len);
  if (!buf) return false;
  uint8_t* it = buf;
  memcpy(it, mark_begin, mark_begin_len);
  it += mark_begin_len;

  memcpy(it, data, len);
  it += len;

  memcpy(it, mark_close, mark_close_len);
  it += mark_close_len;
  Serial.write(buf, it - buf);
  free(buf);
  return true;
}

void Buffer::save() {
  File output;
  bool fileOpenAttempted = false;
  // Reception can continue into the active buffer while the inactive buffer is written.
  for (int pass = 0; pass < 2; ++pass) {
    portENTER_CRITICAL(&bufferMux);
    if (saving || (bufSizeA + bufSizeB) == 0) {
      portEXIT_CRITICAL(&bufferMux);
      break;
    }

    // The inactive buffer is older. If empty, hand the active buffer to the writer.
    bool saveA = !useA;
    if ((saveA ? bufSizeA : bufSizeB) == 0) {
      saveA = useA;
      useA = !useA;
    }
    const uint8_t* data = saveA ? bufA : bufB;
    const uint32_t len = saveA ? bufSizeA : bufSizeB;
    saving = true;
    portEXIT_CRITICAL(&bufferMux);

    // Retry failed serial allocations before writing to disk to avoid duplicate SD records.
    if (this->serial && !saveSerial(data, len)) {
      portENTER_CRITICAL(&bufferMux);
      saving = false;
      portEXIT_CRITICAL(&bufferMux);
      break;
    }
    if (this->fs) {
      if (!fileOpenAttempted) {
        output = fs->open(fileName, FILE_APPEND);
        fileOpenAttempted = true;
        if (!output) Serial.println(text02+fileName+"'");
      }
      if (output) output.write(data, len);
    }

    portENTER_CRITICAL(&bufferMux);
    if (saveA) bufSizeA = 0;
    else bufSizeB = 0;
    saving = false;
    portEXIT_CRITICAL(&bufferMux);
  }
  if (output) output.close();
}
