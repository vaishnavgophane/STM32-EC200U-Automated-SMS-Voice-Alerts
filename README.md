## 🧑‍💻 Author

**Vaishnav Gophane**  
Embedded Firmware & IoT Developer
<br>
Pune, India.

📫 **Connect:** [Gmail](mailto:mr.vaishnavgophane@gmail.com) • [GitHub](https://github.com/vaishnavgophane) • [LinkedIn](https://www.linkedin.com/in/vaishnav-gophane-417686284/)

---

# STM32 Nucleo-F411RE + Quectel EC200U LTE IoT Gateway

---

**Automated SMS + Voice Call IoT Gateway using STM32 DMA + Quectel EC200U LTE Cat1 Module**

Demonstrates **production-ready** cellular communication with:
- ✅ **DMA + IDLE Line Detection** UART for robust AT commands
- ✅ **Automatic SMS** transmission  
- ✅ **Voice calling** with auto-hangup
- ✅ **Network/SIM detection** with error recovery
- ✅ **PWRKEY control** for reliable modem boot

## 🎯 Features
| Feature | Status |
|---------|--------|
| DMA UART RX (USART1) | ✅ Production Ready |
| Debug UART TX (USART2→PuTTY) | ✅ |
| EC200U PWRKEY control (PA5) | ✅ |
| SIM detection (+CPIN?) | ✅ |
| Network registration | ✅ |
| SMS transmission (AT+CMGS) | ✅ |
| Voice calling (ATD) | ✅ |
| Error handling + retries | ✅ |

## UART2 for Debug Messages Printing
**PuTTY**: ST-LINK VCP @ 115200 baud


## 🚀 Usage

1. **Wire EC200U** per diagram above
2. **Insert Airtel SIM** (or any 4G SIM)
3. **Flash firmware** via STM32CubeIDE/ST-Link
4. **Open PuTTY** → ST-LINK COM @ 115200
5. **Watch automation**:

