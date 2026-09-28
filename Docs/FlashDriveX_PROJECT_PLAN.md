# FlashDriveX — Master Project Documentation & Implementation Plan

เอกสารฉบับสมบูรณ์สำหรับ **FlashDriveX**: บิโอเมตริกซ์ฮาร์ดแวร์วอลเล็ต / เซเคียวร์ยูเอสบี (Biometric Hardware Wallet & Secure USB) ระดับพกพา พัฒนาด้วยสถาปัตยกรรม Zero-Trust ร่วมกับชิป NXP EdgeLock SE050E และโมดูลสแกนลายนิ้วมือ FPC100026040 ประมวลผลหลักบน STM32F401RE

---

## 1. Executive Product Overview

### 1.1 The Core Concept (สินค้านี้คืออะไร?)

FlashDriveX เปรียบเสมือน **"ตู้เซฟพกพาส่วนตัว"** สำหรับโลกดิจิทัล ที่ใช้ยืนยันตัวตนด้วย **ลายนิ้วมือของคุณเอง** เพื่ออนุมัติการทำธุรกรรมหรือเข้าถึงข้อมูล ถูกออกแบบมาเพื่อตัดเส้นทางแฮกเกอร์ด้วยการประมวลผลและเก็บข้อมูลสำคัญไว้ใน **ชิปฮาร์ดแวร์ความปลอดภัยสูงที่แยกอิสระจากภายนอกโดยสิ้นเชิง**

### 1.2 Key Value Propositions

- **True Biometric Hardware Security:** ปลดล็อกด้วยลายนิ้วมือผ่านเซนเซอร์สแกนตรงบนตัวอุปกรณ์ (Match-on-Chip) ไม่มีการส่งข้อมูลลายนิ้วมือออกไปภายนอก
- **Bank-Grade Isolation:** กุญแจเข้ารหัสลับ (Private Keys) ทั้งหมดถูกสร้างและเก็บไว้ภายใน **Secure Element Chip (CC EAL 6+)** ซึ่งเป็นเกรดเดียวกับบัตรเครดิตและหนังสือเดินทางอิเล็กทรอนิกส์
- **Zero-Trust Architecture:** แม้คอมพิวเตอร์หรือโทรศัพท์ที่นำไปเสียบใช้งานจะติดไวรัสหรือมัลแวร์ แฮกเกอร์ก็ไม่สามารถขโมย Private Key หรือสั่งทำธุรกรรมปลอมได้หากไม่มีการแตะลายนิ้วมือจริง
- **Multi-Chain Compatibility:** รองรับอัลกอริทึมการเข้ารหัสหลากหลาย ($secp256k1$ สำหรับ Bitcoin/Ethereum, $Ed25519$ สำหรับ Solana/Cardano และ $NIST P-256$)

### 1.3 User Journey

```text
[เสียบ FlashDriveX] ──► [กดทำรายการบน App] ──► [แตะนิ้วมือบนอุปกรณ์] ──► [อนุมัติสำเร็จ 100%]
```

---

## 2. Deep-Dive Technical Architecture Specification

### 2.1 Hardware Chipset & Component Stack

| Component Role | Part Number / Module | Key Technical Specifications | Primary Protocol |
|---|---|---|---|
| Main MCU | STMicroelectronics STM32F401RE | 32-bit ARM® Cortex®-M4 with FPU, 84 MHz, 512 KB Flash, 96 KB SRAM | Internal System Bus |
| Secure Element (SE) | NXP EdgeLock® SE050E | Common Criteria EAL 6+ certified, Hardware TRNG, Crypto Co-processor | I2C (T=1 over I2C Protocol) |
| Biometric Sensor | Fingerprints FPC100026040 | Capacitive Touch Sensor (8 × 8 mm), Active Capacitance Sensing | UART (Packet Command Set) |

### 2.2 Firmware & Toolchain Stack

- **Core Runtime:** STM32 HAL Bare-Metal State Machine Architecture
- **Cryptographic Middleware:** NXP Plug & Trust Middleware (sss Host Library API)
- **Development Environment:**
  - IDE/Editor: VSCodium (Open Source)
  - Indexing Engine: clangd (Configured with GNU ARM Embedded Toolchain Sysroot)
  - Compiler: arm-none-eabi-gcc (C11 / GNU extensions)

### 2.3 Cryptographic Capabilities

| Algorithm / Curve | Standard Support | Target Application / Web3 Use-Case |
|---|---|---|
| Secp256k1 | ECDSA SHA-256 | Bitcoin (BTC), Ethereum (ETH), EVM Compatible Chains |
| Ed25519 | EdDSA | Solana (SOL), Cardano (ADA), Near Protocol, SSH Keys |
| NIST P-256 (secp256r1) | ECDSA SHA-256 | WebAuthn, FIDO2, Passkeys, Enterprise PKI |
| AES-128 / AES-256 | CBC, GCM Mode | Symmetric Storage Encryption (Secure USB Flash Logic) |

---

## 3. Firmware Structure & Directory Map

```text
FlashDriveX_PoC/
├── .clangd                     # Clangd configuration (Sysroot & Target Flags)
├── PROJECT_PLAN.md             # เอกสารแผนงานและข้อมูลสถาปัตยกรรมรวม (ไฟล์นี้)
├── Inc/
│   ├── main.h
│   ├── se050_driver.h          # Header สำหรับ NXP SSS API Wrapper
│   └── fingerprint_driver.h    # Header สำหรับ FPC Sensor UART Communication
└── Src/
    ├── main.c                  # State Machine Logic หลัก
    ├── se050_driver.c          # NXP SE050 ECDSA Sign & Key Management Logic
    └── fingerprint_driver.c    # FPC Sensor Packet Parser & Match Handler
```

---

## 4. Finite State Machine (FSM) Execution Flow

```text
                       ┌─────────────────────────┐
                       │       STATE_IDLE        │
                       └────────────┬────────────┘
                                    │ (App Request / Power On)
                                    ▼
                       ┌─────────────────────────┐
                       │  STATE_WAIT_FINGERPRINT │
                       └────────────┬────────────┘
                                    │
                  ┌─────────────────┴─────────────────┐
                  │                                    │
         (Match Success)                        (Match Fail / Timeout)
                  ▼                                    ▼
   ┌─────────────────────────────┐         ┌─────────────────────┐
   │      STATE_CRYPTO_SIGN      │         │     STATE_ERROR      │
   │ (NXP SE050 ECDSA Signature) │         └──────────┬───────────┘
   └──────────────┬──────────────┘                    │
                  │                                    │
                  └─────────────────┬──────────────────┘
                                    │
                                    ▼
                       ┌─────────────────────────┐
                       │       STATE_IDLE        │
                       └─────────────────────────┘
```

---

## 5. Development Roadmap & Phased Execution

### Phase 1: Environment & Architecture Setup (Status: COMPLETED)

- [x] จัดตั้งโครงการและโครงสร้างไดเรกทอรีบน VSCodium
- [x] ตั้งค่า .clangd ร่วมกับ GNU ARM Toolchain Sysroot (แก้ไขปัญหา stdio.h / size_t error)
- [x] เขียน State Machine เบื้องต้นลงใน Src/main.c (STATE_IDLE, WAIT_FINGERPRINT, CRYPTO_SIGN, ERROR)
- [x] สร้าง Abstraction Layer: se050_driver.c/h และ fingerprint_driver.c/h
- [x] รวบรวมเอกสารสถาปัตยกรรมเข้าสู่ PROJECT_PLAN.md

### Phase 2: Hardware Integration & Peripheral Setup (Status: PENDING HARDWARE)

- [ ] ต่อสายฮาร์ดแวร์บอร์ด NUCLEO-F401RE เข้ากับ NXP SE050 (I2C1) และ FPC Sensor (USART2)
- [ ] กำหนดค่า Pin Mapping และเปิดใช้งานพอร์ต I2C1 และ USART2 ผ่าน STM32CubeMX / .ioc
- [ ] ทดสอบการเชื่อมต่อ Bus (I2C Probe / UART Ping Test)

### Phase 3: Driver Integration & Biometric Execution

- [ ] ปรับปรุง fingerprint_driver.c ให้รองรับ Real-time Receive Interrupt (UART Ring Buffer)
- [ ] ทดสอบขั้นตอนการบันทึกลายนิ้วมือ (Enrollment) และการตรวจสอบ Match-on-Chip
- [ ] เชื่อมโยง Match Flag จาก FPC Sensor เพื่อเป็นเงื่อนไขในการปลดล็อก SE050_SignDigest()

### Phase 4: Cryptographic Implementation & Security Hardening

- [ ] ทดสอบการสร้าง Key Pair แบบ Hardware-based TRNG บน NXP SE050 ($secp256k1$ / $Ed25519$)
- [ ] เปิดใช้งาน Encrypted Session (T=1 over I2C) ระหว่าง STM32F401RE และ NXP SE050E
- [ ] ทดสอบการลงลายเซ็น Transaction Hash (ECDSA SHA-256) และส่งผลลัพธ์ผ่าน Console/USB

### Phase 5: Host Companion Application & Final PoC

- [ ] พัฒนา Host CLI / GUI Application ขนาดเล็กเพื่อส่ง Transaction Hash ให้ฮาร์ดแวร์ Sign
- [ ] ทดสอบ End-to-End Cycle: App Request → Touch Fingerprint → SE050 Sign → App Verify Signature
- [ ] จัดทำรายงานสรุปผล PoC และเตรียมงานทำ Custom PCB สำหรับ Production

---

## 6. Development Troubleshooting & Environment Reference

### 6.1 สาเหตุของปัญหา 'stdio.h' file not found & size_t undeclared

- **Target `--target=arm-none-eabi` ไม่มี Sysroot:** เมื่อเปิดโหมด Bare-Metal Cross-Compilation ใน clangd ตัวระบบจะไม่ค้นหา C Standard Library บน Host (`/usr/include`)
- **การใช้ตัวแปร `${workspaceFolder}`:** clangd ไม่รองรับ syntax ตัวแปร `${workspaceFolder}` ของ VS Code ส่งผลให้ Include Path ล้มเหลวทั้งหมด
- **Cascading Errors:** เมื่อ `<stdio.h>` หรือ `<stddef.h>` โหลดไม่สำเร็จ จะส่งผลให้ชนิดข้อมูล `size_t` ไม่ถูกประกาศ

### 6.2 .clangd Configuration Reference (Fix)

```yaml
CompileFlags:
  Add:
    - "--target=arm-none-eabi"
    - "-mcpu=cortex-m4"
    - "-mthumb"
    - "-DSTM32F401xE"
    - "-DUSE_HAL_DRIVER"

    # ระบุ Sysroot และ Standard Library Headers (Absolute Path)
    - "--sysroot=/home/fidaxc/st/stm32cubeide_2.2.0/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.linux64_1.0.100.202602081740/tools/arm-none-eabi"
    - "-isystem"
    - "/home/fidaxc/st/stm32cubeide_2.2.0/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.linux64_1.0.100.202602081740/tools/lib/gcc/arm-none-eabi/14.3.1/include"

    # Include Path ทั้งหมดในโปรเจกต์
    - "-I/home/fidaxc/STM32CubeIDE/workspace_2.2.0/FlashDriveX_PoC/Inc"
    - "-I/home/fidaxc/STM32CubeIDE/workspace_2.2.0/FlashDriveX_PoC/Drivers/STM32F4xx_HAL_Driver/Inc"
    - "-I/home/fidaxc/STM32CubeIDE/workspace_2.2.0/FlashDriveX_PoC/Drivers/CMSIS/Device/ST/STM32F4xx/Include"
    - "-I/home/fidaxc/STM32CubeIDE/workspace_2.2.0/FlashDriveX_PoC/Drivers/CMSIS/Include"
    - "-I/home/fidaxc/STM32CubeIDE/workspace_2.2.0/FlashDriveX_PoC/Middlewares/NXP_PlugAndTrust"
    - "-I/home/fidaxc/STM32CubeIDE/workspace_2.2.0/FlashDriveX_PoC/Middlewares/NXP_PlugAndTrust/sss/inc"
    - "-I/home/fidaxc/STM32CubeIDE/workspace_2.2.0/FlashDriveX_PoC/Middlewares/NXP_PlugAndTrust/hostlib/hostLib/inc"

  Remove:
    - "-mabi=*"
```
