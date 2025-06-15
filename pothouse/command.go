package main

import (
	"errors"

	remoteexecution "github.com/bazelbuild/remote-apis/build/bazel/remote/execution/v2"
)

type Command struct {
	Arguments             []string                                      `json:"arguments"`
	Environment           map[string]string                             `json:"environment"`
	OutputPaths           []string                                      `json:"output_paths"`
	WorkingDirectory      string                                        `json:"working_directory"`
	OutputNodeProperties  []string                                      `json:"output_node_properties"`
	OutputDirectoryFormat remoteexecution.Command_OutputDirectoryFormat `json:"output_directory_format"`
}

func (server *Server) getCommand(
	cmd *remoteexecution.Command,
) (c Command, err error) {
	c.Arguments = cmd.Arguments
	c.Environment = map[string]string{}
	for _, env := range cmd.EnvironmentVariables {
		if env.Name == "" || env.Value == "" {
			return c, errors.New("invalid command environment variable")
		}
		c.Environment[env.Name] = env.Value
	}
	c.OutputPaths = cmd.OutputPaths
	c.WorkingDirectory = cmd.WorkingDirectory
	c.OutputNodeProperties = []string{}
	if cmd.OutputNodeProperties != nil {
		c.OutputNodeProperties = append(c.OutputNodeProperties, cmd.OutputNodeProperties...)
	}
	c.OutputDirectoryFormat = cmd.OutputDirectoryFormat
	return c, nil
}
