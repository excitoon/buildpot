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

func (directory *Directory) Size() (size int) {
	for _, file := range directory.Files {
		size += len(file.Data)
	}
	for _, subdir := range directory.Directories {
		size += subdir.Size()
	}
	return
}

func (directory *Directory) getTree(root string, prefix string) []string {
	formatName := func(path, name string) string {
		return fmt.Sprintf("\033[1;33m%s\033[0m", name)
	}

	var tree []string
	count := len(directory.Directories) + len(directory.Files) + len(directory.Symlinks)
	i := 0
	for name, subdir := range directory.Directories {
		i++
		path := fmt.Sprintf("%s/%s", root, name)
		size := subdir.Size()
		var suffix string
		if i == count {
			suffix = "└──"
		} else {
			suffix = "├──"
		}
		tree = append(tree, fmt.Sprintf("%s%s %s [%d bytes]\n", prefix, suffix, formatName(path, name), size))
		if i == count {
			suffix = "    "
		} else {
			suffix = "│   "
		}
		tree = append(tree, subdir.getTree(fmt.Sprintf("%s/%s", root, name), prefix+suffix)...)
	}
	for name, file := range directory.Files {
		i++
		path := fmt.Sprintf("%s/%s", root, name)
		var suffix string
		if i == count {
			suffix = "└──"
		} else {
			suffix = "├──"
		}
		tree = append(tree, fmt.Sprintf("%s%s %s [%d bytes]\n", prefix, suffix, formatName(path, name), len(file.Data)))
	}
	for name, symlink := range directory.Symlinks {
		i++
		path := fmt.Sprintf("%s/%s", root, name)
		var suffix string
		if i == count {
			suffix = "└──"
		} else {
			suffix = "├──"
		}
		tree = append(tree, fmt.Sprintf("%s%s %s -> %s\n", prefix, suffix, formatName(path, name), formatName(symlink.Target, symlink.Target)))
	}
	return tree
}

func (directory *Directory) PrintTree() {
	for _, line := range directory.getTree("", "") {
		fmt.Print(line)
	}
}
