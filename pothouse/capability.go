package main

import (
	"context"

	remoteexecution "github.com/bazelbuild/remote-apis/build/bazel/remote/execution/v2"
	"github.com/bazelbuild/remote-apis/build/bazel/semver"
	"google.golang.org/grpc"
)

func (server *Server) GetCapabilities(
	ctx context.Context,
	req *remoteexecution.GetCapabilitiesRequest,
) (*remoteexecution.ServerCapabilities, error) {
	return &remoteexecution.ServerCapabilities{
		ExecutionCapabilities: &remoteexecution.ExecutionCapabilities{
			ExecEnabled:    true,
			DigestFunction: remoteexecution.DigestFunction_SHA256,
		},
		CacheCapabilities: &remoteexecution.CacheCapabilities{
			DigestFunctions: []remoteexecution.DigestFunction_Value{
				remoteexecution.DigestFunction_SHA256,
			},
			ActionCacheUpdateCapabilities: &remoteexecution.ActionCacheUpdateCapabilities{
				UpdateEnabled: true,
			},
			CachePriorityCapabilities:   nil,
			MaxBatchTotalSizeBytes:      0,
			SymlinkAbsolutePathStrategy: remoteexecution.SymlinkAbsolutePathStrategy_DISALLOWED,
		},
		LowApiVersion: &semver.SemVer{
			Major: 2,
			Minor: 2,
			Patch: 0,
		},
		HighApiVersion: &semver.SemVer{
			Major: 2,
			Minor: 2,
			Patch: 0,
		},
	}, nil
}

func (server *Server) registerCapability(grpcServer *grpc.Server) {
	remoteexecution.RegisterCapabilitiesServer(grpcServer, server)
}
