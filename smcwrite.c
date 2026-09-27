#include "smc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// smcwrite KEY HEXBYTES: write raw bytes to an SMC key (length must match the key), print the
// result, then read the key back. Needs root.
int main(int argc, char **argv) {
  if (argc != 3) { fprintf(stderr, "usage: smcwrite KEY HEXBYTES\n"); return 2; }
  io_connect_t c = SMCOpen(); if (!c) { puts("open fail"); return 1; }
  SMCKeyData_keyInfo_t info;
  kern_return_t r = SMCGetKeyInfo(c, argv[1], &info);
  if (r != KERN_SUCCESS) { printf("keyinfo %s: 0x%x\n", argv[1], r); return 1; }
  size_t n = strlen(argv[2]) / 2;
  if (n != info.dataSize) { printf("%zu bytes given, key %s holds %u\n", n, argv[1], info.dataSize); return 1; }
  SMCBytes_t b = {0};
  for (size_t i = 0; i < n; i++) { char h[3] = {argv[2][2*i], argv[2][2*i+1], 0}; b[i] = (char)strtol(h, 0, 16); }
  r = SMCWriteKey(c, argv[1], info.dataType, b, info.dataSize);
  printf("write %s = %s: 0x%x%s\n", argv[1], argv[2], r, r == KERN_SUCCESS ? " ok" : "");
  SMCKeyData_t v;
  if (SMCReadKey(c, argv[1], &v) == KERN_SUCCESS) {
    printf("read back %s: ", argv[1]);
    for (unsigned j = 0; j < v.keyInfo.dataSize && j < 16; j++) printf("%02x", (unsigned char)v.bytes[j]);
    puts("");
  }
  return r != KERN_SUCCESS;
}
