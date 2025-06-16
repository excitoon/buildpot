package main

import (
	"context"
	"errors"
	"fmt"
	"log"
	"sync"
	"time"

	remoteexecution "github.com/bazelbuild/remote-apis/build/bazel/remote/execution/v2"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/proto"
)

type Action struct {
	Command    Command        `json:"command"`
	InputRoot  Directory      `json:"input_root"`
	Timeout    *time.Duration `json:"timeout,omitempty"`
	DoNotCache bool           `json:"-"`
	Salt       []byte         `json:"-"`
	Platform   Platform       `json:"-"`
}

type ActionResult struct {
	Files    map[string]File `json:"files"`
	ExitCode int32           `json:"exit_code"`
	Stdout   []byte          `json:"stdout"`
	Stderr   []byte          `json:"stderr"`
}

type Actions struct {
	mutex   sync.RWMutex
	actions map[string]*remoteexecution.ActionResult
}

func NewActions() Actions {
	return Actions{
		actions: make(map[string]*remoteexecution.ActionResult),
	}
}

func (server *Server) getAction(action *remoteexecution.Action) (result Action, err error) {
	if action == nil {
		err = errors.New("action is nil")
		return
	}

	commandDigest := action.CommandDigest
	if commandDigest == nil || commandDigest.Hash == "" {
		err = fmt.Errorf("missing command digest in action")
		return
	}
	commandBytes, ok := server.contents.Get(commandDigest.Hash)
	if !ok {
		err = fmt.Errorf("command blob not found in CAS: %s", commandDigest.Hash)
		return
	}
	command := remoteexecution.Command{}
	if err = proto.Unmarshal(commandBytes, &command); err != nil {
		return
	}

	result.Command, err = server.getCommand(&command)
	if err != nil {
		return
	}
	result.InputRoot, err = server.getDirectory(action.InputRootDigest)
	if err != nil {
		return
	}
	if action.Timeout != nil {
		t := action.Timeout.AsDuration()
		result.Timeout = &t
	}
	result.DoNotCache = action.DoNotCache
	result.Salt = action.Salt
	result.Platform, err = server.getPlatform(action.Platform)
	if err != nil {
		return
	}
	return
}

func (actions *Actions) Get(hash string) (*remoteexecution.ActionResult, bool) {
	actions.mutex.RLock()
	defer actions.mutex.RUnlock()
	result, ok := actions.actions[hash]
	return result, ok
}

func (actions *Actions) Set(hash string, result *remoteexecution.ActionResult) {
	actions.mutex.Lock()
	actions.actions[hash] = result
	actions.mutex.Unlock()
}

func (server *Server) GetActionResult(
	ctx context.Context,
	req *remoteexecution.GetActionResultRequest,
) (*remoteexecution.ActionResult, error) {
	log.Printf("GetActionResult: %v", req)
	if req.ActionDigest == nil || req.ActionDigest.Hash == "" {
		return nil, status.Error(codes.InvalidArgument, "missing action digest")
	}
	result, ok := server.actions.Get(req.ActionDigest.Hash)
	if !ok {
		return nil, status.Error(codes.NotFound, "action result not found")
	}
	return result, nil
}

func (server *Server) UpdateActionResult(
	ctx context.Context,
	req *remoteexecution.UpdateActionResultRequest,
) (*remoteexecution.ActionResult, error) {
	log.Printf("UpdateActionResult: %v", req)
	if req.ActionDigest == nil || req.ActionDigest.Hash == "" {
		return nil, errors.New("missing action digest")
	}
	if req.ActionResult == nil {
		return nil, errors.New("missing action result")
	}
	server.actions.Set(req.ActionDigest.Hash, req.ActionResult)
	return req.ActionResult, nil
}

func (server *Server) registerAction(grpcServer *grpc.Server) {
	remoteexecution.RegisterActionCacheServer(grpcServer, server)
}
