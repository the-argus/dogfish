[
  (_: super: {
    zig = super.callPackage ./zig {
      llvmPackages = super.llvmPackages_16;
    };
  })
]
