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

/*****************************************************************
 * STM32 + EC200U MODEM FLOW (Standard Procedure) (DMA + FSM, PRODUCTION READY)
 *****************************************************************/

/* ================= MODEM POWER ON =================
 * PWRKEY (PA5):
 * HIGH → 500ms → LOW (1.2s) → HIGH
 * Correct pulse length is mandatory for EC200U boot.
 */

/* ================= UART DMA RX =================
 * Zero-CPU reception using DMA + IDLE line detection
 */
HAL_UART_Receive_DMA(&huart1, dma_rx_buf, RX_BUF_SIZE);

/* ================= UART IDLE ISR ================= */
void USART1_IRQHandler(void)
{
    if (UART_IDLE_DETECTED)
    {
        CLEAR_IDLE_FLAG();
        rx_len = RX_BUF_SIZE - DMA_NDTR;                                 // Exact bytes received
        copy_and_trim(modem_resp, dma_rx_buf, rx_len);
        modem_data_ready = 1;                                            // Signal main()
        HAL_UART_Receive_DMA(&huart1, dma_rx_buf, RX_BUF_SIZE);          // Restart
    }
    HAL_UART_IRQHandler(&huart1);
}

/* ===================== 4. FINITE STATE MACHINE =====================

AT_CHECK  -> OK
ECHO_OFF  -> OK
SIM_CHECK -> +CPIN: READY
SIM_VOLT  -> +QUSIM
NET_CHECK -> +CREG: 1 or 5
SMS_CMD   -> OK
SMS_BODY  -> +CMGS
CALLING   -> OK / NO CARRIER
IDLE
*/

/* ================= MODEM FSM ================= */
switch (modem_state)
{
case AT_CHECK:
    send("AT\r\n");
    if (OK()) modem_state = ECHO_OFF;
    break;

case ECHO_OFF:
    send("ATE0\r\n");
    if (OK()) modem_state = SIM_CHECK;
    break;

case SIM_CHECK:
    send("AT+CPIN?\r\n");
    if (RESP("+CPIN: READY")) modem_state = NET_CHECK;
    break;

case NET_CHECK:
    send("AT+CREG?\r\n");
    if (RESP("+CREG: 0,1") || RESP("+CREG: 0,5"))
        modem_state = SMS_SEND;
    break;

case SMS_SEND:
    send("AT+CMGF=1\r\n");
    send("AT+CMGS=\"+91XXXXXXXXXX\"\r\n");
    send("Hello from STM32!\x1A");   // Ctrl-Z = SMS end
    if (RESP("+CMGS")) modem_state = IDLE;
    break;

case IDLE:
    /* System ready */
    break;
}

/* ================= ERROR RECOVERY =================
 * +CME ERROR → AT+CFUN=1,1 (Modem reset)
 * Retry previous FSM state
 */
if (RESP("+CME ERROR"))
{
    send("AT+CFUN=1,1\r\n");
    modem_state = PREV_STATE;
}

/* ================= DESIGN SUMMARY =================
 * DMA + IDLE   : Zero CPU overhead
 * FSM          : Deterministic control
 * Exact parse  : Robust AT handling
 * Auto-recover : Self-healing system
 * RAM usage    : ~2 KB (RX + response buffers)
 */
