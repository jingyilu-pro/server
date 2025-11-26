
mkdir -p ./protocol

../../build/third/protobuf/bin/protoc -I=./ --cpp_out=./protocol ./*.proto