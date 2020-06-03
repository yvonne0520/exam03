#include "mbed.h"
#include "mbed_rpc.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include <math.h>

#define UINT14_MAX        16383
// FXOS8700CQ I2C address
#define FXOS8700CQ_SLAVE_ADDR0 (0x1E<<1) // with pins SA0=0, SA1=0
#define FXOS8700CQ_SLAVE_ADDR1 (0x1D<<1) // with pins SA0=1, SA1=0
#define FXOS8700CQ_SLAVE_ADDR2 (0x1C<<1) // with pins SA0=0, SA1=1
#define FXOS8700CQ_SLAVE_ADDR3 (0x1F<<1) // with pins SA0=1, SA1=1
// FXOS8700CQ internal register addresses
#define FXOS8700Q_STATUS 0x00
#define FXOS8700Q_OUT_X_MSB 0x01
#define FXOS8700Q_OUT_Y_MSB 0x03
#define FXOS8700Q_OUT_Z_MSB 0x05
#define FXOS8700Q_M_OUT_X_MSB 0x33
#define FXOS8700Q_M_OUT_Y_MSB 0x35
#define FXOS8700Q_M_OUT_Z_MSB 0x37
#define FXOS8700Q_WHOAMI 0x0D
#define FXOS8700Q_XYZ_DATA_CFG 0x0E
#define FXOS8700Q_CTRL_REG1 0x2A
#define FXOS8700Q_M_CTRL_REG1 0x5B
#define FXOS8700Q_M_CTRL_REG2 0x5C
#define FXOS8700Q_WHOAMI_VAL 0xC7
#define PI 3.1416
I2C i2c( PTD9,PTD8);
RawSerial pc(USBTX, USBRX);
RawSerial xbee(D12, D11);

EventQueue queue(32 * EVENTS_EVENT_SIZE);
Thread t;

int m_addr = FXOS8700CQ_SLAVE_ADDR1;
void xbee_rx_interrupt(void);
void xbee_rx(void);
void reply_messange(char *xbee_reply, char *messange);
void check_addr(char *xbee_reply, char *messenger);
// accelermeter
void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len);
void FXOS8700CQ_writeRegs(uint8_t * data, int len);
void GetData(Arguments *in, Reply *out);
RPCFunction rpcFXOS8700CQ(&GetData, "GetData");
void acc(void);

int count;
float x, y, z;

int main(){
    pc.baud(9600);

    char xbee_reply[4];

    // XBee setting
    xbee.baud(9600);
    xbee.printf("+++");
    xbee_reply[0] = xbee.getc();
    xbee_reply[1] = xbee.getc();
    if(xbee_reply[0] == 'O' && xbee_reply[1] == 'K') {
      	pc.printf("enter AT mode.\r\n");
      	xbee_reply[0] = '\0';
      	xbee_reply[1] = '\0';
    }
    xbee.printf("ATMY 0x305\r\n");
    reply_messange(xbee_reply, "setting MY : 0x305");

    xbee.printf("ATDL 0x205\r\n");
    reply_messange(xbee_reply, "setting DL : 0x205");

    xbee.printf("ATID 0x1\r\n");
    reply_messange(xbee_reply, "setting PAN ID : 0x1");

    xbee.printf("ATWR\r\n");
    reply_messange(xbee_reply, "write config");

    xbee.printf("ATMY\r\n");
    check_addr(xbee_reply, "MY");

    xbee.printf("ATDL\r\n");
    check_addr(xbee_reply, "DL");

    xbee.printf("ATCN\r\n");
    reply_messange(xbee_reply, "exit AT mode");
    xbee.getc();

    // start
    pc.printf("start\r\n");
    t.start(callback(&queue, &EventQueue::dispatch_forever));

    // Setup a serial interrupt function of receiving data from xbee
    xbee.attach(xbee_rx_interrupt, Serial::RxIrq);
    acc();
}

void xbee_rx_interrupt(void)
{
    xbee.attach(NULL, Serial::RxIrq);
    queue.call(&xbee_rx);
}

void xbee_rx(void)
{
    char buf[100] = {0};
    char outbuf[100] = {0};
    while(xbee.readable()){
      	for (int i=0; ; i++) {
        	char recv = xbee.getc();
      		if (recv == '\r') {
        		break;
      		}
     	    buf[i] = recv;
    	}
    	RPC::call(buf, outbuf);
        xbee.printf("%s", outbuf);
        pc.printf("%s\r\n", outbuf);
        wait(0.1);
  	}
  	xbee.attach(xbee_rx_interrupt, Serial::RxIrq); // reattach interrupt
}

void reply_messange(char *xbee_reply, char *messange){
  	xbee_reply[0] = xbee.getc();
  	xbee_reply[1] = xbee.getc();
  	xbee_reply[2] = xbee.getc();
  	if(xbee_reply[1] == 'O' && xbee_reply[2] == 'K'){
    	pc.printf("%s\r\n", messange);
    	xbee_reply[0] = '\0';
    	xbee_reply[1] = '\0';
    	xbee_reply[2] = '\0';
  	}
}

void check_addr(char *xbee_reply, char *messenger){
  	xbee_reply[0] = xbee.getc();
  	xbee_reply[1] = xbee.getc();
  	xbee_reply[2] = xbee.getc();
  	xbee_reply[3] = xbee.getc();
  	pc.printf("%s = %c%c%c\r\n", messenger, xbee_reply[1], xbee_reply[2], xbee_reply[3]);
  	xbee_reply[0] = '\0';
  	xbee_reply[1] = '\0';
  	xbee_reply[2] = '\0';
  	xbee_reply[3] = '\0';
}
void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len) {
    char t = addr;
    i2c.write(m_addr, &t, 1, true);
    i2c.read(m_addr, (char *)data, len);
}

void FXOS8700CQ_writeRegs(uint8_t * data, int len) {
    i2c.write(m_addr, (char *)data, len);
}
void acc(void)
{
   pc.baud(115200);
   uint8_t who_am_i, value[2], res[6];
   int16_t acc16;
   float t[3];

   // Enable the FXOS8700Q
   FXOS8700CQ_readRegs( FXOS8700Q_CTRL_REG1, &value[1], 1);
   value[1] |= 0x01;
   value[0] = FXOS8700Q_CTRL_REG1;
   FXOS8700CQ_writeRegs(value, 2);

   // Get the slave address
   FXOS8700CQ_readRegs(FXOS8700Q_WHOAMI, &who_am_i, 1);

   while (true) {
    FXOS8700CQ_readRegs(FXOS8700Q_OUT_X_MSB, res, 6);

    acc16 = (res[0] << 6) | (res[1] >> 2);
    if (acc16 > UINT14_MAX/2)
        acc16 -= UINT14_MAX;
    t[0] = ((float)acc16) / 4096.0f;
    acc16 = (res[2] << 6) | (res[3] >> 2);
    if (acc16 > UINT14_MAX/2)
        acc16 -= UINT14_MAX;
    t[1] = ((float)acc16) / 4096.0f;
    acc16 = (res[4] << 6) | (res[5] >> 2);
    if (acc16 > UINT14_MAX/2)
        acc16 -= UINT14_MAX;
    t[2] = ((float)acc16) / 4096.0f;

    pc.printf("%1.4f\r\n%1.4f\r\n%1.4f\r\n", t[0], t[1], t[2]);
    count++;

    if (t[0] > 0.5 || t[0] < -0.5 || t[1] > 0.5 || t[1] < -0.5)
      wait(0.1);
    else
      wait(0.5);

  }
}

void GetData(Arguments *in, Reply *out)   {
    char outbuf[3];

    if (count > 99) {
      outbuf[0] = '9';
      outbuf[1] = '9';
      outbuf[2] = '\0';
    }
    else {
      outbuf[0] = count / 10 + '0';
      outbuf[1] = count - (count / 10) * 10 + '0';
      outbuf[2] = '\0';
    }

    count = 0;
    out->putData(outbuf);
}