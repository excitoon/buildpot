package main

import (
	"errors"
	"sync"

	"github.com/google/uuid"
)

var errJobNotFound = errors.New("job not found")

type Pipeline struct {
	mu   sync.Mutex
	jobs map[uuid.UUID]*Job
}

func NewPipeline() Pipeline {
	return Pipeline{
		jobs: make(map[uuid.UUID]*Job),
	}
}

func (p *Pipeline) AddJob(server *Server, job *Job) {
	p.mu.Lock()
	p.jobs[job.ID] = job
	p.mu.Unlock()
	go job.Execute(server)
}

func (p *Pipeline) GetJob(id uuid.UUID) (job *Job, err error) {
	p.mu.Lock()
	defer p.mu.Unlock()
	job, exists := p.jobs[id]
	if !exists {
		err = errJobNotFound
	}
	return
}

func (p *Pipeline) RemoveJob(id uuid.UUID) (err error) {
	p.mu.Lock()
	defer p.mu.Unlock()
	job, exists := p.jobs[id]
	if exists {
		delete(p.jobs, id)
		job.Cancel()
	} else {
		err = errJobNotFound
	}
	return
}
