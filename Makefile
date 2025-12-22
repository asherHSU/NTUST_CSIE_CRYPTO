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
            $(SRC_DIR)/rudraksh_poly.c \
			$(SRC_DIR)/rudraksh_randombytes.c\
			$(SRC_DIR)/rudraksh_generator.c\
			$(SRC_DIR)/rudraksh_crypto.c

# 將 .c 檔案列表轉換為 .o (Object file) 列表
# 例如: src/ntt.c -> build/ntt.o
CORE_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(CORE_SRCS))

# 單獨測試 :  Random 、 matrix A
CORE_OBJS_RND = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(CORE_SRCS_RND))
CORE_OBJS_Gen = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(CORE_SRCS_Gen))
# ==========================================
# Targets (指令目標)
# ==========================================

# 預設目標：當你只打 'make' 時，會執行這個

ALL_TESTS := \
	test_random \
	test_generator \
	test_ntt \
	test_math \
	test_debug \
	test_pke \
	test_kem \
	test_crypto

ALL_TESTS_L := \
	test_random_l \
	test_generator_l \
	test_ntt_l \
	test_math_l \
	test_debug_l \
	test_pke_l \
	test_kem_l \
	test_crypto_l

all: dirs $(ALL_TESTS) 		# windows all
$(ALL_TESTS):| dirs
linux: ldirs $(ALL_TESTS_L)    # Linux all
$(ALL_TESTS_L):| ldirs

# windows
random:   dirs test_random
gen:      dirs test_generator
ntt:      dirs test_ntt
crypto:   dirs test_crypto	# 加解密最後終測試
debug:    dirs test_debug
pke:      dirs test_pke
math:     dirs test_math
kem:      dirs test_kem

# linux 
lrandom:   ldirs test_random_l
lgen:      ldirs test_generator_l
lntt:      ldirs test_ntt_l
lcrypto:   ldirs test_crypto_l	
ldebug:    ldirs test_debug_l
lpke:      ldirs test_pke_l
lmath:     ldirs test_math_l
lkem:      ldirs test_kem_l

# 建立必要的資料夾 (避免編譯時報錯說資料夾不存在)
# for Linux / GitHub Actions
ldirs: 
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

# 	windows 原生 cmd -p 會被當作資料夾名稱
dirs:
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
# 測試程式編譯規則 Windows
# ------------------------------------------

# 編譯 NTT 單元測試
test_ntt: $(CORE_OBJS) $(TEST_DIR)/test_ntt.c
	@echo "Building NTT Unit Test..."
	$(CC) $(CFLAGS) $(TEST_DIR)/test_ntt.c $(CORE_OBJS) -o $(BIN_DIR)/test_ntt.exe
	@echo "Build Success! Run with: ./$(BIN_DIR)/test_ntt.exe"
	./$(BIN_DIR)/test_ntt.exe

# 編譯 random 單元測試
test_random: $(CORE_SRCS) $(TEST_DIR)/test_random.c
	@echo "Building Random Unit Test..."
	$(CC) $(CFLAGS) $(TEST_DIR)/test_random.c $(CORE_SRCS) -o $(BIN_DIR)/test_random.exe
	@echo "Build Success! Run with: ./$(BIN_DIR)/test_random.exe"
	./$(BIN_DIR)/test_random.exe

# 編譯 生成器 單元測試
test_generator: $(CORE_SRCS) $(TEST_DIR)/test_generator.c
	@echo "Building Generator Unit Test..."
	$(CC) $(CFLAGS) $(TEST_DIR)/test_generator.c $(CORE_SRCS) -o $(BIN_DIR)/test_generator.exe
	@echo "Build Success! Run with: ./$(BIN_DIR)/test_generator.exe"
	./$(BIN_DIR)/test_generator.exe

# 編譯 加解密模型 最終測試
test_crypto: $(CORE_OBJS) $(TEST_DIR)/test_crypto.c
	@echo "Building PKE/KEM Test..."
	$(CC) $(CFLAGS) $(TEST_DIR)/test_crypto.c $(CORE_OBJS) -o $(BIN_DIR)/test_crypto.exe
	@echo "Build Success! Run with: ./$(BIN_DIR)/test_crypto.exe"
	./$(BIN_DIR)/test_crypto.exe

# 編譯 pke debug 測試
test_debug: $(CORE_OBJS) $(TEST_DIR)/test_debug.c
	@echo "Building Debug Test..."
	$(CC) $(CFLAGS) $(TEST_DIR)/test_debug.c $(CORE_OBJS) -o $(BIN_DIR)/test_debug.exe
	@echo "Build Success! Run with: ./$(BIN_DIR)/test_debug.exe"
	./$(BIN_DIR)/test_debug.exe

# 編譯 PKE 最小模型 測試
test_pke: $(CORE_OBJS) $(TEST_DIR)/test_pke.c
	@echo "Building pke Test..."
	$(CC) $(CFLAGS) $(TEST_DIR)/test_pke.c $(CORE_OBJS) -o $(BIN_DIR)/test_pke.exe
	@echo "Build Success! Run with: ./$(BIN_DIR)/test_pke.exe"
	./$(BIN_DIR)/test_pke.exe

# 編譯 Math 單元測試
test_math: $(CORE_OBJS) $(TEST_DIR)/test_math.c
	@echo "Building ntt mult/add/sub Test..."
	$(CC) $(CFLAGS) $(TEST_DIR)/test_math.c $(CORE_OBJS) -o $(BIN_DIR)/test_math.exe
	@echo "Build Success! Run with: ./$(BIN_DIR)/test_math.exe"
	./$(BIN_DIR)/test_math.exe

# 編譯 KEM 最小模型 單元測試
test_kem: $(CORE_OBJS) $(TEST_DIR)/test_kem.c
	@echo "Building KEM Test..."
	$(CC) $(CFLAGS) $(TEST_DIR)/test_kem.c $(CORE_OBJS) -o $(BIN_DIR)/test_kem.exe
	@echo "Build Success! Run with: ./$(BIN_DIR)/test_kem.exe"
	./$(BIN_DIR)/test_kem.exe

# ------------------------------------------
# 測試程式編譯規則 Linux
# ------------------------------------------

# 編譯 NTT 單元測試
test_ntt_l: $(CORE_OBJS) $(TEST_DIR)/test_ntt.c
	@echo "Building NTT Unit Test..."
	$(CC) $(CFLAGS) $(TEST_DIR)/test_ntt.c $(CORE_OBJS) -o $(BIN_DIR)/test_ntt
	@echo "Build Success! Run with: ./$(BIN_DIR)/test_ntt"
	./$(BIN_DIR)/test_ntt

# 編譯 random 單元測試
test_random_l: $(CORE_SRCS) $(TEST_DIR)/test_random.c
	@echo "Building Random Unit Test..."
	$(CC) $(CFLAGS) $(TEST_DIR)/test_random.c $(CORE_SRCS) -o $(BIN_DIR)/test_random
	@echo "Build Success! Run with: ./$(BIN_DIR)/test_random"
	./$(BIN_DIR)/test_random

# 編譯 生成器 單元測試
test_generator_l: $(CORE_SRCS) $(TEST_DIR)/test_generator.c
	@echo "Building Generator Unit Test..."
	$(CC) $(CFLAGS) $(TEST_DIR)/test_generator.c $(CORE_SRCS) -o $(BIN_DIR)/test_generator
	@echo "Build Success! Run with: ./$(BIN_DIR)/test_generator"
	./$(BIN_DIR)/test_generator

# 編譯 加解密模型 最終測試
test_crypto_l: $(CORE_OBJS) $(TEST_DIR)/test_crypto.c
	@echo "Building PKE/KEM Test..."
	$(CC) $(CFLAGS) $(TEST_DIR)/test_crypto.c $(CORE_OBJS) -o $(BIN_DIR)/test_crypto
	@echo "Build Success! Run with: ./$(BIN_DIR)/test_crypto"
	./$(BIN_DIR)/test_crypto

# 編譯 pke debug 測試
test_debug_l: $(CORE_OBJS) $(TEST_DIR)/test_debug.c
	@echo "Building Debug Test..."
	$(CC) $(CFLAGS) $(TEST_DIR)/test_debug.c $(CORE_OBJS) -o $(BIN_DIR)/test_debug
	@echo "Build Success! Run with: ./$(BIN_DIR)/test_debug"
	./$(BIN_DIR)/test_debug

# 編譯 PKE 最小模型 測試
test_pke_l: $(CORE_OBJS) $(TEST_DIR)/test_pke.c
	@echo "Building pke Test..."
	$(CC) $(CFLAGS) $(TEST_DIR)/test_pke.c $(CORE_OBJS) -o $(BIN_DIR)/test_pke
	@echo "Build Success! Run with: ./$(BIN_DIR)/test_pke"
	./$(BIN_DIR)/test_pke

# 編譯 Math 單元測試
test_math_l: $(CORE_OBJS) $(TEST_DIR)/test_math.c
	@echo "Building ntt mult/add/sub Test..."
	$(CC) $(CFLAGS) $(TEST_DIR)/test_math.c $(CORE_OBJS) -o $(BIN_DIR)/test_math
	@echo "Build Success! Run with: ./$(BIN_DIR)/test_math"
	./$(BIN_DIR)/test_math

# 編譯 KEM 最小模型 單元測試
test_kem_l: $(CORE_OBJS) $(TEST_DIR)/test_kem.c
	@echo "Building KEM Test..."
	$(CC) $(CFLAGS) $(TEST_DIR)/test_kem.c $(CORE_OBJS) -o $(BIN_DIR)/test_kem
	@echo "Build Success! Run with: ./$(BIN_DIR)/test_kem"
	./$(BIN_DIR)/test_kem

# ------------------------------------------
# 清理規則
# ------------------------------------------
lclean:
	@echo "Cleaning up..."
	rm -rf $(BUILD_DIR)/* $(BIN_DIR)/*

.PHONY: all dirs clean test_ntt

clean:
	@echo "(Windows) Cleaning up..."
	rmdir /s /q $(BUILD_DIR) $(BIN_DIR)