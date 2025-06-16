# Buildpot

Teapot which can build. Buildpot is an implementation of [Bazel Remote Execution](https://bazel.build/remote/rbe) for Microsoft Windows NT 3.1.

<img src="examples/full.png" width="100%"/>

Buildpot can be built using Visual C++ 1.0 (for NT).

```bash
win32 % bazel build ...
INFO: Invocation ID: ec843c4d-875d-4b95-8802-6ea0e6e50a17
INFO: Analyzed target //:buildpot (5 packages loaded, 36 targets configured).
INFO: From Action UTILS.OBJ:
UTILS.CPP
INFO: From Action BASE64.OBJ:
BASE64.CPP
INFO: From Action STRING.OBJ:
STD\STRING.CPP
INFO: From Action EXECUTER.OBJ:
EXECUTER.CPP
INFO: From Action SERVICE.OBJ:
SERVICE.CPP
INFO: From Action COMMON.OBJ:
COMMON.CPP
INFO: From Action CRC32.OBJ:
CRC32.CPP
INFO: From Action HTTP.OBJ:
HTTP.CPP
INFO: From Action MUTEX.OBJ:
STD\MUTEX.CPP
INFO: From Action JSON.OBJ:
JSON.CPP
INFO: From Action LOGGER.OBJ:
LOGGER.CPP
INFO: From Action MAIN.OBJ:
MAIN.CPP
INFO: From Action SHA2.OBJ:
SHA2.CPP
INFO: From Action UNISTD.OBJ:
UNISTD.CPP
INFO: Found 1 target...
Target //:buildpot up-to-date:
  bazel-bin/buildpot.exe
INFO: Elapsed time: 5.420s, Critical Path: 2.97s
INFO: 20 processes: 5 internal, 15 remote.
INFO: Build completed successfully, 20 total actions
```

## Known issues

- Processes do not run from Windows service.
