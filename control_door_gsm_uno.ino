#include "GSM_MQTT.h"
#include <SoftwareSerial.h>

#define DEV_ID "z1868881605"                  //设备ID号
String MQTT_HOST = "106.14.26.130";           //MQTT服务器IP
String MQTT_PORT = "1995";                    //MQTT服务器端口号

#ifdef DE_BUG    //   在GSM_MQTT.h 中启用
SoftwareSerial mySerial(11, 12); // 软件串口 D11 -> RX, D12 -> TX      
#endif
 
const int led = 13;       //led引脚定义D13
const int locked = 6;     //继电器引脚定义D6
const int gsm_reset = 2;  //GSM模块复位引脚定义D2
char tmp[64]={0};         //消息缓存数组
char csq[4]={0};          //信号强度

GSM_MQTT MQTT(20);

void GSM_MQTT::AutoConnect(void)                  //mqtt连接请求
{
  memset( tmp,0,sizeof(tmp) );
  sprintf(tmp,"{\"devId\":\"%s\",\"state\":0}",DEV_ID);
  connect(DEV_ID, 1, 1, "zx_admin", "ZxNumber1", 1, 1, 1, 1, "zx/door/lastword",tmp);  //mqtt连接指定用户名、密码、遗言内容和遗言发送的频道
}

void GSM_MQTT::OnConnect(void)                     //连接请求成功后
{
    digitalWrite(led,HIGH);   //上线指示灯点亮
    
    memset( tmp,0,sizeof(tmp) );
    sprintf(tmp,"zx/door/opt/%s",DEV_ID);
    subscribe(0, _generateMessageID(), tmp, 1);
    
    memset( tmp,0,sizeof(tmp) );
    sprintf(tmp,"{\"devId\":\"%s\",\"state\":1,\"signal\":%s}",DEV_ID,csq);
    publish(0, 1, 0, _generateMessageID(), "zx/door/firstword",tmp);
}

void GSM_MQTT::OnMessage(char *Topic, int TopicLength, char *Message, int MessageLength)  //当收到消息时被调用
{
  if( !strcmp(Message,"{\"optcode\":1}") )
  {
      digitalWrite(locked,HIGH);    //开锁
      delay(1000);
      digitalWrite(locked,LOW);
  }
  else if( !strcmp(Message,"{\"optcode\":110}") )
  {
      memset( tmp,0,sizeof(tmp) );
      sprintf(tmp,"{\"devId\":\"%s\",\"state\":1,\"signal\":%s}",DEV_ID,csq);
      publish(0, 1, 0, _generateMessageID(), "zx/door/firstword",tmp);    //推送上线消息
  }
}
/*
   20 is the keepalive duration in seconds
*/
void IO_init()
{
    unsigned char i = 0;
    pinMode(led, OUTPUT);        //设置IO口为输出模式
    pinMode(locked ,OUTPUT);
    pinMode(gsm_reset ,OUTPUT);
    digitalWrite(locked,LOW);   //继电器IO初始化为低电平
    digitalWrite(led,LOW);      //led灯初始化为低电平

    digitalWrite(gsm_reset,LOW);  //GSM模块复位初始化
    delay(500);
    digitalWrite(gsm_reset,HIGH); //置低500ms然后拉高
    
    for(i=0;i<3;i++)             //启动时LED闪烁三次
    {
        delay(500);
        digitalWrite(led,HIGH);
        delay(500);
        digitalWrite(led,LOW);
    }
}

void setup()
{
  IO_init();    //IO口初始化
  MQTT.begin(); //MQTT初始化
  wdt_enable(WDTO_8S); //开启看门狗，并设置溢出时间为8秒
}

void loop()
{
  MQTT.processing();
  wdt_reset(); //喂狗操作
}

