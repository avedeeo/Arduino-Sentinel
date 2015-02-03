#include <SoftwareSerial.h>
#include <String.h>
#include <CapacitiveSensor.h> //Get from 
         
const char SMS_NUMBER[] = "+56865738013"; //numero para mandar el mensaje 
                                           
const unsigned long MIN_MS_BETWEEN_SMS = 60000; //time lapse entre mensajes para no gastar mucho
const boolean SMS_ENABLED = false;
const boolean SEND_SMS_AT_STARTUP = true;
const boolean CAP_SENSOR_ENABLED = true; 

const long CAP_LOW_THRESHOLD = 200; //si el la capacitancia es mas baja que esto no se tomara como activado
const long CAP_HIGH_THRESHOLD = 600; //si la capacitancia es mas alta que esto se mostrara como activado

/* Pines */
#define LIGHT_SENSOR A0
#define PIR_POWER 3
#define PIR_SENSOR 2
#define PIR_GROUND 4
#define CONTACT_SENSOR 10
#define CAPACITIVE_SEND 12
#define CAPACITIVE_RECEIVE 11

/* Variables (globales) */
String message = String(""); //make a new string and set it to empty
unsigned long last_sms_time = 0;
boolean pir_reading = LOW;
boolean contact_reading;
boolean capacitive_reading = false;
long raw_capacitive_reading;
CapacitiveSensor cap_sensor = CapacitiveSensor(CAPACITIVE_RECEIVE, CAPACITIVE_SEND);

SoftwareSerial cellularSerial(7, 8);

void setup()
{
  
  
  Serial.begin(115200);   // Set your Serial Monitor baud rate to this to see
                          // output from this sketch on your computer when 
                          // plugged in via USB.
  Serial.println(F("Holaquetal.")); //saludo para saber que entro
  pinMode(LIGHT_SENSOR, INPUT_PULLUP);
  pinMode(PIR_GROUND, OUTPUT);
  pinMode(PIR_POWER, OUTPUT);
  pinMode(PIR_SENSOR, INPUT);
  digitalWrite(PIR_GROUND, LOW);
  digitalWrite(PIR_POWER, HIGH);
  
  pinMode(CONTACT_SENSOR, INPUT_PULLUP);
  contact_reading = digitalRead(CONTACT_SENSOR);
  
  //El sensor optico tarda unos segundos en iniciar
  
  cellularSerial.begin(19200);  // el baud rate predeterminado para celular
  
  turnOnCellModule();
  
  enableNetworkTime(); //Obtiene el tiempo de la red celular y no funciona el timestamp en el serial monitor

  if (CAP_SENSOR_ENABLED)
  {  
    raw_capacitive_reading = cap_sensor.capacitiveSensor(30);
    if (raw_capacitive_reading < CAP_LOW_THRESHOLD)
    {
      capacitive_reading = false;
    } else if (raw_capacitive_reading > CAP_HIGH_THRESHOLD)
    {
      capacitive_reading = true;
    }
  }
  
  //Intervalo de tiempo para que el programa enter cuando el sensor se active
  while (millis() < 10000)
  {
    //do nothing
  }
  
  if (SEND_SMS_AT_STARTUP && SMS_ENABLED)
  {
    startTextMessage();
    cellularSerial.println(F("La alarma silenciosa esta lista.")); // Te avisa cuando el programa entra despues del sensor (entra bien, pues)
    endTextMessage();
  }
  Serial.println(F("La alarma silenciosa esta lista."));
}

void loop()
{
  //This function runs over and over after setup().
  drainSoftwareSerial(true);
  
  if (digitalRead(PIR_SENSOR) != pir_reading)
  {
    pir_reading = !pir_reading;
    if (pir_reading)
    {
      Serial.println(F("PIR acaba de ser activado."));//mensaje de activacion del sensor optico
      message = String("PIR activado:");
      sendTextMessage();
    } else
    {
      Serial.println(F("PIR desactivado."));
    }
  }
  
  if (digitalRead(CONTACT_SENSOR) != contact_reading)
  {
    contact_reading = !contact_reading;
    if (!contact_reading)
    {
      Serial.println(F("Sensor de presion activado.")); //mensaje de activacion del sensor de presion activado
      message = String("Sensor presionado:"); //este mensaje aparece cuando el sensor sigue presionado pero esa no es la razon del mensaje (que por ejemmplo el sensor siga 
                                              //activado pero en eso se active recientemente el PIR, emitiendo una senal que produce un mensaje)
      sendTextMessage();//llama el metodo de enviar mensaje
    } else
    {
      Serial.println(F("Sensor de presion desactivado."));
      message = String("Sensor de presion desactivado:");
      sendTextMessage();    
    }
  }
  
  if (CAP_SENSOR_ENABLED)
  {
    raw_capacitive_reading = cap_sensor.capacitiveSensor(30);
    Serial.print("Capacitancia: "); //indica el factor de capacitancia para informacion del usuario
    Serial.println(raw_capacitive_reading);
    
    if (capacitive_reading && raw_capacitive_reading < CAP_LOW_THRESHOLD)
    {
      capacitive_reading = !capacitive_reading;
      Serial.println("Sensor de proximidad desactivado.");
      message = String("Sensor de proximidad desactivado.");
      sendTextMessage();
    } else if (!capacitive_reading && raw_capacitive_reading > CAP_HIGH_THRESHOLD)
    {
      capacitive_reading = !capacitive_reading;
      Serial.println(F("Sensor de proximidad activado."));
      message = String("Sensor de proximidad activado.");
      sendTextMessage();
    }
  }
    
  delay(100);
}

void drainSoftwareSerial(boolean printToSerial)
{
  //Esta funcion lee la informacion disponible del puerto serial del software,
  //y se detiene si no hay datos entrantes. Si printToSerial es verdadero
  //imprime la informacion del puerto serial hacia el hardware, asi aparece en el serial monitor. 
  //(por eso las lineas constantes de "Capacitancia")

  char b;
  while (cellularSerial.available())
  {
    b = cellularSerial.read();
    if (printToSerial)
    {
      Serial.print(b);
    }
  }
}

boolean toggleAndCheck()
{
 //verificacion
  
  togglePower(); //aun no sabemos si el cell module esta activo
  drainSoftwareSerial(true); //vaciar los datos 
                          
  delay(50); //delay en lo que enciende el module
  cellularSerial.print(F("AT+GMI\r")); //obtener datos del prestador de servicios 
                                      
  delay(125); //esperando que responda...
  if (cellularSerial.available())     //responde entonces funciona
  {
    drainSoftwareSerial(false);
    delay(200);
    return false; 
  }
  return true;
}

void turnOnCellModule()
{
  //esto reinicia el cell module: si esta apagado lo inicia, si esta prendido lo inicia (por si acaso)
  
  while (toggleAndCheck()) //cicla el metodo hasta que obtengamos respuesta del prestador de servicios 
                     
  {
    Serial.println(F("Reintentando."));
  }
}

void togglePower() //del wiki de SeedStudio (en la bibliografia)
{
  Serial.println(F("Encendiendo el sistema celular."));
  pinMode(9, OUTPUT); 
  digitalWrite(9, LOW);
  delay(1000);
  digitalWrite(9, HIGH);
  delay(2000);
  digitalWrite(9, LOW);
  delay(3000);
}


void enableNetworkTime()
{
  
  cellularSerial.print(F("AT+CLTS=1\r"));
  delay(100);
  drainSoftwareSerial(true);
}


void getTime()
{
  //Esta funcion obtiene el tiempo directamente de la red movil y lo pone en el mensaje.

  drainSoftwareSerial(true); 

  cellularSerial.print(F("AT+CCLK?\r"));
  delay(100);
  //start reading
  char b;
  boolean inside_quotes = false;
  while (cellularSerial.available())
  {
    b = cellularSerial.read(); 
    if (b == '"')
    {
      inside_quotes = !inside_quotes;  
    }
    
    if (inside_quotes && b != '"') 
    {
      message += b;
    } 
  } 
}

void startTextMessage()
{
  //Esta funcion procesa los comandos necesarios para enviar un mensaje por via SMS
  
  cellularSerial.print(F("AT+CMGF=1\r"));    //Envia el SMS en formato de texto
  delay(100);
  cellularSerial.print(F("AT + CMGS = \""));
  cellularSerial.print(SMS_NUMBER);
  cellularSerial.println(F("\""));
  delay(100);
}

void endTextMessage()
{
  //Envia un solo mensaje
  delay(100);
  cellularSerial.println((char)26); //control-z
  delay(100);
  cellularSerial.println();
}
 
void sendTextMessage()
{
  //Aqui se verifica si debe eviarse SMS o solo escribir en el serial monitor segun SMS_ENABLED y
  //MIN_MS_BETWEEN_SMS
  
  boolean send_sms = false;
  if (!SMS_ENABLED) //envia las alertas por SMS
  {
    Serial.println(F("SMS desactivado por configuracion."));
  } else
  {
    if (last_sms_time > 0 && millis() < last_sms_time + MIN_MS_BETWEEN_SMS)
    {
      Serial.println(F("SMS bloqueado por tiempo.")); //muchos SMS
    } else
      send_sms = true;
  }
  
  annotateMessage(); //envia un mensaje de confirmacion de SMS al puerto serial
  
  if (send_sms)
  {
    startTextMessage();
    cellularSerial.print(message);
    endTextMessage();
    
    last_sms_time = millis();
    Serial.print(F("Se ha enviado SMS a "));
  } else
  {
    Serial.print(F("No se ha enviado SMS a "));
  }
      Serial.print(SMS_NUMBER);
      Serial.print(F(":"));
      Serial.println(message);
}

void annotateMessage()
{
  //Adjunta la lectura de los sensores al mensaje
  
  message += " Luz: ";
  message += analogRead(LIGHT_SENSOR);
  
  message += " Sensor de presion: ";
  if (digitalRead(CONTACT_SENSOR))
  {
    message += "desactivado";
  } else
  {
    message += "activado";
  }
  
  if (CAP_SENSOR_ENABLED)
  {
    message += " Proximidad: ";
    message += cap_sensor.capacitiveSensor(30);
  }
  
  message += " (enviado en ";
  getTime();
  message += ")";
}
