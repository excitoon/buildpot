package main

import (
	"errors"
	"fmt"
	"time"

	remoteexecution "github.com/bazelbuild/remote-apis/build/bazel/remote/execution/v2"
	"google.golang.org/protobuf/proto"
)

type File struct {
	Name             string            `json:"name"`
	Data             []byte            `json:"data"`
	IsExecutable     bool              `json:"is_executable"`
	Properties       map[string]string `json:"properties"`
	ModificationTime *time.Time        `json:"mtime,omitempty"`
	Mode             *uint32           `json:"mode,omitempty"`
}

type Symlink struct {
	Name             string            `json:"name"`
	Target           string            `json:"target"`
	Properties       map[string]string `json:"properties"`
	ModificationTime *time.Time        `json:"mtime,omitempty"`
	Mode             *uint32           `json:"mode,omitempty"`
}

type Directory struct {
	Files            map[string]File      `json:"files"`
	Directories      map[string]Directory `json:"directories"`
	Symlinks         map[string]Symlink   `json:"symlinks"`
	Properties       map[string]string    `json:"properties"`
	ModificationTime *time.Time           `json:"mtime,omitempty"`
	Mode             *uint32              `json:"mode,omitempty"`
}

func (server *Server) getProperties(
	nodeProperties *remoteexecution.NodeProperties,
) (propertiesMap map[string]string, mtime *time.Time, mode *uint32) {
	propertiesMap = map[string]string{}
	if nodeProperties == nil {
		return
	}
	if nodeProperties.Mtime != nil {
		t := nodeProperties.Mtime.AsTime()
		mtime = &t
	}
	if nodeProperties.UnixMode != nil {
		m := nodeProperties.UnixMode.Value
		mode = &m
	}
	if nodeProperties.Properties != nil {
		for _, prop := range nodeProperties.Properties {
			propertiesMap[prop.Name] = prop.Value
		}
	}
	return
}

func (server *Server) getDirectory(
	digest *remoteexecution.Digest,
) (result Directory, err error) {
	result.Directories = map[string]Directory{}
	result.Files = map[string]File{}
	result.Symlinks = map[string]Symlink{}
	result.Properties = map[string]string{}

	if digest == nil || digest.Hash == "" {
		err = errors.New("missing digest for directory")
		return
	}
	directoryBytes, ok := server.contents.Get(digest.Hash)
	if !ok {
		err = fmt.Errorf("directory blob not found in CAS: %s", digest.Hash)
		return
	}
	directory := remoteexecution.Directory{}
	if err = proto.Unmarshal(directoryBytes, &directory); err != nil {
		return
	}

	result = Directory{
		Files:       make(map[string]File),
		Directories: make(map[string]Directory),
		Symlinks:    make(map[string]Symlink),
	}

	for _, fileNode := range directory.Files {
		properties, mtime, mode := server.getProperties(fileNode.NodeProperties)
		file := File{
			Name:             fileNode.Name,
			IsExecutable:     fileNode.IsExecutable,
			Properties:       properties,
			ModificationTime: mtime,
			Mode:             mode,
		}
		if fileNode.Digest == nil {
			err = fmt.Errorf("missing digest for file node: %s", fileNode.Name)
			return
		}
		var ok bool
		file.Data, ok = server.contents.Get(fileNode.Digest.Hash)
		if !ok || file.Data == nil {
			err = fmt.Errorf("file %s not found in CAS", fileNode.Name)
			return
		}
		result.Files[fileNode.Name] = file
	}

	for _, dirNode := range directory.Directories {
		var subDir Directory
		subDir, err = server.getDirectory(dirNode.Digest)
		if err != nil {
			return
		}
		result.Directories[dirNode.Name] = subDir
	}

	for _, symlinkNode := range directory.Symlinks {
		properties, mtime, mode := server.getProperties(symlinkNode.NodeProperties)
		symlink := Symlink{
			Name:             symlinkNode.Name,
			Target:           symlinkNode.Target,
			Properties:       properties,
			ModificationTime: mtime,
			Mode:             mode,
		}
		result.Symlinks[symlinkNode.Name] = symlink
	}
	properties, mtime, mode := server.getProperties(directory.NodeProperties)
	result.Properties = properties
	result.ModificationTime = mtime
	result.Mode = mode

	return
}
