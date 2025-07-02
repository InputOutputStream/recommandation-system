# Directories
CLIENT_DIR = client
SERVER_DIR = server
INCLUDE_DIR = include
OBJ_DIR = obj
BIN_DIR = bin
LIBS_DIR = libs

# Library subdirectories
GRAPH_DIR = $(LIBS_DIR)/graph
KNN_DIR = $(LIBS_DIR)/knn
MF_DIR = $(LIBS_DIR)/mf

# Library names
GRAPH_LIB = libgraph.a
KNN_LIB = libknn.a
MF_LIB = libmf.a

# Source files
SERVER_SRCS = $(wildcard $(SERVER_DIR)/*.c)
CLIENT_SRCS = $(wildcard $(CLIENT_DIR)/*.c)

# Library source files
GRAPH_SRCS = $(wildcard $(GRAPH_DIR)/*.c)
KNN_SRCS = $(wildcard $(KNN_DIR)/*.c)
MF_SRCS = $(wildcard $(MF_DIR)/*.c)

# Object files
SERVER_OBJS = $(patsubst $(SERVER_DIR)/%.c,$(OBJ_DIR)/server_%.o,$(SERVER_SRCS))
CLIENT_OBJS = $(patsubst $(CLIENT_DIR)/%.c,$(OBJ_DIR)/client_%.o,$(CLIENT_SRCS))

# Library object files
GRAPH_OBJS = $(patsubst $(GRAPH_DIR)/%.c,$(OBJ_DIR)/graph_%.o,$(GRAPH_SRCS))
KNN_OBJS = $(patsubst $(KNN_DIR)/%.c,$(OBJ_DIR)/knn_%.o,$(KNN_SRCS))
MF_OBJS = $(patsubst $(MF_DIR)/%.c,$(OBJ_DIR)/mf_%.o,$(MF_SRCS))

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -g -I$(INCLUDE_DIR) -I$(LIBS_DIR)
LDFLAGS = -lpthread -lndmath -lm
LIB_LDFLAGS = -L$(OBJ_DIR) -lgraph -lknn -lmf $(LDFLAGS)

all: directories libraries client server

directories:
	mkdir -p $(OBJ_DIR) $(BIN_DIR) 
	
# ========== LIBRARY COMPILATION RULES ==========

# Compile library object files
$(OBJ_DIR)/graph_%.o: $(GRAPH_DIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJ_DIR)/knn_%.o: $(KNN_DIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJ_DIR)/mf_%.o: $(MF_DIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

# Create static libraries
$(OBJ_DIR)/$(GRAPH_LIB): $(GRAPH_OBJS)
	ar rcs $@ $^

$(OBJ_DIR)/$(KNN_LIB): $(KNN_OBJS)
	ar rcs $@ $^

$(OBJ_DIR)/$(MF_LIB): $(MF_OBJS)
	ar rcs $@ $^

# Library targets
libgraph: $(OBJ_DIR)/$(GRAPH_LIB)
libknn: $(OBJ_DIR)/$(KNN_LIB)
libmf: $(OBJ_DIR)/$(MF_LIB)
libraries: libgraph libknn libmf

# Compile application object files
$(OBJ_DIR)/server_%.o: $(SERVER_DIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJ_DIR)/client_%.o: $(CLIENT_DIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

# Link binaries (with libraries)
server: $(SERVER_OBJS) libraries
	$(CC) -o $(BIN_DIR)/server $(SERVER_OBJS) $(LIB_LDFLAGS)

client: $(CLIENT_OBJS) libraries
	$(CC) -o $(BIN_DIR)/client $(CLIENT_OBJS) $(LIB_LDFLAGS)

# ========== UTILITY TARGETS ==========

# Clean build
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Clean only libraries
clean-libs:
	rm -f $(OBJ_DIR)/*.a $(OBJ_DIR)/*.so $(OBJ_DIR)/graph_*.o $(OBJ_DIR)/knn_*.o $(OBJ_DIR)/mf_*.o

# Install libraries to system
install-libs: libraries
	sudo cp $(OBJ_DIR)/*.a /usr/local/lib/
	sudo ldconfig

# Show library info
lib-info:
	@echo "Graph library objects: $(GRAPH_OBJS)"
	@echo "KNN library objects: $(KNN_OBJS)"
	@echo "MF library objects: $(MF_OBJS)"

.PHONY: all directories clean clean-libs libraries libgraph libknn libmf install-libs lib-info client server