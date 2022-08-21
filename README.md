# vfingerd
vfingerd is a a virtual finger server configured with config files, rather
than reading your /etc/passwd (unless configured to do so!). Users can
customize their finger output by having vfingerd serve their
[plan](https://en.wikipedia.org/wiki/Finger_(protocol)) file. By default
vfingerd only tries to show your username, real name, and your plan file.

### Installing
Run `make install` to install vfingerd to the system (in `/usr/local/bin` by
default). If you want to install vfingerd elsewhere, change the `PREFIX` on
first line of the Makefile. By default, vfingerd will try to read your config
file at `/etc/vfingerd.conf` (you may specify an alternative config file with
the `-c` flag). To copy the default config file, run `cp vfingerd.conf.def
/etc/vfingerd.conf`.
