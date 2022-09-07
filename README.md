# Sonda

## Índice
* [Introdução](#introdução)
* [Arquitetura](#arquetura)
* [Sensores](#sensores)
	* [Temperatura](#temperatura)
	* [pH](#ph)
	* [Turbidez](#turbidez)
	* [TDS](#tds)
* [Protótipo](#prototipo)

---

## Introdução

A água é o elemento essencial para a vida. Logo, mensurar sua qualidade é fundamental nas mais diversas áreas: políticas públicas, conservação ambiental,
agropecuária, tratamento de resíduos ambientais. 

Análises de laboratório são muito precisas, porém tem um custo considerável, 
é necessário ter uma equipe para coletar corretamente as amostras e, por fim, o tempo até se ter um resultado. Como pontuado por [Helmi et al., 2014](https://ieeexplore.ieee.org/document/7086223), 
em um reservatório de água de grande porte são necessárias análises de vários pontos para ser real noção 
da qualidade da água. Já [Xu et al., 2011](https://www.sciencedirect.com/science/article/abs/pii/S0025326X11004036) ressalta a importância de dados de qualidade de 
água em longa duração, isto é, a medição periódica por vários anos. 

Nesse contexto, uma sonda multiparametros de operação remota é uma solução aplicável. Não necessita de coletas, consegue gerar um grande volume de dados por um longo tempo e tem apenas um custo inicial.

---

## Arquitetura

O projeto tem como base a placa [TTGO-T-Beam](https://github.com/LilyGO/TTGO-T-Beam), um kit de desenvolvimento 
[ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) que já conta com GPS e LoRa. 
Sensores de temperatura, pH, TDS e turbidez fazem a aquisição dos dados, além do GPS. O armazenamento se dá através de um módulo de cartão SD. 
A transmissão dos dados se dá através de LoRa. O controle desses módulos é feito através do [FreeRTOS](https://www.freertos.org).
A alimentação é feita por uma placa solar com um regulador stepdown.

A arquitetura do projeto pode ser vista abaixo:

![Arquitetura](./images/arquitetura.jpg)

---

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

![thermistor](https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcT4gyBLFEU9Bi2EIZIgDxRK5_8cXr6qh4mv8tjLU7s5FUcYz1KBjWSHsbwck3qdBqTh_1o&usqp=CAU)

### pH

Sørensen descreveu pela primeira vez o pH como a concentração de íons de hidrogênio. [Atualmente](https://www.degruyter.com/document/doi/10.1351/pac200274112169/html), 
é entendido pelo cologaritmo da atividade dos íons de hidrogênio. 
Sendo dado pela relação: $pH = -\log{a_H}$, onde $a_H$ é a atividade dos íons em $\frac{mol}{dm^3}$.
O pH consegue descrever o quanto uma solução é ácida ou básica. Sua escala, que é logaritimica, vai de 0, extremamente ácida, até 14, extremamente básica. 

É um fator muito relevante em análises de qualidade de água, pois alterações na acidez sugerem que fenômenos importantes estão acontecendo. 
A [acidificação oceânica](https://books.google.com.br/books?id=eoxpAgAAQBAJ&hl=pt-BR), por exemplo, é um fenômeno potencializado pelas mudanças climáticas
que prejudica diversos organismos e fotossíntese, e é mensurado através da escala de pH ao longo do tempo. Em reservatórios é uma grandeza que pode sugerir algum 
processo de [eutrofização](https://sciencing.com/eutrophication-affect-ph-12036599.html), que pode ser resultado de poluição por esgotos ou fertilizantes.

Como sensor de Ph utiza-se, um [sensor](https://www.usinainfo.com.br/index.php?controller=attachment&id_attachment=553) analógico de baixo custo, 
comumente utilizado em aquários.

![sensor de pH](https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcR1UMuvRR4jMIf_MzYRWwL7j3kl0JXh_7xvHQ&usqp=CAU)

Este tipo de sensor funciona medindo a diferença de tensão entre dois 
eletrodos, um de referência e outro sensor. Ambos estão isolados em tubos 
de vidro poroso para íons de hidrogênio. 
O eletrodo de referência fica imerso em uma solução de pH neutro. Enquanto 
o eletrodo sensor fica imerso em uma solução saturada de cloreto de 
potássio. O conjunto é mergulhado na solução a ser medida. Conforme os 
íons de hidrogênio se acumulam no vidro e substituem os íons metálicos do 
tubo, há a formação de uma corrente elétrica, que é capturada pelos 
eletrodos e a diferença entre as tensões é convertida em pH, pelo pHmetro.

Um exemplo desse tipo de montagem é mostrado a seguir:

![funcionamento pH](https://2.bp.blogspot.com/-XEanRs1BSxU/Vdn3oqHU8CI/AAAAAAAAEoo/k0gyucmU9Ow/s1600/pH-meter.png)

### Turbidez

A aparência nebulosa da água devido a suspensão de partículas finas é denominada 
[turbidez](https://onlinelibrary.wiley.com/doi/epdf/10.1111/j.1752-1688.2001.tb03624.x). Quanto maior a quantidade de partículas em suspensão, mais turva
é a água. Possui uma importância biológica grande, uma água que se torna turva repentinamente pode indicar a presença de poluição, por exemplo. As partículas 
podem prejudicar as brânquias dos peixes e diminuir a incidência de luz no ambiente sub aquático.

Como sensor, utiliza-se um módulo analógico de baixo custo, [TS-300B](http://images.100y.com.tw/pdf_file/46-TS-300B.pdf). Comumente utilizado
em máquinas de lavar roupa ou lavar pratos. Com a função de fazer uma 
medição da turbidez da água, antes e depois das lavagens, para saber se é 
necessário mais um ciclo de limpeza.

![sensor de turbidez](https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcTIhEUWTij3a4OiTMQDXJAizL-Tf4o1KTccMw&usqp=CAU)

Esse tipo de sensor funciona com a emissão de uma luz entre dois pontos 
através da água. Ou seja, um ponto emite luz e o outro a percebe. 
Quanto mais clara estiver a água, maior a penetração da luz, e por sua vez,
maior a quantidade de luz percebida pelo sensor. Da mesma forma, quanto
mais turva a água, menor é a quantidade percebida pelo sensor.


### TDS

TDS é o acrônimo em inglês para Total Dissolved Solid, ou sólidos 
dissolvidos totais. Pode ser descrito como [a medida de sais inorgânicos, matéria orgânica e outros materiais dissolvidos na água](https://citeseerx.ist.psu.edu/viewdoc/download;jsessionid=C9AC49848D8E364703B4E4C7957C5399?doi=10.1.1.483.218&rep=rep1&type=pdf).
Essa grandeza está intimamente relacionada com a [salinidade](https://iopscience.iop.org/article/10.1088/1755-1315/118/1/012019/pdf) da água.

Se tratando de água potável, um valor alto de TDS implica em um [sabor ruim](https://doi.org/10.1016/j.desal.2018.04.017). 
No ambiente, o aumento dessa escala implica em um aumento de salinidade, que pode levar a mortandade de espécies. Também já foi [descrita](https://cdnsciencepub.com/doi/10.1139/f85-199) uma 
correlação negativa significante entre a presença de íons $Na^+, Mg^{2+}, SO_4^{2-}, HCO_3^-$ e $CO_3^{2-}$ e a produção de clorofila tipo a.

Diferente da turbidez, que mede os sólidos em suspensão, o TDS mede os 
sólidos dissolvidos. Na prática, as moléculas desses sólidos se quebram em 
íons. Esses íons aumentam a condutividade elétrica da água. E os sensores
medem justamente essa condutividade, quanto mais condutiva, mais íons e 
mais sólidos dissolvidos.

O sensor utilizado no projeto é o [keyestudio TDS Meter v1.0](https://wiki.keyestudio.com/KS0429_keyestudio_TDS_Meter_V1.0).

![TDS sensor](https://wiki.keyestudio.com/images/thumb/a/a7/KS0429-1.png/600px-KS0429-1.png)


---

## Protótipo

No momento o projeto é um protótipo capaz de registrar a temperatura em um cartão sd, em um arquivo .csv, e transmitir mensagens via LoRa. Novos sensores, 
a comunicação GPS será estabelecida e questões de alimentação são pontos a serem adicionados e resolvidos em breve.
Uma foto do hardware atual do projeto pode ser vista a seguir:
![prototype](images/prototype.jpg)

O [placa com display](https://heltec.org/project/wifi-lora-32/) se trata do cliente LoRa, e recebe as mensagens enviadas pelo projeto.

## Esquemático

![wire diagram](images\wire_diagram.jpg)
