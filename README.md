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
## How to use example
We encourage the users to use the example as a template for the new projects.
A recommended way is to follow the instructions on a [docs page](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html#start-a-new-project).

## Example folder contents

The project **sample_project** contains one source file in C language [main.c](main/main.c). The file is located in folder [main](main).

ESP-IDF projects are built using CMake. The project build configuration is contained in `CMakeLists.txt`
files that provide set of directives and instructions describing the project's source files and targets
(executable, library, or both). 

Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   └── main.c
└── README.md                  This is the file you are currently reading
```
Additionally, the sample project contains Makefile and component.mk files, used for the legacy Make based build system. 
They are not used or needed when building with CMake and idf.py.
