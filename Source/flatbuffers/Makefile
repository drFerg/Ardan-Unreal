# Simple Makefile to generate flatbuffers
# copy to relevant directories after compiling

all: unrealpkts.fbs
	flatc -o ./c/ --cpp unrealpkts.fbs
	flatc -o ./java/ --java unrealpkts.fbs
