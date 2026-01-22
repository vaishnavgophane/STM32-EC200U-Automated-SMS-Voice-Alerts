## 🧑‍💻 Author

**Vaishnav Gophane**  
Embedded Firmware & IoT Developer
<br>
Pune, India.

📫 **Connect:** [Gmail](mailto:mr.vaishnavgophane@gmail.com) • [GitHub](https://github.com/vaishnavgophane) • [LinkedIn](https://www.linkedin.com/in/vaishnav-gophane-417686284/)

---

# STM32 Nucleo-F411RE + Quectel EC200U LTE IoT Gateway


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
| DMA UART RX (USART1) | ✅ |
| Debug UART TX (USART2→PuTTY) | ✅ |
| EC200U PWRKEY control (PA5) | ✅ |
| SIM detection (+CPIN?) | ✅ |
| Network registration | ✅ |
| SMS transmission (AT+CMGS) | ✅ |
| Voice calling (ATD) | ✅ |
| Error handling + retries | ✅ |

## 🚀 Usage

1. **Wire EC200U** per diagram above
2. **Insert Airtel SIM** (or any 4G SIM)
3. **Flash firmware** via STM32CubeIDE/ST-Link
4. **Open PuTTY** → ST-LINK COM @ 115200
5. **Watch automation**:

---
# STM32 + EC200U MODEM FLOW (Standard Procedure) (DMA + FSM, PRODUCTION READY)

## MODEM POWER ON SEQUENCE
    PWRKEY (PA5):
    HIGH → 500ms → LOW (1.2s) → HIGH
    Correct pulse length is mandatory for EC200U boot.

## UART DMA RX 
* Zero-CPU reception using DMA + IDLE line detection
    ```c
    HAL_UART_Receive_DMA(&huart1, dma_rx_buf, RX_BUF_SIZE);
    ```
    
## UART IDLE ISR 
```c
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
```

## Finite State Machine (FSM)

| State       | AT Command / Action | Expected Response            | Description                          |
|------------|---------------------|------------------------------|--------------------------------------|
| AT_CHECK   | `AT`                | `OK`                         | Check modem communication             |
| ECHO_OFF  | `ATE0`              | `OK`                         | Disable command echo                  |
| SIM_CHECK | `AT+CPIN?`           | `+CPIN: READY`               | Verify SIM card status                |
| SIM_VOLT  | `AT+QUSIM?`          | `+QUSIM`                     | Check SIM voltage / presence          |
| NET_CHECK | `AT+CREG?`           | `+CREG: 1` or `+CREG: 5`     | Network registration status           |
| SMS_CMD   | `AT+CMGS`            | `OK`                         | Initiate SMS send command             |
| SMS_BODY  | SMS Payload          | `+CMGS`                      | SMS sent confirmation                 |
| CALLING   | `ATD<number>;`       | `OK` / `NO CARRIER`          | Call status                           |
| CALL_HANGUP    | `ATH`       | `OK`          | Auto Hang up call               |
| IDLE      | —                   | —                            | Waiting / standby state               |


## MODEM FSM
```c
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
    send("Hello from STM32!\x1A");      // Ctrl-Z
    if (RESP("+CMGS")) modem_state = CALLING;
    break;

case CALLING:
    send("ATD+91XXXXXXXXXX;\r\n");      // Voice call
    if (RESP("OK") || RESP("NO CARRIER"))
        modem_state = CALL_HANGUP;
    break;

case CALL_HANGUP:
    send("ATH\r\n");                     // Hangup
    if (RESP("OK")) modem_state = IDLE;
    break;

case IDLE:
    // System ready
    break;
}
```

## ERROR RECOVERY
+CME ERROR → AT+CFUN=1,1 (Modem reset)
   * Retry previous FSM state
 ```c
if (RESP("+CME ERROR"))
{
    send("AT+CFUN=1,1\r\n");
    modem_state = PREV_STATE;
}
```
## Design Summary

| Feature        | Description                              |
|---------------|------------------------------------------|
| DMA + IDLE    | Zero CPU overhead during UART reception  |
| FSM           | Deterministic and predictable control    |
| Exact Parse   | Robust and reliable AT command handling  |
| Auto-recover  | Self-healing system on errors or faults  |
| RAM Usage     | ~2 KB (RX buffer + response buffers)     |

