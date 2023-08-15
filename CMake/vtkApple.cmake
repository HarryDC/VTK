# NOTE: this file is required to execute early in the build, before any other
# VTK modules or related options have been parsed.

# Prior to CMake 3.16.7 the OBJCXX language was not supported directly, VTK
# provided its own internal solution.  We now require using the OBJCXX language.
if (DEFINED VTK_REQUIRED_OBJCXX_FLAGS)
  # NOTE: do not use vtk_deprecated_setting, there is no default value.
  message(DEPRECATION
    "VTK_REQUIRED_OBJCXX_FLAGS is not supported anymore.  Please use CMake "
    "3.16.7+ to configure VTK, and configure with "
    "CMAKE_OBJCXX_FLAGS=<VTK_REQUIRED_OBJCXX_FLAGS>.")
  set(CMAKE_OBJCXX_FLAGS "${VTK_REQUIRED_OBJCXX_FLAGS}")
endif()
if (CMAKE_VERSION VERSION_LESS 3.16.7)
  message(FATAL_ERROR "CMake 3.16.7+ are required on Apple.")
endif()
include(CheckLanguage)
check_language(OBJCXX)
if (CMAKE_OBJCXX_COMPILER)
  enable_language(OBJCXX)
else()
  message(FATAL_ERROR "Unable to enable to the OBJCXX language.")
endif()

# Previously `VTK_IOS_BUILD` was the VTK option to build frameworks, but is no
# longer supported.
if (DEFINED VTK_IOS_BUILD)
  message(DEPRECATION
    "VTK_IOS_BUILD is no longer supported.  Configure VTK with "
    "VTK_FRAMEWORK_BUILD instead.")
  set(VTK_FRAMEWORK_BUILD "${VTK_IOS_BUILD}"
    CACHE BOOL "Install vtk.framework for apple targets." FORCE)
endif()

# Previously an external project was configured for IOS_SIMULATOR_ARCHITECTURES
# and IOS_DEVICE_ARCHITECTURES were configured as separate ExternalProject_Add's
# whereas now CMAKE_OSX_ARCHITECTURES should be used.  CMAKE_OSX_ARCHITECTURES
# must be set prior to any project() / enable_language() calls.  An error is
# required here since setting the value now is too late.
if (DEFINED IOS_SIMULATOR_ARCHITECTURES OR DEFINED IOS_DEVICE_ARCHITECTURES)
  message(FATAL_ERROR
    "IOS_SIMULATOR_ARCHITECTURES and IOS_DEVICE_ARCHITECTURES are not "
    "supported, use CMAKE_OSX_ARCHITECTURES.  A separate build for simulator / "
    "device frameworks is now required.")
endif()

# Previously IOS_DEPLOYMENT_TARGET was used for CMAKE_OSX_DEPLOYMENT_TARGET.
# CMAKE_OSX_DEPLOYMENT_TARGET must be set prior to any project() /
# enable_language() calls.  An error is required here since setting the value
# now is too late.
if (DEFINED IOS_DEPLOYMENT_TARGET)
  message(FATAL_ERROR
    "IOS_DEPLOYMENT_TARGET is no longer supported, use "
    "CMAKE_OSX_DEPLOYMENT_TARGET instead.")
endif()

# Previously IOS_EMBED_BITCODE was used for this Xcode build attribute.
# The Xcode generator settings need to be set prior to this code.
if (DEFINED IOS_EMBED_BITCODE)
  message(FATAL_ERROR
    "IOS_EMBED_BITCODE is no longer supported, configure with "
    "CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE instead, and make sure to use the "
    "Xcode generator (-G Xcode).")
endif()

# Previously vtkIOS.cmake would configure two separate projects via
# ExternalProject_Add (one for devices, one for simulator) using separate
# toolchain files managed internally.  These flags were only ever set by the
# previous iOS build and are no longer supported.
if (DEFINED APPLE_IOS OR DEFINED TARGET_OS_IPHONE OR DEFINED TARGET_IPHONE_SIMULATOR)
  message(FATAL_ERROR
    "APPLE_IOS, TARGET_OS_IPHONE, and TARGET_IPHONE_SIMULATOR are no longer "
    "supported.  Use VTK_FRAMEWORK_BUILD=ON instead.")
endif()
# The APPLE_IOS boolean was used to flag special case behaviors when building
# vtk.framework, this flag is now VTK_APPLE_IOS and should only be set to ON
# when we are building for iOS.
# NOTE: CMAKE_SYSTEM_NAME=iOS should be true for both device and simulator,
# what should change is the CMAKE_OSX_SYSROOT (to e.g., the simulator .sdk).
set(VTK_APPLE_IOS OFF)
string(TOLOWER "${CMAKE_SYSTEM_NAME}" _cmake_system_name_lower)
if (_cmake_system_name_lower STREQUAL "ios")
  set(VTK_APPLE_IOS ON)
  message(STATUS "VTK is building for an iOS target.")
endif()

# Frameworks must be static libraries.  No add_library calls have occurred yet
# so we can correct this value now.
if (VTK_FRAMEWORK_BUILD AND BUILD_SHARED_LIBS)
  message(WARNING
    "VTK_FRAMEWORK_BUILD requires BUILD_SHARED_LIBS=OFF, overriding.")
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries?" FORCE)
endif()

# When this is ON the resultant framework is not usable for combining into an
# XCFramework (only device or simulator but not both are allowed).
if (VTK_FRAMEWORK_BUILD AND DEFINED CMAKE_IOS_INSTALL_COMBINED AND CMAKE_IOS_INSTALL_COMBINED)
  message(WARNING
    "CMAKE_IOS_INSTALL_COMBINED is not supported, vtk.framework should be "
    "compiled once for devices and separately for simulators.")
endif()

mark_as_advanced(
  CMAKE_OSX_ARCHITECTURES
  CMAKE_OSX_DEPLOYMENT_TARGET
  CMAKE_OSX_SYSROOT)

if (CMAKE_OSX_DEPLOYMENT_TARGET AND
    CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS "10.7")
  message(FATAL_ERROR
    "Minimum OS X deployment target is 10.7, please update "
    "CMAKE_OSX_DEPLOYMENT_TARGET.")
endif ()

# Set defaults for the framework build.
if (VTK_FRAMEWORK_BUILD)
  # Redirect headers, licenses, and binaries to install to vtk.framework.
  # Libraries (and CMake config files) are combined into a single file
  # vtk.framework/vtk file, see vtk/CMake/vtkFrameworkFinalize.cmake.  The final
  # install tree will have CMAKE_INSTALL_PREFIX/lib and
  # CMAKE_INSTALL_PREFIX/vtk.framework, there is no way to prevent the `lib`
  # directory from being created.
  set(CMAKE_INSTALL_INCLUDEDIR "vtk.framework/Headers")
  set(CMAKE_INSTALL_DATAROOTDIR "vtk.framework/Resources")
  set(CMAKE_INSTALL_BINDIR "vtk.framework/Resources/bin")

  # The set of modules we want to build by default for the vtk.xcframework.
  # This list was originally curated for the immediate needs: RenderingOpenGL2
  # and some Image loading / exporting.
  #
  # Everything else was determined by manually curating the final list of
  # dependencies (and more importantly reducing the number of things built for
  # a vtk.xcframework, since most of the modules are not applicable / useful).
  #
  # The two lists _vtk_framework_build_{requested,rejected}_modules are set here
  # in this file since the lists are quite long.  These lists are used in the
  # root CMakeLists.txt of VTK.
  set(_vtk_framework_build_requested_modules
    VTK::InteractionImage
    VTK::InteractionStyle
    VTK::InteractionWidgets
    VTK::IOExport
    VTK::IOImage
    VTK::RenderingOpenGL2
    # Build third party libraries we want into it directly.
    VTK::diy2
    VTK::doubleconversion
    VTK::eigen
    VTK::expat
    VTK::exprtk
    VTK::fast_float
    VTK::fmt
    VTK::freetype
    VTK::glew
    VTK::jpeg
    VTK::kissfft
    VTK::lz4
    VTK::lzma
    VTK::png
    VTK::pugixml
    VTK::tiff
    VTK::utf8
    VTK::verdict
    VTK::zlib)
  set(_vtk_framework_build_rejected_modules
    # Prevent external libraries we do not want from being built.
    VTK::cgns
    VTK::cli11
    VTK::exodusII
    VTK::fides
    VTK::gl2ps
    VTK::h5part
    VTK::hdf5
    VTK::ioss
    VTK::jsoncpp
    VTK::libharu
    VTK::libproj
    VTK::libxml2
    VTK::loguru
    VTK::netcdf
    VTK::nlohmannjson
    VTK::ogg
    VTK::pegtl
    VTK::sqlite
    VTK::theora
    VTK::vpic
    VTK::xdmf2
    VTK::xdmf3
    VTK::zfp
    # Disable VTK modules that either do not work, or bring in too many dependencies.
    VTK::AcceleratorsVTKmCore
    VTK::AcceleratorsVTKmDataModel
    VTK::AcceleratorsVTKmFilters
    VTK::ChartsCore
    VTK::CommonArchive
    VTK::DomainsChemistry
    VTK::DomainsChemistryOpenGL2
    VTK::DomainsMicroscopy
    VTK::DomainsParallelChemistry
    VTK::FiltersCellGrid
    VTK::FiltersDSP
    VTK::FiltersFlowPaths
    VTK::FiltersGeneric
    VTK::FiltersGeometryPreview
    VTK::FiltersImaging
    VTK::FiltersOpenTURNS
    VTK::FiltersParallel
    VTK::FiltersParallelDIY2
    VTK::FiltersParallelFlowPaths
    VTK::FiltersParallelGeometry
    VTK::FiltersParallelImaging
    VTK::FiltersParallelMPI
    VTK::FiltersParallelStatistics
    VTK::FiltersParallelVerdict
    VTK::FiltersPoints
    VTK::FiltersProgrammable
    VTK::FiltersReduction
    VTK::FiltersReebGraph
    VTK::FiltersSMP
    VTK::FiltersSelection
    VTK::FiltersTopology
    VTK::GUISupportQt
    VTK::GUISupportQtQuick
    VTK::GUISupportQtSQL
    VTK::GeovisCore
    VTK::GeovisGDAL
    VTK::InfovisBoost
    VTK::InfovisBoostGraphAlgorithms
    VTK::InfovisCore
    VTK::InfovisLayout
    # Most IO modules will need extra attention, disable all.
    VTK::IOADIOS2
    VTK::IOAMR
    VTK::IOAsynchronous
    VTK::IOCGNSReader
    VTK::IOCONVERGECFD
    VTK::IOCellGrid
    VTK::IOCesium3DTiles
    VTK::IOChemistry
    VTK::IOCityGML
    VTK::IOEnSight
    VTK::IOExodus
    VTK::IOExport
    VTK::IOExportGL2PS
    VTK::IOExportPDF
    VTK::IOFFMPEG
    VTK::IOFides
    VTK::IOGDAL
    VTK::IOGeoJSON
    VTK::IOH5Rage
    VTK::IOH5part
    VTK::IOHDF
    VTK::IOIOSS
    VTK::IOImport
    VTK::IOInfovis
    VTK::IOLAS
    VTK::IOLSDyna
    VTK::IOMINC
    VTK::IOMPIImage
    VTK::IOMPIParallel
    VTK::IOMotionFX
    VTK::IOMovie
    VTK::IOMySQL
    VTK::IONetCDF
    VTK::IOOCCT
    VTK::IOODBC
    VTK::IOOMF
    VTK::IOOggTheora
    VTK::IOOpenVDB
    VTK::IOPDAL
    VTK::IOPIO
    VTK::IOPLY
    VTK::IOParallel
    VTK::IOParallelExodus
    VTK::IOParallelLSDyna
    VTK::IOParallelNetCDF
    VTK::IOParallelXML
    VTK::IOParallelXdmf3
    VTK::IOPostgreSQL
    VTK::IOSQL
    VTK::IOSegY
    VTK::IOTRUCHAS
    VTK::IOTecplotTable
    VTK::IOVPIC
    VTK::IOVeraOut
    VTK::IOVideo
    VTK::IOXdmf2
    VTK::IOXdmf3
    VTK::ParallelMPI
    VTK::PythonInterpreter
    VTK::RenderingCellGrid  # Does not support iOS.
    VTK::RenderingContextOpenGL2  # Does not support iOS.
    VTK::RenderingExternal
    VTK::RenderingFFMPEGOpenGL2
    VTK::RenderingFreeTypeFontConfig
    VTK::RenderingLICOpenGL2
    VTK::RenderingLOD
    VTK::RenderingLabel
    VTK::RenderingMatplotlib
    VTK::RenderingOpenVR
    VTK::RenderingOpenXR
    VTK::RenderingParallel
    VTK::RenderingParallelLIC
    VTK::RenderingQt
    VTK::RenderingRayTracing
    VTK::RenderingSceneGraph
    VTK::RenderingVR  # Does not support iOS.
    VTK::RenderingVtkJS
    VTK::RenderingZSpace
    VTK::TestingCore
    VTK::TestingDataModel
    VTK::TestingGenericBridge
    VTK::TestingIOSQL
    VTK::TestingRendering
    VTK::UtilitiesBenchmarks
    VTK::ViewsContext2D
    VTK::ViewsCore
    VTK::ViewsInfovis
    VTK::ViewsQt
    VTK::WebCore
    VTK::WebGLExporter
    VTK::WrappingPythonCore
    VTK::WrappingTools)
endif()
