package main

import (
	"context"
	"net/url"

	"google.golang.org/grpc"
)

type Server struct {
	buildpotURL url.URL
	pipeline    Pipeline
	actions     Actions
	contents    Contents
	context     context.Context
}

func NewServer(ctx context.Context, rawBuildpotURL string, grpcServer *grpc.Server) (server *Server, err error) {
	buildpotURL, err := url.Parse(rawBuildpotURL)
	if err != nil {
		return
	}
	server = &Server{
		buildpotURL: *buildpotURL,
		pipeline:    NewPipeline(),
		actions:     NewActions(),
		contents:    NewContents(),
		context:     ctx,
	}
	modules := []func(*grpc.Server){
		server.registerExecution,
		server.registerCapability,
		server.registerAction,
		server.registerContent,
		server.registerOperation,
		server.registerStream,
	}
	for _, module := range modules {
		module(grpcServer)
	}
	return
}
