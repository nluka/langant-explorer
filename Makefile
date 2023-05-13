# Compiler and flags
CXX = g++
CXXFLAGS = -g -Wall -Wextra -std=c++20
LDFLAG = -lstdc++ -lm -lboost_program_options -lboost_system

# Directories
SRC_DIR = src
BIN_DIR = bin

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BIN_DIR)/%.o, $(SRCS))

# Rules
.PHONY: all clean

default:
	make next_cluster
	mv next_cluster staging/
	make make_image
	mv make_image staging/
	make make_states
	mv make_states staging/
	make simulate_one
	mv simulate_one staging/
	make simulate_many
	mv simulate_many staging/

core = $(BIN_DIR)/fregex.o $(BIN_DIR)/logger.o $(BIN_DIR)/pgm8.o $(BIN_DIR)/program_options.o $(BIN_DIR)/simulation_misc.o $(BIN_DIR)/simulation_parse_state.o $(BIN_DIR)/simulation_run.o $(BIN_DIR)/simulation_save_state.o $(BIN_DIR)/util.o $(BIN_DIR)/term.o

next_cluster: $(core) $(BIN_DIR)/next_cluster_main.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAG)
make_image: $(core) $(BIN_DIR)/make_image_main.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAG)
make_states: $(core) $(BIN_DIR)/make_states_main.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAG)
simulate_one: $(core) $(BIN_DIR)/simulate_one_main.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAG)
simulate_many: $(core) $(BIN_DIR)/simulate_many_main.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAG)
test: $(core) $(BIN_DIR)/ntest.o $(BIN_DIR)/testing_main.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAG)
	./test

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/fregex.o: $(SRC_DIR)/fregex.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BIN_DIR)/logger.o: $(SRC_DIR)/logger.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BIN_DIR)/make_image_main.o: $(SRC_DIR)/make_image_main.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BIN_DIR)/make_states_main.o: $(SRC_DIR)/make_states_main.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BIN_DIR)/next_cluster_main.o: $(SRC_DIR)/next_cluster_main.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BIN_DIR)/ntest.o: $(SRC_DIR)/ntest.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BIN_DIR)/pgm8.o: $(SRC_DIR)/pgm8.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BIN_DIR)/program_options.o: $(SRC_DIR)/program_options.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BIN_DIR)/simulate_many_main.o: $(SRC_DIR)/simulate_many_main.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BIN_DIR)/simulate_one_main.o: $(SRC_DIR)/simulate_one_main.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BIN_DIR)/simulation_misc.o: $(SRC_DIR)/simulation_misc.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BIN_DIR)/simulation_parse_state.o: $(SRC_DIR)/simulation_parse_state.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BIN_DIR)/simulation_run.o: $(SRC_DIR)/simulation_run.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BIN_DIR)/simulation_save_state.o: $(SRC_DIR)/simulation_save_state.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BIN_DIR)/term.o: $(SRC_DIR)/term.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BIN_DIR)/testing_main.o: $(SRC_DIR)/testing_main.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BIN_DIR)/util.o: $(SRC_DIR)/util.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -r -f $(BIN_DIR)
