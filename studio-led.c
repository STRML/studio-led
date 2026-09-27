// studio-led: breathe the Mac Studio's front LED while the machine is working, hold it at its
// normal brightness when idle. Runs as root (SMC writes need it).
//
//   studio-led [-k KEY] [-l LOW] [-p PERIOD_S] [-w BUSY_WATTS] [-F FULL_WATTS] [-f LOCKDIR]
//
// Working = whole-board power (SMC PD0R) at or above BUSY_WATTS (default 50), or LOCKDIR exists
// (optional: a path your long jobs create while they run). Breathing is a raised
// cosine between LOW and a peak on KEY (default LSLN, the LED's brightness level) with a
// PERIOD_S cycle. The peak follows smoothed board power: 0xffff at FULL_WATTS (default 180),
// 0xc000 at 60 W, 0x6000 at BUSY_WATTS. Idle writes the key's original value back.
// SIGTERM/SIGINT/SIGHUP restore it too.
#include "smc.h"
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static volatile sig_atomic_t stop = 0;
static void on_signal(int s) { (void)s; stop = 1; }

static double board_watts(io_connect_t c) {
  SMCKeyData_t v;
  float w;
  if (SMCReadKey(c, "PD0R", &v) != KERN_SUCCESS || v.keyInfo.dataSize != 4) return -1;
  memcpy(&w, v.bytes, 4);
  return w;
}

// Pulse peak for a board power: full at `full` W, still bright (0xc000) at 60 W.
static unsigned peak_for(double w, double busy, double full) {
  if (w >= full) return 0xffff;
  if (w >= 60) return 0xc000 + (unsigned)((w - 60) / (full - 60) * 0x3fff);
  if (w <= busy) return 0x6000;
  return 0x6000 + (unsigned)((w - busy) / (60 - busy) * 0x6000);
}

static int write_u16(io_connect_t c, const char *key, unsigned v) {
  SMCBytes_t b = {0};
  b[0] = (char)(v >> 8); b[1] = (char)(v & 0xff);
  return SMCWriteKey(c, key, ('u' << 24) | ('i' << 16) | ('1' << 8) | '6', b, 2) == KERN_SUCCESS;
}

int main(int argc, char **argv) {
  const char *key = "LSLN", *lock = NULL;
  unsigned low = 0x0000;
  double period = 2.4;
  double busy_watts = 50, full_watts = 180;
  for (int o; (o = getopt(argc, argv, "k:l:p:w:F:f:")) != -1;) {
    switch (o) {
      case 'k': key = optarg; break;
      case 'l': low = (unsigned)strtoul(optarg, 0, 16); break;
      case 'p': period = atof(optarg); break;
      case 'w': busy_watts = atof(optarg); break;
      case 'F': full_watts = atof(optarg); break;
      case 'f': lock = optarg; break;
      default: fprintf(stderr, "usage: studio-led [-k KEY] [-l LOWHEX] [-p PERIOD_S] [-w BUSY_WATTS] [-F FULL_WATTS] [-f LOCKDIR]\n"); return 2;
    }
  }
  if (busy_watts >= 60 || full_watts <= 60) { fprintf(stderr, "need BUSY_WATTS < 60 < FULL_WATTS\n"); return 2; }
  io_connect_t c = SMCOpen();
  if (!c) { fprintf(stderr, "SMC open failed\n"); return 1; }
  SMCKeyData_t v;
  if (SMCReadKey(c, key, &v) != KERN_SUCCESS || v.keyInfo.dataSize != 2) { fprintf(stderr, "cannot read %s as ui16\n", key); return 1; }
  unsigned orig = ((unsigned char)v.bytes[0] << 8) | (unsigned char)v.bytes[1];
  if (!write_u16(c, key, orig)) { fprintf(stderr, "write %s failed: run as root\n", key); return 1; }
  signal(SIGTERM, on_signal);
  signal(SIGINT, on_signal);
  signal(SIGHUP, on_signal);
  fprintf(stderr, "studio-led: %s orig %04x, breathe from %04x every %.1fs, busy at %.0f W or %s, full at %.0f W\n",
          key, orig, low, period, busy_watts, lock ? lock : "(no lock path)", full_watts);

  const useconds_t tick = 50000;  // 20 Hz animation
  int ticks_per_check = 10;       // activity checked every 0.5 s
  int idle_grace = 4;             // 2 s of quiet before going idle
  int busy = 0, quiet = 0, n = 0;
  double phase = 0, watts = 0;
  unsigned last = orig;
  while (!stop) {
    if (n++ % ticks_per_check == 0) {
      struct stat st;
      double w = board_watts(c);
      if (w >= 0) watts = 0.7 * watts + 0.3 * w;  // smooth so the peak doesn't jump
      int active = w >= busy_watts || (lock && stat(lock, &st) == 0);
      if (active) { if (!busy) phase = 0; busy = 1; quiet = 0; }
      else if (busy && ++quiet >= idle_grace) busy = 0;
    }
    unsigned want = orig;
    if (busy) {
      phase += (tick / 1e6) / period;
      double b = 0.5 + 0.5 * cos(2 * M_PI * phase);  // starts bright, dips, returns
      unsigned peak = peak_for(watts, busy_watts, full_watts);
      want = peak <= low ? peak : low + (unsigned)(b * (peak - low));
    }
    if (want != last && write_u16(c, key, want)) last = want;
    usleep(tick);
  }
  write_u16(c, key, orig);
  fprintf(stderr, "studio-led: restored %s to %04x\n", key, orig);
  return 0;
}
