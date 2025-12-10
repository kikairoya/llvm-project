set(LLVM_LINK_COMPONENTS Support)

add_clang_tool(offload-arch OffloadArch.cpp NVPTXArch.cpp AMDGPUArchByKFD.cpp
               AMDGPUArchByHIP.cpp LevelZeroArch.cpp)

# Legacy binary names.
add_clang_symlink(amdgpu-arch offload-arch)
add_clang_symlink(nvptx-arch offload-arch)

clang_target_link_libraries(offload-arch
  PRIVATE
  DISABLE_CLANG_LINK_CLANG_DYLIB
  clangBasic)
