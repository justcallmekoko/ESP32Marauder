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

// Serializes the double-buffer size/select fields between the RX-callback writer
// (WiFi-driver task) and the main-task save(). Only SHORT sections are held under the
// spinlock (memcpy/size updates, and the save() swap) -- the blocking SD/serial drain
// runs OUTSIDE the lock on a buffer the writer no longer touches.
static portMUX_TYPE buf_mux = portMUX_INITIALIZER_UNLOCKED;

void Buffer::add(const uint8_t* buf, uint32_t len, bool is_pcap){
  // Drop the WHOLE record if it won't fit the active buffer (record = payload + up to
  // the 16-byte pcap header). The mid-cycle self-switch was removed -- save() owns the
  // ping-pong swap now, so add() never mutates useA (which the main task also touches).
  // write() re-bounds-checks under the lock, so a stale unlocked read here can only
  // cause a benign spurious drop, never an overrun.
  {
    volatile uint32_t &sz = useA ? bufSizeA : bufSizeB;
    if(sz + len + 16 > BUF_SIZE) return;
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

// Gate on `writing` (set from the SavePCAP check in openFile) instead of hitting NVS
// via loadSetting() on EVERY captured frame in the RX callback -- a flash-backed read
// per packet is slow and pointless (write() already early-returns when !writing).
void Buffer::append(wifi_promiscuous_pkt_t *packet, int len) {
  if (writing) add(packet->payload, len, true);
}

void Buffer::append(String log) {
  if (writing) add((const uint8_t*)log.c_str(), log.length(), false);
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
  // Copy + size-bump under the spinlock so save()'s swap can't zero/flip the buffer
  // mid-write. Bounds-checked so a full buffer drops the record instead of overrunning
  // (was the old while(saving)delay busy-wait / TOCTOU).
  portENTER_CRITICAL(&buf_mux);
  if(useA){
    if(bufSizeA + len <= BUF_SIZE){ memcpy(&bufA[bufSizeA], buf, len); bufSizeA += len; }
  }else{
    if(bufSizeB + len <= BUF_SIZE){ memcpy(&bufB[bufSizeB], buf, len); bufSizeB += len; }
  }
  portEXIT_CRITICAL(&buf_mux);
}

void Buffer::saveFs(const uint8_t* buf, uint32_t len){
  file = fs->open(fileName, FILE_APPEND);
  if (!file) {
    Serial.println(text02+fileName+"'");
    return;
  }
  if(len > 0) file.write(buf, len);
  file.close();
}

void Buffer::saveSerial(const uint8_t* src, uint32_t len) {
  // Saves to main console UART, user-facing app will ignore these markers
  // Uses / and ] in markers as they are illegal characters for SSIDs
  const char* mark_begin = "[BUF/BEGIN]";
  const size_t mark_begin_len = strlen(mark_begin);
  const char* mark_close = "[BUF/CLOSE]";
  const size_t mark_close_len = strlen(mark_close);

  // Additional buffer and memcpy's so that a single Serial.write() is called
  // This is necessary so that other console output isn't mixed into buffer stream
  uint8_t* buf = (uint8_t*)malloc(mark_begin_len + len + mark_close_len);
  if(!buf) return;
  uint8_t* it = buf;
  memcpy(it, mark_begin, mark_begin_len);
  it += mark_begin_len;
  if(len > 0){ memcpy(it, src, len); it += len; }
  memcpy(it, mark_close, mark_close_len);
  it += mark_close_len;
  Serial.write(buf, it - buf);
  free(buf);
}

void Buffer::save() {
  // Ping-pong drain. Under the spinlock, snapshot the active buffer + flip useA so the
  // RX-callback writer immediately fills the OTHER (already-drained, empty) buffer; then
  // drain the snapshotted buffer OUTSIDE the lock (the writer no longer touches it) ->
  // no writer/drainer overlap, no TOCTOU (replaces the while(saving) delay busy-wait).
  uint8_t* drainBuf = nullptr;
  uint32_t drainLen = 0;
  portENTER_CRITICAL(&buf_mux);
  if(useA){
    if(bufSizeA > 0){ drainBuf = bufA; drainLen = bufSizeA; bufSizeA = 0; useA = false; }
  } else {
    if(bufSizeB > 0){ drainBuf = bufB; drainLen = bufSizeB; bufSizeB = 0; useA = true; }
  }
  portEXIT_CRITICAL(&buf_mux);

  if(!drainBuf || drainLen == 0) return;

  if(this->fs)     saveFs(drainBuf, drainLen);
  if(this->serial) saveSerial(drainBuf, drainLen);
}
