# Sonda

## Introdução

A água é o elemento essencial para a vida. Logo, mensurar sua qualidade é fundamental nas mais diversas áreas: políticas públicas, conservação ambiental,
agropecuária, tratamento de resíduos ambientais. 

Análises de laboratório são muito precisas, porém tem um custo considerável, 
é necessário ter uma equipe para coletar corretamente as amostras e, por fim, o tempo até se ter um resultado. Como pontuado por [Helmi et al., 2014](https://ieeexplore.ieee.org/document/7086223), 
em um reservatório de água de grande porte são necessárias análises de vários pontos para ser real noção 
da qualidade da água. Já [Xu et al., 2011](https://www.sciencedirect.com/science/article/abs/pii/S0025326X11004036) ressalta a importância de dados de qualidade de 
água em longa duração, isto é, a medição periódica por vários anos. 

Nesse contexto, uma sonda multiparametros de operação remota é uma solução aplicável. Não necessita de coletas, consegue gerar um grande volume de dados por um longo tempo e tem apenas um custo inicial.


## Metodologia

O projeto tem como base a placa [TTGO-T-Beam](https://github.com/LilyGO/TTGO-T-Beam), um kit de desenvolvimento 
[ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) que já conta com GPS e LoRa. 
Sensores de temperatura, pH, TDS e turbidez fazem a aquisição dos dados, além do GPS. O armazenamento se dá através de um módulo de cartão SD. 
A transmissão dos dados se dá através de LoRa. O controle desses módulos é feito através do [FreeRTOS](https://www.freertos.org).
A alimentação é feita por uma placa solar com um regulador stepdown.

A arquitetura do projeto pode ser vista abaixo:

![Arquitetura](./images/arquitetura.jpg)

## Temperatura

Segundo a [Enciclopédia Britannica](https://www.britannica.com/science/temperature), temperatura é uma medida de calor ou frio, 
que é expressa dentro uma escala arbitrária, e indica a direção do fluxo espontâneo de energia térmica. Isto é, de um corpo mais quente 
(com maior temperatura), para um corpo mais frio (com menor temperatura).
É um parâmetro com uma grande importância biológica. Alterações abruptas, ou que ultrapassem um intervalo ótimo, podem gerar mortandade de espécies. Por exemplo, 
[Niswar et al., 2018](https://ieeexplore.ieee.org/document/8600828) trata de um ambiente de produção de caranguejos (*Portunus Pelagicus* e *Scylla serrata*)
e mostra que fora de um intervalo certo suas larvas não se desenvolvem.

Como sensor de temperatura, utiliza-se o termistor [MF-58](https://cdn.awsli.com.br/945/945993/arquivos/Datasheet%20MF58.pdf). Que é do tipo NTC e possui 
resistência interna de 10k ohms. Está conectado a placa principal em conjunto a um divisor de tensão. 
E a temperatura é computada através da equação de [Steinhart & Hart, 1968](https://www.sciencedirect.com/science/article/abs/pii/0011747168900570?via%3Dihub).

