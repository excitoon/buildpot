load("//:cl_binary.bzl", "cl_binary")

config_setting(
    name = "debug",
    values = {"compilation_mode": "dbg"},
)

config_setting(
    name = "release",
    values = {"compilation_mode": "opt"},
)

# TODO: implement libraries
# TODO: implement shared libraries

cl_binary(
    name = "buildpot",
    srcs = [
        "BASE64.CPP",
        "BASE64.H",
        "COMMON.CPP",
        "COMMON.H",
        "CRC32.CPP",
        "CRC32.H",
        "EXECUTER.CPP",
        "EXECUTER.H",
        "HTTP.CPP",
        "HTTP.H",
        "JSON.CPP",
        "JSON.H",
        "LOGGER.CPP",
        "LOGGER.H",
        "MAIN.CPP",
        "SERVICE.CPP",
        "SERVICE.H",
        "SHA2.CPP",
        "SHA2.H",
        "UNISTD.CPP",
        "UNISTD.H",
        "UTILS.CPP",
        "UTILS.H",
        "STD/MUTEX.CPP",
        "STD/MUTEX",
        "STD/STRING.CPP",
        "STD/STRING",
    ],
    includes = [
        "STD",
        ".",
    ],
    defines = [
        "_X86_",
        "WIN32_LEAN_AND_MEAN",
        "STRICT",
    ],
    copts = [
        "/nologo",
        "/W3",
        "/FR",
        "/MT",
        "/YX",
        "/FpBUILDPOT.PCH",
    ] + select({
        ":debug": [
            "/D_DEBUG",
            "/Z7",
            "/FdBUILDPOT.PDB",
        ],
        ":release": [
            "/DNDEBUG",
            "/O2",
        ],
        "//conditions:default": [],
    }),
    linkopts = [
        "/NOLOGO",
        "/SUBSYSTEM:console",
        "advapi32.lib",
        "wsock32.lib",
    ] + select({
        ":debug": [
            "/DEBUG",
            "/DEBUGTYPE:cv",
        ],
        ":release": [
        ],
        "//conditions:default": [],
    }),
)
