# NOTE: this file is required to execute early in the build, before any other
# VTK modules or related options have been parsed.

# Prior to CMake 3.16.7 the OBJCXX language was not supported directly, VTK
# provided its own internal solution.  We now require using the OBJCXX language.
if (DEFINED VTK_REQUIRED_OBJCXX_FLAGS)
  # NOTE: do not use vtk_deprecated_setting, there is no default value.
  message(DEPRECATION "VTK_REQUIRED_OBJCXX_FLAGS is not supported anymore.  "
    "Please use CMake 3.16.7+ to configure VTK, and configure with "
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
  message(DEPRECATION "VTK_IOS_BUILD is no longer supported.  Configure VTK "
    "with VTK_FRAMEWORK_BUILD instead.")
  set(VTK_FRAMEWORK_BUILD "${VTK_IOS_BUILD}"
    CACHE BOOL "Install vtk.framework for apple targets." FORCE)
endif()

# Previously an external project was configured for IOS_SIMULATOR_ARCHITECTURES
# and IOS_DEVICE_ARCHITECTURES were configured as separate ExternalProject_Add's
# whereas now CMAKE_OSX_ARCHITECTURES should be used.  CMAKE_OSX_ARCHITECTURES
# must be set prior to any project() / enable_language() calls.  An error is
# required here since setting the value now is too late.
if (DEFINED IOS_SIMULATOR_ARCHITECTURES OR DEFINED IOS_DEVICE_ARCHITECTURES)
  message(FATAL_ERROR "IOS_SIMULATOR_ARCHITECTURES and "
    "IOS_DEVICE_ARCHITECTURES are not supported, use CMAKE_OSX_ARCHITECTURES.  "
    "A separate build for simulator / device frameworks is now required.")
endif()

# Previously IOS_DEPLOYMENT_TARGET was used for CMAKE_OSX_DEPLOYMENT_TARGET.
# CMAKE_OSX_DEPLOYMENT_TARGET must be set prior to any project() /
# enable_language() calls.  An error is required here since setting the value
# now is too late.
if (DEFINED IOS_DEPLOYMENT_TARGET)
  message(FATAL_ERROR "IOS_DEPLOYMENT_TARGET is no longer supported, use "
    "CMAKE_OSX_DEPLOYMENT_TARGET instead.")
endif()

# Previously IOS_EMBED_BITCODE was used for this Xcode build attribute.
# The Xcode generator settings need to be set prior to this code.
if (DEFINED IOS_EMBED_BITCODE)
  message(FATAL_ERROR "IOS_EMBED_BITCODE is no longer supported, configure "
    "with CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE instead, and make sure to use "
    "the Xcode generator (-G Xcode).")
endif()

# Previously vtkIOS.cmake would configure two separate projects via
# ExternalProject_Add (one for devices, one for simulator) using separate
# toolchain files managed internally.  These flags were only ever set by the
# previous iOS build and are no longer supported.
if (DEFINED APPLE_IOS OR DEFINED TARGET_OS_IPHONE OR DEFINED TARGET_IPHONE_SIMULATOR)
  message(FATAL_ERROR "APPLE_IOS, TARGET_OS_IPHONE, and TARGET_IPHONE_SIMULATOR"
    " are no longer supported.  Use VTK_FRAMEWORK_BUILD=ON instead.")
endif()
# The APPLE_IOS boolean was used to flag special case behaviors when building
# vtk.framework, this flag is now VTK_APPLE_IOS and should only be set to ON
# when we are building for iOS.
# NOTE: CMAKE_SYSTEM_NAME=iOS should be true for both device and simulator,
# what should change is the CMAKE_OSX_SYSROOT (to e.g., the simulator .sdk).
set(VTK_APPLE_IOS OFF)
string(TOLOWER "${CMAKE_SYSTEM_NAME}" _cmake_system_name_lower)
if (_cmake_system_name_lower STREQUAL ios)
  set(VTK_APPLE_IOS ON)
  message(STATUS "VTK is building for an iOS target.")
endif()

# Frameworks must be static libraries.  No add_library calls have occurred yet
# so we can correct this value now.
if (VTK_FRAMEWORK_BUILD AND BUILD_SHARED_LIBS)
  message(WARNING "VTK_FRAMEWORK_BUILD requires BUILD_SHARED_LIBS=OFF, overriding.")
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries?" FORCE)
endif()

# When this is ON the resultant framework is not usable for combining into an
# XCFramework (only device or simulator but not both are allowed).
if (VTK_FRAMEWORK_BUILD AND DEFINED CMAKE_IOS_INSTALL_COMBINED AND CMAKE_IOS_INSTALL_COMBINED)
  message(WARNING "CMAKE_IOS_INSTALL_COMBINED is not supported, vtk.framework "
    "should be compiled once for devices and separately for simulators.")
endif()

mark_as_advanced(
  CMAKE_OSX_ARCHITECTURES
  CMAKE_OSX_DEPLOYMENT_TARGET
  CMAKE_OSX_SYSROOT)

if (CMAKE_OSX_DEPLOYMENT_TARGET AND
    CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS "10.7")
  message(FATAL_ERROR "Minimum OS X deployment target is 10.7, please update CMAKE_OSX_DEPLOYMENT_TARGET.")
endif ()

# Set defaults for the framework build.
if (VTK_FRAMEWORK_BUILD)
  # Redirect headers, licenses, and binaries to install to vtk.framework.
  # Libraries (and CMake config files) are combined into a single file
  # vtk.framework/vtk file, see vtk/CMake/vtkFrameworkFinalize.cmake.  The final
  # install tree will have CMAKE_INSTALL_PREFIX/lib and
  # CMAKE_INSTALL_PREFIX/vtk.framework, there is no way to prevent the `lib`
  # directory from being created.
  set(CMAKE_INSTALL_INCLUDEDIR "vtk.framework/Headers" CACHE STRING "Install header files to the framework.")
  set(CMAKE_INSTALL_DATAROOTDIR "vtk.framework/Resources" CACHE STRING "Install license files to the framework resources directory.")
  set(CMAKE_INSTALL_BINDIR "vtk.framework/Resources/bin" CACHE STRING "Install binaries to the framework resources directory.")

  # Default options: a relatively minimal build with most of VTK disabled.  Each
  # `set` statement includes a guard to avoid overwriting user provided options
  # on the commandline.  This file must be executed before any of the remaining
  # VTK options have been populated.
  if (NOT DEFINED VTK_ENABLE_WRAPPING)
    set(VTK_ENABLE_WRAPPING OFF CACHE BOOL "Whether wrapping is available or not")
  endif()

  # Include: modules known to work in vtk.framework.
  set(vtk_module_include_list
    CommonColor
    CommonComputationalGeometry
    CommonExecutionModel
    CommonImplicitArrays
    CommonMath
    CommonMisc
    CommonSystem
    CommonTransforms
    DICOMParser # Required by IOImage
    FiltersAMR
    FiltersCore
    FiltersExtraction  # Required by InteractionStyle
    FiltersGeneral
    FiltersGeometry
    FiltersHybrid # Required by InteractionWidgets
    FiltersModeling  # Required by InteractionWidgets
    FiltersSources  # Required for RenderingVolumeOpenGL2
    FiltersStatistics
    FiltersTexture # Required by InteractionWidgets
    FiltersHyperTree # Required by RenderingOpenGL2
    FiltersVerdict
    ImagingColor
    ImagingCore
    ImagingFourier
    ImagingGeneral
    ImagingHybrid
    ImagingMath
    ImagingMorphological
    ImagingOpenGL2
    ImagingSources
    ImagingStatistics
    ImagingStencil
    InteractionImage
    InteractionStyle
    InteractionWidgets
    IOCore
    IOImage  # Required by RenderingOpenGL2
    IOGeometry
    IOLegacy  # Required by ParallelCore
    IOXML  # Required by ParallelCore
    IOXMLParser
    ParallelDIY  # Required by FiltersExtraction
    ParallelCore  # Required by ParallelDIY
    RenderingAnnotation # Required by InteractionWidgets
    RenderingContext2D
    RenderingCore
    RenderingFreeType
    RenderingImage
    RenderingOpenGL2
    RenderingUI
    RenderingVolume
    RenderingVolumeAMR
    RenderingVolumeOpenGL2)
  foreach (mod IN LISTS vtk_module_include_list)
    set(var "VTK_MODULE_ENABLE_VTK_${mod}")
    if (NOT DEFINED ${var})
      set(${var} "YES" CACHE STRING "Enable the VTK::${mod} module.")
    endif()
    unset(var)
  endforeach()

  # Exclude: modules that either have problems or too many dependencies.
  set(vtk_module_exclude_list
    AcceleratorsVTKmCore
    AcceleratorsVTKmDataModel
    AcceleratorsVTKmFilters
    ChartsCore
    CommonArchive
    CommonCore
    CommonDataModel
    DomainsChemistry
    DomainsChemistryOpenGL2
    DomainsMicroscopy
    DomainsParallelChemistry
    FiltersCellGrid
    FiltersDSP
    FiltersFlowPaths
    FiltersGeneric
    FiltersGeometryPreview
    FiltersImaging
    FiltersOpenTURNS
    FiltersParallel
    FiltersParallelDIY2
    FiltersParallelFlowPaths
    FiltersParallelGeometry
    FiltersParallelImaging
    FiltersParallelMPI
    FiltersParallelStatistics
    FiltersParallelVerdict
    FiltersPoints
    FiltersProgrammable
    FiltersReduction
    FiltersReebGraph
    FiltersSMP
    FiltersSelection
    FiltersTopology
    GUISupportQt
    GUISupportQtQuick
    GUISupportQtSQL
    GeovisCore
    GeovisGDAL
    InfovisBoost
    InfovisBoostGraphAlgorithms
    InfovisCore
    InfovisLayout
    # Most IO modules will need extra attention, disable all.
    IOADIOS2
    IOAMR
    IOAsynchronous
    IOCGNSReader
    IOCONVERGECFD
    IOCellGrid
    IOCesium3DTiles
    IOChemistry
    IOCityGML
    IOEnSight
    IOExodus
    IOExport
    IOExportGL2PS
    IOExportPDF
    IOFFMPEG
    IOFides
    IOGDAL
    IOGeoJSON
    IOH5Rage
    IOH5part
    IOHDF
    IOIOSS
    IOImport
    IOInfovis
    IOLAS
    IOLSDyna
    IOMINC
    IOMPIImage
    IOMPIParallel
    IOMotionFX
    IOMovie
    IOMySQL
    IONetCDF
    IOOCCT
    IOODBC
    IOOMF
    IOOggTheora
    IOOpenVDB
    IOPDAL
    IOPIO
    IOPLY
    IOParallel
    IOParallelExodus
    IOParallelLSDyna
    IOParallelNetCDF
    IOParallelXML
    IOParallelXdmf3
    IOPostgreSQL
    IOSQL
    IOSegY
    IOTRUCHAS
    IOTecplotTable
    IOVPIC
    IOVeraOut
    IOVideo
    IOXdmf2
    IOXdmf3
    ParallelMPI
    PythonInterpreter
    RenderingCellGrid  # Does not support iOS
    RenderingContextOpenGL2  # Does not support iOS
    RenderingExternal
    RenderingFFMPEGOpenGL2
    RenderingFreeTypeFontConfig
    RenderingLICOpenGL2
    RenderingLOD
    RenderingLabel
    RenderingMatplotlib
    RenderingOpenVR
    RenderingOpenXR
    RenderingParallel
    RenderingParallelLIC
    RenderingQt
    RenderingRayTracing
    RenderingSceneGraph
    RenderingVR  # Does not support iOS.
    RenderingVtkJS
    RenderingZSpace
    TestingCore
    TestingDataModel
    TestingGenericBridge
    TestingIOSQL
    TestingRendering
    UtilitiesBenchmarks
    ViewsContext2D
    ViewsCore
    ViewsInfovis
    ViewsQt
    WebCore
    WebGLExporter
    WrappingPythonCore
    WrappingTools)
  foreach (mod IN LISTS vtk_module_exclude_list)
    set(var "VTK_MODULE_ENABLE_VTK_${mod}")
    if (NOT DEFINED ${var})
      set(${var} "NO" CACHE STRING "Enable the VTK::${mod} module.")
    endif()
    unset(var)
  endforeach()

  # Developer safeguard: if you have an entry in the include list, and also put
  # it in the exclude list, it will be *included* since that cache entry gets
  # set first.  To aid development, modules found in both result in an error.
  set(invalid_list)
  foreach (include_mod IN LISTS vtk_module_include_list)
    foreach (exclude_mod IN LISTS vtk_module_exclude_list)
      if ("${include_mod}" STREQUAL "${exclude_mod}")
        list(APPEND invalid_list "${include_mod}")
      endif()
    endforeach()
  endforeach()

  list(LENGTH invalid_list invalid_list_length)
  if (invalid_list_length GREATER 0)
    message(FATAL_ERROR
      "vtkApple.cmake: the following module(s) appear in both "
      "`vtk_module_include_list` and `vtk_module_exclude_list`, duplicates are "
      "not allowed: ${invalid_list}")
  endif()

  unset(vtk_module_include_list)
  unset(vtk_module_exclude_list)
  unset(invalid_list)
  unset(invalid_list_length)
endif()
