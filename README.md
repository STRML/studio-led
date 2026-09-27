# studio-led

Turns the Mac Studio's front power LED into a load meter. It pulses while the machine works, brighter the more power it draws, and holds its normal glow when idle.

```
board power (SMC PD0R)        LED
  idle, < 50 W        ─▶  steady, stock brightness
  50 W                ─▶  slow pulse, dim peak
  60 W                ─▶  pulse, peak ~75%
  180 W and up        ─▶  pulse, full peak
```

Tested on one machine: a Mac Studio with M5 Ultra, macOS 27. Other Apple Silicon Macs may name their keys differently. Run `sudo ./led-probe.sh` to find out.

## Run it

```sh
make
sudo ./studio-led
```

It needs root because every SMC write does. Ctrl-C, SIGTERM, or a dropped ssh session puts the LED back the way it found it.

| Flag | Default | What it does |
|---|---|---|
| `-w WATTS` | 50 | board power that counts as working |
| `-F WATTS` | 180 | board power for a full-brightness peak |
| `-p SECONDS` | 2.4 | one breath |
| `-l HEX` | 0000 | the dim end of each breath |
| `-f PATH` | none | also count as working while this path exists (a lock dir your long jobs hold) |
| `-k KEY` | LSLN | the brightness key to drive |

The 50 W default is a guess, since the idle draw hasn't been measured yet. Check yours with `./smcdump PD0R` before you rely on it.

## Run it at boot

```sh
make
sudo make install     # /usr/local/libexec/studio-led + /Library/LaunchDaemons/local.studio-led.plist
tail -1 /var/log/studio-led.log
```

`sudo make uninstall` stops it, which restores the LED, and removes both files. It has to be a LaunchDaemon rather than a LaunchAgent, because a LaunchAgent runs as your user and SMC writes need root. The binary is copied to a root-owned path, so the daemon never runs a file your account can edit. To pass flags, add them to `ProgramArguments` in `local.studio-led.plist` before you install.

## The keys

Apple documents none of this. These are the `LS*` SMC keys on the M5 Ultra Studio and what writing them did to the LED:

| Key | Stock | Write | LED |
|---|---|---|---|
| `LSLN` | `ffff` | `0000`...`ffff` | smooth dimmer; `0000` is dim, not off |
| `LS0S` | `b366` | `0000` | very dim |
| `LS0C` | `ffff` | `0000` | dim |
| `LS0P` | `66b3` | `0000` | bright |
| `LSOF`, `LSLB`, `LSLF` | | | no visible change |
| `LS1*` | | | little or no change (probably a second channel the Studio doesn't wire up) |

Some writes push the LED brighter than stock, but which key does it isn't pinned down yet. `LS0P` and `LS0S` are byte-swapped copies of each other, so they probably work as a pair. Keys such as `LSOO` and `LSSB` fail on read. They drove the sleep light on Intel Macs, and they may be write-only here.

## Tools

| File | What it does |
|---|---|
| `studio-led` | the daemon |
| `smcdump [prefix]` | prints every SMC key with that prefix, with its type and value; no root |
| `smcwrite KEY HEX` | writes raw bytes to one key and reads it back; root |
| `led-probe.sh` | steps through the `LS*` keys one at a time and restores each one; root |

`smc.c` and `smc.h` come from [mactop](https://github.com/metaspartan/mactop) (MIT).
