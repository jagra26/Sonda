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


## Arquitetura

O projeto tem como base a placa [TTGO-T-Beam](https://github.com/LilyGO/TTGO-T-Beam), um kit de desenvolvimento 
[ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) que já conta com GPS e LoRa. 
Sensores de temperatura, pH, TDS e turbidez fazem a aquisição dos dados, além do GPS. O armazenamento se dá através de um módulo de cartão SD. 
A transmissão dos dados se dá através de LoRa. O controle desses módulos é feito através do [FreeRTOS](https://www.freertos.org).
A alimentação é feita por uma placa solar com um regulador stepdown.

A arquitetura do projeto pode ser vista abaixo:

![Arquitetura](./images/arquitetura.jpg)

## Sensores

A sonda possui sensores de [temperatura](temperatura), [pH](ph), [turbidez](turbidez) e [TDS](tds). Todos eles são de baixo custo e analógicos.

### Temperatura

Segundo a [Enciclopédia Britannica](https://www.britannica.com/science/temperature), temperatura é uma medida de calor ou frio, 
que é expressa dentro uma escala arbitrária, e indica a direção do fluxo espontâneo de energia térmica. Isto é, de um corpo mais quente 
(com maior temperatura), para um corpo mais frio (com menor temperatura).
É um parâmetro com uma grande importância biológica. Alterações abruptas, ou que ultrapassem um intervalo ótimo, podem gerar mortandade de espécies. Por exemplo, 
[Niswar et al., 2018](https://ieeexplore.ieee.org/document/8600828) trata de um ambiente de produção de caranguejos (*Portunus Pelagicus* e *Scylla serrata*)
e mostra que fora de um intervalo certo suas larvas não se desenvolvem.

Como sensor de temperatura, utiliza-se o termistor [MF-58](https://cdn.awsli.com.br/945/945993/arquivos/Datasheet%20MF58.pdf). Que é do tipo NTC e possui 
resistência interna de 10k ohms. Está conectado a placa principal em conjunto a um divisor de tensão. 
E a temperatura é computada através da equação de [Steinhart & Hart, 1968](https://www.sciencedirect.com/science/article/abs/pii/0011747168900570?via%3Dihub).

### pH

Sørensen descreveu pela primeira vez o pH como a concentração de íons de hidrogênio. [Atualmente](https://www.degruyter.com/document/doi/10.1351/pac200274112169/html), 
é entendido pelo cologaritmo da atividade dos íons de hidrogênio. 
Sendo dado pela relação: $pH = -\log{a_H}$, onde $a_H$ é a atividade dos íons em $\frac{mol}{dm^3}$.
O pH consegue descrever o quanto uma solução é ácida ou básica. Sua escala, que é logaritimica, vai de 0, extremamente ácida, até 14, extremamente básica. 

É um fator muito relevante em análises de qualidade de água, pois alterações na acidez sugerem que fenômenos importantes estão acontecendo. 
A [acidificação oceânica](https://books.google.com.br/books?id=eoxpAgAAQBAJ&hl=pt-BR), por exemplo, é um fenômeno potencializado pelas mudanças climáticas
que prejudica diversos organismos e fotossíntese, e é mensurado através da escala de pH ao longo do tempo. Em reservatórios é uma grandeza que pode sugerir algum 
processo de [eutrofização](https://sciencing.com/eutrophication-affect-ph-12036599.html), que pode ser resultado de poluição por esgotos ou fertilizantes.

Como sensor de Ph utiza-se, um [sensor](https://www.usinainfo.com.br/index.php?controller=attachment&id_attachment=553) analógico de baixo custo, com uma casa decimal 
de precisão. 

### Turbidez

A aparência nebulosa da água devido a suspensão de partículas finas é denominada 
[turbidez](https://onlinelibrary.wiley.com/doi/epdf/10.1111/j.1752-1688.2001.tb03624.x). Quanto maior a quantidade de partículas em suspensão, mais turva
é a água. Possui uma importância biológica grande, uma água que se torna turva repentinamente pode indicar a presença de poluição, por exemplo. As partículas 
podem prejudicar as brânquias dos peixes e diminuir a incidência de luz no ambiente sub aquático.

Como sensor, utiliza-se um módulo analógico de baixo custo, [TS-300B](http://images.100y.com.tw/pdf_file/46-TS-300B.pdf).

![sensor de turbidez](http://images.100y.com.tw/images/product_jpg_original/A061801.jpg)

//TODO: Adicionar fotos

## Protótipo

No momento o projeto é um protótipo capaz de registrar a temperatura em um cartão sd, em um arquivo .csv, e transmitir mensagens via LoRa. Novos sensores, 
a comunicação GPS será estabelecida e questões de alimentação são pontos a serem adicionados e resolvidos em breve.
Uma foto do hardware atual do projeto pode ser vista a seguir:
![prototype](images/prototype.jpg)

O [placa com display](https://heltec.org/project/wifi-lora-32/) se trata do cliente LoRa, e recebe as mensagens enviadas pelo projeto.

## Esquemático

![wire diagram](images\wire_diagram.jpg)
