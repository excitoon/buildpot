def _cl_binary_impl(ctx):
    src_targets = ctx.attr.srcs
    src_files = ctx.files.srcs
    copts = ctx.attr.copts if hasattr(ctx.attr, "copts") else []
    linkopts = ctx.attr.linkopts if hasattr(ctx.attr, "linkopts") else []
    defines = ctx.attr.defines if hasattr(ctx.attr, "defines") else []
    includes = ctx.attr.includes if hasattr(ctx.attr, "includes") else []
    define_flags = ["/D" + d for d in defines]
    include_flags = ["/I" + include.replace("/", "\\") for include in includes]

    cpp_exts = (".c", ".cc", ".cpp", ".cxx", ".C", ".CC", ".CPP", ".CXX")

    cpp_files = []
    header_files = []

    for f in src_files:
        if f.basename.endswith(cpp_exts):
            cpp_files.append(f)
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
                '/Fo' + obj.path.replace("/", "\\"),
            ],
            executable = "cl",
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
        "-OUT:" + output.path.replace("/", "\\"),
    ] + map_file_option + def_file_option + [
        "-implib:" + (ctx.label.name + lib_ext).replace("/", "\\"),
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
        "linkopts": attr.string_list(default = []),
        "defines": attr.string_list(default = []),
        "includes": attr.string_list(default = []),
    },
    executable = True,
)
