package main

import (
	"bytes"
	"context"
	"encoding/json" // Add this import
	"fmt"
	"io"
	"net/http"
	"net/url"

	"github.com/google/uuid"
)

type Job struct {
	ID     uuid.UUID
	Error  error
	Result string

	action  Action
	done    chan bool
	context context.Context
	stop    context.CancelFunc
}

func NewJob(parentContext context.Context, action Action) (job *Job) {
	context, stop := context.WithCancel(parentContext)
	job = &Job{
		ID:      uuid.New(),
		action:  action,
		done:    make(chan bool),
		context: context,
		stop:    stop,
	}
	return
}

func (job *Job) Wait() (result string, err error) {
	<-job.done
	result, err = job.Result, job.Error
	return
}

func (job *Job) Cancel() {
	job.stop()
}

func (job *Job) Execute(server *Server) {
	defer close(job.done)
	client := http.Client{}
	reqURL, err := url.JoinPath(server.buildpotURL.String(), "execute")
	if err != nil {
		job.Error = fmt.Errorf("failed to build request URL: %w", err)
		return
	}

	actionJSON, err := json.Marshal(job.action)
	if err != nil {
		job.Error = fmt.Errorf("failed to marshal action to JSON: %w", err)
		return
	}

	httpReq, err := http.NewRequestWithContext(job.context, "POST", reqURL, bytes.NewBuffer(actionJSON))
	if err != nil {
		job.Error = fmt.Errorf("failed to create HTTP request: %w", err)
		return
	}
	httpReq.Header.Set("Content-Type", "application/json")
	httpReq.Header.Set("Accept", "application/json")

	httpResp, err := client.Do(httpReq)
	if err != nil {
		// Check if error is due to context cancellation
		select {
		case <-job.context.Done():
			job.Error = job.context.Err()
		default:
			job.Error = fmt.Errorf("HTTP request failed: %w", err)
		}
		return
	}
	defer httpResp.Body.Close()

	bodyBytes, err := io.ReadAll(httpResp.Body)
	if err != nil {
		job.Error = fmt.Errorf("failed to read response body: %w", err)
		return
	}
	job.Result = string(bodyBytes)
}
