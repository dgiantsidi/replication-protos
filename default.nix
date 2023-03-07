with import <nixpkgs> {};
let
  kernel = linuxPackages_4_14.kernel;

  in stdenv.mkDerivation {
    name = "env";
    buildInputs = [
      bashInteractive
      numactl
      pkg-config
      boost
      glog
      folly
      gtest
      linuxHeaders
      cmake
      valgrind
      protobuf
      vimPlugins.vim-clang-format
      clang-tools
      gflags
      jemalloc
      double-conversion
      fmt
      iftop
      openjdk8
      ant
    ];


    NIXOS_KERNELDIR = "${kernel.dev}/lib/modules/${kernel.modDirVersion}/build";
  }
