#include "Buffer.h"
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

  bufSizeB = 0;

  writing = true;

  if (is_pcap) {
    write(uint32_t(0xa1b2c3d4)); // magic number
    write(uint16_t(2)); // major version number
    write(uint16_t(4)); // minor version number
    write(int32_t(0)); // GMT to local correction
    write(uint32_t(0)); // accuracy of timestamps
    write(uint32_t(SNAP_LEN)); // max length of captured packets, in octets
    write(uint32_t(105)); // data link type
  }
}

String Buffer::getFileName() {
  return this->fileName;
}

void Buffer::openFile(const char* file_name, fs::FS* fs, bool serial, bool is_pcap, bool is_gpx) {
  // A fresh capture: reset the host-frame sequence so the host can tell one
  // capture session from the next, and tag what kind of data it carries.
  this->seq_no = 0;
  this->stream_type = is_pcap ? STREAM_PCAP : (is_gpx ? STREAM_GPX : STREAM_LOG);

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
  // buffer is full -> drop packet. Count it so the loss is reported explicitly
  // (as an {"t":"drop","n":N} line) instead of vanishing silently.
  if((useA && bufSizeA + len >= BUF_SIZE && bufSizeB > 0) || (!useA && bufSizeB + len >= BUF_SIZE && bufSizeA > 0)){
    dropped++;
    return;
  }
  
  if(useA && bufSizeA + len + 16 >= BUF_SIZE && bufSizeB == 0){
    useA = false;
    //Serial.println("\nswitched to buffer B");
  }
  else if(!useA && bufSizeB + len + 16 >= BUF_SIZE && bufSizeA == 0){
    useA = true;
    //Serial.println("\nswitched to buffer A");
  }

  uint32_t microSeconds = micros(); // e.g. 45200400 => 45s 200ms 400us
  uint32_t seconds = (microSeconds/1000)/1000; // e.g. 45200400/1000/1000 = 45200 / 1000 = 45s

  microSeconds -= seconds*1000*1000; // e.g. 45200400 - 45*1000*1000 = 45200400 - 45000000 = 400us (because we only need the offset)
  
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
  if(!writing) return;
  while(saving) delay(10);
  
  if(useA){
    memcpy(&bufA[bufSizeA], buf, len);
    bufSizeA += len;
  }else{
    memcpy(&bufB[bufSizeB], buf, len);
    bufSizeB += len;
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

// CRC-32 (IEEE 802.3, poly 0xEDB88320) over a contiguous buffer. Matches the
// host-side Crc32 that validates each streamed frame.
static uint32_t buf_crc32(const uint8_t* data, size_t len) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int k = 0; k < 8; k++)
      crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)(-(int32_t)(crc & 1u)));
  }
  return ~crc;
}

static inline void put_le32(uint8_t* p, uint32_t v) {
  p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

void Buffer::saveSerial() {
  // Emit one self-describing binary frame per flush so a host can carve the
  // capture out of the mixed text/binary serial stream WITHOUT scanning the
  // payload. (The old [BUF/BEGIN]..[BUF/CLOSE] text markers were unsafe:
  // arbitrary pcap bytes can contain those exact sequences by chance.) Layout:
  //
  //   SYNC(4: FE ED FA CE) | seq(4 LE) | type(1) | len(4 LE) | payload(len) | crc32(4 LE)
  //
  // SYNC begins with 0xFE, a byte that never occurs in valid UTF-8, so the
  // host's line reader switches into binary-frame mode unambiguously. crc32
  // covers seq..payload. The whole frame is written with a single
  // Serial.write() so console text can never interleave inside it.
  const uint32_t total = bufSizeA + bufSizeB;
  const size_t HDR = 4 + 4 + 1 + 4;         // sync + seq + type + len
  const size_t frameLen = HDR + total + 4;  // + crc32

  uint8_t* out = (uint8_t*)malloc(frameLen);
  if (!out) return; // out of memory: skip this block rather than crash

  out[0] = 0xFE; out[1] = 0xED; out[2] = 0xFA; out[3] = 0xCE; // SYNC
  put_le32(out + 4, seq_no);
  out[8] = stream_type;
  put_le32(out + 9, total);

  uint8_t* it = out + HDR;
  if (useA) {
    if (bufSizeB > 0) { memcpy(it, bufB, bufSizeB); it += bufSizeB; }
    if (bufSizeA > 0) { memcpy(it, bufA, bufSizeA); it += bufSizeA; }
  } else {
    if (bufSizeA > 0) { memcpy(it, bufA, bufSizeA); it += bufSizeA; }
    if (bufSizeB > 0) { memcpy(it, bufB, bufSizeB); it += bufSizeB; }
  }

  // crc32 over seq(4) + type(1) + len(4) + payload(total) = everything between
  // the SYNC and the crc field.
  put_le32(it, buf_crc32(out + 4, 9 + total));
  it += 4;

  Serial.write(out, it - out);
  free(out);
  seq_no++;
}

uint32_t Buffer::takeDropped() {
  uint32_t d = dropped;
  dropped = 0;
  return d;
}

void Buffer::save() {
  saving = true;

  if((bufSizeA + bufSizeB) == 0){
    saving = false;
    return;
  }

  if(this->fs) saveFs();
  if(this->serial) {
    saveSerial();
    // Report any packets the ring couldn't hold, right after the frame, so the
    // host can account for them exactly instead of seeing a silent gap.
    uint32_t d = takeDropped();
    if (d) {
      Serial.print(F("@J {\"t\":\"drop\",\"n\":"));
      Serial.print(d);
      Serial.println(F("}"));
    }
  }

  bufSizeA = 0;
  bufSizeB = 0;

  saving = false;
}
