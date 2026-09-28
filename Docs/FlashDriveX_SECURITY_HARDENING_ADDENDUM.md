# FlashDriveX — Security Hardening & Roadmap Addendum

เอกสารเสริมนี้รวบรวมข้อเสนอแนะเชิงลึกด้านความปลอดภัยและสถาปัตยกรรม ต่อยอดจาก `FlashDriveX_PROJECT_PLAN.md`, `TECHNICAL_SPEC.md` และ `PRODUCT_OVERVIEW.md` โดยมีเป้าหมายเพื่อปิดช่องโหว่เชิงสถาปัตยกรรมก่อนเข้าสู่ขั้นตอน Production PCB และการขอ Certification

---

## 1. Critical Gap: Blind Signing Risk

### ปัญหา
ปัจจุบันอุปกรณ์เซ็นข้อมูล (`SE050_SignDigest()`) จากค่า Transaction Hash ที่ Host App ส่งมา โดยไม่มีการแสดงผลใดๆ บนตัวอุปกรณ์เอง ลายนิ้วมือยืนยันเพียงว่า **"มีมนุษย์แตะเซนเซอร์จริง"** แต่ไม่ได้ยืนยันว่า **"มนุษย์คนนั้นเห็นและอนุมัติธุรกรรมที่ถูกต้อง"**

หาก Host Computer ติดมัลแวร์ (ซึ่งอยู่ในขอบเขต Threat Model ของโปรเจกต์นี้อยู่แล้ว) แอปพลิเคชันสามารถแสดงผล "โอน 1 ETH ให้ Alice" บนหน้าจอ ในขณะที่ส่ง Hash จริงของ "โอน 10 ETH ให้ผู้โจมตี" ไปให้ SE050 เซ็น — นี่คือช่องโหว่คลาสสิกของ Hardware Wallet ที่บั่นทอนหลักการ Zero-Trust ที่ตั้งไว้

### แนวทางแก้ไข
- **เพิ่มจอแสดงผล (OLED/E-ink)** แสดง Destination Address (หรือ Checksum) และจำนวนเงินก่อนปลดล็อกการเซ็น
- **หรือออกแบบแบบ Air-Gapped เต็มรูปแบบ**: ใช้ QR Code ในการรับ-ส่งข้อมูลธุรกรรมแทน USB (แนวทางเดียวกับ Keystone) เพื่อตัดพื้นผิวการโจมตีผ่าน USB ทั้งหมด

---

## 2. Firmware & MCU Trust Boundary

### ปัญหา
SE050 ปกป้องเฉพาะ **กุญแจ** แต่ STM32F401RE ที่รัน State Machine เป็น MCU ทั่วไป ไม่มี Secure Boot หรือ Code Signing ผู้โจมตีที่เข้าถึงฮาร์ดแวร์ได้ (หรือผ่านการอัปเดตเฟิร์มแวร์ที่เป็นอันตราย) สามารถ Reflash STM32 เพื่อปลอมสัญญาณ "ลายนิ้วมือ Match แล้ว" และสั่ง `SE050_SignDigest()` ได้ตามต้องการ

### แนวทางแก้ไข
- เปิดใช้งาน **Secure Boot / Signed Firmware Image**
- ตั้งค่า **RDP Level 2** (Readout Protection) บน Option Bytes สำหรับ Production (ไม่ใช่แค่ระดับ Dev)
- ตรวจสอบลายเซ็นดิจิทัลทุกครั้งที่มีการอัปเดตเฟิร์มแวร์ พร้อม Anti-Rollback Counter
- พิจารณาให้ SE050 เป็นผู้ตรวจสอบสัญญาณ "Match เกิดขึ้นจริง" แบบ Cryptographically Attested แทนที่จะเชื่อ GPIO/Flag จาก MCU เพียงอย่างเดียว

---

## 3. Biometric-Specific Risks

- **Liveness / Anti-Spoof Detection:** เซนเซอร์ Capacitive อย่าง FPC100026040 อาจถูกหลอกด้วยลายนิ้วมือปลอม (ซิลิโคน/เจลาติน) ได้ในบางกรณี ควรระบุค่า FAR/FRR และสถานะ Liveness Detection ของโมดูลอย่างชัดเจน หรือบันทึกเป็น Residual Risk ที่ยอมรับได้
- **Lockout / Rate-Limiting:** เพิ่มตัวนับความล้มเหลว พร้อม Exponential Backoff หรือ Hard Lockout หลังพยายามจับคู่ผิดครบ N ครั้ง เพื่อป้องกัน Brute-Force Presentation Attack
- **Backup Authentication Factor:** ลายนิ้วมืออาจใช้งานไม่ได้ในบางสถานการณ์ (นิ้วเปียก/บาดเจ็บ/เซนเซอร์เสื่อม) จำเป็นต้องมี Fallback ที่ชัดเจน เช่น PIN ผ่านช่องทางที่ปลอดภัย หรือ Recovery Flow ไม่เช่นนั้นผู้ใช้อาจถูกล็อกออกจากกุญแจของตัวเองอย่างถาวร

---

## 4. Key Backup & Recovery Strategy

ปัจจุบันเอกสารยังไม่ระบุว่าจะเกิดอะไรขึ้นหากอุปกรณ์สูญหาย ถูกขโมย หรือเสียหาย เนื่องจากกุญแจถูกออกแบบให้ Non-Exportable (ซึ่งดีต่อความปลอดภัย) จึงต้องมีคำตอบที่ชัดเจนต่อสถานการณ์นี้ ให้เลือกแนวทางใดแนวทางหนึ่งอย่างจงใจ:

- **Recovery Seed แบบ BIP-39:** สร้างระหว่างการ Enrollment ครั้งแรก แสดงครั้งเดียวให้ผู้ใช้จดบันทึก (มีข้อขัดแย้งกับหลักการ "กุญแจไม่ออกจาก Secure Element" ต้องตัดสินใจให้ชัดเจน)
- **No-Recovery Model:** ประกาศชัดเจนว่า "กุญแจตายไปพร้อมอุปกรณ์" แบบเดียวกับ HSM บางรุ่น
- **Multi-Device Enrollment / Shamir's Secret Sharing:** กระจายกุญแจสำรองผ่านอุปกรณ์ FlashDriveX หลายตัว

---

## 5. Physical & Anti-Tamper Hardening

- **Epoxy Potting** ครอบชิป SE050 และ MCU เพื่อป้องกัน Chip-off / Decapping Attack
- **Tamper-Evident Enclosure** ที่แสดงร่องรอยชัดเจนหากถูกเปิด
- **Tamper-Detect Switch** ที่ล้าง Session State ทันทีหากตัวเครื่องถูกเปิด

---

## 6. USB/Transport Channel Integrity

การเชื่อมต่อ I2C ระหว่าง STM32 และ SE050 มีการเข้ารหัสแล้ว แต่ช่องทาง STM32 ↔ Host ผ่าน USB (เช่น HID) ยังไม่ระบุการป้องกัน หาก Driver Stack บน Host ถูกยึดครอง อาจมีการปลอมแปลง Digest ก่อนส่งถึง SE050 ได้ (เกี่ยวโยงกับข้อ 1 — การแสดงผลบนอุปกรณ์คือทางแก้หลัก แต่ควรเพิ่ม Transport Integrity Check ร่วมด้วย)

---

## 7. Additional Roadmap Considerations

- **Watchdog Timer** ในเฟิร์มแวร์ เพื่อกู้คืนจากอาการค้างกลางธุรกรรม
- **Unit / Hardware-in-Loop Test Plan** สำหรับ Phase 3-4 ที่ปัจจุบันยังเป็นแค่ Checkbox โดยไม่มีเกณฑ์ทดสอบหรือ Coverage ที่ชัดเจน
- **FIDO Alliance / Common Criteria Certification** สำหรับผลิตภัณฑ์ทั้งระบบ ไม่ใช่แค่ตัวชิป SE050 — สำคัญมากต่อความน่าเชื่อถือระดับองค์กร
- **NFC Interface** ที่ระบุไว้เป็น Future Roadmap ควรพิจารณาเลื่อนลำดับความสำคัญขึ้นมา เนื่องจากช่วยให้เซ็นผ่านมือถือได้โดยไม่ต้องพึ่ง USB เลย

---

## Phase 6: Security Hardening & Trust Verification

- [ ] ออกแบบและเลือกจอแสดงผล (OLED/E-ink) สำหรับ Transaction Verification ก่อนเซ็น
- [ ] ประเมินความเป็นไปได้ของสถาปัตยกรรม Air-Gapped ผ่าน QR Code แทน USB
- [ ] เปิดใช้งาน Secure Boot และ RDP Level 2 บน STM32F401RE
- [ ] ออกแบบระบบตรวจสอบลายเซ็นเฟิร์มแวร์ (Firmware Signing) พร้อม Anti-Rollback Counter
- [ ] ทดสอบและบันทึกค่า FAR/FRR ของเซนเซอร์ FPC100026040 พร้อมประเมิน Liveness Detection
- [ ] Implement Lockout Counter พร้อม Exponential Backoff บนความล้มเหลวของการจับคู่ลายนิ้วมือ
- [ ] กำหนด Fallback Authentication Path (PIN / Recovery Flow) สำหรับกรณีไบโอเมตริกซ์ใช้งานไม่ได้
- [ ] ตัดสินใจและเอกสาร Key Recovery Strategy (BIP-39 Seed / No-Recovery / Multi-Device Sharding)
- [ ] ออกแบบ Physical Anti-Tamper: Epoxy Potting, Tamper-Evident Enclosure, Tamper-Detect Switch
- [ ] เพิ่ม Transport Integrity Check บนช่องทาง USB ระหว่าง Host และ STM32
- [ ] Implement Watchdog Timer ในเฟิร์มแวร์เพื่อกู้คืนจากอาการค้าง
- [ ] จัดทำ Hardware-in-Loop Test Plan พร้อมเกณฑ์ Test Coverage สำหรับ Phase 3-4
- [ ] ศึกษาแนวทางขอ FIDO Alliance / Common Criteria Certification ระดับผลิตภัณฑ์
- [ ] จัดลำดับความสำคัญใหม่ให้ NFC Interface เลื่อนขึ้นมาก่อนใน Roadmap
