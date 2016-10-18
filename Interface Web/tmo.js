(function() {
	window.Main = {};
	Main.Page = (function() {
		var mosq = null;
		function Page() {
			var _this = this;
			mosq = new Mosquitto();

			$('#connect-button').click(function() {
				return _this.connect();
			});
			$('#disconnect-button').click(function() {
				return _this.disconnect();
			});
			$('#subscribe-button').click(function() {
				return _this.subscribe();
			});
			$('#unsubscribe-button').click(function() {
				return _this.unsubscribe();
			});
			
			
			$('#publish-reset_consumo').click(function() {
				return _this.ResetConsumo();
			});			
			
			
			$('#publish-inicia-calibracao').click(function() {
				return _this.IniciaCalibracao();
			});	
			
			$('#publish-finaliza-calibracao').click(function() {
				return _this.FinalizaCalibracao();
			});	

			mosq.onconnect = function(rc){
				var p = document.createElement("p");
				var topic = $('#pub-subscribe-text')[0].value;
				p.innerHTML = "<font color='green'>Conexão realizada com sucesso</font>";
				$("#debug").append(p);
				mosq.subscribe(topic, 0);
				
			};
			mosq.ondisconnect = function(rc){
				var p = document.createElement("p");
				var url = "ws://iot.eclipse.org/ws"
				
				p.innerHTML = "<font color='red'>Conexão interromida</font>";
				$("#debug").append(p);				
			};
			mosq.onmessage = function(topic, payload, qos){
				var p = document.createElement("p");
				var payload_parseado = payload.split("-");				
				
				
			
				p.innerHTML = " <table width='100%' border=0><tr><th><table width='100%' border=0><tr><th  width='20%'>Consumo acumulado: "+payload_parseado[0]+" litros<br><img src='consumo.jpg'><th  width='20%'>Vazão instantânea: "+payload_parseado[1]+" l/h<br><img src='vazao.png'></th><th  width='20%'>Luminosidade: <br><img src='"+payload_parseado[3]+".jpg'></th></tr><tr><th  width='20%'>Temperatura: "+parseInt(payload_parseado[2], 10)+"ºC<br><img src='temperatura.jpg'></th></tr></table>";	
			
				$("#status_io").html(p); 
			};
		}
		Page.prototype.connect = function(){		    
			var url = "ws://iot.eclipse.org/ws";			
			mosq.connect(url);
		};
		Page.prototype.disconnect = function(){
			mosq.disconnect();
		};
		Page.prototype.subscribe = function(){
			var topic = $('#sub-topic-text')[0].value;
			mosq.subscribe(topic, 0);
		};
		Page.prototype.unsubscribe = function(){
			var topic = $('#sub-topic-text')[0].value;
			mosq.unsubscribe(topic);
		};
				
	
		Page.prototype.ResetConsumo = function(){
			var payload = "R";  
			var TopicPublish = $('#pub-topic-text')[0].value;
			mosq.publish(TopicPublish, payload, 0);		
		};
		
		Page.prototype.IniciaCalibracao = function(){
			var payload = "E";  
			var TopicPublish = $('#pub-topic-text')[0].value;
			mosq.publish(TopicPublish, payload, 0);	
		};

		Page.prototype.FinalizaCalibracao = function(){
			var payload = "S";  
			var TopicPublish = $('#pub-topic-text')[0].value;
			mosq.publish(TopicPublish, payload, 0);	
		};
		
		return Page;
	})();
	$(function(){
		return Main.controller = new Main.Page;
	});
}).call(this);

