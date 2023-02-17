## Modernize vtk.framework build and add xcframework helper script

VTK's previous iOS build (via `vtkIOS.cmake`) was written prior to many features
becoming available in CMake.  The new way to compile `vtk.framework` is to use
the boolean option `VTK_FRAMEWORK_BUILD=ON`.  Previously a user would set
`IOS_SIMULATOR_ARCHITECTURES` and `IOS_DEVICE_ARCHITECTURES` variables and
internall `vtkIOS.cmake` would configure and compile separate CMake projects,
once for simulator and once for device.  In the new format, this is not possible
and a user must compile each separately (setting `CMAKE_OSX_ARCHITECTURES` and
defining `CMAKE_OSX_SYSROOT` accordingly).

A new `Utilities/build_xcframework.py` script is added to aid in compiling
multiple architectures for multiple platforms.  It will first compile each
`vtk.framework` for the requested platform (iOS, iOS Simulator, etc) and
requested architectures (e.g., `arm64` or `arm64e`), and then combine all of
these into a single `vtk.xcframework`.  By default the script will produce
a `vtk.xcframework` with iOS architectures `arm64` and `arm64e`, and iOS
simulator architectures `arm64` and `x86_64`.  Refer to the script documentation
for more information, including how to add in the macOS platform if desired.
Run the script with `--dry-run` to see what steps would take place in the event
that you desire to use your own build script / toolchain.  It will print out
information about variables such as `CMAKE_OSX_SYSROOT` (and more) that will
help guide your efforts.
