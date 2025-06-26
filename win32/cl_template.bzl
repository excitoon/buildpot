_HEADERS_EXTENSIONS = ("H", "HPP", "HXX", "INC", "h", "hpp", "hxx", "hh", "inc")

def _ext_lower(s):
    if "." not in s:
        return s
    else:
        name, ext = s.rsplit(".", 1)
        return name + "." + ext.lower()

def _append_basename(s, suffix):
    if "." not in s:
        return s + suffix
    else:
        name, ext = s.rsplit(".", 1)
        return name + suffix + "." + ext

def _cl_template_impl(ctx):
    header = ctx.outputs.header
    inline = ctx.outputs.inline
    implementation = ctx.outputs.implementation
    template = ctx.file.template
    specs = ctx.attr.specs

    include_guard_name = header.short_path.replace("/", "_").upper() + "_DEFINED"

    inlines = []
    headers = []
    implementations = []
    for i, (class_name, spec) in enumerate(specs.items()):
        header_name = _append_basename(header.basename, "@" + str(i))
        ext = header_name.rsplit(".", 1)[-1]
        if ext not in _HEADERS_EXTENSIONS:
            header_name += ".h"
        headers.append(ctx.actions.declare_file(header_name))
        inlines.append(ctx.actions.declare_file(_append_basename(inline.basename, "@" + str(i))))
        implementations.append(ctx.actions.declare_file(_append_basename(implementation.basename, "@" + str(i))))

        ctx.actions.run(
            outputs = [headers[-1], inlines[-1], implementations[-1]],
            inputs = [template],
            arguments = [
                spec + " " + class_name,
                _ext_lower(template.path).replace("/", "\\"),
                headers[-1].path.replace("/", "\\"),
                _ext_lower(inlines[-1].path).replace("/", "\\"),
                _ext_lower(implementations[-1].path).replace("/", "\\"),
            ],
            executable = "templdef",
        )

    ctx.actions.run(
        inputs = headers,
        outputs = [header],
        arguments = [
            "/C",
            "echo #ifndef {} >GUARDPREFIX 2>NUL && echo #define {} >>GUARDPREFIX 2>NUL && echo #endif // {} >GUARDSUFFIX 2>NUL && type {} >{} 2>NUL".format(
                include_guard_name,
                include_guard_name,
                include_guard_name,
                " ".join(['"GUARDPREFIX"'] + ['"' + h.path.replace("/", "\\").replace('"', '""') + '"' for h in headers] + ['"GUARDSUFFIX"']),
                '"' + header.path.replace("/", "\\").replace('"', '""') + '"',
            ),
        ],
        executable = "cmd",
    )

    ctx.actions.run(
        inputs = implementations,
        outputs = [implementation],
        arguments = ["/c", "type {} > {} 2>NUL".format(" ".join([i.path.replace("/", "\\") for i in implementations]), implementation.path)],
        executable = "cmd",
    )

    ctx.actions.run(
        inputs = inlines,
        outputs = [inline],
        arguments = ["/c", "type {} > {} 2>NUL".format(" ".join([i.path.replace("/", "\\") for i in inlines]), inline.path)],
        executable = "cmd",
    )

    return [DefaultInfo(files = depset([header, inline, implementation]))]

_cl_template = rule(
    implementation = _cl_template_impl,
    attrs = {
        "template": attr.label(allow_single_file = True, mandatory = True),
        "header": attr.output(mandatory = True),
        "inline": attr.output(mandatory = True),
        "implementation": attr.output(mandatory = True),
        "specs": attr.string_dict(mandatory = True),
    },
    executable = False,
)

def cl_template(name, template, specs, **kwargs):
    path_and_basename = name.rsplit("/", 1)
    basename = path_and_basename[-1]
    if len(path_and_basename) > 1:
        prefix = path_and_basename[0] + "/"
    else:
        prefix = ""
    if basename.isupper():
        inl_ext = ".INL"
        cpp_ext = ".CPP"
    else:
        inl_ext = ".inl"
        cpp_ext = ".cpp"

    name_with_ext = basename.rsplit(".", 1)
    if len(name_with_ext) > 1 and name_with_ext[1] in _HEADERS_EXTENSIONS:
        basename = name_with_ext[0]

    _cl_template(
        name = name + "_templ",
        template = template,
        specs = specs,
        header = name,
        inline = prefix + basename + inl_ext,
        implementation = prefix + basename + cpp_ext,
        **kwargs,
    )
