package main

import (
	"context"
	"net/url"

	"google.golang.org/grpc"
)

type Server struct {
	balancer Balancer
	pipeline Pipeline
	actions  Actions
	contents Contents
	context  context.Context
}

func NewServer(ctx context.Context, rawBuildpotURLs []string, grpcServer *grpc.Server) (server *Server, err error) {
	buildpotURLs := make([]url.URL, 0, len(rawBuildpotURLs))
	for _, rawBuildpotURL := range rawBuildpotURLs {
		var buildpotURL *url.URL
		buildpotURL, err = url.Parse(rawBuildpotURL)
		if err != nil {
			return
		}
		buildpotURLs = append(buildpotURLs, *buildpotURL)
	}
	server = &Server{
		balancer: NewBalancer(buildpotURLs),
		pipeline: NewPipeline(),
		actions:  NewActions(),
		contents: NewContents(),
		context:  ctx,
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
