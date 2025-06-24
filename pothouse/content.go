package main

import (
	"context"
	"log"
	"sync"

	remoteexecution "github.com/bazelbuild/remote-apis/build/bazel/remote/execution/v2"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

type Contents struct {
	mutex    sync.RWMutex
	contents map[string][]byte
}

func NewContents() Contents {
	return Contents{
		contents: make(map[string][]byte),
	}
}

func (contents *Contents) Get(hash string) ([]byte, bool) {
	contents.mutex.RLock()
	defer contents.mutex.RUnlock()
	data, ok := contents.contents[hash]
	return data, ok
}

func (contents *Contents) Set(hash string, data []byte) {
	contents.mutex.Lock()
	defer contents.mutex.Unlock()
	contents.contents[hash] = data
}

func (contents *Contents) Has(hash string) bool {
	contents.mutex.RLock()
	defer contents.mutex.RUnlock()
	_, ok := contents.contents[hash]
	return ok
}

func (server *Server) FindMissingBlobs(
	ctx context.Context,
	req *remoteexecution.FindMissingBlobsRequest,
) (*remoteexecution.FindMissingBlobsResponse, error) {
	missing := []*remoteexecution.Digest{}
	for _, d := range req.BlobDigests {
		if !server.contents.Has(d.Hash) {
			missing = append(missing, d)
		}
	}
	return &remoteexecution.FindMissingBlobsResponse{
		MissingBlobDigests: missing,
	}, nil
}

func (server *Server) BatchUpdateBlobs(
	ctx context.Context,
	req *remoteexecution.BatchUpdateBlobsRequest,
) (*remoteexecution.BatchUpdateBlobsResponse, error) {
	log.Printf("BatchUpdateBlobs: %v", req)
	responses := make([]*remoteexecution.BatchUpdateBlobsResponse_Response, len(req.Requests))
	for i, r := range req.Requests {
		if r.Digest == nil {
			responses[i] = &remoteexecution.BatchUpdateBlobsResponse_Response{
				Digest: r.Digest,
				Status: status.New(codes.InvalidArgument, "missing digest or data").Proto(),
			}
			continue
		}
		server.contents.Set(r.Digest.Hash, r.Data)
		responses[i] = &remoteexecution.BatchUpdateBlobsResponse_Response{
			Digest: r.Digest,
			Status: status.New(codes.OK, "ok").Proto(),
		}
	}
	return &remoteexecution.BatchUpdateBlobsResponse{Responses: responses}, nil
}

func (server *Server) BatchReadBlobs(
	ctx context.Context,
	req *remoteexecution.BatchReadBlobsRequest,
) (*remoteexecution.BatchReadBlobsResponse, error) {
	log.Printf("BatchReadBlobs: %v", req)
	responses := make([]*remoteexecution.BatchReadBlobsResponse_Response, len(req.Digests))
	for i, d := range req.Digests {
		data, ok := server.contents.Get(d.Hash)
		if ok {
			responses[i] = &remoteexecution.BatchReadBlobsResponse_Response{
				Digest: d,
				Data:   data,
				Status: status.New(codes.OK, "ok").Proto(),
			}
		} else {
			responses[i] = &remoteexecution.BatchReadBlobsResponse_Response{
				Digest: d,
				Status: status.New(codes.NotFound, "blob not found").Proto(),
			}
		}
	}
	return &remoteexecution.BatchReadBlobsResponse{Responses: responses}, nil
}

func (server *Server) GetTree(
	req *remoteexecution.GetTreeRequest,
	stream remoteexecution.ContentAddressableStorage_GetTreeServer,
) error {
	log.Printf("GetTree: %v", req)
	// Not implemented
	return status.Error(codes.Unimplemented, "GetTree not implemented")
}

func (server *Server) registerContent(grpcServer *grpc.Server) {
	remoteexecution.RegisterContentAddressableStorageServer(grpcServer, server)
}
