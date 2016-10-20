#include <ESP8266WiFi.h> // Importa a Biblioteca ESP8266WiFi
#include <PubSubClient.h> // Importa a Biblioteca PubSubClient

//----------------
// Defines gerais
//----------------
#define VERSAO               "V1.00"
#define TAMANHO_VERSAO       5
#define SIM                  1
#define NAO                  0

//defines - mapeamento de pinos do NodeMCU
#define D0    16
#define D1    5
#define D2    4
#define D3    0
#define D4    2
#define D5    14
#define D6    12
#define D7    13
#define D8    15
#define D9    3
#define D10   1


//----------------
// Defines gerais
//----------------
#define VERSAO               "V1.00"
#define TAMANHO_VERSAO       5
#define SIM                  1
#define NAO                  0

//defines do protocolo
#define MAX_TAM_BUFFER    20
#define STX               0x02
#define CHECKSUM_OK       1
#define FALHA_CHECKSUM    0
#define RECEPCAO_OK       1
#define SEM_RECEPCAO      0

//defines dos estados
#define ESTADO_STX        1
#define ESTADO_OPCODE     3
#define ESTADO_TAMANHO    4
#define ESTADO_CHECKSUM   5
#define ESTADO_BUFFER     6

//defines dos opcodes do protocolo
#define OPCODE_RESET_CONSUMO         'R'
#define OPCODE_ENTRA_EM_CALIBRACAO   'E'
#define OPCODE_SAI_DA_CALIBRACAO     'S'
#define OPCODE_LEITURA_MEDICOES      'M'

//----------------
// Estruturas / 
// typedefs
//----------------
typedef struct
{	
    char Opcode;
    char TamanhoMensagem;						
    char CheckSum;					
    char Buffer[MAX_TAM_BUFFER];					
} TDadosProtocoloLeitor;


//----------------
// variáveis globais
//----------------
TDadosProtocoloLeitor DadosProtocoloLeitor;
volatile char EstadoSerial;
char RecebeuBufferCompleto;
char DeveEnviarLuminosidade;
char DeveEnviarVersao;
char IndiceBuffer;
char MedicoesPIC[23];
char SolicitacaoPICReset[]=          {0x02, OPCODE_RESET_CONSUMO, 0x00, 0x00};
char SolicitacaoPICMedicoes[]=       {0x02, OPCODE_LEITURA_MEDICOES, 0x00, 0x00};
char SolicitacaoPICEntraCalibracao[]={0x02, OPCODE_ENTRA_EM_CALIBRACAO, 0x00, 0x00};
char SolicitacaoPICSaiCalibracao[]=  {0x02, OPCODE_SAI_DA_CALIBRACAO, 0x00, 0x00};

// WIFI
const char* SSID = " "; // SSID da Rede
const char* PASSWORD = " "; // Senha da Rede
 
// MQTT
const char* BROKER_MQTT = "iot.eclipse.org"; // IP/URL DO BROKER MQTT
int BROKER_PORT = 1883; // Porta do Broker MQTT
#define ID_MQTT  "MQTTMasterBrasil001" 
#define TOPICO_ENVIO      "MQTTMasterBrasilEnvia"
#define TOPICO_RECEPCAO   "MQTTMasterBrasilRecebe"

//objetos de comunicação wifi e MQTT
WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient

//----------------
// Prototypes
//----------------
void initSerial();
void initWiFi();
void initMQTT();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void TrataMensagem(void);
void MontaEEnviaMensagem(void);
void AguardaSTX();
void AguardaOpcode(char ByteRecebido);
void AguardaTamanho(char ByteRecebido);
void AguardaCheckSum(char ByteRecebido);
void AguardaBuffer(char ByteRecebido);
char CalculaCheckSum(void);
void MaquinaEstadoSerial(char ByteRecebido);
void EnviaLuminosidade(int LeituraADC);
void EnviaVersao(void);
 
 
void setup() 
{
  EstadoSerial = ESTADO_STX;
  initSerial();
  initWiFi();
  initMQTT();
}
 
// Função para iniciar a comunicação serial
void initSerial() {
  Serial.begin(9600);

}
 
// Função para iniciar a Conexão com a rede WiFi
void initWiFi() {
  delay(10);
 
 
  WiFi.begin(SSID, PASSWORD); // Conecta na Rede Wireless
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);

  }
  
}
 
// Funcão para se conectar ao Broker MQTT
void initMQTT() {
     MQTT.setServer(BROKER_MQTT, BROKER_PORT); // Atribui a biblioteca MQTT o Broker MQTT
     MQTT.setCallback(mqtt_callback); // tribui a biblioteca MQTT o callback
}
 
//Função que recebe as mensagens publicadas
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
 
  String message;
  for (int i = 0; i < length; i++) {
    char c = (char)payload[i];
    message += c;
  }

  
  if (message.charAt(0) == OPCODE_RESET_CONSUMO)
	  Serial.write(SolicitacaoPICReset,sizeof(SolicitacaoPICReset));
	
  if (message.charAt(0) == OPCODE_ENTRA_EM_CALIBRACAO)
	  Serial.write(SolicitacaoPICEntraCalibracao,sizeof(SolicitacaoPICEntraCalibracao));	
	
  if (message.charAt(0) == OPCODE_SAI_DA_CALIBRACAO)
	  Serial.write(SolicitacaoPICSaiCalibracao,sizeof(SolicitacaoPICSaiCalibracao));		
}
 
// Função para se reconectar ao Broker MQTT caso cai'a a comunicação
void reconnectMQTT() {
  while (!MQTT.connected()) {

    if (MQTT.connect(ID_MQTT)) {

      MQTT.subscribe(TOPICO_RECEPCAO); // Assina o tópico estação/lat/long
 
    } else {
      delay(2000);
    }
  }
}
 
// Função para se reconectar a rede WiFi caso caia a comunicação com a rede
void recconectWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    //                                                                                                                                                                                                                 
  }
}


//Aguarda STX - função da máquina de estados da comunicação serial
//parametros: byte recebido
//saida: nenhum
void AguardaSTX(char ByteRecebido)
{
    if (ByteRecebido == STX)
    {
	      memset(&DadosProtocoloLeitor, 0, sizeof(TDadosProtocoloLeitor));   //limpa dados do protocolo
	      EstadoSerial = ESTADO_OPCODE;
    }
    else
	      EstadoSerial = ESTADO_STX;
}

//Aguarda Opcode da mensagem - função da máquina de estados da comunicação serial
//parametros: byte recebido
//saida: nenhum
void AguardaOpcode(char ByteRecebido)
{
    DadosProtocoloLeitor.Opcode = ByteRecebido;
    EstadoSerial = ESTADO_TAMANHO;	
}

//Aguarda tamanho da mensagem - função da máquina de estados da comunicação serial
//parametros: byte recebido
//saida: nenhum
void AguardaTamanho(char ByteRecebido)
{
    if (ByteRecebido > MAX_TAM_BUFFER)
  	    EstadoSerial = ESTADO_STX;   //tamanho recebido é inválido (maior que o máximo permitido). A máquina de estados é resetada.
    else
    {	
      	DadosProtocoloLeitor.TamanhoMensagem = ByteRecebido;
	    EstadoSerial = ESTADO_CHECKSUM;	
    }
}

//Aguarda checksum da mensagem - função da máquina de estados da comunicação serial
//parametros: byte recebido
//saida: nenhum
void AguardaCheckSum(char ByteRecebido)
{	
    DadosProtocoloLeitor.CheckSum = ByteRecebido;
    
    if(DadosProtocoloLeitor.TamanhoMensagem > 0)
    {
        IndiceBuffer = 0;
	    EstadoSerial = ESTADO_BUFFER;
    }
    else
    {
       	RecebeuBufferCompleto = RECEPCAO_OK;
	    EstadoSerial = ESTADO_STX;
    }
}

//Aguarda buffer da mensagem - função da máquina de estados da comunicação serial
//parametros: byte recebido
//saida: nenhum
void AguardaBuffer(char ByteRecebido)
{
    if(IndiceBuffer < DadosProtocoloLeitor.TamanhoMensagem)
    {
	      DadosProtocoloLeitor.Buffer[IndiceBuffer] = ByteRecebido;
	      IndiceBuffer++;
    }
    else
    {
	      //buffer completo. Faz o tratamento da mensagem e reinicia máquina de estados
	      if (CalculaCheckSum() == CHECKSUM_OK)
            RecebeuBufferCompleto = RECEPCAO_OK;
	      else
            RecebeuBufferCompleto = SEM_RECEPCAO;
	
	      EstadoSerial = ESTADO_STX;
    }
}

//Função: checa o checksum da mensagem recebida pela UART
//parametros: nenhum
//saida: nenhum
char CalculaCheckSum(void)
{
    char CheckSumCalculado;
    char i;
	
    CheckSumCalculado = 0;
	
    for(i=0; i<DadosProtocoloLeitor.TamanhoMensagem; i++)
	      CheckSumCalculado = CheckSumCalculado + DadosProtocoloLeitor.Buffer[i];
		
    CheckSumCalculado = (~CheckSumCalculado) +1;
	
    if (CheckSumCalculado == DadosProtocoloLeitor.CheckSum)
	      return CHECKSUM_OK;
    else
	      return FALHA_CHECKSUM;
}


//Função que faz o gerenciamento dos estados da máquina de estado. É chamada sempre que um byte chega da serial
//parametros: byte recebido pela serial
//saida: nenhum
void MaquinaEstadoSerial(char ByteRecebido)
{
    switch(EstadoSerial)
    {
        case ESTADO_STX:
        {
            AguardaSTX(ByteRecebido);
            break; 
        }

	      		
        case ESTADO_OPCODE:
        {
            AguardaOpcode(ByteRecebido);
            break; 
        }

        case ESTADO_TAMANHO:
        {
            AguardaTamanho(ByteRecebido);
            break; 
        }

        case ESTADO_CHECKSUM:
        {
            AguardaCheckSum(ByteRecebido);
            break; 
        }

        case ESTADO_BUFFER:
        {
            AguardaBuffer(ByteRecebido);
            break; 
        }

        
        default:   //se o estado tiver qualquer valro diferente dos esperados, significa que algo corrompeu seu valor (invasão de memória RAM). Logo a máquina de estados é reuniciada.
        {
            EstadoSerial=ESTADO_STX;
            RecebeuBufferCompleto = SEM_RECEPCAO;
            memset(&DadosProtocoloLeitor, 0, sizeof(TDadosProtocoloLeitor));   //limpa dados do protocolo
            break;
        }
    }
}

//Função: trata mensagem recebida pela UART
//parametros: nenhum
//saida: nenhum
void TrataMensagem(void)
{
    switch (DadosProtocoloLeitor.Opcode)
    {
        case OPCODE_LEITURA_MEDICOES:
        {
            memcpy(MedicoesPIC,DadosProtocoloLeitor.Buffer,DadosProtocoloLeitor.TamanhoMensagem);
            break;
        }

        default:
        {
            RecebeuBufferCompleto = SEM_RECEPCAO;
            EstadoSerial = ESTADO_STX;

            //limpa dados do protocolo 
            memset(&DadosProtocoloLeitor, 0, sizeof(TDadosProtocoloLeitor));  
        }
    }    
}

//Função: formata e envia a mensagem
//parametros: nenhum
//saida: nenhum
void MontaEEnviaMensagem(void)
{
    char PayloadChar[25];
    char CheckSumMsg;
    char i;

   //envio da mensagem
   MQTT.publish(TOPICO_ENVIO, MedicoesPIC);

} 

void loop() 
{	
  if (!MQTT.connected()) 
  {
    reconnectMQTT(); // Caso o ESP se desconecte do Broker, ele tenta se reconectar ao Broker
  }
  recconectWiFi(); // Caso o ESP perca conexão com a rede WiFi, ele tenta se reconetar a rede.
 
  //solicita as informações ao PIC
  delay(150);
  Serial.write(SolicitacaoPICMedicoes,sizeof(SolicitacaoPICMedicoes));
  while(RecebeuBufferCompleto != RECEPCAO_OK )
  {
    if (Serial.available() > 0)  
       MaquinaEstadoSerial(Serial.read());    
  } 
  TrataMensagem();
	
  //envia todos os dados ao broker MQTT  
  MontaEEnviaMensagem();
 
  delay(5000);
  MQTT.loop();
}
