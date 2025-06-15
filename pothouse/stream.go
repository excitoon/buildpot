package main

import (
	"context"
	"io"
	"log"
	"strings"

	bytestream "google.golang.org/genproto/googleapis/bytestream"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

func (server *Server) Write(stream bytestream.ByteStream_WriteServer) error {
	var resourceName string
	var data []byte

	for {
		req, err := stream.Recv()
		if err == io.EOF {
			break
		}
		if err != nil {
			return err
		}
		if resourceName == "" {
			resourceName = req.ResourceName
		}
		data = append(data, req.Data...)
	}

	log.Printf("Write stream started: %s, size: %d bytes", resourceName, len(data))

	// Parse digest hash from resourceName: "blobs/{hash}/{size}"
	parts := strings.Split(resourceName, "/")
	var hash string
	for i := 0; i < len(parts)-2; i++ {
		if parts[i] == "blobs" {
			hash = parts[i+1]
			break
		}
	}
	if hash == "" {
		return status.Error(codes.InvalidArgument, "invalid resource name")
	}

	server.contents.Set(hash, data)

	return stream.SendAndClose(&bytestream.WriteResponse{
		CommittedSize: int64(len(data)),
	})
}

func (server *Server) Read(req *bytestream.ReadRequest, stream bytestream.ByteStream_ReadServer) error {
	log.Printf("Read: %v", req)
	// Parse digest hash from resourceName: works for both "blobs/<hash>/<size>" and "uploads/<uuid>/blobs/<hash>/<size>"
	parts := strings.Split(req.ResourceName, "/")
	var hash string
	for i := 0; i < len(parts)-2; i++ {
		if parts[i] == "blobs" {
			hash = parts[i+1]
			break
		}
	}
	if hash == "" {
		return status.Error(codes.InvalidArgument, "invalid resource name")
	}

	data, ok := server.contents.Get(hash)
	if !ok {
		return status.Error(codes.NotFound, "blob not found")
	}

	const chunkSize = 2 * 1024 * 1024
	for offset := int64(0); offset < int64(len(data)); offset += chunkSize {
		end := offset + chunkSize
		if end > int64(len(data)) {
			end = int64(len(data))
		}
		if err := stream.Send(&bytestream.ReadResponse{
			Data: data[offset:end],
		}); err != nil {
			return err
		}
	}
	return nil
}

func (server *Server) QueryWriteStatus(
	ctx context.Context,
	req *bytestream.QueryWriteStatusRequest,
) (*bytestream.QueryWriteStatusResponse, error) {
	log.Printf("QueryWriteStatus: %v", req)
	// For in-memory, always report "complete" with zero committed size.
	return &bytestream.QueryWriteStatusResponse{
		CommittedSize: 0,
		Complete:      true,
	}, nil
}

func (server *Server) registerStream(grpcServer *grpc.Server) {
	bytestream.RegisterByteStreamServer(grpcServer, server)
}
