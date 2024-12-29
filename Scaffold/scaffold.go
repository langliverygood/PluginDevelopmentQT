package main

import (
	"flag"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
)

var (
	projectName string
	className   string
	fileName    string
)

// 临时文件路径
const tmpFolderPath = "tmp/"

// 资源文件前缀
const resFolderPath = "resource" // 后面不能加斜线

func init() {
	flag.StringVar(&projectName, "init", "Example", "1请输入输入项目名")
}

func main() {
	flag.Parse()
	// 去掉下划线和中划线
	className = firstUpper(strings.Map(func(r rune) rune {
		if r == '_' || r == '-' {
			return -1
		}
		return r
	}, projectName))
	fileName = strings.ToLower(className)

	// 使用Mkdir创建文件夹
	err := os.Mkdir(tmpFolderPath, os.ModePerm)
	if err != nil {
		// 如果文件夹已存在，也可以选择忽略错误
		if !os.IsExist(err) {
			fmt.Println("Error creating directory:", err)
			return
		}
	}

	// 解压dir目录到项目目录下
	if err = RestoreAssets(tmpFolderPath, resFolderPath); err != nil {
		fmt.Println("Error RestoreAssets directory:", err)
		return
	}

	if projectName != "Example" {
		changeFile()
	}
	moveFile()
}

// firstUpper 字符串首字母大写
func firstUpper(s string) string {
	if s == "" {
		return ""
	}
	return strings.ToUpper(s[:1]) + s[1:]
}

// firstLower 字符串首字母小写
func firstLower(s string) string {
	if s == "" {
		return ""
	}
	return strings.ToLower(s[:1]) + s[1:]
}

// 替换文件中的字符串
func replaceInFile(filePath, oldStr, newStr string) error {
	// 读取文件内容
	data, err := os.ReadFile(filePath)
	if err != nil {
		return err
	}

	// 替换字符串
	content := strings.ReplaceAll(string(data), oldStr, newStr)

	// 写入新内容到文件
	return os.WriteFile(filePath, []byte(content), 0644)
}

func changeFile() error {
	replaceInFile(tmpFolderPath+resFolderPath+"/App/App.pro", "Example", projectName)
	replaceInFile(tmpFolderPath+resFolderPath+"/App/mainwindow.cpp", "Example", className)
	replaceInFile(tmpFolderPath+resFolderPath+"/Plugin/Plugin.pro", "Example", className)
	replaceInFile(tmpFolderPath+resFolderPath+"/Plugin/Plugin.pro", "example", fileName)
	replaceInFile(tmpFolderPath+resFolderPath+"/Plugin/res.qrc", "Example", fileName)
	replaceInFile(tmpFolderPath+resFolderPath+"/Plugin/example.cpp", "Example", className)
	replaceInFile(tmpFolderPath+resFolderPath+"/Plugin/example.cpp", "example.h", fileName+".h")
	replaceInFile(tmpFolderPath+resFolderPath+"/Plugin/example.h", "EXAMPLE_H", strings.ToUpper(className)+"_H")
	replaceInFile(tmpFolderPath+resFolderPath+"/Plugin/example.h", "Example", className)
	replaceInFile(tmpFolderPath+resFolderPath+"/Plugin/example.h", "example.json", fileName+".json")
	os.Rename(tmpFolderPath+resFolderPath+"/Plugin/exampleResources", tmpFolderPath+resFolderPath+"/Plugin/"+fileName+"Resources")
	os.Rename(tmpFolderPath+resFolderPath+"/Plugin/example.cpp", tmpFolderPath+resFolderPath+"/Plugin/"+fileName+".cpp")
	os.Rename(tmpFolderPath+resFolderPath+"/Plugin/example.h", tmpFolderPath+resFolderPath+"/Plugin/"+fileName+".h")
	os.Rename(tmpFolderPath+resFolderPath+"/Plugin/example.json", tmpFolderPath+resFolderPath+"/Plugin/"+fileName+".json")
	os.Rename(tmpFolderPath+resFolderPath+"/Example.pro", tmpFolderPath+resFolderPath+"/"+projectName+".pro")
	return nil
}

func moveFile() {
	// 使用Mkdir创建文件夹
	err := os.Mkdir(filepath.Join("./", projectName), os.ModePerm)
	if err != nil {
		// 如果文件夹已存在，也可以选择忽略错误
		if !os.IsExist(err) {
			fmt.Println("Error creating directory:", err)
			return
		}
	}
	path, _ := os.Getwd()
	sysType := runtime.GOOS
	if sysType == "linux" {
		c := exec.Command("/bin/bash", "-c", "cp", "-r", filepath.Join(path, "tmp/resource")+"/*", filepath.Join(path, projectName))
		if err = c.Run(); err != nil {
		}
	}
	if sysType == "windows" {
		c := exec.Command("cmd", "/C", "xcopy", filepath.Join(path, "tmp/resource"), filepath.Join(path, projectName), "/e")
		if err = c.Run(); err != nil {
		}
	}
	os.RemoveAll(filepath.Join("./", tmpFolderPath))
}
