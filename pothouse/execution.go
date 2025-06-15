package main

import (
	"fmt"
	"log"

	"cloud.google.com/go/longrunning/autogen/longrunningpb"
	remoteexecution "github.com/bazelbuild/remote-apis/build/bazel/remote/execution/v2"
	"github.com/google/uuid"
	"google.golang.org/genproto/googleapis/rpc/status"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	status2 "google.golang.org/grpc/status"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/types/known/anypb"
)

func getExecutionError(jobID uuid.UUID, err error) *longrunningpb.Operation {
	return &longrunningpb.Operation{
		Name: jobID.String(),
		Done: true,
		Result: &longrunningpb.Operation_Error{
			Error: &status.Status{
				Code:    1,
				Message: err.Error(),
			},
		},
	}
}

func getExecutionResponse(jobID uuid.UUID, result string) (operation *longrunningpb.Operation, err error) {
	execResp := &remoteexecution.ExecuteResponse{
		Result: &remoteexecution.ActionResult{
			StdoutRaw: []byte(result),
		},
		Status: &status.Status{
			Code:    0,
			Message: "OK",
		},
	}
	anyResp, err := anypb.New(execResp)
	if err != nil {
		err = fmt.Errorf("failed to marshal ExecuteResponse to Any: %w", err)
		return
	}
	operation = &longrunningpb.Operation{
		Name: jobID.String(),
		Done: true,
		Result: &longrunningpb.Operation_Response{
			Response: anyResp,
		},
	}
	return
}

func (server *Server) waitAndGet(job *Job) (operation *longrunningpb.Operation, err error) {
	result, err := job.Wait()
	if err != nil {
		operation = getExecutionError(job.ID, err)
		err = nil
	} else {
		operation, err = getExecutionResponse(job.ID, result)
	}
	return
}

func (server *Server) waitAndSend(job *Job, stream remoteexecution.Execution_ExecuteServer) (err error) {
	operation, err := server.waitAndGet(job)
	if err != nil {
		return
	}
	err = stream.Send(operation)
	return
}

func (server *Server) Execute(
	request *remoteexecution.ExecuteRequest,
	stream remoteexecution.Execution_ExecuteServer,
) (err error) {
	log.Printf("Executing request: %v", request)

	actionDigest := request.ActionDigest
	if actionDigest == nil || actionDigest.Hash == "" {
		return fmt.Errorf("missing action digest")
	}
	actionBytes, ok := server.contents.Get(actionDigest.Hash)
	if !ok {
		return fmt.Errorf("action blob not found in CAS: %s", actionDigest.Hash)
	}
	a := remoteexecution.Action{}
	if err := proto.Unmarshal(actionBytes, &a); err != nil {
		return fmt.Errorf("failed to unmarshal action: %w", err)
	}

	action, err := server.getAction(&a)
	if err != nil {
		return fmt.Errorf("failed to get action: %w", err)
	}

	job := server.execute(action)
	stream.Send(&longrunningpb.Operation{
		Name: job.ID.String(),
		Done: false,
	})

	err = server.waitAndSend(job, stream)
	return
}

func (server *Server) WaitExecution(
	request *remoteexecution.WaitExecutionRequest,
	stream remoteexecution.Execution_WaitExecutionServer,
) (err error) {
	log.Printf("Waiting for execution: %s", request.Name)

	// Find the job by ID from the pipeline
	jobID, err := uuid.Parse(request.Name)
	if err != nil {
		return status2.Error(codes.NotFound, "job not found")
	}
	job, err := server.pipeline.GetJob(jobID)
	if err != nil {
		return status2.Error(codes.NotFound, "job not found")
	}

	err = server.waitAndSend(job, stream)
	return
}

func (server *Server) execute(action Action) (job *Job) {
	job = NewJob(server.context, action)
	server.pipeline.AddJob(server, job)
	return
}

func (server *Server) registerExecution(grpcServer *grpc.Server) {
	remoteexecution.RegisterExecutionServer(grpcServer, server)
}
