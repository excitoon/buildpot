package main

import (
	"bytes"
	"context"
	"crypto/sha256"
	"encoding/hex"
	"encoding/json" // Add this import
	"fmt"
	"io"
	"log"
	"net/http"
	"net/url"

	"github.com/google/uuid"
)

type Job struct {
	ID     uuid.UUID
	Error  error
	Result ActionResult

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

func (job *Job) Wait() (result ActionResult, err error) {
	<-job.done
	result, err = job.Result, job.Error
	return
}

func (job *Job) Cancel() {
	job.stop()
}

func (job *Job) Execute(server *Server) {
	log.Printf("Executing job: %s", job.ID)
	job.action.InputRoot.PrintTree()
	log.Printf("Output paths (%d):", len(job.action.Command.OutputPaths))
	for _, path := range job.action.Command.OutputPaths {
		log.Printf("- %s", path)
	}

	defer close(job.done)
	client := http.Client{}
	reqURL, err := url.JoinPath(server.buildpotURL.String(), "execute")
	if err != nil {
		job.Error = fmt.Errorf("failed to build request URL: %w", err)
		log.Printf("Job %s failed: %v", job.ID, job.Error)
		return
	}

	actionJSON, err := json.Marshal(job.action)
	if err != nil {
		job.Error = fmt.Errorf("failed to marshal action to JSON: %w", err)
		log.Printf("Job %s failed: %v", job.ID, job.Error)
		return
	}

	log.Printf("Job %s sending action: %d bytes.", job.ID, len(actionJSON))

	httpReq, err := http.NewRequestWithContext(job.context, "POST", reqURL, bytes.NewBuffer(actionJSON))
	if err != nil {
		job.Error = fmt.Errorf("failed to create HTTP request: %w", err)
		log.Printf("Job %s failed: %v", job.ID, job.Error)
		return
	}
	httpReq.Header.Set("Content-Type", "application/json")
	httpReq.Header.Set("Accept", "application/json")

	httpResp, err := client.Do(httpReq)
	if err != nil {
		select {
		case <-job.context.Done():
			job.Error = job.context.Err()
		default:
			job.Error = fmt.Errorf("HTTP request failed: %w", err)
		}
		log.Printf("Job %s failed: %v", job.ID, job.Error)
		return
	}
	defer httpResp.Body.Close()

	bodyBytes, err := io.ReadAll(httpResp.Body)
	if err != nil {
		job.Error = fmt.Errorf("failed to read response body: %w", err)
		log.Printf("Job %s failed: %v", job.ID, job.Error)
		return
	}

	log.Printf("Job %s received response: %d bytes.", job.ID, len(bodyBytes))

	if httpResp.StatusCode != http.StatusOK {
		job.Error = fmt.Errorf("HTTP request failed with status %s: %s", httpResp.Status, string(bodyBytes))
		log.Printf("Job %s failed: %v", job.ID, job.Error)
		return
	}

	if err := json.Unmarshal(bodyBytes, &job.Result); err != nil {
		job.Error = fmt.Errorf("failed to unmarshal ActionResult: %w", err)
		log.Printf("Job %s failed: %v", job.ID, job.Error)
		return
	}

	for name, file := range job.Result.Files {
		file.Name = name
		job.Result.Files[name] = file
	}

	for name, file := range job.Result.Files {
		sum := sha256.Sum256(file.Data)
		digest := hex.EncodeToString(sum[:])
		server.contents.Set(digest, file.Data)
		job.Result.Files[name] = file
	}

	totalSize := 0
	for _, f := range job.Result.Files {
		totalSize += len(f.Data)
	}

	names := []string{}
	for _, file := range job.Result.Files {
		names = append(names, file.Name)

	}

	log.Printf(
		"Job %s completed successfully, producing %d output files out of %d requested: %v, %d bytes total.",
		job.ID,
		len(job.Result.Files),
		len(job.action.Command.OutputPaths),
		names,
		totalSize,
	)
}
