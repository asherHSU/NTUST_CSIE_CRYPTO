# NTUST_CSIE_CRYPTO

### Rudraksh-C: 輕量級後量子 KEM 的 C 語言實作

![Language](https://img.shields.io/badge/language-C99-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Status](https://img.shields.io/badge/status-Phase_3_Alpha-orange.svg)

## 📖 專案簡介 (Overview)

本儲存庫（Repository）包含了 **Rudraksh** 的純 C 語言實作。Rudraksh 是一種基於 Module-LWE 問題的緊湊型輕量級後量子金鑰封裝機制（KEM）。

本專案旨在復現 2025 年論文 **"Rudraksh: A compact and lightweight post-quantum key-encapsulation mechanism"** 中提出的方案，重點在於驗證其軟體執行效率，並與 NIST 標準算法 **CRYSTALS-Kyber** 進行比較。

### 📄 參考文獻
* **論文標題:** Rudraksh: A compact and lightweight post-quantum key-encapsulation mechanism
* **作者:** Suparna Kundu, Archisman Ghosh, Angshuman Karmakar, Shreyas Sen, Ingrid Verbauwhede
* **來源:** [arXiv:2501.13799](https://arxiv.org/abs/2501.13799)
* **核心特色:**
    * 使用 **ASCON** (NIST 輕量級密碼學競賽贏家) 取代 Keccak/SHA-3，實現輕量化設計。
    * 使用較小的多項式次數 (**n=64**) 以大幅降低記憶體佔用。
    * 提供 **IND-CCA** 安全性，具備 NIST Level-1 (AES-128 同級) 的防護能力。

---

## ⚙️ 技術參數 (Technical Parameters)

我們嚴格遵循論文中 **KEM-poly64** 的參數設定：

| 參數 (Parameter) | 數值 (Value) | 說明 (Description) |
| :--- | :--- | :--- |
| **n** | 64 | 多項式次數 (Kyber 使用 256) |
| **q** | 7681 | 質數模數 (滿足 $q \equiv 1 \pmod{2n}$ 的 NTT 條件) |
| **l** | 9 | 模組秩 (Module rank，矩陣維度 $9 \times 9$) |
| **$\eta$** | 2 | 中心二項式分佈 (CBD) 參數 |
| **NTT** | Complete | 啟用完整數論轉換 ($\zeta = 202$) |

---

## 📂 專案結構 (Project Structure)

```text
NTUST_CSIE_CRYPTO/
├── src/                 # 核心實作 (C 原始碼與標頭檔)
│   ├── rudraksh_params.h    # 全域參數定義 (N=64, Q=7681, K=9)
│   ├── rudraksh_math.h      # 數學運算與資料結構定義 (poly, polyvec)
│   ├── rudraksh_ntt.c       # NTT/INTT 與基礎模運算
│   ├── rudraksh_ntt_data.c  # 預先計算的旋轉因子表 (Twiddle Factors)
│   ├── rudraksh_poly.c      # 多項式壓縮、解壓縮、編碼與序列化
│   ├── rudraksh_random.h    # 亂數生成 與 ASCON 高層定義
│   ├── rudraksh_generator.c # 矩陣 A 生成與 CBD 取樣 (GenMatrix, GenSecret)
│   ├── rudraksh_randombytes.c # 系統級亂數生成器 (Windows/Linux)
│   ├── rudraksh_ascon.c     # ASCON 輕量級加密核心 (Hash, PRF, XOF)
│   ├── rudraksh_crypto.h    # PKE/KEM 高層 API 宣告
│   ├── rudraksh_crypto.c    # PKE/KEM 函式化包裝
│   └── ascon/               # ASCON 原始實作庫
├── tests/               # 單元測試
│   ├── test_ntt.c           # 驗證 Forward/Inverse NTT 正確性
│   ├── test_math.c          # 驗證 矩陣向量乘法、向量乘法 的 NTT域運算(棄用) 及 mod q 暴力乘法 
│   ├── test_random.c        # 驗證 ASCON Hash/PRF 與亂數生成
│   ├── test_generator.c     # 驗證矩陣生成與誤差分佈 (CBD)
│   ├── test_pke.c           # 除錯 PKE 測試檔 (從最小功能模型除錯到完整功能模型) 
│   ├── test_kem.c           # 除錯 KEM 測試檔 (從最小功能模型除錯到完整功能模型) 
│   └── test_crypto.c        # PKE 與 KEM 函式的完整測試
├── tools/               # 預計算輔助工具
│   ├── find_zeta.c          # 尋找原根 (Primitive roots) 的腳本
│   └── gen_table.c          # 產生旋轉因子表的腳本
├── bin/                 # [Artifact] 編譯完成的執行檔 (.exe)
├── build/               # [Artifact] 編譯過程的中間檔 (.o)
└── Makefile             # 自動化編譯腳本
```

---

## 🚀 如何執行 (Getting Started)
### 環境需求* GCC 編譯器 (支援 C99 標準)
* Make (Windows 使用者可安裝 MinGW 或透過 WSL 執行)

### 1. 編譯環境準備若為 Windows 環境，請先建立輸出目錄：

```bash
make dirs
```

*(Linux/Mac 使用者請用 `make ldirs`)*

### 2. 執行單元測試 ####A. NTT 數學核心測試驗證數論轉換與反轉換的正確性：

```bash
make test_ntt
./bin/test_ntt

```

**預期輸出:** `Round-Trip Test (INTT(NTT(x)) == x) PASSED!`

#### B. ASCON 與亂數測試驗證 Hash、PRF 以及系統亂數生成功能：

```bash
make random

```

*(此指令會自動編譯並執行 `test_random.exe`)*

**預期輸出:**

* Determinism Check: PASSED
* Avalanche Check: PASSED
* Random Bytes Test: 0/1 bits 分佈約為 50%

#### C. 生成器與取樣測試驗證矩陣 A 的生成邏輯與 CBD 誤差分佈：

```bash
make gen

```

*(此指令會自動編譯並執行 `test_generator.exe`)*

**預期輸出:**

* Gen polys: ...
* Avg coffe: ... (檢查係數是否均勻分佈)
* 分佈測試: 驗證 CBD 輸出是否集中於 -2 到 2 之間

### 3. 清理專案 (Clean Build)若需要重新編譯，可執行：

```bash
make clean

```

*(Windows 使用者請用 `make wclean`)*

---