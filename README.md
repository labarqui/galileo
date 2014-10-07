Resources for working with the Intel Galileo Board
==================================================

## Documentation

- Arduino home page: http://arduino.cc/en/ArduinoCertified/IntelGalileo
- Intel resources (Gen-2 based): http://www.intel.com/content/www/us/en/do-it-yourself/galileo-maker-quark-board.html
- Sparkfun's guide: https://learn.sparkfun.com/tutorials/galileo-getting-started-guide
- Wikipedia: http://en.wikipedia.org/wiki/Intel\_Galileo
- Intel Quark SoC X1000: http://ark.intel.com/products/79084
- Yocto Project (Galileo's Linux flavour): https://www.yoctoproject.org
- Meshcentral, Galileo boards cluster: https://software.intel.com/en-us/blogs/2014/01/20/meshcentralcom-intelr-galileo-tower
- GPIO docs: http://www.malinov.com/Home/sergey-s-blog/intelgalileo-programminggpiofromlinux


## Examples

See the `code` folder for examples.

## Upgrading `opkg`

1. Copy the pkg sources config to the board:

  `scp base-feeds.conf root@ipaddress:/etc/opkg/base-feeds.conf`

2. Update the uClibc library: 
  
  `opkg install --force-overwrite uclibc`
3. Update the repo sources: 

  `opkg update`

4. Upgrade the packages: 

  `opkg upgrade`

5. Install basic packages:

  `opkg install vim git make gcc-dev gcc-symlinks cpp cpp-symlinks g++ g++-symlinks uclibc-dev uclibc-utils binutils-dev binutils-symlinks python-dev coreutils-dev`

Based on [this](http://alextgalileo.altervista.org/package-repo-configuration-instructions.html)
