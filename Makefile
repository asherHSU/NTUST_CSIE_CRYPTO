# ==========================================
# Rudraksh Project Makefile
# ==========================================

# 編譯器設定
CC = gcc
# CFLAGS: 編譯參數
# -O3: 開啟最高效能優化 (Rudraksh 需要測速，這很重要)
# -Wall -Wextra: 開啟所有警告 (幫你抓 Bug)
# -I src: 告訴編譯器去 src 資料夾找 .h 檔
CFLAGS = -O3 -Wall -Wextra -I src -I src/ascon

# 專案路徑設定
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
TEST_DIR = tests

# 核心原始碼列表 (如果有新檔案，例如 ascon.c，加在這裡)
CORE_SRCS = $(SRC_DIR)/rudraksh_ntt.c \
            $(SRC_DIR)/rudraksh_ntt_data.c \
			$(SRC_DIR)/rudraksh_ascon.c \
            $(SRC_DIR)/rudraksh_poly.c

CORE_SRCS_ASCON = 	$(SRC_DIR)/rudraksh_ascon.c 
CORE_SRCS_RND = 	$(SRC_DIR)/rudraksh_randombytes.c

# 將 .c 檔案列表轉換為 .o (Object file) 列表
# 例如: src/ntt.c -> build/ntt.o
CORE_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(CORE_SRCS))

# 單獨測試 :  ASCON 和 Random 所需的物件檔
CORE_OBJS_ASCON = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(CORE_SRCS_ASCON))
CORE_OBJS_RND = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(CORE_SRCS_RND))
# ==========================================
# Targets (指令目標)
# ==========================================

# 預設目標：當你只打 'make' 時，會執行這個
all: dirs test_ntt

ascon: wdirs test_ascon
random: wdirs test_randombytes

# 建立必要的資料夾 (避免編譯時報錯說資料夾不存在)
dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

# 	windows 不能加上-p會被當成資料夾
wdirs:
	@mkdir  $(BUILD_DIR)
	@mkdir  $(BIN_DIR)

# ------------------------------------------
# 核心物件檔編譯規則
# ------------------------------------------

# 怎麼從 src/%.c 產生 build/%.o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# ------------------------------------------
# 測試程式編譯規則
# ------------------------------------------

# 編譯 NTT 單元測試
test_ntt: $(CORE_OBJS) $(TEST_DIR)/test_ntt.c
	@echo "Building NTT Unit Test..."
	$(CC) $(CFLAGS) $(TEST_DIR)/test_ntt.c $(CORE_OBJS) -o $(BIN_DIR)/test_ntt
	@echo "Build Success! Run with: ./$(BIN_DIR)/test_ntt"

# 編譯 ASCON
test_ascon: $(CORE_OBJS_ASCON) $(TEST_DIR)/test_ascon.c
	@echo "Building ASCON Unit Test..."
	$(CC) $(CFLAGS) $(TEST_DIR)/test_ascon.c $(CORE_OBJS_ASCON) -o $(BIN_DIR)/test_ascon
	@echo "Build Success! Run with: ./$(BIN_DIR)/test_ascon"

# 編譯 random 測試
test_randombytes: $(CORE_OBJS_RND) $(TEST_DIR)/test_randombytes.c
	@echo "Building Random Unit Test..."
	$(CC) $(CFLAGS) $(TEST_DIR)/test_randombytes.c $(CORE_OBJS_RND) -o $(BIN_DIR)/test_randombytes
	@echo "Build Success! Run with: ./$(BIN_DIR)/test_randombytes"

# ------------------------------------------
# 清理規則
# ------------------------------------------
clean:
	@echo "Cleaning up..."
	rm -rf $(BUILD_DIR)/* $(BIN_DIR)/*

.PHONY: all dirs clean test_ntt

wclean:
	@echo "(Windows) Cleaning up..."
	rmdir /s /q $(BUILD_DIR) $(BIN_DIR)