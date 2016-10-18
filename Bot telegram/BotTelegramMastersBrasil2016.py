#Bot de Telegram com Linux
#Autor: Pedro Bertoleti

#imports necessarios:
import telegram			
import paho.mqtt.client as mqtt
import sys
import os
from time import sleep 
import urllib2
import json

#imports de bibliotecas para tratar erros de conexao URL.
try:
    from urllib.error import URLError
except ImportError:
    from urllib2 import URLError

#variaveis globais e definicoes:
TokenBotTelegram = '256044695:AAFDbcpUZ7uNOQxHLQgqKc3TjHTl-E-xDzI'	#Bot: MastersBrasilIoT_bot
Broker = "iot.eclipse.org"
PortaBroker = 1883
KeepAliveBroker = 15
TimeoutConexao = 20
TopicoSubscribe = "MQTTMasterBrasilEnvia" 
VazaoRecebida = " "
ConsumoRecebido = " "
TemperaturaRecebida = " "
LuminosidadeRecebida = " "

#Funcao: Callback - conexao ao broker realizada
def on_connect(client, userdata, flags, rc):
    print("[MQTT] Conectado ao Broker. Resultado de conexao: "+str(rc))

    #faz subscribe automatico no topico
    client.subscribe(TopicoSubscribe)

#Funcao: Callback - mensagem recebida do broker
def on_message(client, userdata, msg):
	global VazaoRecebida
        global ConsumoRecebido
	global TemperaturaRecebida
	global LuminosidadeRecebida
	
	MensagemRecebida = str(msg.payload)
        print("[MQTT_MSG] Topico: "+msg.topic+" / Mensagem: "+MensagemRecebida)
	
	#faz o parse da mensagem
	CamposMensagem=MensagemRecebida.split("-")
	VazaoRecebida = CamposMensagem[0]
	ConsumoRecebido = CamposMensagem[1]
	TemperaturaRecebida = CamposMensagem[2]
	LuminosidadeRecebida = CamposMensagem[3]
	
	print("[SENSORES] Vazao: "+VazaoRecebida+"l/h")
	print("           Consumo total: "+VazaoRecebida+"l")
	print("           Temperatura: "+TemperaturaRecebida+"C")

	if (LuminosidadeRecebida == "DIA"): 
		print("           Luminosidade: DIA")
		
	if (LuminosidadeRecebida == "NOI"): 
		print("           Luminosidade: NOITE")	

#Funcao: Trata mensagem recebida
def TrataMensagem(MsgRecebida,chat_id):
    global VazaoRecebida
    global ConsumoRecebido
    global TemperaturaRecebida
    global LuminosidadeRecebida
	
    MsgRecebidaLC = MsgRecebida.lower()
   
    if "123456" in MsgRecebida:
       print "[USUARIO] Requisicao de informacoes"
       bot.sendMessage(chat_id=chat_id, text='Segue abaixo suas informacoes:')
       bot.sendMessage(chat_id=chat_id, text='Consumo de agua: '+ConsumoRecebido+'l')
       bot.sendMessage(chat_id=chat_id, text='Vazao de agua: '+VazaoRecebida+'l/h')
       bot.sendMessage(chat_id=chat_id, text='Temperatura: '+TemperaturaRecebida+'C')
       
       if (LuminosidadeRecebida == "NOI"): 
           bot.sendMessage(chat_id=chat_id, text='Status da luminosidade: NOITE')

       if (LuminosidadeRecebida == "DIA"): 
           bot.sendMessage(chat_id=chat_id, text='Status da luminosidade: DIA')

    else:
       print "[USUARIO]: requisicao de contato"
       bot.sendMessage(chat_id=chat_id, text='Ola! Tudo bem? Sou o bot IoT da demo do Mastes Brasil 2016. Por favor, informe seu codigo unico para eu poder lhe informar suas medicoes.')
    


def ExecutaBot(bot, update_id):
    #Requisita atualizacoes depois da ultima id de update - update_id
    #bot.getUpdates(offset, timeout) - offset eh o ponto de partida em
    #que comeca a procurar novas atualizacoes de mensagens, timeout eh 
    # tempo minimo de espera para retorno da requisicao de resposta.
    for update in bot.getUpdates(offset=update_id, timeout=2):
        
        #o chat_id eh a id do chat de comunicacao Telegram
        #eh necessaria para o bot identificar a conversa e
	#gerar e enviar a resposta
        chat_id = update.message.chat_id
	
        #atualiza o indice update_id - para ref novas mensagens
        update_id = update.update_id + 1
	
        #captura a mensagem de texto enviada ao bot no dado chat_id
        message = update.message.text

        if message:
            #Ha mensagem recebida. 
            #Faz o tratamento e envia a resposta pelo Telegram
            TrataMensagem(message,chat_id)

    # retorna o ultimo update_id para servir de referencia
    return update_id
	
###################	
#Programa principal
###################   
#Variavel update_id (usada pelo Telegram)
update_id = None

#Inicializacao do Bot
bot = telegram.Bot(TokenBotTelegram)
print '[BOT] Bot Telegram iniciado. Aguardando comandos.'

#inicializa MQTT:
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect(Broker, PortaBroker, KeepAliveBroker)


while True:
   StatusConexaoBroker = client.loop(TimeoutConexao)

   if (StatusConexaoBroker > 0):
	client.connect(Broker, PortaBroker, KeepAliveBroker)  #se ocorrer algum erro, reconecta-se ao broker
  

   try:
       try:
           update_id = ExecutaBot(bot, update_id)
       except telegram.TelegramError as e:
           #Se ocorrer algum problema (como lentidao, por ex):
           if e.message in ("Bad Gateway", "Timed out"): 
                sleep(1) 
           else: 
                raise e
       except URLError as e:
           #Ha problemas de rede
           sleep(1)
   except KeyboardInterrupt:
        print "Ctrl+c pressionada. A aplicacao sera encerrada."
        exit(1) 
