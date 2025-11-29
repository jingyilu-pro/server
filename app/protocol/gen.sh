
mkdir -p ./protocol

../../build/libs/protobuf/bin/protoc -I=./ --cpp_out=./protocol ./*.proto