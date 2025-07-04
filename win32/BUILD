load("//:cl_binary.bzl", "cl_binary")
load("//:cl_template.bzl", "cl_template")

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

cl_template(
    name = "STD/VECTOR",
    template = "STD/VECTOR.CTT",
    hdrs = [
        "string",
        "utility",
        "vector",
        "http.h",
        "json.h",
    ],
    specs = {
        "StringVector": "vector<string>",
        "StringPairVector": "vector<StringPair>",
        "JSONVector": "vector<JSON>",
        "RouteVector": "vector<Route>",
        "StringJSONPairVector": "vector<StringJSONPair>",
    },
)

cl_template(
    name = "STD/UTILITY",
    template = "STD/UTILITY.CTT",
    hdrs = [
        "string",
        "utility",
        "common.h",
        "json.h",
    ],
    specs = {
        "StringPair": "pair<string, string>",
        "StringBoolPair": "pair<string, bool>",
        "StringJSONPair": "pair<string, JSON>",
    },
)

cl_template(
    name = "STD/MEMORY",
    template = "STD/MEMORY.CTT",
    hdrs = [
        "memory",
        "string",
    ],
    specs = {
        "StringSharedPtr": "shared_ptr<string>",
    },
)

cl_binary(
    name = "buildpot",
    srcs = [
        "AUTH.CPP",
        "AUTH.H",
        "BASE64.CPP",
        "BASE64.H",
        "COMMON.CPP",
        "COMMON.H",
        "CRC32.CPP",
        "CRC32.H",
        "DOWNLOAD.CPP",
        "DOWNLOAD.H",
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
        "UPGRADE.CPP",
        "UPGRADE.H",
        "UPLOAD.CPP",
        "UPLOAD.H",
        "UTILS.CPP",
        "UTILS.H",
        "STD/CCTYPE.CPP",
        "STD/CCTYPE",
        "STD/FSTREAM.CPP",
        "STD/FSTREAM",
        "STD/MEMORY.CPP",
        "STD/MEMORY",
        "STD/MUTEX.CPP",
        "STD/MUTEX",
        "STD/UTILITY.CPP",
        "STD/UTILITY",
        "STD/STDEXCEPT.CPP",
        "STD/STDEXCEPT",
        "STD/STRING.CPP",
        "STD/STRING",
        "STD/VECTOR.CPP",
        "STD/VECTOR",
    ],
    includes = [
        "STD",
        ".",
    ],
    defines = [
        "_X86_",
        "WIN32_LEAN_AND_MEAN",
        "STRICT",
        "_INC_CTYPE",
    ] + select({
        ":debug": [
            "_DEBUG",
        ],
        ":release": [
            "NDEBUG",
        ],
        "//conditions:default": [],
    }),
    copts = [
        "/nologo",
        "/W3",
        "/FR",
        "/MT",
        "/YX",
        "/FpBUILDPOT.PCH",
    ] + select({
        ":debug": [
            "/Z7",
            "/FdBUILDPOT.PDB",
        ],
        ":release": [
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
