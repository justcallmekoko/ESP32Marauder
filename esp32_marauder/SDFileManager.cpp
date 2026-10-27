#include "SDFileManager.h"

#ifdef HAS_SD

#include "mbedtls/base64.h"
#include "esp_rom_crc.h"

// Positional arguments after the subcommand, with "-x" flags removed.
static LinkedList<String> positionals(LinkedList<String>* args) {
  LinkedList<String> out;
  for (int i = 2; i < args->size(); i++) {
    String a = args->get(i);
    if (a.length() > 1 && a.charAt(0) == '-')
      continue;
    out.add(a);
  }
  return out;
}

static bool hasFlag(LinkedList<String>* args, const char* flag) {
  for (int i = 2; i < args->size(); i++) {
    if (args->get(i) == flag)
      return true;
  }
  return false;
}

void SDFileManager::ok(const char* sub, String detail) {
  if (detail.length())
    Serial.printf("@SD:OK %s %s\n", sub, detail.c_str());
  else
    Serial.printf("@SD:OK %s\n", sub);
}

void SDFileManager::err(const char* sub, const char* reason) {
  Serial.printf("@SD:ERR %s %s\n", sub, reason);
}

String SDFileManager::normalize(String path) {
  path.trim();
  path.replace("\\", "/");
  if (!path.startsWith("/"))
    path = "/" + path;
  while (path.indexOf("//") >= 0)
    path.replace("//", "/");
  if (path.length() > 1 && path.endsWith("/"))
    path.remove(path.length() - 1);
  return path;
}

String SDFileManager::baseName(String path) {
  int slash = path.lastIndexOf('/');
  return slash >= 0 ? path.substring(slash + 1) : path;
}

String SDFileManager::joinPath(String dir, String name) {
  if (dir.endsWith("/"))
    return dir + name;
  return dir + "/" + name;
}

bool SDFileManager::exists(String path) {
  return path == "/" || SD.exists(path);
}

bool SDFileManager::isDir(String path) {
  File f = SD.open(path);
  bool dir = f && f.isDirectory();
  f.close();
  return dir;
}

bool SDFileManager::makeDirs(String path) {
  if (path == "/" || isDir(path))
    return true;
  int slash = path.lastIndexOf('/');
  if (slash > 0 && !makeDirs(path.substring(0, slash)))
    return false;
  return SD.mkdir(path);
}

bool SDFileManager::ensureParent(String path) {
  int slash = path.lastIndexOf('/');
  if (slash <= 0)
    return true;
  return makeDirs(path.substring(0, slash));
}

bool SDFileManager::removeRecursive(String path, uint8_t depth) {
  if (depth > SD_FM_MAX_DEPTH)
    return false;
  if (!isDir(path))
    return SD.remove(path);

  LinkedList<String> children;
  File dir = SD.open(path);
  while (true) {
    File entry = dir.openNextFile();
    if (!entry)
      break;
    children.add(joinPath(path, baseName(entry.name())));
    entry.close();
  }
  dir.close();

  for (int i = 0; i < children.size(); i++) {
    if (!removeRecursive(children.get(i), depth + 1))
      return false;
  }
  return path == "/" ? true : SD.rmdir(path);
}

bool SDFileManager::copyFile(String src, String dst) {
  File in = SD.open(src, FILE_READ);
  if (!in)
    return false;
  File out = SD.open(dst, FILE_WRITE);
  if (!out) {
    in.close();
    return false;
  }

  uint8_t* buf = (uint8_t*)malloc(SD_FM_COPY_BUF);
  bool success = buf != nullptr;
  while (success && in.available()) {
    size_t n = in.read(buf, SD_FM_COPY_BUF);
    if (n == 0 || out.write(buf, n) != n)
      success = false;
    yield();
  }
  free(buf);
  in.close();
  out.close();
  if (!success)
    SD.remove(dst);
  return success;
}

bool SDFileManager::copyRecursive(String src, String dst, uint8_t depth) {
  if (depth > SD_FM_MAX_DEPTH)
    return false;
  if (!isDir(src))
    return copyFile(src, dst);
  if (!makeDirs(dst))
    return false;

  LinkedList<String> children;
  File dir = SD.open(src);
  while (true) {
    File entry = dir.openNextFile();
    if (!entry)
      break;
    children.add(baseName(entry.name()));
    entry.close();
  }
  dir.close();

  for (int i = 0; i < children.size(); i++) {
    String name = children.get(i);
    if (!copyRecursive(joinPath(src, name), joinPath(dst, name), depth + 1))
      return false;
  }
  return true;
}

bool SDFileManager::fileCrc(String path, uint32_t& crc, size_t& size) {
  File f = SD.open(path, FILE_READ);
  if (!f || f.isDirectory()) {
    f.close();
    return false;
  }
  uint8_t* buf = (uint8_t*)malloc(SD_FM_COPY_BUF);
  if (!buf) {
    f.close();
    return false;
  }
  crc = 0;
  size = 0;
  while (f.available()) {
    size_t n = f.read(buf, SD_FM_COPY_BUF);
    if (n == 0)
      break;
    crc = esp_rom_crc32_le(crc, buf, n);
    size += n;
    yield();
  }
  free(buf);
  f.close();
  return true;
}

void SDFileManager::printTree(String path, uint8_t depth) {
  if (depth > SD_FM_MAX_DEPTH)
    return;
  LinkedList<String> dirs;
  File dir = SD.open(path);
  while (true) {
    File entry = dir.openNextFile();
    if (!entry)
      break;
    String child = joinPath(path, baseName(entry.name()));
    if (entry.isDirectory()) {
      Serial.printf("D\t%s\n", child.c_str());
      dirs.add(child);
    }
    else {
      Serial.printf("F\t%s\t%u\n", child.c_str(), (unsigned)entry.size());
    }
    entry.close();
  }
  dir.close();
  for (int i = 0; i < dirs.size(); i++)
    printTree(dirs.get(i), depth + 1);
}

String SDFileManager::readLine(uint32_t timeout_ms) {
  String line = "";
  uint32_t start = millis();
  while (millis() - start < timeout_ms) {
    while (Serial.available() > 0) {
      char c = (char)Serial.read();
      if (c == '\n') {
        line.trim();
        return line;
      }
      line += c;
      start = millis();
    }
    delay(1);
  }
  return "";
}

void SDFileManager::cmdLs(LinkedList<String>* args) {
  LinkedList<String> pos = positionals(args);
  String path = normalize(pos.size() ? pos.get(0) : "/");
  if (!isDir(path)) {
    err("ls", "NOT_A_DIRECTORY");
    return;
  }
  int count = 0;
  File dir = SD.open(path);
  while (true) {
    File entry = dir.openNextFile();
    if (!entry)
      break;
    if (entry.isDirectory())
      Serial.printf("D\t%s\n", baseName(entry.name()).c_str());
    else
      Serial.printf("F\t%s\t%u\n", baseName(entry.name()).c_str(), (unsigned)entry.size());
    entry.close();
    count++;
  }
  dir.close();
  ok("ls", String(count));
}

void SDFileManager::cmdTree(LinkedList<String>* args) {
  LinkedList<String> pos = positionals(args);
  String path = normalize(pos.size() ? pos.get(0) : "/");
  if (!isDir(path)) {
    err("tree", "NOT_A_DIRECTORY");
    return;
  }
  printTree(path, 0);
  ok("tree");
}

void SDFileManager::cmdStat(LinkedList<String>* args) {
  LinkedList<String> pos = positionals(args);
  if (!pos.size()) {
    err("stat", "MISSING_PATH");
    return;
  }
  String path = normalize(pos.get(0));
  if (!exists(path)) {
    err("stat", "NOT_FOUND");
    return;
  }
  File f = SD.open(path);
  if (f.isDirectory())
    ok("stat", "D " + path);
  else
    ok("stat", "F " + path + " " + String((unsigned)f.size()) + " " + String((unsigned long)f.getLastWrite()));
  f.close();
}

void SDFileManager::cmdDf() {
  uint64_t total = SD.totalBytes();
  uint64_t used = SD.usedBytes();
  Serial.printf("Total: %llu MB\nUsed: %llu MB\nFree: %llu MB\n",
                total / (1024 * 1024), used / (1024 * 1024), (total - used) / (1024 * 1024));
  ok("df", String((unsigned long long)total) + " " + String((unsigned long long)used));
}

void SDFileManager::cmdCat(LinkedList<String>* args) {
  LinkedList<String> pos = positionals(args);
  if (!pos.size()) {
    err("cat", "MISSING_PATH");
    return;
  }
  File f = SD.open(normalize(pos.get(0)), FILE_READ);
  if (!f || f.isDirectory()) {
    f.close();
    err("cat", "NOT_A_FILE");
    return;
  }
  uint8_t buf[256];
  while (f.available()) {
    size_t n = f.read(buf, sizeof(buf));
    Serial.write(buf, n);
    yield();
  }
  f.close();
  Serial.println();
  ok("cat");
}

void SDFileManager::cmdCrc(LinkedList<String>* args) {
  LinkedList<String> pos = positionals(args);
  if (!pos.size()) {
    err("crc", "MISSING_PATH");
    return;
  }
  uint32_t crc;
  size_t size;
  if (!fileCrc(normalize(pos.get(0)), crc, size)) {
    err("crc", "NOT_A_FILE");
    return;
  }
  char detail[32];
  snprintf(detail, sizeof(detail), "%08lx %u", (unsigned long)crc, (unsigned)size);
  ok("crc", detail);
}

void SDFileManager::cmdMkdir(LinkedList<String>* args) {
  LinkedList<String> pos = positionals(args);
  if (!pos.size()) {
    err("mkdir", "MISSING_PATH");
    return;
  }
  String path = normalize(pos.get(0));
  if (exists(path) && !isDir(path)) {
    err("mkdir", "FILE_EXISTS");
    return;
  }
  if (makeDirs(path))
    ok("mkdir", path);
  else
    err("mkdir", "FAILED");
}

void SDFileManager::cmdRm(LinkedList<String>* args) {
  LinkedList<String> pos = positionals(args);
  if (!pos.size()) {
    err("rm", "MISSING_PATH");
    return;
  }
  String path = normalize(pos.get(0));
  if (path == "/") {
    err("rm", "REFUSED_ROOT");
    return;
  }
  if (!exists(path)) {
    err("rm", "NOT_FOUND");
    return;
  }
  bool success;
  if (isDir(path)) {
    if (!hasFlag(args, "-r")) {
      // Without -r only empty directories are removed.
      success = SD.rmdir(path);
      if (!success) {
        err("rm", "DIRECTORY_NOT_EMPTY");
        return;
      }
    }
    else {
      success = removeRecursive(path, 0);
    }
  }
  else {
    success = sd_obj.removeFile(path);
  }
  if (success)
    ok("rm", path);
  else
    err("rm", "FAILED");
}

void SDFileManager::cmdMv(LinkedList<String>* args) {
  LinkedList<String> pos = positionals(args);
  if (pos.size() < 2) {
    err("mv", "MISSING_PATH");
    return;
  }
  String src = normalize(pos.get(0));
  String dst = normalize(pos.get(1));
  if (!exists(src) || src == "/") {
    err("mv", "NOT_FOUND");
    return;
  }
  if (isDir(dst))
    dst = joinPath(dst, baseName(src));
  if (dst.startsWith(src + "/")) {
    err("mv", "INTO_ITSELF");
    return;
  }
  if (exists(dst)) {
    err("mv", "DESTINATION_EXISTS");
    return;
  }
  if (!ensureParent(dst)) {
    err("mv", "FAILED");
    return;
  }
  if (SD.rename(src, dst))
    ok("mv", dst);
  else
    err("mv", "FAILED");
}

void SDFileManager::cmdCp(LinkedList<String>* args) {
  LinkedList<String> pos = positionals(args);
  if (pos.size() < 2) {
    err("cp", "MISSING_PATH");
    return;
  }
  String src = normalize(pos.get(0));
  String dst = normalize(pos.get(1));
  if (!exists(src)) {
    err("cp", "NOT_FOUND");
    return;
  }
  bool src_dir = isDir(src);
  if (src_dir && !hasFlag(args, "-r")) {
    err("cp", "IS_DIRECTORY_USE_-r");
    return;
  }
  if (isDir(dst))
    dst = joinPath(dst, baseName(src));
  if (dst == src || dst.startsWith(src + "/")) {
    err("cp", "INTO_ITSELF");
    return;
  }
  if (exists(dst)) {
    err("cp", "DESTINATION_EXISTS");
    return;
  }
  if (!ensureParent(dst)) {
    err("cp", "FAILED");
    return;
  }
  if (copyRecursive(src, dst, 0))
    ok("cp", dst);
  else
    err("cp", "FAILED");
}

void SDFileManager::cmdGet(LinkedList<String>* args) {
  LinkedList<String> pos = positionals(args);
  if (!pos.size()) {
    err("get", "MISSING_PATH");
    return;
  }
  File f = SD.open(normalize(pos.get(0)), FILE_READ);
  if (!f || f.isDirectory()) {
    f.close();
    err("get", "NOT_A_FILE");
    return;
  }

  uint8_t raw[SD_FM_GET_CHUNK];
  unsigned char b64[((SD_FM_GET_CHUNK + 2) / 3) * 4 + 1];
  uint32_t crc = 0;
  Serial.printf("@SD:GET %u\n", (unsigned)f.size());
  while (f.available()) {
    size_t n = f.read(raw, sizeof(raw));
    if (n == 0)
      break;
    crc = esp_rom_crc32_le(crc, raw, n);
    size_t out_len = 0;
    mbedtls_base64_encode(b64, sizeof(b64), &out_len, raw, n);
    b64[out_len] = '\0';
    Serial.print("@SD:D ");
    Serial.println((const char*)b64);
    yield();
  }
  f.close();
  char detail[12];
  snprintf(detail, sizeof(detail), "%08lx", (unsigned long)crc);
  ok("get", detail);
}

void SDFileManager::cmdPut(LinkedList<String>* args) {
  LinkedList<String> pos = positionals(args);
  if (pos.size() < 3) {
    err("put", "USAGE_sd_put_<path>_<size>_<crc32>");
    return;
  }
  String path = normalize(pos.get(0));
  size_t size = (size_t)strtoul(pos.get(1).c_str(), nullptr, 10);
  uint32_t expected_crc = (uint32_t)strtoul(pos.get(2).c_str(), nullptr, 16);
  if (path == "/" || isDir(path)) {
    err("put", "IS_DIRECTORY");
    return;
  }
  if (!ensureParent(path)) {
    err("put", "FAILED");
    return;
  }

  String part = path + ".part";
  SD.remove(part);
  File out = SD.open(part, FILE_WRITE);
  if (!out) {
    err("put", "OPEN_FAILED");
    return;
  }

  Serial.printf("@SD:READY %u\n", (unsigned)SD_FM_PUT_CHUNK);

  uint8_t raw[SD_FM_PUT_CHUNK];
  size_t received = 0;
  uint32_t crc = 0;
  const char* failure = nullptr;
  while (received < size) {
    String line = readLine(SD_FM_PUT_TIMEOUT_MS);
    if (line == "") {
      failure = "TIMEOUT";
      break;
    }
    if (line == "@SD:ABORT") {
      failure = "ABORTED";
      break;
    }
    if (!line.startsWith("@SD:P ")) {
      failure = "BAD_CHUNK";
      break;
    }
    size_t n = 0;
    const unsigned char* data = (const unsigned char*)line.c_str() + 6;
    if (mbedtls_base64_decode(raw, sizeof(raw), &n, data, line.length() - 6) != 0 ||
        n == 0 || received + n > size) {
      failure = "BAD_CHUNK";
      break;
    }
    if (out.write(raw, n) != n) {
      failure = "WRITE_FAILED";
      break;
    }
    crc = esp_rom_crc32_le(crc, raw, n);
    received += n;
    Serial.printf("@SD:ACK %u\n", (unsigned)received);
  }
  out.close();

  if (!failure && crc != expected_crc)
    failure = "CRC_MISMATCH";
  if (!failure && exists(path) && !SD.remove(path))
    failure = "REPLACE_FAILED";
  if (!failure && !SD.rename(part, path))
    failure = "RENAME_FAILED";

  if (failure) {
    SD.remove(part);
    err("put", failure);
    return;
  }
  char detail[12];
  snprintf(detail, sizeof(detail), "%08lx", (unsigned long)crc);
  ok("put", detail);
}

void SDFileManager::run(LinkedList<String>* args) {
  String sub = args->size() > 1 ? args->get(1) : "";

  if (sub == "" || sub == "help") {
    Serial.println(HELP_SD_FM_CMD);
    return;
  }
  if (!sd_obj.supported) {
    err(sub.c_str(), "NO_SD_CARD");
    return;
  }

  if (sub == "ls") cmdLs(args);
  else if (sub == "tree") cmdTree(args);
  else if (sub == "stat") cmdStat(args);
  else if (sub == "df") cmdDf();
  else if (sub == "cat") cmdCat(args);
  else if (sub == "crc") cmdCrc(args);
  else if (sub == "mkdir") cmdMkdir(args);
  else if (sub == "rm") cmdRm(args);
  else if (sub == "mv") cmdMv(args);
  else if (sub == "cp") cmdCp(args);
  else if (sub == "get") cmdGet(args);
  else if (sub == "put") cmdPut(args);
  else err(sub.c_str(), "UNKNOWN_SUBCOMMAND");
}

#endif
