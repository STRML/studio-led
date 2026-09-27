SMC = smc.c -framework IOKit -framework CoreFoundation
BINS = studio-led smcdump smcwrite

all: $(BINS)

$(BINS): %: %.c smc.c smc.h
	clang -O2 -Wall -o $@ $< $(SMC)

clean:
	rm -f $(BINS)

PLIST = /Library/LaunchDaemons/local.studio-led.plist

# sudo make install: copy the binary to a root-owned path and start the LaunchDaemon.
install: studio-led
	install -d -m 755 /usr/local/libexec
	install -o root -g wheel -m 755 studio-led /usr/local/libexec/studio-led
	install -o root -g wheel -m 644 local.studio-led.plist $(PLIST)
	-launchctl bootout system/local.studio-led 2>/dev/null && sleep 1
	launchctl bootstrap system $(PLIST)

# sudo make uninstall: stop the daemon (it restores the LED) and remove it.
uninstall:
	-launchctl bootout system/local.studio-led
	rm -f $(PLIST) /usr/local/libexec/studio-led

.PHONY: all clean install uninstall
