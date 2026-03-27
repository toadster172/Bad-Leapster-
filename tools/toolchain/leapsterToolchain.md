## Building a compiler for the Leapster

(Note: this is largely taken from the [ODDev Wiki's](https://wiki.osdev.org/) [GCC Cross-Compiler guide](https://wiki.osdev.org/GCC_Cross-Compiler))

Download and unpack the sources for binutils and GCC. This guide was tested and known working as of GCC 15.2 and binutils 2.46.0, but future versions should work just as well as long as they don't drop ARC support or render the patches for GCC broken. Copy `leapster.patch` into the directory the GCC source has been unpacked to.

Export environment variables:
```sh
export PREFIX="$HOME/opt/leapsterSDK"
export PATH="$PREFIX/bin:$PATH"
```

`PREFIX` is the path that the toolchain will be installed to and may be changed as desired.

Binutils must be built first, so `cd` to its directory, then:
```sh
mkdir build
cd build
../configure --target=arc-elf32 --with-cpu=arc600_mul64 --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make -j$(nproc)
make install
```

Then `cd` to the directory where the gcc source has been unpatched and:
```sh
patch -p0 < leapster.patch
mkdir build
cd build
../configure --target=arc-elf32 --with-cpu=arc600_mul64 --enable-languages="c,c++" --prefix="$PREFIX" --without-headers --disable-nls --disable-hosted-libstdcxx
make -j$(nproc) all-gcc
make -j$(nproc) all-target-libgcc
make -j$(nproc) all-target-libstdc++-v3
make install-gcc
make install-target-libgcc
make install-target-libstdc++-v3
```

The patch is required because the Leapster uses r58 (mmid, normally stores middle 32 bits of multiplication result) instead of r57 (mlo, normally stores lower 32 bits of multiplication result) for the low 32 bits of multiplication. 
To ensure that multiplication compiles / assembles correctly, either GCC or binutils must be patched and GCC has been arbitrarily chosen for that. 
Note that this will cause outputted assembly to use mmid in place of mlo.

Note: [GCC doesn't officially have an option to target the Arctangent-A5 anymore](https://github.com/gcc-mirror/gcc/commit/fb155425c4cd07dcc0326aae14d094c1078ce62e). 
For codegen purposes, ARC600 is equivalent in the vast majority of cases, but if a compiled program exhibits unexpected behaviour on hardware, it's likely a good idea to check generated assembly to ensure there's nothing causing pipeline hazards on Arctangent-A5, especially with regards to zero-overhead loops.

The compiler toolchain should now be installed to `PREFIX` with executables prefixed with `arc-elf32`.
