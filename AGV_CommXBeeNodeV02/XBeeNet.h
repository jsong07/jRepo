#ifndef XBeeNet_h
#define XBeeNet_h

#include <XBee.h>


class XBEENWK {

  public:
    int statusLed = 13;
    int errorLed = 13;

    static const int payloadSize = 16;
    uint8_t payload[payloadSize];

    enum StatusTxCode {
      STATUS_TX_OK = 0,
      STATUS_TX_FAIL = 1,
      STATUS_ERROR_PACKET = 2,
      STATUS_XBEE_NOSTATUS = 3
    };

    enum StatusRxCode {
      STATUS_NO_PACKET = 0,
      STATUS_RX_OK = 1,
      STATUS_MODEM_RSP = 2,
      STATUS_UNEXPECTED = 3,
      STATUS_READ_ERR = 4
    };

    // A struct used for passing the rx result
    typedef struct {
      ZBRxResponse rx;
    } RxResult;

    //member variable
    RxResult rxResultXbee;

    XBEENWK(uint32_t addressDH, uint32_t addressDL);
    XBEENWK(uint32_t addressDH, uint32_t addressDL, Stream &serial);
    void flashLed(int pin, int times, int wait) ;

    void initXBee() ;
    int TransmitData(uint8_t *data) ;
    byte ReceiveData();

    /**
   * Specify the serial port.  Only relevant for Arduinos that support multiple serial ports (e.g. Mega)
   */
    void setSerial(Stream &serial);

  protected:
    uint32_t _addressDH;
    uint32_t _addressDL;

  private:
    Stream* _serial;

};
#endif
