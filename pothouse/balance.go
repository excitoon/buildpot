package main

import (
	"net/url"
	"sync"
)

type Balancer struct {
	buildpotURLs []url.URL
	mutex        sync.Mutex
	lastWorker   int
}

func NewBalancer(buildpotURLs []url.URL) Balancer {
	return Balancer{
		buildpotURLs: buildpotURLs,
		lastWorker:   0,
	}
}

func (b *Balancer) NextWorker() url.URL {
	b.mutex.Lock()
	defer b.mutex.Unlock()

	if len(b.buildpotURLs) == 0 {
		return url.URL{}
	}

	worker := b.buildpotURLs[b.lastWorker]
	b.lastWorker = (b.lastWorker + 1) % len(b.buildpotURLs)
	return worker
}
