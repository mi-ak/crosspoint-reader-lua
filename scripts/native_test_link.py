"""
PlatformIO pre: extra script for native_test environment.

PlatformIO 6.x does not compile src/ files for test binaries automatically.
env.BuildSources() is the correct SCons/PlatformIO 6 API: it creates
compilation nodes AND adds the resulting objects to PIOBUILDFILES so the
linker picks them up without manual LINKFLAGS manipulation.
"""
Import("env")  # noqa: F821  (SCons provides this built-in)
import os

if env.get("PIOENV") != "native_test":
    Return()  # noqa: F821

# BuildSources() uses build_flags but NOT lib_deps include paths.
# Add ArduinoJson include dir explicitly so JsonSettingsIO.cpp can find it.
libdeps_dir = env.subst("$PROJECT_LIBDEPS_DIR")
pioenv = env.subst("$PIOENV")
arduino_json_inc = os.path.join(libdeps_dir, pioenv, "ArduinoJson", "src")
if os.path.isdir(arduino_json_inc):
    env.Append(CPPPATH=[arduino_json_inc])

# BuildSources compiles the specified src/ files and appends their .o nodes
# to PIOBUILDFILES, which PlatformIO links into the final test binary.
env.BuildSources(
    os.path.join("$BUILD_DIR", "src"),  # variant output dir for .o files
    "$PROJECT_SRC_DIR",                  # source root (src/)
    "-<*> +<CrossPointState.cpp> +<CrossPointSettings.cpp> +<JsonSettingsIO.cpp>",
)
