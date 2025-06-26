def _path_join(lhs, rhs):
    if lhs in ("", ".", "./"):
        return rhs
    elif rhs in ("", ".", "./"):
        return lhs
    else:
        return lhs + "/" + rhs

def _cl_binary_impl(ctx):
    src_targets = ctx.attr.srcs
    src_files = ctx.files.srcs
    copts = ctx.attr.copts if hasattr(ctx.attr, "copts") else []
    rcopts = ctx.attr.rcopts if hasattr(ctx.attr, "rcopts") else []
    linkopts = ctx.attr.linkopts if hasattr(ctx.attr, "linkopts") else []
    defines = ctx.attr.defines if hasattr(ctx.attr, "defines") else []
    includes = ctx.attr.includes if hasattr(ctx.attr, "includes") else []
    extended_includes = list(includes)
    for include in includes:
        extended_includes.append(_path_join(ctx.bin_dir.path, include))
    define_flags = ["/D" + d for d in defines]
    include_flags = ["/I" + include.replace("/", "\\") for include in extended_includes]
    rc_define_flags = ["-d" + d for d in defines]
    rc_include_flags = ["-i" + include.replace("/", "\\") for include in extended_includes]

    cpp_exts = (".c", ".cc", ".cpp", ".cxx", ".C", ".CC", ".CPP", ".CXX")
    rc_exts = (".rc", ".RC")

    cpp_files = []
    header_files = []
    rc_files = []

    for f in src_files:
        if f.basename.endswith(cpp_exts):
            cpp_files.append(f)
        elif f.basename.endswith(rc_exts):
            rc_files.append(f)
        else:
            header_files.append(f)

    objects = []
    for src_file in cpp_files:
        if src_file.basename.isupper():
            ext = ".OBJ"
        else:
            ext = ".obj"
        obj = ctx.actions.declare_file(src_file.basename.rsplit(".", 1)[0] + ext)
        ctx.actions.run(
            outputs = [obj],
            inputs = [src_file] + header_files,
            arguments = define_flags + include_flags + copts + [
                "/c", src_file.path.replace("/", "\\"),
                "/Fo" + obj.path.replace("/", "\\"),
            ],
            executable = "cl",
        )
        objects.append(obj)

    for src_file in rc_files:
        if src_file.basename.isupper():
            ext = ".RES"
        else:
            ext = ".res"
        obj = ctx.actions.declare_file(src_file.basename.rsplit(".", 1)[0] + ext)
        ctx.actions.run(
            outputs = [obj],
            inputs = [src_file] + header_files,
            arguments = rc_define_flags + rc_include_flags + rcopts + [
                "-r", "-fo" + obj.path.replace("/", "\\"),
                src_file.path.replace("/", "\\"),
            ],
            executable = "rc",
        )
        objects.append(obj)

    if ctx.label.name.isupper():
        crf_ext = ".CRF"
        exe_ext = ".EXE"
        lib_ext = ".LIB"
    else:
        crf_ext = ".crf"
        exe_ext = ".exe"
        lib_ext = ".lib"

    crf_file = ctx.actions.declare_file(ctx.label.name + crf_ext)
    output = ctx.actions.declare_file(ctx.label.name + exe_ext)
    map_file_option = []
    def_file_option = []

    arguments = ['"' + obj.path.replace("/", "\\").replace('"', '""') + '"' for obj in objects] + [
        '-OUT:"' + output.path.replace("/", "\\").replace('"', '""') + '"',
    ] + map_file_option + def_file_option + [
        '-implib:"' + (ctx.label.name + lib_ext).replace("/", "\\").replace('"', '""') + '"',
    ]
    crf_content = "\n".join(arguments) + "\n"

    ctx.actions.write(
        output = crf_file,
        content = crf_content,
    )

    ctx.actions.run(
        outputs = [output],
        inputs = objects + [crf_file],
        arguments = linkopts + [
            "@" + crf_file.path.replace("/", "\\"),
        ],
        executable = "link",
    )

    return [DefaultInfo(files = depset([output]), executable = output)]

cl_binary = rule(
    implementation = _cl_binary_impl,
    attrs = {
        "srcs": attr.label_list(
            allow_files = True,
        ),
        "copts": attr.string_list(default = []),
        "rcopts": attr.string_list(default = []),
        "linkopts": attr.string_list(default = []),
        "defines": attr.string_list(default = []),
        "includes": attr.string_list(default = []),
    },
    executable = True,
)
