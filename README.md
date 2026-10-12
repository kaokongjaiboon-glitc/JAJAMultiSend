# JAJA Multi Send (VST3)

ปลั๊กอินวางบน **Master** ใน Ableton — เสียงผ่านไปตามปกติ และส่งสำเนาไปยัง **อุปกรณ์เสียงอื่นได้พร้อมกัน 3 ทาง** (เช่น CABLE Input สำหรับ TikTok, หูฟัง, อุปกรณ์อัดเสียง)

- ปรับ On/Off และ Gain แยกแต่ละทาง
- ปรับสัญญาณนาฬิกาให้อัตโนมัติ (resample แบบ adaptive) กันเสียงแตก/buffer ล้นเมื่ออุปกรณ์ clock ไม่ตรงกัน
- Buffer ปรับได้ 20–300 ms (ค่าเริ่มต้น 60 ms) — ยิ่งต่ำยิ่งดีเลย์น้อย ยิ่งสูงยิ่งเสถียร
- จำอุปกรณ์ที่เลือกไว้ในโปรเจกต์ Ableton

## วิธีได้ไฟล์ .vst3 (ไม่ต้องลงคอมไพเลอร์)

1. สร้าง repository บน GitHub แล้วอัปโหลดโฟลเดอร์นี้ทั้งหมด (รวมโฟลเดอร์ `.github`)
2. ไปที่แท็บ **Actions** → รอ workflow "Build VST3" ทำงานเสร็จ (ประมาณ 10–15 นาที)
3. เข้าไปที่ run ล่าสุด ดาวน์โหลด **JAJA-Multi-Send-Windows** จากส่วน Artifacts

### Build เองบนเครื่อง Windows
ต้องมี Visual Studio 2022 (Desktop development with C++) และ CMake

```
cmake -B build
cmake --build build --config Release
```
ไฟล์อยู่ที่ `build/JAJAMultiSend_artefacts/Release/VST3/JAJA Multi Send.vst3`

## ติดตั้ง
1. คัดลอกโฟลเดอร์ `JAJA Multi Send.vst3` ไปที่ `C:\Program Files\Common Files\VST3`
2. Ableton → Preferences → Plug-ins → เปิด **Use VST3 Plug-in System Folders** แล้วกด Rescan

## ตั้งค่าสำหรับ live TikTok
1. ติดตั้ง **VB-Cable** (ฟรี)
2. ใน Ableton ตั้ง Output Device ตามปกติ (การ์ดเสียง/หูฟังที่ใช้อยู่) — **ไม่ต้องเปลี่ยนเป็น CABLE**
3. ใส่ JAJA Multi Send บน **Master** (ท้ายสุดของ chain)
4. Send 1: เลือก **CABLE Input** → เปิด On
5. ใน TikTok LIVE Studio ตั้งไมโครโฟนเป็น **CABLE Output**
6. ถ้าต้องพูดสดด้วย ใช้ VoiceMeeter มิกซ์ไมค์จริงกับ CABLE Output ก่อนเข้า TikTok

## ข้อควรรู้
- อย่าเลือก device เดียวกับที่ Ableton ใช้ output อยู่ (ถ้าเป็น ASIO เปิดซ้ำไม่ได้)
- ใช้ WASAPI (โหมด shared) จึงมีดีเลย์เพิ่มประมาณเท่าค่า Buffer แต่เสียงที่ได้ยินในหูฟังไม่ได้รับผลกระทบ
- รองรับ Stereo เท่านั้น
- ถ้า sample rate ของอุปกรณ์ไม่ตรงกับ Ableton ปลั๊กอินแปลงให้เอง
