# Documentação Técnica - Sistema de Semáforo Inteligente com FreeRTOS

## Descrição Geral

Este projeto implementa um sistema de semáforo inteligente utilizando o microcontrolador RP2040 na placa BitDog Lab. O sistema emprega o FreeRTOS para gerenciamento de tarefas concorrentes e inclui funcionalidades de IoT mediante um servidor web integrado, permitindo monitoramento e controle remoto através de uma rede WiFi.

O sistema de semáforo opera em dois modos distintos: o modo diurno, com ciclo completo de cores (verde → amarelo → vermelho), e o modo noturno, caracterizado pelo amarelo piscante. Para cada estado, o sistema fornece feedback visual através de LEDs RGB, matriz de LED 5x5, display OLED, além de sinais sonoros específicos emitidos por um buzzer, criando um sistema acessível para pessoas com deficiência visual.

## Componentes de Hardware e Software

O projeto utiliza a placa BitDog Lab com RP2040, equipada com display OLED (endereço I2C 0x3C), matriz de LED 5x5, LEDs RGB individuais, buzzer e botões de controle. A conectividade WiFi é implementada através do chip CYW43439 integrado, permitindo que o dispositivo se conecte a redes sem fio e opere como um servidor web.

No aspecto de software, o sistema é construído sobre o SDK do Raspberry Pi Pico e FreeRTOS, com bibliotecas adicionais como SSD1306 para controle do display OLED e lwIP para a pilha de rede TCP/IP. O PIO (Programmable I/O) do RP2040 é utilizado para controlar a matriz de LEDs WS2812B com temporização precisa.

## Interface Web

O sistema implementa um servidor web na porta 80, acessível através do endereço IP atribuído automaticamente via DHCP. A interface web oferece uma página HTML responsiva com os seguintes recursos:

- Botão para alternar entre os modos diurno e noturno remotamente
- Botão para ativar/desativar o sistema de alerta sonoro
- Indicadores visuais do estado atual do sistema (modo e status do alerta sonoro)

A comunicação é implementada via solicitações HTTP GET, processadas pelo manipulador `user_request()` que interpreta comandos específicos como `/modo` e `/buzzer` para alterar o estado do sistema.

## Arquitetura do Software

O sistema é estruturado em seis tarefas FreeRTOS independentes:

1. **button_task**: Monitora os botões físicos para controle local
2. **buzzer_task**: Controla os sinais sonoros específicos para cada estado
3. **display_task**: Atualiza o display OLED com informações contextuais
4. **led_task**: Controla os LEDs RGB e gerencia a transição de estados
5. **matriz_led_task**: Controla a matriz de LEDs com padrões visuais distintos
6. **wifi_task**: Realiza polling da interface WiFi para manter a conexão e processar requisições web

A comunicação entre tarefas é implementada através de variáveis globais compartilhadas como `modo_diurno`, `alerta_sonoro` e `estado_semaforo`, sendo esta última declarada como volátil para garantir visibilidade entre núcleos.

## Funcionalidades de Rede

O sistema inicializa automaticamente a conexão WiFi utilizando as credenciais configuradas (SSID e senha), estabelecendo um cliente DHCP para obtenção de endereço IP. Uma vez conectado, o sistema inicia um servidor TCP na porta 80 que responde a requisições HTTP GET, permitindo controle remoto.

A pilha lwIP é configurada para operação sem sistema operacional (NO_SYS=1), com suporte a TCP, UDP, DHCP e HTTP, otimizada para o ambiente embarcado com alocação de memória limitada (4KB). O servidor web implementa uma interface HTML responsiva com estilização CSS para melhor experiência do usuário.

## Padrões Sonoros e Visuais

Cada estado do semáforo possui representações sonoras e visuais específicas:

- **Verde**: 1 beep por segundo (1000ms), LED verde aceso, padrão "V" na matriz
- **Amarelo**: Beeps rápidos intermitentes (100ms), LEDs verde e vermelho acesos, linha vertical na matriz
- **Vermelho**: Tom contínuo curto (500ms ligado, 1.5s desligado), LED vermelho aceso, padrão "X" na matriz
- **Modo Noturno**: Beep lento a cada 2 segundos, LEDs verde e vermelho piscando alternadamente

## Instruções de Uso

### Controle Local:
- **Botão A**: Alterna entre os modos diurno e noturno
- **Botão B**: Aciona o modo BOOTSEL para programação

### Controle Remoto:
1. Conecte-se à mesma rede WiFi do dispositivo
2. Acesse o endereço IP do dispositivo (exibido no console durante inicialização)
3. Use os botões da interface web para controlar o semáforo:
   - "alternar modo": Alterna entre modo diurno e noturno
   - "desabilitar alerta sonoro": Ativa ou desativa os sinais sonoros

## Observações Importantes

- A funcionalidade de controle remoto permite integração com sistemas de automação e controle centralizado de múltiplos dispositivos
- A variável `alerta_sonoro` permite desativar os sinais do buzzer sem alterar o comportamento visual do semáforo
- O sistema mantém consistência entre os controles locais (botões físicos) e remotos (interface web)
- A interface WiFi utiliza polling contínuo através da tarefa dedicada `wifi_task` para garantir responsividade

O sistema demonstra a aplicação prática de um dispositivo IoT integrado com RTOS, permitindo operação autônoma com capacidade de monitoramento e controle remoto, características essenciais para sistemas de sinalização inteligentes em ambientes urbanos conectados.