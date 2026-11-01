#pragma once

#ifndef SDFileManager_h
#define SDFileManager_h

#include "configs.h"

#ifdef HAS_SD

#include <Arduino.h>
#include <LinkedList.h>
#include "SDInterface.h"

extern SDInterface sd_obj;

// Serial file manager for the SD card: "sd <subcommand> ...".
//
// Every subcommand finishes with exactly one result line so a PC tool can
// parse output even when other firmware logging is interleaved:
//   @SD:OK <subcommand> [detail]
//   @SD:ERR <subcommand> <reason>
//
// Transfers use base64 lines and a CRC32 (zlib polynomial):
//   download: "sd get <path>"
//     -> @SD:GET <size>
//     -> @SD:D <base64>          (repeated)
//     -> @SD:OK get <crc32hex>
//   upload:   "sd put <path> <size> <crc32hex>"
//     -> @SD:READY <max raw bytes per chunk>
//     <- @SD:P <base64>          (PC waits for each ACK before the next chunk)
//     -> @SD:ACK <bytes received>
//     -> @SD:OK put <crc32hex>   (file renamed into place only if CRC matches)
//     "@SD:ABORT" from the PC cancels and removes the partial file.

#define SD_FM_GET_CHUNK 384
#define SD_FM_PUT_CHUNK 256
#define SD_FM_COPY_BUF 4096
#define SD_FM_PUT_TIMEOUT_MS 10000
#define SD_FM_MAX_DEPTH 16

const char PROGMEM SD_FM_CMD[] = "sd";
const char PROGMEM HELP_SD_FM_CMD[] =
  "sd ls [dir] | tree [dir] | stat <path> | df | cat <file> | crc <file>\r\n"
  "   mkdir <dir> | rm [-r] <path> | mv <src> <dst> | cp [-r] <src> <dst>\r\n"
  "   get <file> | put <file> <size> <crc32> - SD file manager";

class SDFileManager {
  private:
    String normalize(String path);
    String baseName(String path);
    String joinPath(String dir, String name);
    bool exists(String path);
    bool isDir(String path);
    bool ensureParent(String path);
    bool makeDirs(String path);
    bool removeRecursive(String path, uint8_t depth);
    bool copyFile(String src, String dst);
    bool copyRecursive(String src, String dst, uint8_t depth);
    bool fileCrc(String path, uint32_t& crc, size_t& size);
    void printTree(String path, uint8_t depth);
    String readLine(uint32_t timeout_ms);

    void ok(const char* sub, String detail = "");
    void err(const char* sub, const char* reason);

    void cmdLs(LinkedList<String>* args);
    void cmdTree(LinkedList<String>* args);
    void cmdStat(LinkedList<String>* args);
    void cmdDf();
    void cmdCat(LinkedList<String>* args);
    void cmdCrc(LinkedList<String>* args);
    void cmdMkdir(LinkedList<String>* args);
    void cmdRm(LinkedList<String>* args);
    void cmdMv(LinkedList<String>* args);
    void cmdCp(LinkedList<String>* args);
    void cmdGet(LinkedList<String>* args);
    void cmdPut(LinkedList<String>* args);

  public:
    // args.get(0) is "sd", args.get(1) the subcommand.
    void run(LinkedList<String>* args);
};

#endif
#endif
