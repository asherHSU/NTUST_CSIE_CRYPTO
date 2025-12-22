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

### 編譯並運行指令
##### Windows - cmd / PowerShell 編譯環境
```bash
# all tests
make 

# single test
make random
make gen
make ntt
make math
make debug
make pke
make kem
make crypto

# clean test
make clean
```
-----
##### Linux / GitHub Actions  編譯環境
```bash
# all tests
make linux

# single test
make lrandom
make lgen
make lntt
make lmath
make ldebug
make lpke
make lkem
make lcrypto

# clean test
make lclean
```

### 單元測試說明 
##### 1. 隨機亂數測試 (test_ntt.c)
```bash
# 編譯並執行
    # windows
make random
    # linux
make lrandom
```
**測試內容:**
1. ASCON hash 測試
2. MatrixA PRF 測試
3. CBD PRF 測試
4. Random Bytes 生成測試


**預期輸出:** 
###### [1] Hash
`Determinism Check: PASSED`
`Avalanche Check  : PASSED`
###### [2] Matrix A PRF
`Determinism Check: PASSED`
`Key Sensitivity  : PASSED`
`Nonce-i Sensitivity: PASSED`
###### [3] CBD PRF
`Determinism Check: PASSED`
`Nonce Sensitivity : PASSED`
###### Random Bytes
`Total Bits: 8388608`
`0 Bits    : 419xxxx (約50.00%)`
`1 Bits    : 419xxxx (約50.00%)`

-----
##### 2. Matrix A / CBD 生成器測試 (test_generator.c)
```bash
# 編譯並執行
    # windows
make gen
    # linux
make lgen
```
**測試內容:**
1. Matrix A 生成
2. CBD 生成分佈
3. CBD 固定種子生成


**預期輸出:** 
###### [1] Matrix A
`Avg coffe : 38xx( Avg = 3840 )`
###### [2] CBD
```
Distribution test:
-2 : 78 (80)
-1 : 332 (320)
0 : 465 (480)
1 : 321 (320)
2 : 84 (80)
```
###### [2.2] CBD Fixed input
```
-------------------
test: 0 , Ans: 0
test: 0 , Ans: 0
-------------------
test: 0 , Ans: 0
test: 0 , Ans: 0
-------------------
test: -1 , Ans: -1
test: 1 , Ans: 1
```
-----
##### 3. NTT 測試 (test_ntt.c)
```bash
# 編譯並執行
    # windows
make ntt
    # linux
make lntt
```
**測試內容:**
1. NTT 與 INTT 轉換
2. NTT域 乘法 測試 
`(測試乘法已更改邏輯, 從 NTT域 修正至 NTT前q模環內)`

**預期輸出:** 
###### [1] NTT 與 INTT 轉換
`NTT Test PASSED! (Basic Property Check)`
`Round-Trip Test (INTT(NTT(x)) == x) PASSED!`
###### [2] NTT域 乘法 測試 (邏輯已變更)
`Vector-Vector Mul Test FAILED!` `(邏輯已變更)`
`Matrix-Vector Mul Test FAILED!` `(邏輯已變更)`

-----
##### 4. 數學模運算 測試 (test_math.c)
```bash
# 編譯並執行
    # windows
make math
    # linux
make lmath
```
**測試內容:**
1. 多項式 Add / Sub 測試
2. 多項式 Mul 測試

**預期輸出:** 
###### [1] 多項式 Add / Sub 測試
`[Test 1] Poly Add/Sub: PASSED`
###### [2] 多項式 Mul 測試
`[Test 2] Poly BaseMul Acc: PASSED`

-----
##### 5. PKE debug (test_debug.c)
```bash
# 編譯並執行
    # windows
make debug
    # linux
make ldebug
```
**測試內容:**
1. 序列化測試 (struct to bit stream)
2. 加解 壓縮 u 測試
3. 加解 壓縮 v 測試
4. 加解碼測試


**預期輸出:** 
###### [1] 序列化測試 (struct to bit stream)
`[PASS] Serialization 13-bit roundtrip OK`
###### [2] 加解 壓縮 u 測試
`[PASS] Compression error is within expected range. `
###### [3] 加解 壓縮 v 測試
```
[PASS] Scaling for B=2 (0, 1920, 3840, 5760) is correct.
[PASS] Decode logic correctly recovers 0, 1, 2, 3.
[PASS] Decode is robust against small noise (+/- 200).
```
###### [4] 加解碼測試
`[PASS] V compression error is within theoretical bounds.`

-----
##### 6. PKE 最小模組擴充除錯 (test_pke.c)
```bash
# 編譯並執行
    # windows
make pke
    # linux
make lpke
```
**測試內容:**
最小模組測試 (only 數學)
-> 增加功能 -> 測試 -> 修改 (循環)
-> 最終結果測試


**預期輸出:** 
###### [1] 實際 / 標準 比較表
```
[Comparison Result mod q (7681)]
 Index  |  Actual  |  standard |   Diff
-----------------------------------------------        
      0 |      262 |         0 |   -262
      1 |     2183 |      1920 |   -263
      2 |     3471 |      3840 |    369
      3 |     6037 |      5760 |   -277
      4 |      128 |         0 |   -128
      5 |     2333 |      1920 |   -413
      6 |     4220 |      3840 |   -380
      7 |     6228 |      5760 |   -468
-----------------------------------------------        
Forecast| stand+dif|     -     |  < 500 (mod q)
```
###### [2] NTT域 乘法 測試 (邏輯已變更)
```
[Comparison Result]
Index | Original | Recovered | Status
-------------------------------------
    0 |        0 |         0 | OK
    1 |        1 |         1 | OK
    2 |        2 |         2 | OK
    3 |        3 |         3 | OK
    4 |        0 |         0 | OK
    5 |        1 |         1 | OK
    6 |        2 |         2 | OK
    7 |        3 |         3 | OK
```
-----
##### 7. KEM 最小模組擴充 (test_kem.c)
```bash
# 編譯並執行
    # windows
make kem
    # linux
make lkem
```
**測試內容:**
最小模組測試 (PKE + m/m'對照)
-> 增加功能 -> 測試 -> 修改 (循環)
-> 最終結果測試

**預期輸出:** 
###### [1] m / m' 比較
```
M  : *一串8進制 
M' : *一串8進制 
Pass
```
###### [2] kr / kr' 比較
`Kr  : *一串8進制 `
`Kr' : *一串8進制 `
Pass
###### [3] pk 比較 InGen / InEnc / InDec
`PK G-E Pass`
`PK G-D Pass`
###### [4] 解密是否成功
`V : pass`
`PASS: Encryption is deterministic.`

-----
##### 8. 密碼學模型 (PKE + KEM) 加解密 測試 (test_crypto.c)
```bash
# 編譯並執行
    # windows
make crypto
    # linux
make lcrypto
```
**測試內容:**
1. PKE / KEM 各 function 測試
2. PKE / KEM 綜合測試
3. KEM 雜訊測試
4. 壓力測試 ( 100次 KEM )

**預期輸出:** 
###### [1] PKE / KEM 各 function 測試
```
--- [Test] PKE KeyGen ---
[PASS] PKE KeyGen finished without errors.

--- [Test] PKE Encryption ---
[PASS] PKE Encryption generated non-zero ciphertext.

--- [Test] PKE Decryption ---
[PASS] PKE Decrypted message matches original

--- [Test] KEM KeyGen ---
[PASS] KEM KeyGen successful.

--- [Test] KEM Encapsulation ---
[PASS] KEM Encapsulation generated output.

--- [Test] KEM Decapsulation ---
[PASS] KEM Shared Secrets Match
```
###### [2] PKE / KEM 綜合測試
`[PASS] Decrypted message matches original`
`[PASS] Shared secrets match`
###### [3] KEM 雜訊測試
`=== Test 4: KEM Implicit Rejection (Security) ===
[PASS] Rejected invalid ciphertext (Keys do NOT match)`
###### [4] 壓力測試 ( 100次 KEM )
`[PASS] All 100 iterations successful.`
