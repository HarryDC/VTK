#!/usr/bin/env python3
"""
Build vtk.xcframework on Apple using Xcode.  An XCFramework is Apple's preferred binary
distribution mechanism intended to succeed Frameworks, since Frameworks are not able to
support combining multiple platforms and architectures together.

To produce an XCFramework, each desired platform (macOS, iOS, iOS Simulator) must be
configured and compiled into ``vtk.framework`` separately first.  After each is
successfully built, the script will combine them together at the end using
``xcodebuild -create-xcframework``.  For a given platform, the relevant architectures
may be requested via the command line options (or set to ``None`` to exclude).  Using
the defaults of this script, the following XCFramework will be produced:

    vtk.xcframework/
    ├── Info.plist
    ├── ios-arm64_arm64e
    │   └── vtk.framework
    │       ├── Headers
    │       ├── Resources
    │       └── vtk
    └── ios-arm64_x86_64-simulator
        └── vtk.framework
            ├── Headers
            ├── Resources
            └── vtk

If you were to call the script with ``-m arm64,x86_64``, then in addition to the
architectures above you would have:

    vtk.xcframework/
    ├── Info.plist
    ├── ios-arm64_arm64e
    │   └── vtk.framework
    │       ├── Headers
    │       ├── Resources
    │       └── vtk
    ├── ios-arm64_x86_64-simulator
    │   └── vtk.framework
    │       ├── Headers
    │       ├── Resources
    │       └── vtk
    └── macos-arm64_x86_64
        └── vtk.framework
            ├── Headers
            ├── Resources
            └── vtk

Since this script is designed to be able to change which architectures are being
compiled for a given platform, we elect to supply most of the arguments that would
traditionally be in a toolchain file (``CMAKE_*`` variables).  Typically the
architectures and deployment target a given project supports are static and updated
manually over time, making it more appropriate to use a "toolchain file":

https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html

Provide the --dry-run argument to the script to see what commands will execute, in the
output every ``CMAKE_*`` variable **except** for ``CMAKE_INSTALL_PREFIX`` would just as
appropriately be put in a toolchain file.  Most of these variables must be set either
from the command line at configure time, or via a toolchain file -- using ``set()`` in
your ``CMakeLists.txt`` is too late as many checks required for configuring the cross
compiling environment take place with the ``project()`` call.

**Note**: no compatibility checks for whether the deployment target and requested
architectures are valid, invalid configurations will fail at build time by Xcode.

For more information on XCFrameworks, see:
https://developer.apple.com/videos/play/wwdc2019/416
"""

import argparse
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Iterable, Optional


class Platform:
    """
    Represent an Apple system platform to bundle a number of common CMake
    configure arguments for the given platform.  See also:

    https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html#cross-compiling-for-ios-tvos-or-watchos

    **Attributes**

    name:
        The value for ``CMAKE_SYSTEM_NAME``.  Must be one of
        :data:`Platform.SUPPORTED_SDKS`.  This name will be used to query the
        full ``sdk_path`` via ``xcrun``.

    generator:
        The string name of the CMake generator.  Supported values: ``Xcode``,
        ``Ninja Multi-Config``.

    arch_list:
        List of architectures for ``CMAKE_OSX_ARCHITECTURES``.

    system_name:
        The value for ``CMAKE_OSX_SYSROOT``, which should be one of the
        entries available in SUPPORTED_SDKS.  This class will query the full
        sdk path using ``xcrun``.

    deployment_target:
        If provided, the operating system deployment target for
        ``CMAKE_OSX_DEPLOYMENT_TARGET``.  The ``arch_list`` and this variable
        are interdependent (e.g., some architectures are not available for a
        given Apple deployment target).

    sdk_path:
        The full path to the SDK populated using ``system_name``.

    is_mobile:
        ``True`` when the provided name is a mobile target, ``False`` otherwise.
    """

    SUPPORTED_SYSTEM_NAMES = {"Darwin", "iOS", "tvOS", "watchOS"}
    """The available system names for CMake."""

    SUPPORTED_SDKS = {
        "macosx",
        "iphoneos",
        "iphonesimulator",
        "appletvos",
        "appletvsimulator",
        "watchos",
        "watchsimulator",
    }
    """
    The names of SDKs that can be queried using ``xcrun``.  Note that ``xcrun``
    desires lower-case entries.
    """

    def __init__(
        self,
        *,
        name: str,
        generator: str,
        arch_list: Iterable[str],
        system_name: str,
        deployment_target: Optional[str] = None,
    ) -> None:
        if name.lower() not in self.SUPPORTED_SDKS:
            raise ValueError(
                f"Unsupported name '{name}', must be one of: "
                f"{', '.join(self.SUPPORTED_SDKS)}"
            )
        self.name: str = name
        self.generator: str = generator
        if generator not in {"Xcode", "Ninja Multi-Config"}:
            raise ValueError(
                f"Unsupported CMake generator {generator}, must be one of: "
                "`Xcode`, `Ninja Multi-Config`."
            )
        self.arch_list: Iterable[str] = arch_list
        if system_name not in self.SUPPORTED_SYSTEM_NAMES:
            raise ValueError(
                f"Unsupported system_name '{system_name}', must be one of: "
                f"{', '.join(self.SUPPORTED_SYSTEM_NAMES)}"
            )
        self.system_name: str = system_name
        self.deployment_target: Optional[str] = deployment_target
        self.sdk_path: str = self._find_sdk_path()
        self.gcc_path, self.gxx_path = self._find_gcc_gxx()
        self.is_mobile: bool = self.name.lower() != "macosx"

    def _find_sdk_path(self) -> str:
        """Return the full path to the SDK via ``xcrun``."""
        proc = subprocess.run(
            ["xcrun", "-sdk", self.name.lower(), "-show-sdk-path"],
            check=True,
            capture_output=True,
        )
        return proc.stdout.decode("utf-8").strip()

    def _find_gcc_gxx(self) -> tuple[str, str]:
        """Return the full paths to ``(gcc, g++)`` via ``xcrun``."""
        gcc_proc = subprocess.run(
            ["xcrun", "-sdk", self.name.lower(), "-find", "gcc"],
            check=True,
            capture_output=True,
        )
        gxx_proc = subprocess.run(
            ["xcrun", "-sdk", self.name.lower(), "-find", "g++"],
            check=True,
            capture_output=True,
        )
        return (
            gcc_proc.stdout.decode("utf-8").strip(),
            gxx_proc.stdout.decode("utf-8").strip(),
        )

    def configure_args(self) -> Iterable[str]:
        """Return the CMake configure arguments for this platform."""
        ret = [
            # NOTE: for historical reasons, some of the CMAKE_* variables have
            # an OSX prefix.  These variables affect more than just macOS.
            # Architecture selection.
            f"-DCMAKE_OSX_ARCHITECTURES={';'.join(a for a in self.arch_list)}",
            # System configuration.
            f"-DCMAKE_OSX_SYSROOT={self.sdk_path}",
            f"-DCMAKE_C_COMPILER={self.gcc_path}",
            f"-DCMAKE_CXX_COMPILER={self.gxx_path}",
            # NOTE: setting this will mean CMAKE_CROSS_COMPILING=ON, which is desired.
            f"-DCMAKE_SYSTEM_NAME={self.system_name}",
        ]
        if self.generator == "Xcode":
            ret += [
                # BUILD_LIBRARY_DISTRIBUTION=YES is required to create xcframework.
                "-DCMAKE_XCODE_ATTRIBUTE_BUILD_LIBRARY_FOR_DISTRIBUTION=YES",
                # ONLY_ACTIVE_ARCH=NO is required to build multiple architectures.
                "-DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO",
            ]
        if self.deployment_target is not None:
            ret.append(f"-DCMAKE_OSX_DEPLOYMENT_TARGET={self.deployment_target}")
        if self.is_mobile:
            # `xcodebuild -create-xcframework` does not allow an input framework that
            # has both device and simulator targets.
            ret.append("-DCMAKE_IOS_INSTALL_COMBINED=OFF")

            # The following entries affect `find_package`, in particular OpenGL.
            ret += [
                f"-DCMAKE_FIND_ROOT_PATH={self.sdk_path}",
                "-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER",
                "-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY",
                "-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY",
            ]

        return ret


def main() -> None:
    this_file = Path(__file__).absolute()
    this_file_dir = this_file.parent

    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "-n",
        "--dry-run",
        dest="dry_run",
        action="store_true",
        help="Print the commands that would run, without executing them.",
    )
    parser.add_argument(
        "-G",
        "--generator",
        type=str,
        choices={"Xcode", "Ninja Multi-Config"},
        default="Ninja Multi-Config",
        help="CMake build system to generate and use.  Default: %(default)s",
    )
    parser.add_argument(
        "--toolchain",
        type=Path,
        default=None,
        help=(
            "CMake toolchain file to include, e.g., code signing variables such as "
            "MACOSX_FRAMEWORK_INFO_PLIST.  See also: "
            "https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html"
        ),
    )
    # We are in Utilities/build_xcframework.py, by default we want to do an out of
    # source build in a sibling directory to the vtk work tree.
    container = this_file_dir.parent.parent / "vtk_xcframework"
    parser.add_argument(
        "-b",
        "--build-root",
        dest="build_root",
        type=Path,
        default=container / "build",
        help="Root directory to store build trees in.  Default: %(default)s",
    )
    parser.add_argument(
        "-i",
        "--install-root",
        dest="install_root",
        type=Path,
        default=container / "install",
        help=(
            "The directory in which vtk.xcframework should be installed.  Default: "
            "%(default)s"
        ),
    )
    parser.add_argument(
        "--build-type",
        dest="build_type",
        type=str,
        default="Release",
        choices=("Release", "MinSizeRel", "RelWithDebInfo", "Debug"),
        help="The type of build to produce.  Default: %(default)s",
    )
    parser.add_argument(
        "-t",
        "--deployment-target",
        dest="deployment_target",
        type=float,
        default=12.0,
        help="The value to use for CMAKE_OSX_DEPLOYMENT_TARGET.  Default: %(default)s",
    )
    # From:
    # https://www.innerfence.com/howto/apple-ios-devices-dates-versions-instruction-sets
    supported_ios_device_architectures = {"arm64", "arm64e", "armv6", "armv7", "armv7s"}
    parser.add_argument(
        "-d",
        "--ios-device-architectures",
        dest="ios_device_architectures",
        type=str,
        default="arm64,arm64e",
        help=(
            "Comma separated list of iOS device architectures to build.  Supported "
            f"values: {', '.join(supported_ios_device_architectures)}.  Set to 'None' "
            "to skip building the iOS device targets.  Default: %(default)s"
        ),
    )
    supported_ios_simulator_architectures = {"arm64", "x86_64", "i386"}
    parser.add_argument(
        "-s",
        "--ios-simulator-architectures",
        dest="ios_simulator_architectures",
        type=str,
        default="arm64,x86_64",
        help=(
            "Comma separated list of iOS simulator architectures to build.  Supported "
            f"values: {', '.join(supported_ios_simulator_architectures)}.  Set to "
            "'None' to skip building the iOS simulator targets.  Default: %(default)s"
        ),
    )
    supported_macos_architectures = {"arm64", "x86_64"}
    parser.add_argument(
        "-m",
        "--macos-architectures",
        dest="macos_architectures",
        type=str,
        default="None",
        help=(
            "Comma separated list of macOS architectures to build.  Supported "
            f"values: {', '.join(supported_macos_architectures)}.  Set to 'None' to "
            "skip building the macOS targets.  Default: %(default)s"
        ),
    )
    parser.add_argument(
        "--debug",
        action="store_true",
        help="Add -DVTK_DEBUG_MODULE=ON -DVTK_DEBUG_MODULE_ALL=ON?",
    )
    parser.add_argument(
        "--devcheck", action="store_true", help="Format and lint this script."
    )
    # NOTE: these must be provided last.
    parser.add_argument(
        "extra_cmake_args",
        nargs="*",
        help=(
            "Any additional CMake configure arguments to pass on the command line.  "
            "No validation of these flags is performed."
        ),
    )

    args = parser.parse_args()

    if args.toolchain is not None:
        toolchain = Path(args.toolchain).absolute()
        if not toolchain.is_file():
            parser.error(
                f"the provided toolchain file '{args.toolchain}' is not a file."
            )
        args.toolchain = toolchain

    args.deployment_target = str(args.deployment_target)
    args.build_root = args.build_root.absolute()
    args.install_root = args.install_root.absolute()

    # Turn comma separated string into list of strings.
    def to_arch_list(arg: str) -> set[str]:
        if arg.lower() == "none":
            return set()
        return set([a.strip() for a in arg.split(",")])

    # Fail if an unsupported architecture is provided.
    def fail_if_unsupported(name: str, architectures: set[str], supported: set[str]):
        invalid = architectures - supported
        if invalid:
            parser.error(
                f"The {name} architecture(s) {', '.join(invalid)} are not supported.  "
                f"Allowed values: {', '.join(supported)}."
            )

    args.ios_device_architectures = to_arch_list(args.ios_device_architectures)
    fail_if_unsupported(
        "iOS device", args.ios_device_architectures, supported_ios_device_architectures
    )

    args.ios_simulator_architectures = to_arch_list(args.ios_simulator_architectures)
    fail_if_unsupported(
        "iOS simulator",
        args.ios_simulator_architectures,
        supported_ios_simulator_architectures,
    )

    args.macos_architectures = to_arch_list(args.macos_architectures)
    fail_if_unsupported(
        "macOS", args.macos_architectures, supported_macos_architectures
    )

    if (
        len(args.ios_device_architectures) == 0
        and len(args.ios_simulator_architectures) == 0
        and len(args.macos_architectures) == 0
    ):
        parser.error(
            "No XCFramework can be built -- 'None' architecture requested for macOS, "
            "iOS device, and iOS simulator.  At least one must be non-None."
        )

    # Short-circuit when developer check mode requested.
    if args.devcheck:
        # python3 -m venv venv
        # source venv/bin/activate
        # pip install -U pip
        # pip install black mypy flake8 flake8-import-order isort
        line_length = "88"
        subprocess.check_call(
            ["black", "--line-length", line_length, str(this_file)],
        )
        subprocess.check_call(["isort", "--line-length", line_length, str(this_file)])
        subprocess.check_call(["mypy", str(this_file)])
        subprocess.check_call(
            ["flake8", "--max-line-length", line_length, str(this_file)]
        )
        sys.exit(0)

    # Create the full list of requested platforms to build.
    all_platforms = []
    if args.ios_device_architectures:
        all_platforms.append(
            Platform(
                name="iPhoneOS",
                generator=args.generator,
                arch_list=args.ios_device_architectures,
                system_name="iOS",
                deployment_target=args.deployment_target,
            )
        )
    if args.ios_simulator_architectures:
        all_platforms.append(
            Platform(
                name="iPhoneSimulator",
                generator=args.generator,
                arch_list=args.ios_simulator_architectures,
                system_name="iOS",
                deployment_target=args.deployment_target,
            )
        )
    if args.macos_architectures:
        all_platforms.append(
            Platform(
                name="MacOSX",
                generator=args.generator,
                arch_list=args.macos_architectures,
                system_name="Darwin",
                deployment_target=args.deployment_target,
            )
        )

    if args.dry_run:
        print("==> DRY RUN: none of the commands will execute.")

    if args.build_root.exists():
        print(f"==> Removing and recreating: {str(args.build_root)}")
        if not args.dry_run:
            shutil.rmtree(args.build_root)
            args.build_root.mkdir(parents=True)

    if args.install_root.exists():
        print(f"==> Removing and recreating: {str(args.install_root)}")
        if not args.dry_run:
            shutil.rmtree(args.install_root)
            args.install_root.mkdir(parents=True)

    # Configure the build tree for each platform and build a list of `cmake --build`
    # arguments for each.  If the requested configure arguments (architecture,
    # deployment target, etc) cause errors we want to catch these before building.
    all_build_args: list[list[str]] = []
    install_roots: list[Path] = []
    for platform in all_platforms:
        print("=" * 77)
        install_prefix: Path = args.install_root / platform.name
        install_roots.append(install_prefix)
        build_dir: Path = args.build_root / platform.name
        if not args.dry_run:
            build_dir.mkdir(parents=True)

        configure_args = [
            "cmake",
            # See: https://cmake.org/cmake/help/latest/generator/Xcode.html
            "-G",
            args.generator,
            # Install each to their own directory to combine together at the end.
            f"-DCMAKE_INSTALL_PREFIX:STRING={str(install_prefix)}",
            # Platform specific arguments.
            *platform.configure_args(),
            # Static libraries are required for creating an xcframework.
            "-DBUILD_SHARED_LIBS:BOOL=OFF",
            # VTK specific build arguments.
            # https://gitlab.kitware.com/vtk/vtk/-/blob/master/Documentation/docs/build_instructions/build_settings.md
            "-DVTK_BUILD_TESTING:BOOL=OFF",
            "-DVTK_BUILD_EXAMPLES:BOOL=OFF",
            "-DVTK_ENABLE_KITS:BOOL=OFF",
            "-DVTK_ENABLE_WRAPPING:BOOL=OFF",
            "-DVTK_FRAMEWORK_BUILD:BOOL=ON",
            # Do not prefer external libraries.
            "-DVTK_USE_EXTERNAL:BOOL=OFF",
            "-DVTK_BUILD_ALL_MODULES:STRING=NO",
            "-DVTK_ENABLE_REMOTE_MODULES:BOOL=ON",
            "-DVTK_FORBID_DOWNLOADS:BOOL=ON",
            "-Werror=dev",
        ]
        # The toolchain file, if provided.
        if args.toolchain is not None:
            configure_args += ["--toolchain", str(args.toolchain)]
        if args.debug:
            configure_args += [
                "-DVTK_DEBUG_MODULE:BOOL=ON",
                "-DVTK_DEBUG_MODULE_ALL:BOOL=ON",
            ]
        configure_args += [
            # Any user provided CMake configure arguments.
            *args.extra_cmake_args,
            # NOTE: -S {source} -B {build} must be the last two arguments.
            # CMake project source directory.
            "-S",
            str(this_file_dir.parent),
            # CMake build directory.
            "-B",
            str(build_dir),
        ]
        # Dump to the console the full set of configure arguments before running.
        print("==> Configuring:")
        for idx, arg in enumerate(configure_args):
            if idx == 0:
                prefix = ""
            else:
                prefix = "    "
            print(f"{prefix}{arg} \\")

        if not args.dry_run:
            subprocess.check_call(configure_args)

        build_args = [
            "cmake",
            "--build",
            str(build_dir),
            "--config",
            args.build_type,
            "--target",
            "install",
        ]
        all_build_args.append(build_args)

    # Now that each platform has successfully configured, build and install.
    for build_args in all_build_args:
        print(f"==> Building: {' '.join(build_args)}")
        if not args.dry_run:
            subprocess.check_call(build_args)

    # Each installation prefix will have a ${prefix}/vtk.framework to gather
    # and combine into an xcframework.
    frameworks = [f"{str(inst / 'vtk.framework')}" for inst in install_roots]
    xc_framework_args = [
        "xcodebuild",
        "-create-xcframework",
    ]
    for f in frameworks:
        if not Path(f).exists():
            print(f"SKIP: {str(f)} does not exist...")
            continue
        xc_framework_args += ["-framework", f]
    output_xcframework = args.install_root / "vtk.xcframework"
    xc_framework_args += ["-output", str(output_xcframework)]

    print(f"==> Removing '{str(output_xcframework)}'")
    if not args.dry_run:
        shutil.rmtree(output_xcframework, ignore_errors=True)

    print(f"==> Creating XC Framework: {' '.join(xc_framework_args)}")
    if not args.dry_run:
        subprocess.check_call(xc_framework_args)
        print(f"==> Success: {str(output_xcframework)}")


if __name__ == "__main__":
    main()
