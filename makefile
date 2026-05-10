#Usage: 
# make		# compile all binaries
# clean		# remove all binaries

.PHONY := all

.DEFAULT_GOAL := all

all:
	@echo "Building Elevator.run"
	g++ -g -pthread -Wall src/Elevator.cpp src/Floors.cpp src/Log.cpp src/LogBase.cpp src/LogToScreen.cpp src/Main.cpp src/Management.cpp src/People.cpp src/PeopleCallsGenerator.cpp src/SymbolicInterface.cpp -oElevator.run

.PHONY: clean

clean: 
	@echo "Cleaning up..."
	rm -f Elevator.run