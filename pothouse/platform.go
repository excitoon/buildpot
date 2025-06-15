package main

import (
	"errors"

	remoteexecution "github.com/bazelbuild/remote-apis/build/bazel/remote/execution/v2"
)

type Platform struct {
	Properties map[string]string `json:"properties"`
}

func (server *Server) getPlatform(
	platform *remoteexecution.Platform,
) (p Platform, err error) {
	p.Properties = map[string]string{}
	if platform == nil {
		return p, nil
	}
	if platform.Properties == nil {
		return p, nil
	}
	for _, prop := range platform.Properties {
		if prop.Name == "" || prop.Value == "" {
			return p, errors.New("invalid platform property")
		}
		p.Properties[prop.Name] = prop.Value
	}
	return p, nil
}
