# UltraDefrag for Linux
Based on [UltraDefrag for Linux](https://jp-andre.pagesperso-orange.fr/advanced-ntfs-3g.html) a Linux clone of [UltraDefrag for Windows](https://sourceforge.net/projects/ultradefrag/) V5.0.0.
> UltraDefrag is a disk defragmenter for Windows, which supports defragmentation of locked system files by running during the boot process. It is easy to use without any complicated scripting or a huge load of configuration settings. You can filter the files processed by size, number of fragments, file name and path. You can terminate the process early by specifying an execution time limit.

## Purpose of this repository

This repository was primarily created to check if the windows version added any features on top of the linux version it is based on.

The results at the time were clear, no changes just a GUI on top.

## Changes made from base linux version

- [The output was colorized ](https://github.com/749/UltraDefrag4Linux/commit/6a6b3229879b7a482671292a7ca86faad1c92cd9)

## Compiling

```sh
cd src
CFLAGS='-Wno-implict-function-declaration' make
```

## Support

At this moment I am unable to render support to this project, I am happy to merge any fixes or improvements though.
