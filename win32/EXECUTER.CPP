#include <executer.h>

#include <fstream>
#include <mutex>
#include <string>
#include <utility>

#include <stdlib.h>
#include <unistd.h>

#include <windows.h>

#include <base64.h>
#include <common.h>
#include <json.h>
#include <logger.h>
#include <http.h>
#include <service.h>
#include <utils.h>


mutex build_mutex;

bool create_tree(const JSON & root, const string & path)
{
    if (!isdir(path) && !makedirs(path, true))
    {
        Log(Executer, Error, "Failed to create directory `%s`. GetLastError() -> %d.", path.c_str(), GetLastError());
        return false;
    }
    if (root["files"].type() != JSONObject || root["directories"].type() != JSONObject || root["symlinks"].type() != JSONObject)
    {
        Log(Executer, Error, "Invalid JSON structure: `files`, `directories` and `symlinks` must be objects.");
        return false;
    }
    if (root["symlinks"].size() > 0)
    {
        Log(Executer, Error, "Symlinks are not supported in Windows yet.");
        return false;
    }

    Log(Executer, Trace, "Creating %d files and %d directories in `%s`.", root["files"].size(), root["directories"].size(), path.c_str());
    JSON file_names = root["files"].keys();
    for (size_t i = 0; i < file_names.size(); ++i)
    {
        const JSON & name = file_names[i];
        const JSON & file = root["files"][name.string_value()];
        StringBoolPair decoded = base64_decode(file["data"].string_value());
        if (decoded.second)
        {
            Log(Executer, Error, "Failed to decode base64 data for file `%s`.", name.string_value().c_str());
            return false;
        }
        string file_path = path_join(path, name.string_value());
        if (isfile(file_path))
        {
            Log(Executer, Error, "File `%s` already exists.", file_path.c_str());
            return false;
        }
        fstream f(file_path.c_str(), ios_base::out | ios_base::binary);
        if (!f.is_open())
        {
            Log(Executer, Error, "Failed to open `%s` for writing. GetLastError() -> %d.", file_path.c_str(), GetLastError());
            return false;
        }
        f.write(decoded.first.data(), decoded.first.size());
        if (!f)
        {
            Log(Executer, Error, "Failed to write to `%s`. GetLastError() -> %d.", file_path.c_str(), GetLastError());
            return false;
        }
    }

    JSON directory_names = root["directories"].keys();
    for (i = 0; i < directory_names.size(); ++i)
    {
        const JSON & name = directory_names[i];
        const JSON & directory = root["directories"][name.string_value()];
        string sub_path = path_join(path, name.string_value());
        if (!create_tree(directory, sub_path))
        {
            Log(Executer, Error, "Failed to create tree for directory `%s`.", sub_path.c_str());
            return false;
        }
    }
    return true;
}

string fix_separator(const string & path)
{
    return replace(path, "/", "\\");
}

bool ExecuteHandler(BaseConnection & connection, const Request & request, void * arg)
{
    const string * content_type = request.headers.get("Content-Type");
    if (content_type == nullptr || *content_type != "application/json")
    {
        return connection.send_response(HTTP_415_UnsupportedMediaType, "Unsupported Media Type\n", "text/plain; charset=utf-8");
    }

    StringBoolPair body = connection.read_body(request);
    if (body.second)
    {
        return false;
    }

    Log(Executer, Trace, "Received body: %d bytes.", body.first.size());
    JSON json = JSON::from_string(body.first);
    if (json.type() != JSONObject)
    {
        Log(Executer, Error, "Invalid JSON.");
        return connection.send_response(HTTP_400_BadRequest, "Bad Request\n", "text/plain; charset=utf-8");
    }

    if (json["input_root"].type() != JSONObject)
    {
        Log(Executer, Error, "Invalid JSON structure: `input_root` must be an object.");
        return connection.send_response(HTTP_400_BadRequest, "Bad Request\n", "text/plain; charset=utf-8");
    }
    if (json["command"].type() != JSONObject)
    {
        Log(Executer, Error, "Invalid JSON structure: `command` must be an object.");
        return connection.send_response(HTTP_400_BadRequest, "Bad Request\n", "text/plain; charset=utf-8");
    }

    const JSON & action_environment = json["command"]["environment"];
    const JSON & action_arguments = json["command"]["arguments"];
    const JSON & action_working_directory = json["command"]["working_directory"];
    const JSON & action_output_paths = json["command"]["output_paths"];

    if (action_environment.type() != JSONObject)
    {
        Log(Executer, Error, "Invalid JSON structure: `environment` must be an object.");
        return connection.send_response(HTTP_400_BadRequest, "Bad Request\n", "text/plain; charset=utf-8");
    }
    if (action_arguments.type() != JSONArray)
    {
        Log(Executer, Error, "Invalid JSON structure: `arguments` must be an array.");
        return connection.send_response(HTTP_400_BadRequest, "Bad Request\n", "text/plain; charset=utf-8");
    }
    if (action_working_directory.type() != JSONString)
    {
        Log(Executer, Error, "Invalid JSON structure: `working_directory` must be a string.");
        return connection.send_response(HTTP_400_BadRequest, "Bad Request\n", "text/plain; charset=utf-8");
    }
    if (action_output_paths.type() != JSONArray)
    {
        Log(Executer, Error, "Invalid JSON structure: `output_paths` must be an array.");
        return connection.send_response(HTTP_400_BadRequest, "Bad Request\n", "text/plain; charset=utf-8");
    }
    for (size_t i = 0; i < action_arguments.size(); ++i)
    {
        if (action_arguments[i].type() != JSONString)
        {
            Log(Executer, Error, "Invalid JSON structure: `arguments` must be an array of strings.");
            return connection.send_response(HTTP_400_BadRequest, "Bad Request\n", "text/plain; charset=utf-8");
        }
    }
    for (i = 0; i < action_output_paths.size(); ++i)
    {
        if (action_output_paths[i].type() != JSONString)
        {
            Log(Executer, Error, "Invalid JSON structure: `output_paths` must be an array of strings.");
            return connection.send_response(HTTP_400_BadRequest, "Bad Request\n", "text/plain; charset=utf-8");
        }
    }
    JSON action_environment_keys = action_environment.keys();
    for (i = 0; i < action_environment.size(); ++i)
    {
        const JSON & key = action_environment_keys[i];
        if (key.type() != JSONString || action_environment[key.string_value()].type() != JSONString)
        {
            Log(Executer, Error, "Invalid JSON structure: `environment` must be an object with string values.");
            return connection.send_response(HTTP_400_BadRequest, "Bad Request\n", "text/plain; charset=utf-8");
        }
    }

    StringVector parts = path_split(get_executable_path());
    if (parts.size() < 2 || parts[0].size() < 2 || parts[0][1] != ':')
    {
        Log(Executer, Error, "Executable path `%s` is not a valid Windows path.", get_executable_path().c_str());
        return connection.send_response(HTTP_500_InternalServerError, "Internal Server Error\n", "text/plain; charset=utf-8");
    }

    string build_root = parts[0] + "\\B";
    const string * build_root_header = request.headers.get("X-Build-Root");
    if (build_root_header != nullptr)
    {
        build_root = fix_separator(*build_root_header);
    }

    build_mutex.lock();

    Log(Executer, Trace, "Build root is `%s`.", build_root.c_str());
    if (isdir(build_root) && !rmtree(build_root))
    {
        Log(Executer, Error, "Failed to remove build root `%s`. GetLastError() -> %d.", build_root.c_str(), GetLastError());
        build_mutex.unlock();
        return connection.send_response(HTTP_500_InternalServerError, "Internal Server Error\n", "text/plain; charset=utf-8");
    }

    if (!create_tree(json["input_root"], build_root))
    {
        Log(Executer, Error, "Failed to create tree in `%s`.", build_root.c_str());
        rmtree(build_root);
        build_mutex.unlock();
        return connection.send_response(HTTP_500_InternalServerError, "Internal Server Error\n", "text/plain; charset=utf-8");
    }

    StringVector arguments;
    for (i = 0; i < action_arguments.size(); ++i)
    {
        arguments.push_back(action_arguments[i].string_value());
    }

    StringVector output_paths;
    for (i = 0; i < action_output_paths.size(); ++i)
    {
        string output_path = path_join(build_root, fix_separator(action_output_paths[i].string_value()));
        makedirs(dirname(output_path), true);
        output_paths.push_back(output_path);
    }

    string working_directory = path_join(build_root, fix_separator(action_working_directory.string_value()));

    string command_line;
    for (i = 0; i < arguments.size(); ++i)
    {
        if (i)
        {
            command_line += " ";
        }
        string escaped = replace(arguments[i], "\"", "\"\"");
        if (arguments[i].find_first_of(" \t\r\n\"") != string::npos)
        {
            command_line += "\"" + escaped + "\"";
        }
        else
        {
            command_line += escaped;
        }
    }

    const string * job_id_header = request.headers.get("X-Job-ID");
    if (job_id_header != nullptr)
    {
        Log(Executer, Info, "Job ID: `%s`.", job_id_header->c_str());
    }
    Log(Executer, Trace, "Executing action in `%s`.", build_root.c_str());
    Log(Executer, Trace, "Working directory: `%s`.", working_directory.c_str());
    Log(Executer, Trace, "Command line: `%s`.", command_line.c_str());
    Log(Executer, Trace, "Output paths: [`%s`].", join("`, `", output_paths).c_str());

    string environment;
    char * env_system_root = getenv("SystemRoot");
    if (env_system_root != nullptr)
    {
        environment += "SystemRoot=";
        environment += env_system_root;
        environment += '\0';
        Log(Executer, Trace, "Using SystemRoot: `%s`.", env_system_root);
    }
    char * env_com_spec = getenv("ComSpec");
    if (env_com_spec != nullptr)
    {
        environment += "ComSpec=";
        environment += env_com_spec;
        environment += '\0';
        Log(Executer, Trace, "Using ComSpec: `%s`.", env_com_spec);
    }
    char * env_temp = getenv("temp");
    char * env_tmp = getenv("tmp");
    string env_temp_path;
    string env_tmp_path;
    if (env_system_root != nullptr && (env_temp == nullptr || env_tmp == nullptr))
    {
        StringVector system_root_parts = path_split(env_system_root);
        env_temp_path = path_join(system_root_parts[0], "temp");
        env_tmp_path = path_join(system_root_parts[0], "temp");
    }
    if (env_temp != nullptr)
    {
        env_temp_path = env_temp;
    }
    if (env_tmp != nullptr)
    {
        env_tmp_path = env_tmp;
    }
    if (!env_temp_path.empty())
    {
        environment += "temp=";
        environment += env_temp_path;
        environment += '\0';
        Log(Executer, Trace, "Using temp: `%s`.", env_temp_path);
    }
    if (!env_tmp_path.empty())
    {
        environment += "tmp=";
        environment += env_tmp_path;
        environment += '\0';
        Log(Executer, Trace, "Using tmp: `%s`.", env_tmp_path);
    }
    for (i = 0; i < action_environment_keys.size(); ++i)
    {
        const string & key = action_environment_keys[i].string_value();
        const string & value = action_environment[key].string_value();
        environment += key + "=" + value + '\0';
        Log(Executer, Trace, "export %s=%s", key.c_str(), value.c_str());
    }
    if (action_environment.size() == 0)
    {
        char * env_path = getenv("Path");
        if (env_path != nullptr)
        {
            string default_path = env_path;
            environment += "Path=";
            environment += default_path;
            environment += '\0';
            Log(Executer, Trace, "Using default Path: `%s`.", default_path.c_str());
        }
        char * env_include = getenv("include");
        if (env_include != nullptr)
        {
            string default_include = env_include;
            environment += "include=";
            environment += default_include;
            environment += '\0';
            Log(Executer, Trace, "Using default INCLUDE path: `%s`.", default_include.c_str());
        }
        char * env_lib = getenv("lib");
        if (env_lib != nullptr)
        {
            string default_lib = env_lib;
            environment += "lib=";
            environment += default_lib;
            environment += '\0';
            Log(Executer, Trace, "Using default LIB path: `%s`.", default_lib.c_str());
        }
    }
    environment += '\0';

    SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES), nullptr, true };
    HANDLE hStdOutRead = nullptr, hStdOutWrite = nullptr;
    HANDLE hStdErrRead = nullptr, hStdErrWrite = nullptr;

    CreatePipe(&hStdOutRead, &hStdOutWrite, &saAttr, 0);
    CreatePipe(&hStdErrRead, &hStdErrWrite, &saAttr, 0);

    STARTUPINFO si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdErrWrite;
    si.hStdInput = nullptr;

    DWORD start_time = GetTickCount();

    bool success = CreateProcess(
        nullptr,                      // Application name.
        (char *)command_line.c_str(), // Command line.
        nullptr,                      // Process security attributes.
        nullptr,                      // Primary thread security attributes.
        true,                         // Set up pipes for stdout and stderr.
        0,                            // Creation flags.
        (void *)environment.c_str(),  // Use parent's environment.
        working_directory.c_str(),    // Working directory.
        &si,                          // Startup info.
        &pi                           // Process info.
    );
    CloseHandle(hStdOutWrite);
    CloseHandle(hStdErrWrite);

    string stdout_str, stderr_str;
    unsigned long exit_code = 0;

    // FIXME it does not work from a service!
    if (!success)
    {
        Log(Executer, Error, "Failed to create process. GetLastError() -> %d.", GetLastError());
        exit_code = 255;
    }
    else
    {
        char buffer[1024];
        unsigned long bytes_read;

        const string * job_timeout_header = request.headers.get("X-Job-Timeout");
        unsigned long timeout = INFINITE;
        if (job_timeout_header != nullptr)
        {
            char * end;
            unsigned long job_timeout = strtoul(job_timeout_header->c_str(), &end, 10);
            if (*end == '\0' && end != job_timeout_header->c_str())
            {
                timeout = job_timeout;
                Log(Executer, Info, "Job timeout: %d ms.", timeout);
            }
        }

        DWORD wait_result = WaitForSingleObject(pi.hProcess, timeout);
        if (wait_result == WAIT_TIMEOUT)
        {
            TerminateProcess(pi.hProcess, 1);
            WaitForSingleObject(pi.hProcess, INFINITE);
            Log(Executer, Error, "Process %d terminated.", pi.dwProcessId);
        }

        while (ReadFile(hStdOutRead, buffer, sizeof(buffer), &bytes_read, nullptr) && bytes_read > 0)
            stdout_str.append(buffer, bytes_read);

        while (ReadFile(hStdErrRead, buffer, sizeof(buffer), &bytes_read, nullptr) && bytes_read > 0)
            stderr_str.append(buffer, bytes_read);

        GetExitCodeProcess(pi.hProcess, &exit_code);

        CloseHandle(hStdOutRead);
        CloseHandle(hStdErrRead);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        Log(Executer, Trace, "Process exited with code %d in %d ms.", exit_code, GetTickCount() - start_time);
        if (!stdout_str.empty())
        {
            Log(Executer, Trace, "Process stdout:\n%s", stdout_str.c_str());
        }
        if (!stderr_str.empty())
        {
            Log(Executer, Trace, "Process stderr:\n%s", stderr_str.c_str());
        }
        if (stderr_str.empty() && stdout_str.empty())
        {
            Log(Executer, Trace, "Process produced no output.");
        }
    }

    StringVector existing_output_paths;
    StringVector missing_output_paths;
    for (i = 0; i < output_paths.size(); ++i)
    {
        const string & output_path = output_paths[i];
        if (isfile(output_path))
        {
            existing_output_paths.push_back(output_path);
        }
        else
        {
            missing_output_paths.push_back(output_path);
        }
    }
    if (existing_output_paths.size() > 0)
    {
        Log(Executer, Trace, "Created output files:");
        for (i = 0; i < existing_output_paths.size(); ++i)
        {
            if (i < existing_output_paths.size() - 1)
            {
                Log(Executer, Trace, "- `%s`;", existing_output_paths[i].c_str());
            }
            else
            {
                Log(Executer, Trace, "- `%s`.", existing_output_paths[i].c_str());
            }
        }
    }
    if (missing_output_paths.size() > 0)
    {
        Log(Executer, Trace, "Missing output files:");
        for (i = 0; i < missing_output_paths.size(); ++i)
        {
            if (i < missing_output_paths.size() - 1)
            {
                Log(Executer, Trace, "- `%s`;", missing_output_paths[i].c_str());
            }
            else
            {
                Log(Executer, Trace, "- `%s`.", missing_output_paths[i].c_str());
            }
        }
    }

    JSON response_body = JSON(0, nullptr, nullptr);
    response_body["files"] = JSON(0, nullptr, nullptr);
    for (i = 0; i < output_paths.size(); ++i)
    {
        if (!isfile(output_paths[i]))
        {
            continue;
        }
        const string & output_path = output_paths[i];
        const string & original_name = action_output_paths[i].string_value();
        response_body["files"][original_name] = JSON(0, nullptr, nullptr);
        JSON & file = response_body["files"][original_name];
        fstream f(output_path.c_str(), ios_base::in | ios_base::binary);
        if (!f.is_open())
        {
            Log(Executer, Error, "Failed to open output file `%s` for reading. GetLastError() -> %d.", output_path.c_str(), GetLastError());
            rmtree(build_root);
            build_mutex.unlock();
            return connection.send_response(HTTP_500_InternalServerError, "Internal Server Error\n", "text/plain; charset=utf-8");
        }
        string file_data;
        f.seekg(0, ios_base::end);
        unsigned long file_size = f.tellg();
        f.seekg(0, ios_base::beg);
        file_data.resize(file_size);
        f.read(file_data.data(), file_size);
        if (!f)
        {
            Log(Executer, Error, "Failed to read output file `%s`. GetLastError() -> %d.", output_path.c_str(), GetLastError());
            f.close();
            rmtree(build_root);
            build_mutex.unlock();
            return connection.send_response(HTTP_500_InternalServerError, "Internal Server Error\n", "text/plain; charset=utf-8");
        }
        file["data"] = base64_encode(file_data);
    }

    if (request.headers.get("X-Leave-Build-Root") == nullptr)
    {
        rmtree(build_root);
    }
    build_mutex.unlock();

    response_body["exit_code"] = (int)exit_code;
    response_body["stdout"] = base64_encode(stdout_str);
    response_body["stderr"] = base64_encode(stderr_str);

    return connection.send_response(HTTP_200_OK, response_body.to_string(), "application/json");
}
