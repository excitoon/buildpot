#include <executer.h>

#include <mutex>

#include <string.h>
#include <stdio.h>
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
        StringWithError decoded = base64_decode(file["data"].string_value());
        if (!decoded)
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
        FILE * fp = fopen(file_path.c_str(), "wb");
        if (fp == nullptr)
        {
            Log(Executer, Error, "Failed to open `%s` for writing. GetLastError() -> %d.", file_path.c_str(), GetLastError());
            return false;
        }
        if (fwrite(decoded->data(), 1, decoded->size(), fp) != decoded->size())
        {
            Log(Executer, Error, "Failed to write to `%s`. GetLastError() -> %d.", file_path.c_str(), GetLastError());
            fclose(fp);
            return false;
        }
        fclose(fp);
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
    const NameAndValue * content_type = request.headers.get("Content-Type");
    if (content_type == nullptr || content_type->value != "application/json")
    {
        return connection.send_response(HTTP_415_UnsupportedMediaType, "Unsupported Media Type\n", "text/plain; charset=utf-8");
    }

    StringWithError body = connection.read_body(request);
    if (!body)
    {
        return false;
    }

    Log(Executer, Trace, "Received body: %d bytes.", body->size());
    JSON json = JSON::from_string(*body);
    if (json.type() != JSONObject)
    {
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

    Log(Executer, Trace, "Executing build in `%s`.", build_root.c_str());
    Log(Executer, Trace, "Working directory: `%s`.", working_directory.c_str());
    Log(Executer, Trace, "Arguments: [`%s`].", join("`, `", arguments).c_str());
    Log(Executer, Trace, "Output paths: [`%s`].", join("`, `", output_paths).c_str());

    sleep(10);

    rmtree(build_root);

    build_mutex.unlock();

    JSON response_body = JSON(0, nullptr, nullptr);
    return connection.send_response(HTTP_419_ImABuildpot, response_body.to_string() + "\n", "application/json");
}
