_HEADERS_EXTENSIONS = ("H", "HPP", "HXX", "INC", "h", "hpp", "hxx", "hh", "inc")

def _merge_command_line(lines):
    return " && ".join(lines)

def _ext_lower(s):
    if "." not in s:
        return s
    else:
        name, ext = s.rsplit(".", 1)
        return name + "." + ext.lower()

def _append_basename(s, suffix):
    if "." not in s.rsplit("/", 1)[-1]:
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
    hdrs = ctx.attr.hdrs

    include_guard_name = header.short_path.replace("-", "_").replace("/", "_").upper() + "_DEFINED"

    inlines = []
    headers = []
    implementations = []
    for i, (class_name, spec) in enumerate(specs.items()):
        header_name = _append_basename(header.path, "@" + str(i))
        ext = header_name.rsplit(".", 1)[-1]
        if ext not in _HEADERS_EXTENSIONS:
            header_name += ".h"
        headers.append(ctx.actions.declare_file(header_name))
        inlines.append(ctx.actions.declare_file(_append_basename(inline.path, "@" + str(i))))
        implementations.append(ctx.actions.declare_file(_append_basename(implementation.path, "@" + str(i))))

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

    header_prefix = ctx.actions.declare_file(_append_basename(header.path, "@p"))
    header_prefix_lines = [
        "#ifndef {}".format(include_guard_name),
        "#define {}".format(include_guard_name),
        "",
    ]
    ctx.actions.write(header_prefix, "\r\n".join(header_prefix_lines))

    header_suffix = ctx.actions.declare_file(_append_basename(header.path, "@s"))
    header_suffix_lines = [
        "#endif // {}".format(include_guard_name),
        "",
    ]
    ctx.actions.write(header_suffix, "\r\n".join(header_suffix_lines))

    implementation_prefix = ctx.actions.declare_file(_append_basename(implementation.path, "@p"))
    implementation_prefix_lines = ["#include <{}>".format(hdr) for hdr in hdrs] + [
        "",
    ]
    ctx.actions.write(implementation_prefix, "\r\n".join(implementation_prefix_lines))

    # TODO: utilize `execution_requirements` and run locally what we can.

    ctx.actions.run(
        inputs = headers + [header_prefix, header_suffix],
        outputs = [header],
        arguments = [
            "/C",
            _merge_command_line([
                'type "{}" "{}" "{}" >"{}" 2>NUL'.format(
                    header_prefix.path.replace("/", "\\").replace('"', '""'),
                    '" "'.join([h.path.replace("/", "\\").replace('"', '""') for h in headers]),
                    header_suffix.path.replace("/", "\\").replace('"', '""'),
                    header.path.replace("/", "\\").replace('"', '""'),
                ),
            ]),
        ],
        executable = "cmd",
    )

    ctx.actions.run(
        inputs = inlines,
        outputs = [inline],
        arguments = [
            "/C",
            _merge_command_line([
                'type "{}" >"{}" 2>NUL'.format(
                    '" "'.join([i.path.replace("/", "\\").replace('"', '""') for i in inlines]),
                    inline.path.replace("/", "\\").replace('"', '""'),
                ),
            ]),
        ],
        executable = "cmd",
    )

    ctx.actions.run(
        inputs = implementations + [implementation_prefix],
        outputs = [implementation],
        arguments = [
            "/C",
            _merge_command_line([
                'type "{}" "{}" >"{}" 2>NUL'.format(
                    implementation_prefix.path.replace("/", "\\").replace('"', '""'),
                    '" "'.join([i.path.replace("/", "\\").replace('"', '""') for i in implementations]),
                    implementation.path.replace("/", "\\").replace('"', '""'),
                ),
            ]),
        ],
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
        "hdrs": attr.string_list(mandatory = True),
        "specs": attr.string_dict(mandatory = True),
    },
    executable = False,
)

def cl_template(name, template, hdrs=None, impl_hdrs=None, specs={}, **kwargs):
    hdrs = hdrs or []

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
        hdrs = hdrs,
        specs = specs,
        header = name,
        inline = prefix + basename + inl_ext,
        implementation = prefix + basename + cpp_ext,
        **kwargs,
    )
