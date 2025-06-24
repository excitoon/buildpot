package main

import (
	"context"
	"flag"
	"fmt"
	"log"
	"net"
	"strings"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
)

/*
go build ./... && echo Built. && ./pothouse --tls --cert server.crt --key server.key
bazel build ...
*/

type workerList []string

func (w *workerList) String() string {
	return strings.Join(*w, ",")
}

func (w *workerList) Set(val string) error {
	*w = append(*w, val)
	return nil
}

func main() {
	port := 8980
	tls := flag.Bool("tls", false, "enable SSL/TLS")
	certFile := flag.String("cert", "server.crt", "TLS certificate file")
	keyFile := flag.String("key", "server.key", "TLS key file")
	buildpotURLs := workerList{}
	flag.Var(&buildpotURLs, "buildpot", "Buildpot base URL")

	flag.Parse()

	if len(buildpotURLs) == 0 {
		buildpotURLs = append(buildpotURLs, "http://10.10.10.9:18981")
	}

	listener, err := net.Listen("tcp", fmt.Sprintf("localhost:%d", port))
	if err != nil {
		log.Fatalf("failed to listen: %v", err)
	}

	var opts []grpc.ServerOption
	if *tls {
		creds, err := credentials.NewServerTLSFromFile(*certFile, *keyFile)
		if err != nil {
			log.Fatalf("failed to load TLS credentials: %v", err)
		}
		opts = append(opts, grpc.Creds(creds))
	}
	ctx, stop := context.WithCancel(context.Background())
	grpcServer := grpc.NewServer(opts...)
	_, err = NewServer(ctx, buildpotURLs, grpcServer)
	if err != nil {
		log.Fatalf("failed to create server: %v", err)
	}

	grpcServer.Serve(listener)
	stop()
	log.Printf("Server stopped")
}
