# Digital_voltmeter
Multi-channel digital voltmeter for test task. Measures DC (0-3.3V), AC ~40-70Hz (0-3.3V amplitude).

## Особенности реализации
- Проект реализован для отладочной платы Blue Pill от STM. 
- На плате находится МК STM32F103C8, присутствует кварцевый резонатор на 8МГц, а также светодиод индикации, подсоединенный к выводу C13.
- Во время инициализации происходит переключение на тактирование от внешнего кварца, через PLL с умножением на 9. Итоговая частота системного таймера равна 72МГц.
- Из сторонних библиотек только CMSIS и ОС FreeRTOS. Все остальное написано с нуля.
- Конфигурация линковщика: размер стека = 0x800, размер кучи = 0x1200 (файл "raw_freertos_stm32103c8.icf" уже активирован в настройках IAREW проекта)
- Модифицирован файл "CMSIS/src/system_stm32f1xx.c"  (раскомментирована строчка #define USER_VECT_TAB_ADDRESS для верного указания адреса таблицы векторов прерываний: с адреса 0x08000000 (FLASH_BASE) )
- Модифицирован файл "CMSIS/src/startup_stm32f103xb.s" ( переопределен переход на ассемблерные функции FreeRTOS vPortSVCHandler, xPortPendSVHandler, xPortSysTickHandler по соответствующим прерываниям )
- **Внимание!** Согласно документации, размер Flash памяти МК STM32F103C8 равен 64Kb, однако на большинстве этих МК (и на моем) размер flash 128Kb. Прошивка, даже при максимальной оптимизации компилятора, занимает 90Kb. Она помещается на МК на моей плате, однако есть вероятность, что другой аналогичный МК будет иметь меньший размер flash памяти.
- Использовалать IDE IAR Embedded Workbench - Arm 9.30.1

## Сборка
Необходимо открыть файл проекта "IrkutskTask.eww" в среде IAR Embedded Workbench - Arm, после этого собрать проект как обычно.

## Прошивка
Прошивать плату удобно при помощи SWD интерфейса. <br>
Джамперы Boot оба в положении "0"

## Распиновка
| Пин | Предназначение |
|:------:|:-----:|
| A9 | Uart Tx | 
| A10 | Uart Rx |
| A0-A7 | Ch0-Ch7 |
| B0-B1 | Ch8-Ch9 |


![Распиновка](https://github.com/MangustaLangusta/Digital_voltmeter/blob/main/STM32-STM32F1-STM32F103-STM32F103C8T6-pinout-high-resolution.png "Распиновка")



## Работа. 
Общение с устройством осуществляется через консоль.
### Параметры соединения

#### Параметры по умолчанию
| Параметр | Значение |
|:------:|:-----:|
| Скорость | 115200 | 
| Размер посылки | 8 бит |
| Проверка четности | Нет |
| Стоп-биты | 1 |

Скорость по умолчанию можно поменять в файле "stm32uartConfig.h". <br>
Возможно использование других параметров во время инициализации Uart <br>
(см. файл "stm32uart.h", а именно, структуру "UartSettings" и функцию инициализации "InitUart()").

### Система команд
 При запуске устройство передает на консоль строку "Voltmeter Started". <br>
 Светодиод мигает раз в секунду (Idle state, см. ниже обозначения индикации светодиода)
 #### Возможные команды 
 | Команда | Параметры | Результат | Пример команды |
|:------:|:--------:|:------:|:-----------------:|
| status | - | Выводит сообщение о режиме работы, <br> запущенных каналах, наличии ошибок | "status" |
| start | ch<0-9> <none, avg, rms> | Запускает канал ch в режиме мгновенного значения (none), <br>среднего значения (avg), <br>среднеквадратичного (rms) | "start ch3 avg" |
| result | ch<0-9> (dump)| Выводит результат измерений (в вольтах) в консоль. <br>Параметр "dump" - опциональный,<br> выводит сырые значения АЦП данного канала | "result ch3", "result ch3 dump" | 
| stop | ch<0-9> | Останавливает измерения выбранного канала | "stop ch3" |

#### Примечания
- По умолчанию активных каналов может быть максимум три. Ограничение обусловлено частотой вызова программного таймера и памятью. Можно переписать на прерывания по обычному таймеру, тогда, при разумной частоте их вызова, получится добиться возможности работы большего числа каналов одновременно. Это, однако, не сделано в рамках данной тестовой работы.
- Подразумевается, что команды, отправленные из консоли, оканчиваются символом-разделителем (например, '\n' - это значение по умолчанию). Если используемая консоль не добавляет в конец сообшения такие символы автоматически, необходимо делать это вручную. Символ-разделитель можно поменять на другой в файле конфигурации модуля uart (см. ниже stm32uart)
- Нельзя запустить или остановить несколько каналов за одно сообщение. Необходимо вместо этого запускать по очереди (например, "start ch0 none", "start ch1 avg", start ch5 rms"). В случае попытки запуска нескольких каналов запустится только первый в списке. С остановкой все то же самое.
- Если в команде нет обязательных параметров (например, не указан режим или синтаксическая ошибка) канал запущен не будет, а на консоль выведется соответствующее сообщение.
- Нельзя перезапустить уже работающий канал без остановки. Необходимо остановить его командой "stop ch..", а затем запустить.
- Несмотря на то, что состояние "Error" предусмотрено, устройство, однако, не переходит в него, а выводит информацию об ошибках на консоль. Это сделано для удобства. При необходимости путем несложных изменений в коде такое поведение можно поменять. В этом случае предусмотрен выход из состояния Error путем запроса статуса (команда "status"). Тогда вместе со статусом выводятся ошибки, а устройство переходит в нормальный режим работы.
### Индикация светодиода
| Режим | Индикация |
|:------:|:-----:|
| Idle | Мигает редко с частотой 1Hz | 
| Running | Мигает часто (5Hz) |
| Error | Горит постоянно |


## Краткое описание работы модулей
### stm32uart
- Uart выполнен в виде отдельного независимого от остальных частей модуля (см. папку "stm32uart/").
- Модуль настраивается аналогично FreeRTOS в своем файле конфигурации"stm32uartConfig.h".
- Модуль также разделен на логическую (абстрактную) часть и реализацию для конкретной платформы.
Реализация для платформы находится в файле "stm32uart_port.cpp".
Соответственно, для портирования можно переопределить функции, объявленные в данном файле, в соответствии с целевой платформой.
- Uart реализован с использованием кольцевого буфера и DMA в кольцевом режиме. Размер буфера задается в файле конфигурации. 
- Сообщения ограничены по длине. Максимальная длина сообщения задается в файле конфигурации.
- Сообщения должны отделяться друг от друга специальным символом-разделителем (по умолчанию '\n').
- Интерфейс UART описан в файле "stm32uart.h"
### stm32adc
- Для облегчения портирования Adc построен по принципу, описанному выше для uart .
- Adc использует DMA и буфер. Одновременно активных каналов может быть несколько. Каналы можно добавлять в скан-лист и убирать их из него. 
- Интерфейс АЦП описан в файле stm32adc.h
- При добавлении очередного канала в скан-лист счетчик DMA увеличивается на 1, а также выбранный канал добавляется в regular channels ADC.
Как только очередной канал измерен, DMA получает сигнал и перемещает данные в буфер, в ячейку, соответствующую этому каналу. Замеры идут непрерывно (continious / scan mode). Актуальные значения каналов, таким образом, все время хранятся в буфере. Соответствие канала и ячейки буфера хранится внутри специального класса AdcManager.
### led_blinker
- Предназначен для управления светодиодом.
- Реализация простая, см. "task specific/include/led_blinker.h", "task specific/src/led_blinker.cpp"
### Voltmeter
- Реализован в виде класса, все поля и методы которого статические. Управление происходит через команды-сообщения.
- см. файл "task specific/include/voltmeter.h" для ознакомления с объявлением класса.
- Возможно добавление новых команд и модификация уже существующих.
- Хранит список активных каналов и берет на себя работу по взаимодействию с каналами Adc, расположенными ниже по уровню абстракции.
- Каналы имеют общий интерфейсный класс IVoltmeterChannel, от которого наследуются конкретные типы каналов (например, мгновенное значение, среднее, среднеквадратическое). 
Таким образом, легко добавить или модифицировать тип канала.
- По запросу значения АЦП преобразуются к вольтам. Можно настроить пределы, изменив соответствующие коэффициенты. Легко реализовать динамическое изменение значений при помощи uart команд.

#### Реализация различных типов каналов
Данная реализация, хоть и неоптимальная, обеспечивает относительную погрешность около 0.5% (0.01-0.02В при диапазоне 3.3В)
##### Мгновенное значение.
Не хранит никаких значений, вместо этого по запросу возвращается актуальное значение канала, приведенное к вольтам
##### Среднее
Хранит n последних значений канала, измеренных через промежутки времени t. (Например, 20 значений через каждые 2мс)
Значения обновляются по таймеру. Используется программный таймер FreeRTOS.
По запросу ищет максимальное и минимальное значения, находит среднее, приводит к вольтам и возвращает.
Легко модифицировать команды uart, чтобы можно было динамически менять n и t. (Это уже реализовано в конструкторе канала)
##### Среднеквадратическое.
Все так же, как и для среднего значения, но среднее значение вычитается из максимального и домножается на кв корень из 2


