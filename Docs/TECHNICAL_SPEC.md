# FlashDriveX — Deep-Dive Technical Architecture Specification

**FlashDriveX** คือ **Biometric-Authenticated Secure Hardware Security Module (HSM) / Hardware Wallet** แบบพกพา ออกแบบสถาปัตยกรรมความปลอดภัยขั้นสูงด้วย **Zero-Trust & Air-Gapped Key Operations** สำหรับการลงลายเซ็นดิจิทัล (Digital Signature) และบริหารจัดการ Cryptographic Keys

---

## 1. Hardware Chipset & Component Stack

| Component Role | Part Number / Module | Key Technical Specifications | Primary Protocol |
| :--- | :--- | :--- | :--- |
| **Main MCU** | **STMicroelectronics STM32F401RE** | 32-bit ARM® Cortex®-M4 with FPU, 84 MHz, 512 KB Flash, 96 KB SRAM | Internal System Bus |
| **Secure Element (SE)** | **NXP EdgeLock® SE050E** | Common Criteria EAL 6+ certified, Hardware TRNG, Crypto Co-processor | I2C (T=1 over I2C Protocol) |
| **Biometric Sensor** | **Fingerprints FPC100026040** | Capacitive Touch Sensor ($8 \times 8\text{ mm}$), Active Capacitance Sensing | UART (Packet Command Set) |

---

## 2. Firmware & Software Stack Architecture

### 2.1 Low-Level Architecture & Middleware
* **Core Runtime:** STM32 HAL (Hardware Abstraction Layer) Bare-Metal State Machine Architecture
* **Cryptographic Middleware:** NXP Plug & Trust Middleware (`sss` Host Library API) สำหรับควบคุม SE050
* **Development & Toolchain:**
  * **IDE/Environment:** VSCodium (Open Source)
  * **Indexing Engine:** `clangd` (Configured with GNU ARM Embedded Toolchain Sysroot)
  * **Cross-Compiler:** `arm-none-eabi-gcc` (C11 / GNU extensions)

### 2.2 Security Architecture (Isolated Crypto Isolation)
* **Key Generation:** Private Keys ถูกสร้างด้วย Hardware-based True Random Number Generator (TRNG) ภายใน NXP SE050E เท่านั้น
* **Key Non-Exportability:** Private Key ไม่ถูกโหลดหรือส่งผ่านไปยัง RAM ของ STM32 MCU แม้กระทั่งชั่วคราว
* **On-Chip Biometric Matching (Match-on-Chip):** การเปรียบเทียบลายนิ้วมือทำภายในโมดูล Biometric โดยตรง แล้วส่งสัญลักษณ์ Authentication Flag ผ่าน Secure Channel

---

## 3. Cryptographic Capabilities & Algorithms

NXP SE050E ฮาร์ดแวร์ co-processor รองรับ Cryptographic Primitives ครอบคลุมระบบสถาปัตยกรรมบล็อกเชนและ Web3 หลักทั้งหมด:

| Algorithm / Curve | Standard Support | Web3 / Enterprise Application Use-Case |
| :--- | :--- | :--- |
| **Secp256k1** | ECDSA SHA-256 | Bitcoin (BTC), Ethereum (ETH), EVM Compatible Chains |
| **Ed25519** | EdDSA | Solana (SOL), Cardano (ADA), Near Protocol, SSH Keys |
| **NIST P-256 (secp256r1)** | ECDSA SHA-256 | WebAuthn, FIDO2, Passkeys, Enterprise TLS/PKI |
| **AES-128 / AES-256** | CBC, GCM Mode | Symmetric Storage Encryption (Secured USB Flash Drive Logic) |

---

## 4. Key Security & Functional Features

1. **Biometric Gatekeeper (Match-on-Chip Gating):**
   * ลายเซ็นดิจิทัล (`sss_asymmetric_sign_digest`) จะถูกบล็อกการประมวลผลทันที หากยังไม่ผ่านการอนุมัติลายนิ้วมือถูกต้อง
2. **Session Security & Encrypted Transport:**
   * การสื่อสารระหว่าง STM32 MCU และ NXP SE050 ผ่าน I2C ใช้การเข้ารหัส Session (Encrypted Channel) ป้องกันการดักจับขาสัญญาณ Hardware Bus Sniffing
3. **Anti-Tamper & Physical Attack Protection:**
   * ตัวชิป SE050E มีระบบป้องกันการโจมตีทางกายภาพระดับเกรดทหาร (Side-Channel Attack Protection, Fault Injection Countermeasures, DPA/SPA Shielding)
4. **Multi-Protocol Interface (Roadmap Ready):**
   * ออกแบบโครงสร้างรองรับการเชื่อมต่อคู่ผ่าน USB-C (Composite Device: HID / Mass Storage) และ NFC Interface สำหรับอุปกรณ์สมาร์ตโฟน

---

## 5. End-to-End Execution Flow (Technical Lifecycle)

```text
[Host Application] ──► Transmits Transaction Digest (32-byte Hash) via USB/NFC
                             │
[STM32F401RE MCU]   ──► Triggers Biometric Sensor & Enters STATE_WAIT_FINGERPRINT
                             │
[FPC Sensor]        ──► Captures Touch ──► Runs Match-on-Chip Algorithm
                             │
                             ├─► [MATCH FAIL] ──► Returns Error Code to Host
                             │
                             └─► [MATCH SUCCESS] ──► Sends OK Token to MCU
                                                            │
[STM32F401RE MCU]   ──► Invokes `SE050_SignDigest(SE050_KEY_ID_SECP256K1, Digest)`
                                                            │
[NXP SE050E]        ──► Generates ECDSA/EdDSA Signature internally via SE Hardware
                                                            │
[Host Application]  ◄── Receives 64-byte Signature (Private Key Never Exposed)