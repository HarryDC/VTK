## VTK iOS Examples ##

To run the VTK iOS examples, you must first build VTK into an xcframework. The
script and instructions for doing so are here: <VTK>/Utilities/build_xcframework.py

Not all VTK modules are currently supported. You can edit the lists in
<VTK>/CMake/vtkApple.cmake to test out the modules you need. We encourage the
VTK community to provide additional support for VTK modules on iOS.

# Notes on the Examples #

If you built `VTK.xcframework` to the default location, you should be able to
run each example straightaway to either a device or the simulator. However, you
will likely need to first set the correct Development Team within Xcode to set
the correct provisioning and signing.

Each example expects `VTK.xcframework` to be located at <VTK source
root>/xcframework/install. If you built it or moved it elsewhere, be sure to
update your example projects.

The headers are expected at
`<VTK>/xcframework/install/iPhoneOS/vtk.framework/Headers`. If you configured
the `xcframework` build to not include device support, be sure to update this
header path to the simulator equivalent.

The GLPaint example is a modified version of Apple's GLPaint example. So credit
to them for the original example file and framework.
