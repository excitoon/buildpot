package main

import (
	"context"
	"log"

	longrunning "cloud.google.com/go/longrunning/autogen/longrunningpb"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	status2 "google.golang.org/grpc/status"
	"google.golang.org/protobuf/types/known/emptypb"

	"github.com/google/uuid"
)

func (server *Server) GetOperation(ctx context.Context, request *longrunning.GetOperationRequest) (operation *longrunning.Operation, err error) {
	// TODO it waits for the job to finish, which is not the expected behavior
	log.Printf("Getting operation: %s", request.Name)

	jobID, err := uuid.Parse(request.Name)
	if err != nil {
		err = status2.Error(codes.NotFound, "job not found")
		return
	}

	job, err := server.pipeline.GetJob(jobID)
	if err != nil {
		err = status2.Error(codes.NotFound, "job not found")
		return
	}

	operation, err = server.waitAndGet(job)
	return
}

func (server *Server) CancelOperation(ctx context.Context, request *longrunning.CancelOperationRequest) (empty *emptypb.Empty, err error) {
	log.Printf("Cancelling operation: %s", request.Name)

	jobID, err := uuid.Parse(request.Name)
	if err != nil {
		err = status2.Error(codes.NotFound, "job not found")
		return
	}

	job, err := server.pipeline.GetJob(jobID)
	if err != nil {
		err = status2.Error(codes.NotFound, "job not found")
		return
	}
	job.Cancel()
	empty = &emptypb.Empty{}
	return
}

func (server *Server) DeleteOperation(ctx context.Context, request *longrunning.DeleteOperationRequest) (empty *emptypb.Empty, err error) {
	log.Printf("Deleting operation: %s", request.Name)

	jobID, err := uuid.Parse(request.Name)
	if err != nil {
		err = status2.Error(codes.NotFound, "job not found")
		return
	}

	err = server.pipeline.RemoveJob(jobID)
	if err != nil {
		err = status2.Error(codes.NotFound, "job not found")
		return
	}

	empty = &emptypb.Empty{}
	return
}

func (server *Server) ListOperations(ctx context.Context, request *longrunning.ListOperationsRequest) (*longrunning.ListOperationsResponse, error) {
	log.Printf("Listing operations")
	// We can't know job status without waiting for it to finish for now.
	return nil, status2.Error(codes.Unimplemented, "ListOperations is not implemented")
}

func (server *Server) WaitOperation(ctx context.Context, request *longrunning.WaitOperationRequest) (operation *longrunning.Operation, err error) {
	log.Printf("Waiting for operation: %s", request.Name)

	jobID, err := uuid.Parse(request.Name)
	if err != nil {
		err = status2.Error(codes.NotFound, "job not found")
		return
	}

	job, err := server.pipeline.GetJob(jobID)
	if err != nil {
		err = status2.Error(codes.NotFound, "job not found")
		return
	}

	operation, err = server.waitAndGet(job)
	return
}

func (server *Server) registerOperation(grpcServer *grpc.Server) {
	longrunning.RegisterOperationsServer(grpcServer, server)
}
