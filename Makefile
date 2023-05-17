# Compiler
CXX = g++

# Default build type
BUILD_TYPE ?= release

# Flags
CXXFLAGS = -MMD -MP -std=c++20 -Werror -Wall -Wextra -Wpedantic -Wformat -Wmissing-include-dirs -Wuninitialized -Wunreachable-code -Wshadow -Wconversion -Wsign-conversion -Wredundant-decls -Winit-self -Wswitch-default -Wfloat-equal -Wunused-parameter -MMD -MP
ifeq ($(BUILD_TYPE),debug)
	CXXFLAGS += -g
	BIN_DIR = bin/debug
else ifeq ($(BUILD_TYPE),release)
	CXXFLAGS += -O2
	BIN_DIR = bin/release
else
	$(error BUILD_TYPE $(BUILD_TYPE) not supported)
endif

LDFLAG = -lstdc++ -lm -lboost_program_options -lboost_system

# Directories
SRC_DIR = src

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BIN_DIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)

# Rules
.PHONY: default toolchain clean

default: toolchain

toolchain: next_cluster make_image make_states simulate_one simulate_many

core = $(addprefix $(BIN_DIR)/, fregex.o logger.o pgm8.o program_options.o simulation_misc.o simulation_parse_state.o simulation_run.o simulation_save_state.o util.o term.o)

next_cluster: $(core) $(BIN_DIR)/next_cluster_main.o
	@$(CXX) $(CXXFLAGS) -o $(BIN_DIR)/$@ $^ $(LDFLAG)
	@echo 'compiling next_cluster...'
make_image: $(core) $(BIN_DIR)/make_image_main.o
	@$(CXX) $(CXXFLAGS) -o $(BIN_DIR)/$@ $^ $(LDFLAG)
	@echo 'compiling make_image...'
make_states: $(core) $(BIN_DIR)/make_states_main.o
	@$(CXX) $(CXXFLAGS) -o $(BIN_DIR)/$@ $^ $(LDFLAG)
	@echo 'compiling make_states...'
simulate_one: $(core) $(BIN_DIR)/simulate_one_main.o
	@$(CXX) $(CXXFLAGS) -o $(BIN_DIR)/$@ $^ $(LDFLAG)
	@echo 'compiling simulate_one...'
simulate_many: $(core) $(BIN_DIR)/simulate_many_main.o
	@$(CXX) $(CXXFLAGS) -o $(BIN_DIR)/$@ $^ $(LDFLAG)
	@echo 'compiling simulate_many...'
tests: $(core) $(BIN_DIR)/ntest.o $(BIN_DIR)/testing_main.o
	@$(CXX) $(CXXFLAGS) -o $(BIN_DIR)/$@ $^ $(LDFLAG)
	@echo 'compiling tests...'
file_write_benchmark: $(BIN_DIR)/file_write_benchmark.o
	@$(CXX) $(CXXFLAGS) -o $(BIN_DIR)/file_write_benchmark $^ $(LDFLAG)
	@echo 'compiling file_write_benchmark...'

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BIN_DIR)
	@echo 'compiling [$<]...'
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -r -f bin/debug bin/release
	find . -name "*.d" -type f -delete

# Include the generated .d files
-include $(DEPS)