#include <executer.h>

#include <string.h>

#include <unistd.h>

#include <common.h>
#include <json.h>
#include <logger.h>
#include <http.h>


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

    JSON json = JSON::from_string(*body);
    if (json.type() == JSONError)
    {
        return connection.send_response(HTTP_400_BadRequest, "Bad Request\n", "text/plain; charset=utf-8");
    }

    sleep(10);

    JSON response_body = JSON(0, nullptr, nullptr);

    return connection.send_response(HTTP_419_ImABuildpot, response_body.to_string() + "\n", "application/json");
}
