#Usage:
# make            # compile all binaries
# make smoke-test # run a non-interactive functional smoke test
# make clean      # remove all binaries

.PHONY: all clean smoke-test

.DEFAULT_GOAL := all

CXX := g++
CXXFLAGS := -g -pthread -Wall
TARGET := Elevator.run
SRCS := \
	src/Elevator.cpp \
	src/Floors.cpp \
	src/Log.cpp \
	src/LogBase.cpp \
	src/LogToScreen.cpp \
	src/Main.cpp \
	src/Management.cpp \
	src/People.cpp \
	src/PeopleCallsGenerator.cpp \
	src/Configuration.cpp

all:
	@echo "Building $(TARGET)"
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

smoke-test: all
	@echo "Running smoke test (automatic Enter after 3s)"
	@(sleep 3; printf '\n') | ./$(TARGET) > /tmp/elevator-smoke.log
	@grep -q "Press Enter to stop..." /tmp/elevator-smoke.log
	@grep -q "Shutdown requested..." /tmp/elevator-smoke.log
	@grep -q "Management | Shutdown completed" /tmp/elevator-smoke.log
	@echo "Smoke test passed"

clean:
	@echo "Cleaning up..."
	rm -f $(TARGET)
