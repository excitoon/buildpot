package main

import (
	"context"
	"flag"
	"fmt"
	"log"
	"net"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
)

func main() {
	port := 8980
	tls := flag.Bool("tls", false, "enable SSL/TLS")
	certFile := flag.String("cert", "server.crt", "TLS certificate file")
	keyFile := flag.String("key", "server.key", "TLS key file")
	buildpotURL := flag.String("buildpot", "http://10.10.10.9:18980", "Buildpot base URL")
	flag.Parse()

	lis, err := net.Listen("tcp", fmt.Sprintf("localhost:%d", port))
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
	_, err = NewServer(ctx, *buildpotURL, grpcServer)
	if err != nil {
		log.Fatalf("failed to create server: %v", err)
	}

	grpcServer.Serve(lis)
	stop()
	log.Printf("Server stopped")
}
