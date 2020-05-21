# Ubuntu Linux Environment Setup

To prepare all dependencies for the build:

```
$ sudo apt install libfftw3-dev
$ sudo apt install libsndfile-dev
$ sudo apt install libgcrypt-dev
$ sudo apt install libzita-resampler-dev
$ sudo apt install libmpg123-dev
```

To perform the build:
```
$ ./autogen.sh
$ make
$ make install
```

This will then avail `audiowmark` as an executable command.
