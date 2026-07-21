{
  description = "Raspberry Pi Zero W USB mass-storage gadget boot";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      pkgs  = nixpkgs.legacyPackages.x86_64-linux;
      lib   = pkgs.lib;
      cross = pkgs.pkgsCross.raspberryPi;

      init = pkgs.stdenv.mkDerivation {
        name = "rpi-gadget-init";
        nativeBuildInputs = [ cross.stdenv.cc ];
        unpackPhase = "true";
        buildPhase = ''
          cp ${./init.c} init.c
          ${cross.stdenv.cc.targetPrefix}cc -nostdlib -static -Wl,-e,_start -Wl,--build-id=none -O2 -fno-stack-protector -fno-builtin -o init init.c
          ${cross.stdenv.cc.targetPrefix}strip --strip-all init
        '';
        installPhase = "install -Dm755 init $out/init";
      };

      initramfs = pkgs.stdenv.mkDerivation {
        name = "rpi-gadget-initramfs";
        nativeBuildInputs = [ pkgs.cpio ];
        buildCommand = ''
          mkdir -p root/dev $out
          cp ${init}/init root/init
          chmod +x root/init
          ( cd root && find . | sort | cpio -o -H newc ) > $out/initramfs.cpio
        '';
      };

      linuxSource = cross.linux.src;

      kernelConfig = pkgs.writeText "rpi-zero-gadget-linux.config" ''
        ${builtins.readFile ./rpizero-minimal-linux.config}
        CONFIG_INITRAMFS_SOURCE="${initramfs}/initramfs.cpio"
        CONFIG_INITRAMFS_COMPRESSION_NONE=y
      '';

      # ── kernel sanity checks ─────────────────────────────────────────────
      kernelChecks = [
        "^CONFIG_ARCH_BCM2835=y$"
        "^CONFIG_ARCH_MULTI_V6=y$"
        "^CONFIG_CPU_V6K=y$"
        "^CONFIG_CLK_BCM2835=y$"
        "^CONFIG_BCM2835_TIMER=y$"
        "^CONFIG_SERIAL_AMBA_PL011=y$"
        "^CONFIG_SERIAL_AMBA_PL011_CONSOLE=y$"
        "^CONFIG_SERIAL_8250=y$"
        "^CONFIG_SERIAL_8250_CONSOLE=y$"
        "^CONFIG_SERIAL_8250_EXTENDED=y$"
        "^CONFIG_SERIAL_8250_BCM2835AUX=y$"
        "^CONFIG_SERIAL_OF_PLATFORM=y$"
        "^CONFIG_BLK_DEV_INITRD=y$"
        "^CONFIG_BINFMT_ELF=y$"
        "^CONFIG_BLOCK=y$"
        "^CONFIG_MSDOS_PARTITION=y$"
        "^CONFIG_MMC=y$"
        "^CONFIG_MMC_BLOCK=y$"
        "^CONFIG_MMC_BCM2835=y$"
        "^CONFIG_PRINTK=y$"
        "^CONFIG_NOP_USB_XCEIV=y$"
        "^CONFIG_USB_DWC2=y$"
        "^CONFIG_USB_GADGET=y$"
        "^CONFIG_USB_LIBCOMPOSITE=y$"
        "^CONFIG_CONFIGFS_FS=y$"
        "^CONFIG_USB_CONFIGFS=y$"
        "^CONFIG_USB_CONFIGFS_MASS_STORAGE=y$"
        "^CONFIG_USB_F_MASS_STORAGE=y$"
        "^CONFIG_INITRAMFS_SOURCE=\"${initramfs}/initramfs.cpio\"$"
      ];

      checksScript = checks:
        lib.concatMapStrings (pat: ''
          grep -qE ${lib.escapeShellArg pat} build/.config \
            || { echo "MISSING kernel option: ${pat}"; exit 1; }
        '') checks;

      # ── kernel builder ───────────────────────────────────────────────────
      linuxKernel = pkgs.stdenv.mkDerivation {
        name = "rpi-zero-gadget-linux";
        src = linuxSource;
        nativeBuildInputs = with pkgs; [
          bc
          bison
          flex
          perl
          openssl
          ncurses
          cross.stdenv.cc
        ];
        postPatch = "patchShebangs scripts";
        configurePhase = ''
          runHook preConfigure
          export ARCH=arm
          export CROSS_COMPILE=${cross.stdenv.cc.targetPrefix}
          export KCONFIG_ALLCONFIG=${kernelConfig}
          make O=build allnoconfig
          ${checksScript kernelChecks}
          runHook postConfigure
        '';
        buildPhase = ''
          runHook preBuild
          make O=build -j$NIX_BUILD_CORES zImage dtbs
          runHook postBuild
        '';
        installPhase = ''
          runHook preInstall
          export ARCH=arm
          export CROSS_COMPILE=${cross.stdenv.cc.targetPrefix}
          install -Dm644 build/arch/arm/boot/zImage $out/kernel.img
          install -Dm644 build/arch/arm/boot/dts/broadcom/bcm2835-rpi-zero-w.dtb $out/dtbs/bcm2835-rpi-zero-w.dtb
          install -Dm644 build/.config $out/config
          install -Dm644 build/System.map $out/System.map
          install -Dm755 build/vmlinux $out/vmlinux
          make O=build -s kernelrelease > $out/kernelrelease
          runHook postInstall
        '';
      };

      # ── firmware blobs ───────────────────────────────────────────────────
      firmwareBoot = "${pkgs.raspberrypifw}/share/raspberrypi/boot";

      # ── rpiboot boot directory builder ──────────────────────────────────
      mkBootDir = {
        name,
        kernel
      }: pkgs.stdenv.mkDerivation {
        inherit name;
        buildCommand = ''
          install -Dm644 ${firmwareBoot}/bootcode.bin      $out/bootcode.bin
          install -Dm644 ${firmwareBoot}/start.elf         $out/start.elf
          install -Dm644 ${firmwareBoot}/start_cd.elf      $out/start_cd.elf
          install -Dm644 ${firmwareBoot}/fixup.dat         $out/fixup.dat
          install -Dm644 ${firmwareBoot}/fixup_cd.dat      $out/fixup_cd.dat
          install -Dm644 ${kernel}/kernel.img              $out/kernel.img
          install -Dm644 ${kernel}/dtbs/bcm2835-rpi-zero-w.dtb $out/bcm2835-rpi-zero-w.dtb
          install -Dm644 ${firmwareBoot}/overlays/dwc2.dtbo    $out/overlays/dwc2.dtbo
          install -Dm644 ${firmwareBoot}/overlays/disable-bt.dtbo $out/overlays/disable-bt.dtbo
          install -Dm644 ${./config.txt}                   $out/config.txt
          install -Dm644 ${./cmdline.txt}                  $out/cmdline.txt
          install -Dm644 ${kernel}/config                  $out/kernel.config
        '';
      };

      boot = mkBootDir {
        name      = "rpi-zero-gadget-rpiboot";
        kernel    = linuxKernel;
      };
    in {
      packages.x86_64-linux = {
        inherit linuxKernel init initramfs boot;
        default = boot;
      };
    };
}
